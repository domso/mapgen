#include "merge_map.h"

#include <random>
#include <math.h>

void add_gaussian_blur(std::vector<float>& data) {
    std::vector<float> copy = data;

    for (int i = 0; i < data.size(); i++) {
        if (i == 0) {
            copy[i] = (data[i] + data[i + 1]) / 2;
        } else if (i == data.size() - 1) {
            copy[i] = (data[i] + data[i - 1]) / 2;
        } else {
            copy[i] = (data[i] + data[i - 1] + data[i + 1]) / 3;
        }
    }

    data = copy;
}

void merge_map_top(const std::vector<float>& top, std::vector<float>& center, const int width, const int height, const std::vector<float>& ranges, const int i, const int j) {
    float diff = center[j * width + i] - top[top.size() - width + i];
    float range = ranges[i];

    if (j < range) {
        center[j * width + i] -= diff / (range * range) * ((range - j) * (range - j));
    }
}

void merge_map_right(const std::vector<float>& right, std::vector<float>& center, const int width, const int height, const std::vector<float>& ranges, const int i, const int j) {
    float range = ranges[j];
    float diff = center[j * width + i] - right[j * width];
    if (i > width - range) {
        center[j * width + i] -= diff / (range * range) * ((range - width + i) * (range - width + i));
    }
}

void merge_map_bot(const std::vector<float>& bot, std::vector<float>& center, const int width, const int height, const std::vector<float>& ranges, const int i, const int j) {
    float range = ranges[i];
    float diff = center[j * width + i] - bot[i];
    if (j > height - range) {
        center[j * width + i] -= diff / (range * range) * ((range - height + j) * (range - height + j));
    }
}

void merge_map_left(const std::vector<float>& left, std::vector<float>& center, const int width, const int height, const std::vector<float>& ranges, const int i, const int j) {
    float range = ranges[j];
    float diff = center[j * width + i] - left[j * width + width - 1];
    if (i < range) {
        center[j * width + i] -= diff / (range * range) * ((range - i) * (range - i));
    }
}

void merge_map(const std::vector<float>& top, const std::vector<float>& right, const std::vector<float>& bot, const std::vector<float>& left, std::vector<float>& center, const int width, const int height) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(20, 100);

    std::vector<float> ranges(std::max(width, height));

    for (auto& f : ranges) {
        f = dis(gen);
    }

    for (int i = 0; i < 100; i++) {
        add_gaussian_blur(ranges);
    }

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            merge_map_top(top, center, width, height, ranges, i, j);
            merge_map_right(right, center, width, height, ranges, i, j);
            merge_map_bot(bot, center, width, height, ranges, i, j);
            merge_map_left(left, center, width, height, ranges, i, j);
        }
    }
}

