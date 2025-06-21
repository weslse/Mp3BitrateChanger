#include "pch.h"
#include "audiorw/audiorw.hpp"

namespace audiorw {
    std::mutex& get_ffmpeg_mutex() {
        static std::mutex mtx;
        return mtx;
    }
}