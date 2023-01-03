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
