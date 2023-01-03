#pragma once

#include <mutex>
#include <thread>
#include <vector>

#include "image.h"

class terrain_buffer {
public:
    terrain_buffer(const int width, const int height, const int count);
    image<float> pop();
    void stop();
private:
    void start_regeneration();

    int m_width;
    int m_height;
    int m_count;

    std::mutex m_mutex;
    int m_current_num_threads = 0;
    std::vector<image<float>> m_buffer;
};
