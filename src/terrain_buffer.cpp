#include "terrain_buffer.h"
#include "generate_terrain.h"

#include <assert.h>
#include <iostream>

terrain_buffer::terrain_buffer(const int width, const int height, const int count) : m_width(width), m_height(height), m_count(count) {
    for (int i = 0; i < std::thread::hardware_concurrency(); i++) {
        start_regeneration();
    }
}

image<float> terrain_buffer::pop() {
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

void terrain_buffer::stop() {
    std::unique_lock ul(m_mutex);
    m_count = 0;    
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
            std::unique_lock ul(m_mutex);
            m_current_num_threads++; 
            while(true) {
                if (m_buffer.size() < m_count) {
                    std::vector<image<float>> new_terrains;
                    ul.unlock();
                    new_terrains.push_back(std::move(generate_terrain(m_width, m_height)));
                    ul.lock();
                    for (auto& v : new_terrains) {
                        m_buffer.push_back(std::move(v));
                    }
                } else {
                    return;
                }
            }
            m_current_num_threads--;
        }
    });

    t1.detach();
}
