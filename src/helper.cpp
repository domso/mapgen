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
    std::vector<int> kernel = {1, 2, 1, 2, 4, 2, 1, 2, 1};

    for (int j = 1; j < copy.height() - 1; j++) {
        for (int i = 0; i < copy.width(); i++) {
            int i_is_first = static_cast<int>(i == 0);
            int i_is_last = static_cast<int>(i == copy.width() - 1);
            int j_is_first = static_cast<int>(j == 0);
            int j_is_last = static_cast<int>(j == copy.height() - 1);

            int w = j_is_first * 3;
            float factor = 0.0;
            for (int kj = -1 + j_is_first; kj <= 1 - j_is_last; kj++) {
                w += i_is_first;
                for (int ki = -1 + i_is_first; ki <= 1 - i_is_last; ki++) {
                    copy.at(i, j) += img.at(i + ki, j + kj) * kernel[w];
                    factor += kernel[w];
                    w++;
                }
                w += i_is_last;
            }
            copy.at(i, j) /= factor;
        }
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

std::vector<int> generate_grayscale_histogram(const image<float>& img) {
    std::vector<int> histogram(256);

    img.for_each_pixel([&](auto& p) {
        if (p <= 1.0) {
            histogram[p * 255]++;            
        }
    });

    return histogram;
}

void apply_relative_threshold(image<float>& img, const float factor) {
    int threshold = 255;
    int sum = 0;

    std::vector<int> histogram = generate_grayscale_histogram(img);

    while (img.width() * img.height() * factor > sum && threshold >= 0) {
        sum += histogram[threshold];
        threshold--;
    }

    img.for_each_pixel([&](auto& p) {
        if (p * 256 < threshold) {
            p = 0;
        }
    });
}

void apply_circular_fade_out(image<float>& img, const float factor) {
    img.for_each_pixel([&](auto& p, auto x, auto y) {
        auto dx = std::abs(static_cast<int>(x) - static_cast<int>(img.width()) / 2);
        auto dy = std::abs(static_cast<int>(y) - static_cast<int>(img.height()) / 2);
        auto distance = std::sqrt(dx * dx + dy * dy);

        auto start = std::min(img.width(), img.height()) * factor;
        auto end = std::min(img.width(), img.height());
        auto border = end - start;

        if (distance > start) {
            auto scale = 1.0 - (distance - start) / border;
            p *= scale;            
        }
    });
}

void apply_relative_noise(image<float>& img, const float factor) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis_noice(0, 10);

    img.for_each_pixel([&](float& p) {
        if (p > 0) {
            p = p + p * (dis_noice(gen) * factor);
        }
    });

    scale_range(img);
}

image<float> mask_to_regions(const image<float>& image, const int regions) {
    voronoi_generator v;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis_x(0, image.width());
    std::uniform_int_distribution<int> dis_y(0, image.height());

    for (int i = 0; i < regions; i++) {
        bool retry = true;
        int n = 0;

        while (retry && n < 1000000) {
            auto x = dis_x(gen);
            auto y = dis_y(gen);

            if (!v.contains(x, y) && image.contains(x, y) && image.at(x, y) != 0) {
                retry = false;
                v.add(x, y);                
            }
            n++;
        }
    }
    auto result = v.generate(image.width(), image.height());

    result.for_each_pixel([&](auto& p, auto x, auto y) {
        if (image.at(x, y) == 0) {
            p = 0;
        }
    });

    return result;
}

int random_integer(const int min, const int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(min, max);
    return dis(gen);
}

float random_float(const float min, const float bound) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(min, bound);
    return dis(gen);
}

void quantify_image_pre_zero(image<float>& image, const int levels) {
    scale_range(image);

    auto scale = 1.0f / levels;

    image.for_each_pixel([&](auto& p) {
        if (p == 0 || p == 1.0) {
            return;
        }
        auto ratio = static_cast<int>(p / scale) + 1;
        p = std::min(1.0f, ratio * scale);
    });
}

std::pair<image<float>, image<float>> generate_river_level_and_height(const image<float>& rivers, const image<float>& weights, const std::vector<std::pair<size_t, size_t>>& river_endpoints) {
    assert(rivers.size() == weights.size());

    image<float> levels(rivers.width(), rivers.height());
    image<float> heights(rivers.width(), rivers.height());

    for (auto [start_x, start_y] : river_endpoints) {
        std::queue<std::pair<int, int>> queue;
        std::vector<std::pair<int, int>> history;
        queue.push({start_x, start_y});
        levels.at(start_x, start_y) = -1.0f;
        history.push_back({start_x, start_y});

        auto queue_update = [&](auto x, auto y) {
            auto s = levels.get(x, y);
            auto r = rivers.get(x, y);
            if (s && r && **s == 0 && **r == rivers.at(start_x, start_y)) {
                **s = -1.0f;
                queue.push({x, y});
                return true;
            }
            return false;
        };

        while (!queue.empty()) {
            auto [current_x, current_y] = queue.front();
            auto update = false;
            update = queue_update(current_x + 1, current_y) || update;
            update = queue_update(current_x - 1, current_y) || update;
            update = queue_update(current_x, current_y + 1) || update;
            update = queue_update(current_x, current_y - 1) || update;

            if (update) {
                history.push_back({current_x, current_y});
            } else {
                levels.at(current_x, current_y) = 1.0f;
                heights.at(current_x, current_y) = weights.at(current_x, current_y);
            }
            
            queue.pop();
        }

        auto level_update = [&](auto x, auto y) {
            if (auto s = levels.get(x, y)) {
                if (**s > 0) {
                    return **s;
                }
            }
            return 0.0f;
        };

        auto height_update = [&](auto x, auto y) {
            if (auto s = levels.get(x, y)) {
                if (**s > 0) {
                    return weights.at(x, y);
                }
            }
            return std::numeric_limits<float>::infinity();
        };

        for (auto it = history.rbegin(); it != history.rend(); it++) {
            auto [current_x, current_y] = *it;
            levels.at(current_x, current_y) = 
                level_update(current_x + 1, current_y) +
                level_update(current_x - 1, current_y) +
                level_update(current_x, current_y + 1) +
                level_update(current_x, current_y - 1);
            heights.at(current_x, current_y) = std::min(std::min(std::min(std::min(
                height_update(current_x, current_y), 
                height_update(current_x + 1, current_y)), 
                height_update(current_x - 1, current_y)),
                height_update(current_x, current_y + 1)),
                height_update(current_x, current_y - 1));
        }
    }

    return {levels, heights};
}
