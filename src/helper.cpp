#include "helper.h"

bool random_percentage(std::mt19937& gen, const int percent) {
    std::uniform_int_distribution<int> dis(1, 100);
    return dis(gen) <= percent;
}

float calc_distance(const int x, const int y, const int half_size, const int i, const int j) {
    int diff_x = x - (x - half_size + i);
    int diff_y = y - (y - half_size + j);

    return std::sqrt(diff_x * diff_x + diff_y * diff_y);
}

void add_gaussian_blur(image<float>& img) {
    image<float> copy(img.width(), img.height(), 0.0);

    for (int j = 1; j < copy.height() - 1; j++) {
        for (int i = 1; i < copy.width() - 1; i++) {
            auto pixel = img.at(i, j);

            copy.at(i - 1 + 0, j - 1 + 0) += pixel * 1;
            copy.at(i - 1 + 1, j - 1 + 0) += pixel * 2;
            copy.at(i - 1 + 2, j - 1 + 0) += pixel * 1;
            copy.at(i - 1 + 0, j - 1 + 1) += pixel * 2;
            copy.at(i - 1 + 1, j - 1 + 1) += pixel * 4;
            copy.at(i - 1 + 2, j - 1 + 1) += pixel * 2;
            copy.at(i - 1 + 0, j - 1 + 2) += pixel * 1;
            copy.at(i - 1 + 1, j - 1 + 2) += pixel * 2;
            copy.at(i - 1 + 2, j - 1 + 2) += pixel * 1;
        }
    }

    copy.for_each_pixel([](float& p) {
        p /= 16;
    });

    for (int j = 0; j < copy.height(); j++) {
        copy.at(0, j) = img.at(0, j);
        copy.at(copy.width() - 1, j) = img.at(img.width() - 1, j);
    }
    for (int i = 0; i < copy.width(); i++) {
        copy.at(i, 0) = img.at(i, 0);
        copy.at(i, copy.height() - 1) = img.at(i, copy.height() - 1);
    }

    img = std::move(copy);
}

void scale_range(image<float>& img) {
    float min = MAXFLOAT;
    float max = 0;

    img.for_each_pixel([&](float& f) {
        min = std::min<float>(min, f);
        max = std::max<float>(max, f);
    });
    img.for_each_pixel([&](float& f) {
        f = (f - min) * (1.0 / (max - min));
    });
}

std::pair<int, int> min_random_neighbor(std::mt19937& gen, const image<float>& map, const int x, const int y) {
    std::pair<int, int> result = {x, y};
    float min = map.at(x, y);

    for (int dy = -1; dy < 2; dy++) {
        for (int dx = -1; dx < 2; dx++) {
            if (map.contains(x + dx, y + dy)) {
                if (map.at(x + dx, y + dy) < min && random_percentage(gen, 50)) {
                    result = {x + dx, y + dy};
                    min = map.at(x + dx, y + dy);
                }
            }

        }
    }
    
    return result;
}

std::pair<int, int> min_neighbor(const image<float>& map, const int x, const int y) {
    std::pair<int, int> result = {x, y};
    float min = map.at(x, y);

    for (int dy = -1; dy < 2; dy++) {
        for (int dx = -1; dx < 2; dx++) {
            if (map.contains(x + dx, y + dy)) {
                if (map.at(x + dx, y + dy) < min) {
                    result = {x + dx, y + dy};
                    min = map.at(x + dx, y + dy);
                }
            }

        }
    }
    
    return result;
}

image<float> downscale_img(const image<float>& src, const size_t dest_width, const size_t dest_height) {
    image<float> result(dest_width, dest_height, 0.0);

    result.for_each_pixel([&](float& p, const size_t x, const size_t y) {
        auto vertical_fraction = static_cast<double>(y) / dest_height;
        auto horizontal_fraction = static_cast<double>(x) / dest_width;

        auto src_x = static_cast<size_t>(horizontal_fraction * src.width());
        auto src_y = static_cast<size_t>(vertical_fraction * src.height());

        if (src.contains(src_x, src_y)) {
            p = src.at(src_x, src_y);
        }
    });

    add_gaussian_blur(result);
    add_gaussian_blur(result);
    add_gaussian_blur(result);
    add_gaussian_blur(result);

    return result;
}

image<float> extract_non_zero_region(const image<float>& img) {
    size_t min_x = img.width();
    size_t max_x = 0;
    size_t min_y = img.height();
    size_t max_y = 0;

    img.for_each_pixel([&](const float& p, const size_t x, const size_t y) {
        if (p > 0) {
            min_x = std::min(min_x, x);
            max_x = std::max(max_x, x);
            min_y = std::min(min_y, y);
            max_y = std::max(max_y, y);
        }
    });
    
    image<float> result(max_x, max_y);

    if (auto region = img.subregion(0, 0, max_x, max_y)) {
        region->copy_to(result);
    }

    return result;
}

image<float> extract_background(const image<float>& img, const float threshold) {
    auto base_background = img;
    base_background.for_each_pixel([threshold](float& f) {
        if (f > threshold) {
            f = threshold;
        }
    });

    return extract_non_zero_region(base_background);
}
