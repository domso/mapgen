#pragma once

#include <mutex>
#include <thread>
#include <vector>

class terrain_buffer {
public:
    terrain_buffer(const int width, const int height, const int count);
    std::vector<float> pop();
private:
    void start_regeneration();

    int m_width;
    int m_height;
    int m_count;

    std::mutex m_mutex;
    int m_current_num_threads = 0;
    std::vector<std::vector<float>> m_buffer;
};
