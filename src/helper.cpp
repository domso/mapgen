#include "helper.h"

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

image<float> mask_to_border_regions(const image<float>& image, const int regions) {
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

            if (!v.contains(x, y) && image.contains(x, y) && image.at(x, y) == 0) {
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

void invert_image(image<float>& img) {
    img.for_each_pixel([](auto& p) {
        p = 1.0f - p;
    });
}

image<float> resize_and_center(const image<float>& img, const size_t width, const size_t height) {
    image<float> result(width, height);

    auto dx = width - img.width();
    auto dy = height - img.height();

    result.for_each_pixel([&](auto& p, auto x, auto y) {
        auto sx = x - dx / 2;
        auto sy = y - dy / 2;

        if (auto s = img.get(sx, sy)) {
            p = **s;
        }
    });

    return result;
}

void draw_random_manhattan_line(image<float>& img, const int start_x, const int start_y, const int end_x, const int end_y, const float color) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0, 1.0);

    int x = start_x;
    int y = start_y;

    // Calculate total distances to move
    const int dx = std::abs(end_x - start_x);
    const int dy = std::abs(end_y - start_y);

    // Determine direction of movement
    const int sx = (start_x < end_x) ? 1 : -1;
    const int sy = (start_y < end_y) ? 1 : -1;

    // Draw first pixel
    if (img.contains(x, y)) {
        img.at(x, y) = color;
    }

    // Keep track of error to decide whether to move horizontally or vertically
    float error = 0.0f;
    const float slope = (dx == 0) ? std::numeric_limits<float>::infinity() : static_cast<float>(dy) / static_cast<float>(dx);

    while (x != end_x || y != end_y) {
        if (x == end_x) {
            // Only vertical movement remaining
            y += sy;
        }
        else if (y == end_y) {
            // Only horizontal movement remaining
            x += sx;
        }
        else {
            // Decide whether to move horizontally or vertically
            const float error_if_horizontal = std::abs(slope - (static_cast<float>(std::abs(y - start_y)) / static_cast<float>(std::abs(x + sx - start_x))));
            const float error_if_vertical = std::abs(slope - (static_cast<float>(std::abs(y + sy - start_y)) / static_cast<float>(std::abs(x - start_x))));

            auto dice = dis(gen);
            if (dice < 0.3) {
                x += sx;
            } else if (dice < 0.6) {
                y += sy;
            } else {
                if (error_if_horizontal < error_if_vertical) {
                    x += sx;
                } else {
                    y += sy;
                }
            }
        }

        if (img.contains(x, y)) {
            img.at(x, y) = color;
        }
    }
}

void fade_borders(image<float>& map, const int range) {
    for (int y = 0; y < map.height(); y++) {
        for (int x = 0; x < range; x++) {
            double scale = static_cast<double>(x) / static_cast<double>(range);
            map.at(x, y) *= scale;
        }
    }
    for (int y = 0; y < range; y++) {
        for (int x = 0; x < map.width(); x++) {
            double scale = static_cast<double>(y) / static_cast<double>(range);
            map.at(x, y) *= scale;
        }
    }    
    
    for (int y = map.height() - range; y < map.height(); y++) {
        for (int x = 0; x < map.width(); x++) {
            double scale = static_cast<double>(map.height() - y) / static_cast<double>(range);
            map.at(x, y) *= scale;
        }
    }    
    
    for (int y = 0; y < map.height(); y++) {
        for (int x = map.width() - range; x < map.width(); x++) {
            double scale = static_cast<double>(map.width() - x) / static_cast<double>(range);
            map.at(x, y) *= scale;
        }
    }    
}

image<int> generate_ocean_distance_map(const image<float>& mask) {
    image<int> result(mask.width(), mask.height());

    std::queue<std::pair<int, int>> remaining;

    mask.for_each_pixel([&](auto p, int x, int y) {
        if (p == 0) {
            result.at(x, y) = 0;
            remaining.push({x, y});
        } else {
            result.at(x, y) = mask.width() * mask.height();
        }
    });

    auto check_pos = [&](const auto current, const auto x, const auto y) {
        if (auto p = result.get(x, y)) {
            if (current + 1 < **p) {
                **p = current + 1;
                remaining.push({x, y});
            }
        }
    };

    while (!remaining.empty()) {
        auto [x, y] = remaining.front();
        auto current = result.at(x, y);
        check_pos(current, x + 1, y);
        check_pos(current, x - 1, y);
        check_pos(current, x, y + 1);
        check_pos(current, x, y - 1);
        remaining.pop();
    }

    return result; 
}
