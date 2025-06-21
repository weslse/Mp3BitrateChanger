#pragma once

#include <vector>
#include <string>
#include <windows.h>
#include <mutex>

// 전역 뮤텍스 선언 (헤더에 선언, cpp에서 정의)
namespace audiorw {
    std::mutex& get_ffmpeg_mutex(); // 선언만!
}

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

template<typename T>
inline T my_max(const T& a, const T& b) { return (a > b) ? a : b; }

template<typename T>
inline T my_min(const T& a, const T& b) { return (a < b) ? a : b; }

inline std::string wstring_to_utf8(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &strTo[0], size_needed, nullptr, nullptr);
    return strTo;
}

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

    inline bool write(
        const std::vector<std::vector<double>>& audio,
        const std::wstring& filename,
        double sample_rate,
        int loopCnt)
    {
        return audiorw::write(audio, ::wstring_to_utf8(filename), sample_rate, loopCnt);
    }

}
