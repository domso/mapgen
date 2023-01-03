#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <optional>
#include <limits>

template<typename caller_T, typename... Ts>
concept caller_accepts_arguments = requires(caller_T caller, Ts... s) {
    caller(s...);
};

template<typename T>
class image {
public:
    image(const size_t width, const size_t height) : m_width(width), m_height(height), m_offset(0), m_row_stride(width), m_data(std::make_shared<std::vector<T>>(width * height)) {
            m_direct_data = m_data->data();
        }
    image(const size_t width, const size_t height, const T p) : m_width(width), m_height(height), m_offset(0), m_row_stride(width), m_data(std::make_shared<std::vector<T>>(width * height)) {
        m_direct_data = m_data->data();
        for_each_pixel([p](T& f) {
            f = p;
        });
    }
    image(const image& copy) : 
        m_width(copy.m_width),
        m_height(copy.m_height),
        m_offset(copy.m_offset),
        m_row_stride(copy.m_row_stride),
        m_data(std::make_shared<std::vector<T>>(*(copy).m_data)) {
                m_direct_data = m_data->data();
            }
    image(image&& move) : m_width(move.m_width), m_height(move.m_height), m_offset(move.m_offset), m_row_stride(move.m_row_stride), m_data(std::move(move.m_data)) {
        m_direct_data = m_data->data();
        move.m_width = 0;
        move.m_height = 0;
    }
    void operator=(const image& copy) {
        m_width = copy.m_width;
        m_height = copy.m_height;
        m_offset = copy.m_offset;
        m_row_stride = copy.m_row_stride;
        m_data = std::make_shared<std::vector<T>>(*(copy.m_data));
        m_direct_data = m_data->data();
    }
    void operator=(image&& move) {
        m_width = move.m_width;
        m_height = move.m_height;
        m_offset = move.m_offset;
        m_row_stride = move.m_row_stride;
        m_data = std::move(move.m_data);
        m_direct_data = m_data->data();
        move.m_width = 0;
        move.m_height = 0;
    }
    size_t width() const {
        return m_width;
    }
    size_t height() const {
        return m_height;
    }
    bool contains(const int x, const int y) const {
        return 0 <= x && x < m_width && 0 <= y && y < m_height;
    }
    bool contains(const int x, const int y, const int width, const int height) const {
        return 0 <= x && x < m_width && 0 <= y && y < m_height && 0 <= x + width && x + width < m_width && 0 <= y + height && y + height < m_height;
    }
    const T& at(const int x, const int y) const {
        size_t ptr = m_offset + y * m_row_stride + x;
        return (*m_data)[ptr];
    }
    T& at(const int x, const int y) {
        size_t ptr = m_offset + y * m_row_stride + x;
        return m_direct_data[ptr];
    }
    T* data() {
        return m_data->data();
    }
    template<typename exec_T>
        requires caller_accepts_arguments<exec_T, T>
    void for_each_pixel(const exec_T& call) {
        for (size_t y = 0; y < m_height; y++) {
            for (size_t x = 0; x < m_width; x++) {
                call(at(x, y));
            }
        }
    }
    template<typename exec_T>
        requires caller_accepts_arguments<exec_T, T>
    void for_each_pixel(const exec_T& call) const {
        for (size_t y = 0; y < m_height; y++) {
            for (size_t x = 0; x < m_width; x++) {
                call(at(x, y));
            }
        }
    }
    template<typename exec_T>
        requires caller_accepts_arguments<exec_T, T>
    void check_each_pixel(const exec_T& call) {
        for (size_t y = 0; y < m_height; y++) {
            for (size_t x = 0; x < m_width; x++) {
                if (!call(at(x, y))) {
                    return;
                }
            }
        }
    }
    template<typename exec_T>
        requires caller_accepts_arguments<exec_T, T>
    void check_each_pixel(const exec_T& call) const {
        for (size_t y = 0; y < m_height; y++) {
            for (size_t x = 0; x < m_width; x++) {
                if (!call(at(x, y))) {
                    return;
                }
            }
        }
    }
    template<typename exec_T>
        requires caller_accepts_arguments<exec_T, T, size_t, size_t>
    void for_each_pixel(const exec_T& call) {
        for (size_t y = 0; y < m_height; y++) {
            for (size_t x = 0; x < m_width; x++) {
                call(at(x, y), x, y);
            }
        }
    }
    template<typename exec_T>
        requires caller_accepts_arguments<exec_T, T, size_t, size_t>
    void for_each_pixel(const exec_T& call) const {
        for (size_t y = 0; y < m_height; y++) {
            for (size_t x = 0; x < m_width; x++) {
                call(at(x, y), x, y);
            }
        }
    }
    std::optional<image> subregion(const int x, const int y, const int width, const int height) {
        if (contains(x, y, width, height)) {
            image result;
            result.m_width = width;
            result.m_height = height;
            result.m_offset = m_offset + y * m_row_stride + x;
            result.m_row_stride = m_row_stride;
            result.m_data = m_data;
            result.m_direct_data = (result.m_data->data());

            return result;
        }

        return std::nullopt;
    }
    void copy_to(image& dest) const {
        for (size_t y = 0; y < std::min(m_height, dest.m_height); y++) {
            for (size_t x = 0; x < std::min(m_width, dest.m_width); x++) {
                dest.at(x, y) = this->at(x, y);
            }
        }
    }
    void add(const image& src) {
        for (size_t y = 0; y < std::min(m_height, src.m_height); y++) {
            for (size_t x = 0; x < std::min(m_width, src.m_width); x++) {
                this->at(x, y) += src.at(x, y);
            }
        }
    }
    std::pair<T, T> range() const {
        T min = std::numeric_limits<T>::max();
        T max = std::numeric_limits<T>::min();

        for (size_t y = 0; y < m_height; y++) {
            for (size_t x = 0; x < m_width; x++) {
                min = std::min(min, at(x, y));
                max = std::max(max, at(x, y));
            }
        }
       
        return {min, max};
    }
private:
    image() {}
    size_t m_width;
    size_t m_height;
    size_t m_offset;
    size_t m_row_stride;
    std::shared_ptr<std::vector<T>> m_data;
    T* m_direct_data;
};
