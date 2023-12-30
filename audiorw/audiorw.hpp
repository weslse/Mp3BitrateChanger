#pragma once

#include <vector>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
};

struct AVCodecContext;
struct AVFormatContext;
struct SwrContext;
struct AVFrame;
struct AVPacket;

namespace audiorw {

	static const int OUTPUT_BIT_RATE[3] = { 128000, 64000, 32000 };
	static const int DEFAULT_FRAME_SIZE = 2048;

	std::vector<std::vector<double>> read(
		const std::string& filename,
		double& sample_rate,
		double start_seconds = 0,
		double end_seconds = -1);

	bool write(
		const std::vector<std::vector<double>>& audio,
		const std::string& filename,
		double sample_rate,
		int loopCnt);

	void cleanup(
		AVCodecContext* codec_context,
		AVFormatContext* format_context,
		SwrContext* resample_context,
		AVFrame* frame,
		AVPacket& packet);

}
