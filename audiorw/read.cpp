#include <vector>
#include <string>
#include <stdexcept>
#include <ciso646>
#include <limits>
#include <cmath>
#include <algorithm>
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
};

#include "audiorw.hpp"

using namespace audiorw;

std::vector<std::vector<double>> audiorw::read(
    const std::string& filename,
    double& sample_rate,
    double start_seconds,
    double end_seconds) 
{
    std::lock_guard<std::mutex> lock(get_ffmpeg_mutex());

    size_t errbuf_size = 200;
    char errbuf[200];

    AVCodecContext* codec_context = NULL;
    AVFormatContext* format_context = NULL;
    SwrContext* resample_context = NULL;
    AVFrame* frame = NULL;
    AVPacket packet;

    int error = avformat_open_input(&format_context, filename.c_str(), NULL, 0);
    if (error != 0) {
        av_strerror(error, errbuf, errbuf_size);
        throw std::invalid_argument(
            "Could not open audio file: " + filename + "\n" +
            "Error: " + std::string(errbuf));
    }

    if ((error = avformat_find_stream_info(format_context, NULL)) < 0) {
        cleanup(codec_context, format_context, resample_context, frame, packet);
        av_strerror(error, errbuf, errbuf_size);
        throw std::runtime_error(
            "Could not get information about the stream in file: " + filename + "\n" +
            "Error: " + std::string(errbuf));
    }

    AVCodec* codec = NULL;
    int audio_stream_index = av_find_best_stream(
        format_context,
        AVMEDIA_TYPE_AUDIO,
        -1, -1, (const AVCodec**)&codec, 0);
    if (audio_stream_index < 0) {
        cleanup(codec_context, format_context, resample_context, frame, packet);
        throw std::runtime_error(
            "Could not determine the best stream to use in the file: " + filename);
    }

    codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
        cleanup(codec_context, format_context, resample_context, frame, packet);
        throw std::runtime_error(
            "Could not allocate a decoding context for file: " + filename);
    }

    if ((error = avcodec_parameters_to_context(
        codec_context,
        format_context->streams[audio_stream_index]->codecpar
    )) != 0) {
        cleanup(codec_context, format_context, resample_context, frame, packet);
        throw std::runtime_error(
            "Could not set codec context parameters for file: " + filename);
    }

    if ((error = avcodec_open2(codec_context, codec, NULL)) != 0) {
        cleanup(codec_context, format_context, resample_context, frame, packet);
        av_strerror(error, errbuf, errbuf_size);
        throw std::runtime_error(
            "Could not initialize the decoder for file: " + filename + "\n" +
            "Error: " + std::string(errbuf));
    }

    if (codec_context->ch_layout.nb_channels == 0) {
        av_channel_layout_default(&codec_context->ch_layout, codec_context->ch_layout.nb_channels);
    }

    sample_rate = codec_context->sample_rate;
    if (sample_rate <= 0) {
        cleanup(codec_context, format_context, resample_context, frame, packet);
        throw std::runtime_error(
            "Sample rate is " + std::to_string(sample_rate));
    }

    swr_alloc_set_opts2(
        &resample_context,
        &codec_context->ch_layout,
        AV_SAMPLE_FMT_DBL,
        sample_rate,
        &codec_context->ch_layout,
        codec_context->sample_fmt,
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

    if (!(frame = av_frame_alloc())) {
        cleanup(codec_context, format_context, resample_context, frame, packet);
        throw std::runtime_error(
            "Could not allocate audio frame for file: " + filename);
    }

    av_new_packet(&packet, 0);
    packet.data = NULL;
    packet.size = 0;

    start_seconds = my_max(start_seconds, 0.);
    double duration = (format_context->duration) / (double)AV_TIME_BASE;
    if (end_seconds < 0) {
        end_seconds = duration;
    }
    else {
        end_seconds = my_min(end_seconds, duration);
    }
    double start_sample = std::floor(start_seconds * sample_rate);
    double end_sample = std::floor(end_seconds * sample_rate);

    std::vector<std::vector<double>> audio(codec_context->ch_layout.nb_channels);

    int sample = 0;
    while (sample < end_sample) {
        error = av_read_frame(format_context, &packet);
        if (error == AVERROR_EOF) {
            break;
        }
        else if (error < 0) {
            cleanup(codec_context, format_context, resample_context, frame, packet);
            av_strerror(error, errbuf, errbuf_size);
            throw std::runtime_error(
                "Error reading from file: " + filename + "\n" +
                "Error: " + std::string(errbuf));
        }

        if (packet.stream_index != audio_stream_index) {
            continue;
        }

        if ((error = avcodec_send_packet(codec_context, &packet)) < 0) {
            cleanup(codec_context, format_context, resample_context, frame, packet);
            av_strerror(error, errbuf, errbuf_size);
            throw std::runtime_error(
                "Could not send packet to decoder for file: " + filename + "\n" +
                "Error: " + std::string(errbuf));
        }

        while ((error = avcodec_receive_frame(codec_context, frame)) == 0) {
            double* audio_data = new double[audio.size() * frame->nb_samples];
            uint8_t* audio_data_ = reinterpret_cast<uint8_t*>(audio_data);
            const uint8_t** frame_data = const_cast<const uint8_t**>(frame->extended_data);
            if ((error = swr_convert(resample_context,
                &audio_data_, frame->nb_samples,
                frame_data, frame->nb_samples)) < 0) {
                cleanup(codec_context, format_context, resample_context, frame, packet);
                av_strerror(error, errbuf, errbuf_size);
                throw std::runtime_error(
                    "Could not resample frame for file: " + filename + "\n" +
                    "Error: " + std::string(errbuf));
            }

            for (int s = 0; s < frame->nb_samples; s++) {
                int index = sample + s - start_sample;
                if ((0 <= index) && (index < end_sample)) {
                    for (int channel = 0; channel < (int)audio.size(); channel++) {
                        audio[channel].push_back(audio_data[audio.size() * s + channel]);
                    }
                }
            }
            delete[] audio_data;
            sample += frame->nb_samples;
        }

        if (error != AVERROR(EAGAIN)) {
            cleanup(codec_context, format_context, resample_context, frame, packet);
            av_strerror(error, errbuf, errbuf_size);
            throw std::runtime_error(
                "Error receiving packet from decoder for file: " + filename + "\n" +
                "Error: " + std::string(errbuf));
        }
    }

    cleanup(codec_context, format_context, resample_context, frame, packet);

    return audio;
}

void audiorw::cleanup(
    AVCodecContext* codec_context,
    AVFormatContext* format_context,
    SwrContext* resample_context,
    AVFrame* frame,
    AVPacket& packet) {
    avcodec_close(codec_context);
    avcodec_free_context(&codec_context);
    avio_closep(&format_context->pb);
    avformat_free_context(format_context);
    swr_free(&resample_context);
    av_frame_free(&frame);
    av_packet_unref(&packet);
}
