#include "terrain_buffer.h"
#include "generate_terrain.h"

#include <assert.h>
#include <iostream>

terrain_buffer::terrain_buffer(const int width, const int height, const int count) : m_width(width), m_height(height), m_count(count) {
    for (int i = 0; i < std::thread::hardware_concurrency(); i++) {
        start_regeneration();
    }
}

std::vector<float> terrain_buffer::pop() {
    std::unique_lock ul(m_mutex);
    if (m_buffer.empty()) {
        ul.unlock();
        start_regeneration();
        return generate_terrain(m_width, m_height);
    } else {
        auto result = std::move(*m_buffer.rbegin());
        m_buffer.pop_back();
        return result;
    }
}

void terrain_buffer::start_regeneration() {
    {
        std::unique_lock ul(m_mutex);
        if (m_buffer.size() == m_count || m_current_num_threads == std::thread::hardware_concurrency()) {
            return;
        }
    }

    std::thread t1([&]() {
        if (m_current_num_threads < std::thread::hardware_concurrency()) {
            m_current_num_threads++; 
            while(true) {
                std::unique_lock ul(m_mutex);
                if (m_buffer.size() < m_count) {
                    std::vector<float> new_terrain;
                    ul.unlock();
                    new_terrain = generate_terrain(m_width, m_height);
                    ul.lock();
                    std::cout << "generate terrain " << std::this_thread::get_id() << " " << m_buffer.size() << std::endl;
                    m_buffer.push_back(std::move(new_terrain));
                } else {
                    return;
                }
            }
            m_current_num_threads--;
        }
    });

    t1.detach();
}
