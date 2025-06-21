#include <vector>
#include <string>
#include <stdexcept>
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
};

#include "audiorw.hpp"

using namespace audiorw;

bool audiorw::write(
    const std::vector<std::vector<double>>& audio,
    const std::string& filename,
    double sample_rate,
    int loopCnt)
{
    std::lock_guard<std::mutex> lock(get_ffmpeg_mutex());

    size_t errbuf_size = 200;
    char errbuf[200];

    AVCodecContext* codec_context = NULL;
    AVFormatContext* format_context = NULL;
    SwrContext* resample_context = NULL;
    AVFrame* frame = NULL;
    AVPacket packet = AVPacket();

    AVIOContext* output_io_context;
    int error = avio_open(
        &output_io_context,
        filename.c_str(),
        AVIO_FLAG_WRITE);
    if (error < 0) {
        cleanup(codec_context, format_context, resample_context, frame, packet);
        av_strerror(error, errbuf, errbuf_size);
        throw std::invalid_argument(
            "Could not open file:" + filename + "\n" +
            "Error: " + std::string(errbuf));
    }

    if (!(format_context = avformat_alloc_context())) {
        cleanup(codec_context, format_context, resample_context, frame, packet);
        throw std::runtime_error(
            "Could not allocate output format context for file:" + filename);
    }

    format_context->pb = output_io_context;

    if (!(format_context->oformat = av_guess_format(NULL, filename.c_str(), NULL))) {
        cleanup(codec_context, format_context, resample_context, frame, packet);
        throw std::runtime_error(
            "Could not find output file format for file: " + filename);
    }

    if (!(format_context->url = av_strdup(filename.c_str()))) {
        cleanup(codec_context, format_context, resample_context, frame, packet);
        throw std::runtime_error(
            "Could not process file path name for file: " + filename);
    }

    AVCodecID codec_id = av_guess_codec(
        format_context->oformat,
        NULL,
        filename.c_str(),
        NULL,
        AVMEDIA_TYPE_AUDIO);

    auto output_codec = avcodec_find_encoder(codec_id);
    if (!output_codec) {
        cleanup(codec_context, format_context, resample_context, frame, packet);
        throw std::runtime_error(
            "Could not open codec with ID, " + std::to_string(codec_id) + ", for file: " + filename);
    }

    AVStream* stream;
    if (!(stream = avformat_new_stream(format_context, NULL))) {
        cleanup(codec_context, format_context, resample_context, frame, packet);
        throw std::runtime_error(
            "Could not create new stream for output file: " + filename);
    }

    if (!(codec_context = avcodec_alloc_context3(output_codec))) {
        cleanup(codec_context, format_context, resample_context, frame, packet);
        throw std::runtime_error(
            "Could not allocate an encoding context for output file: " + filename);
    }

    codec_context->ch_layout.nb_channels = audio.size();
    av_channel_layout_default(&codec_context->ch_layout, audio.size());
    codec_context->sample_rate = sample_rate;
    codec_context->sample_fmt = output_codec->sample_fmts[0];
    codec_context->bit_rate = OUTPUT_BIT_RATE[loopCnt];

    stream->time_base.den = sample_rate;
    stream->time_base.num = 1;

    if (format_context->oformat->flags & AVFMT_GLOBALHEADER)
        codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if ((error = avcodec_open2(codec_context, output_codec, NULL)) < 0) {
        cleanup(codec_context, format_context, resample_context, frame, packet);
        av_strerror(error, errbuf, errbuf_size);
        throw std::runtime_error(
            "Could not open output codec for file: " + filename + "\n" +
            "Error: " + std::string(errbuf));
    }

    error = avcodec_parameters_from_context(stream->codecpar, codec_context);
    if (error < 0) {
        cleanup(codec_context, format_context, resample_context, frame, packet);
        throw std::runtime_error(
            "Could not initialize stream parameters for file: " + filename);
    }

    swr_alloc_set_opts2(
        &resample_context,
        &codec_context->ch_layout,
        codec_context->sample_fmt,
        sample_rate,
        &codec_context->ch_layout,
        AV_SAMPLE_FMT_DBL,
        sample_rate,
        0, NULL);
    if (!resample_context) {
        cleanup(codec_context, format_context, resample_context, frame, packet);
        throw std::runtime_error(
            "Could not allocate resample context for file: " + filename);
    }

    if ((error = swr_init(resample_context)) < 0) {
        cleanup(codec_context, format_context, resample_context, frame, packet);
        throw std::runtime_error(
            "Could not open resample context for file: " + filename);
    }

    if ((error = avformat_write_header(format_context, NULL)) < 0) {
        cleanup(codec_context, format_context, resample_context, frame, packet);
        throw std::runtime_error(
            "Could not write output file header for file: " + filename);
    }

    if (!(frame = av_frame_alloc())) {
        cleanup(codec_context, format_context, resample_context, frame, packet);
        throw std::runtime_error(
            "Could not allocate output frame for file: " + filename);
    }
    if (codec_context->frame_size <= 0) {
        codec_context->frame_size = DEFAULT_FRAME_SIZE;
    }
    frame->nb_samples = codec_context->frame_size;
    frame->ch_layout = codec_context->ch_layout;
    frame->format = codec_context->sample_fmt;
    frame->sample_rate = codec_context->sample_rate;
    if ((error = av_frame_get_buffer(frame, 0)) < 0) {
        cleanup(codec_context, format_context, resample_context, frame, packet);
        av_strerror(error, errbuf, errbuf_size);
        throw std::runtime_error(
            "Could not allocate output frame samples for file: " + filename + "\n" +
            "Error: " + std::string(errbuf));
    }

    av_new_packet(&packet, 0);
    packet.data = NULL;
    packet.size = 0;

    int sample = 0;
    double* audio_data = new double[audio.size() * codec_context->frame_size];
    while (true) {
        if (sample < (int)audio[0].size()) {
            int frame_size = my_min(codec_context->frame_size, int(audio[0].size() - sample));
            frame->nb_samples = frame_size;
            frame->pts = sample;

            for (int s = 0; s < frame_size; s++) {
                for (int channel = 0; channel < (int)audio.size(); channel++) {
                    audio_data[audio.size() * s + channel] = audio[channel][sample + s];
                }
            }
            sample += frame_size;

            const uint8_t* audio_data_ = reinterpret_cast<uint8_t*>(audio_data);
            if ((error = swr_convert(resample_context,
                frame->extended_data, frame_size,
                &audio_data_, frame_size)) < 0) {
                cleanup(codec_context, format_context, resample_context, frame, packet);
                av_strerror(error, errbuf, errbuf_size);
                throw std::runtime_error(
                    "Could not resample frame for file: " + filename + "\n" +
                    "Error: " + std::string(errbuf));
            }
        }
        else {
            av_frame_free(&frame);
            frame = NULL;
        }

        if ((error = avcodec_send_frame(codec_context, frame)) < 0) {
            cleanup(codec_context, format_context, resample_context, frame, packet);
            av_strerror(error, errbuf, errbuf_size);
            throw std::runtime_error(
                "Could not send packet for encoding for file: " + filename + "\n" +
                "Error: " + std::string(errbuf));
        }

        while ((error = avcodec_receive_packet(codec_context, &packet)) == 0) {
            if ((error = av_write_frame(format_context, &packet)) < 0) {
                cleanup(codec_context, format_context, resample_context, frame, packet);
                av_strerror(error, errbuf, errbuf_size);
                throw std::runtime_error(
                    "Could not write frame for file: " + filename + "\n" +
                    "Error: " + std::string(errbuf));
            }
        }

        if (error == AVERROR_EOF) {
            break;
        }
        else if (error != AVERROR(EAGAIN)) {
            cleanup(codec_context, format_context, resample_context, frame, packet);
            av_strerror(error, errbuf, errbuf_size);
            throw std::runtime_error(
                "Could not encode frame for file: " + filename + "\n" +
                "Error: " + std::string(errbuf));
        }
    }

    if ((error = av_write_trailer(format_context)) < 0) {
        cleanup(codec_context, format_context, resample_context, frame, packet);
        throw std::runtime_error(
            "Could not write output file trailer for file: " + filename);
    }

    delete[] audio_data;
    cleanup(codec_context, format_context, resample_context, frame, packet);

    return true;
}
