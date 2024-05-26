#include "generate_world.h"

#include "helper.h"
#include "generate_rivers.h"
#include "generate_terrain.h"
#include "generate_multi_terrain.h"

void reshape_hills_on_map(image<float>& map) {
    for (int i = 0; i < 1; i++) {
        map.for_each_pixel([](float& m) {
            if (m > 0.05 && m < 1.0) {
                m = 0.05 + (m - 0.05) * (m - 0.05);
            }
        });

        scale_range(map);
    }

    std::vector<int> lut(256);
    for (int i = 0; i < 64; i++) {
        lut[i] = i;
    }
    for (int i = 0; i < 6 * 32; i++) {
        float m = 3.0 / 5.0;
        lut[64 + i] = 64 + m * i;
    }

    map.for_each_pixel([&](float& m) {
        m = std::min(1.0F, std::max(m, 0.0F));

        auto byte = static_cast<int>(m * 255);
        auto next = static_cast<float>(lut[byte]) / 255;
        m = next;
    });

    map.for_each_pixel([](float& m) {
        m = m * m;
    });
    scale_range(map);

    add_gaussian_blur(map);
    add_gaussian_blur(map);
    add_gaussian_blur(map);
    add_gaussian_blur(map);
}

image<float> add_noise_with_terrains(const image<float>& map, const int width, const int height, const int scale) {
    auto result = generate_multi_terrain(width, height, 2 * scale, 2 * scale);

    auto copy = map;

    add_gaussian_blur(copy);
    add_gaussian_blur(copy);
    scale_range(copy);

    result.for_each_pixel([&](auto& p, auto x, auto y) {
        if (copy.contains(x, y)) {
            auto s = copy.at(x, y);

            if (s > 0) {
                p = (s + s * p) / 2;
            } else {
                p = 0;
            }
        }
    });

    if (auto sub = result.subregion(0, 0, result.width() / 2, result.height() / 2)) {
        result = *sub;
    }

    scale_range(result);

    return result;
}

std::vector<image<float>> generate_circle_stack(const int n) {
    image<float> circle(n, n);
    circle.for_each_pixel([&](auto& f, auto x, auto y) {
        auto dx = std::abs(static_cast<int>(x) - (n / 2));
        auto dy = std::abs(static_cast<int>(y) - (n / 2));
        auto distance = std::sqrt(dx * dx + dy * dy);

        if (distance > (n / 2)) {
            f = 0;
        } else {
            f = 1.0;
        }
    });

    std::vector<image<float>> circles;

    for (int i = 0; i < (n - 1); i++) {
        circles.push_back(circle.rescale(i + 1, i + 1));
    }

    return circles;
}

image<float> add_circular_trace(const image<float>& map, const image<float>& color) {
    image<float> result(map.width(), map.height());
    
    auto circles = generate_circle_stack(256);

    map.for_each_pixel([&](auto f, auto x, auto y) {
        if (f > 0) {
            auto val = std::min(255.0F, f * 256) / 2;
            auto src = circles[val];
            src.for_each_pixel([&](auto& f) {
                f *= color.at(x, y);
            });
            if (auto dest = result.subregion(x - src.width() / 2, y - src.height() / 2, src.width(), src.height())) {
                dest->for_each_pixel([&](auto& f, auto x, auto y) {
                    if (src.at(x, y) > 0) {
                        if (f == 0) {
                            f = src.at(x, y);
                        } else {
                            f = std::min(f, src.at(x, y));
                        }
                    }
                });
            }
        }
    });

    return result;
}

image<float> generate_scale_circle(const int n) {
    image<float> circle(n, n);
    circle.for_each_pixel([&](auto& f, auto x, auto y) {
        auto half = n / 2;
        auto dx = std::abs(static_cast<int>(x) - half);
        auto dy = std::abs(static_cast<int>(y) - half);
        auto distance = std::sqrt(dx * dx + dy * dy);

        if (distance > half) {
            f = 1.0;
        } else {
            f = distance / static_cast<float>(half);
        }
    });

    circle.for_each_pixel([&](auto& f) {
        auto s = 1.0f - f;
        auto m = s * s;
        f = 1.0f - m;
    });
    circle.for_each_pixel([&](auto& f) {
        auto s = 1.0f - f;
        auto m = s * s;
        f = 1.0f - m;
    });

    return circle;
}

void apply_mask_on_terrain(image<float>& map, const image<float>& mask) {
    image<float> scale_map(map.width(), map.height(), 1.0);
    image<float> baseline(map.width(), map.height(), 1.0);

    auto circle = generate_scale_circle(64);

    map.for_each_pixel([&](auto&, int x, int y) {
        if (!mask.contains(x, y) || !scale_map.contains(x, y) || !baseline.contains(x, y)) {
            return;
        }

        auto r = mask.at(x, y);

        if (r > 0) {
            circle.for_each_pixel([&](auto f, auto cx, auto cy) {
                auto dest_x = x - circle.width() / 2 + cx;
                auto dest_y = y - circle.height() / 2 + cy;

                if (scale_map.contains(dest_x, dest_y)) {
                    scale_map.at(dest_x, dest_y) = std::min(scale_map.at(dest_x, dest_y), f);
                }
                if (baseline.contains(dest_x, dest_y)) {
                    if (f < 1.0) {
                        baseline.at(dest_x, dest_y) = std::min(baseline.at(dest_x, dest_y), r);
                    }
                }
            });
        }
    });
    map.for_each_pixel([&](auto& f, int x, int y) {
        if (!baseline.contains(x, y)) {
            return;
        }
        f = f * scale_map.at(x, y) + baseline.at(x, y) * (1.0f - scale_map.at(x, y));
    });
}

image<float> random_pattern_tile(const std::vector<image<float>>& terrains, std::mt19937& gen, const int width, const int height) {
    std::uniform_int_distribution<int> dis_terrain(0, terrains.size() - 1);

    auto tile = terrains[dis_terrain(gen)];
    std::uniform_real_distribution<float> dis(0.1, 0.7);

    auto threshold = dis(gen);

    if (threshold == 0) {
        threshold = 0.1;
    }
    tile.for_each_pixel([&](float& m) {
        if (m > threshold) {
            //m = 1.0;
        } else {
            m = threshold;
        }
    });

    scale_range(tile);

    return tile;
}
image<float> generate_segment(const std::vector<image<float>>& terrains, const int width, const int height) {
    std::random_device rd;
    std::mt19937 gen(rd());

    image<float> map(width * 2, height * 2);
    std::uniform_int_distribution<int> dis_width(0, width);
    std::uniform_int_distribution<int> dis_height(0, height);
    std::uniform_real_distribution<float> dis(0, 1.0);

    for (int i = 0; i < 128; i++) {
        auto tile = random_pattern_tile(terrains, gen, width, height);
        auto b = dis(gen);
        auto x = dis_width(gen);
        auto y = dis_height(gen);

        if (auto subregion = map.subregion(x, y, width, height)) {
            subregion->for_each_pixel([&](auto& c, auto x, auto y) {
                //c = std::max(c, b * tile.at(x, y));
                c += std::max(0.0f, b * tile.at(x, y));
            });
        }
    }
    scale_range(map);
    return map;
}

#include <thread>
#include <mutex>
#include <iostream>
image<float> generate_base_world(const int width, const int height, const int scale, const float fade_factor, const float threshold, const float noise_factor) {
    std::random_device rd;
    std::mt19937 gen(rd());

    image<float> map(width * 2 * scale, height * 2 * scale);

    std::vector<image<float>> tiles;
    std::vector<image<float>> terrains;
    std::vector<std::thread> threads;
    std::mutex mutex;

    int target = 512; //scale * scale;
    
    for (int i = 0; i < std::min(8, target); i++) {
        std::thread t1([&]() {
            std::unique_lock ul(mutex);
            while (true) {
                if (terrains.size() > target) {
                    return;
                }
                ul.unlock();
                image<float> terrain = generate_terrain(width, height);
                ul.lock();
                terrains.push_back(std::move(terrain));
                std::cout << terrains.size() << std::endl;
            }
        });

        threads.push_back(std::move(t1));
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    for (int i = 0; i < scale * scale; i++) {
        tiles.push_back(generate_segment(terrains, width, height));
    }

    int i = 0;
    for (int y = 0; y < scale; y++) {  
        for (int x = 0; x < scale; x++) {            
            image<float>& tile = tiles[i];
            i++;
            if (auto region = map.subregion(
                x * tile.width() - 512 * x, 
                y * tile.height() - 512 * y,
                tile.width(),
                tile.height()
            )) {
                (*region).add(tile);
            }
        }
    }
    scale_range(map);

    if (auto result = map.subregion(0, 0, (scale + 1) * width, (scale + 1) * height)) {
        return *result;
    }
    return map;
}

void apply_generated_rivers(const int width, const int height, image<float>& map, const int scale) {
    auto rescaled = map.rescale(width, height);
    auto [rivers, hatches, river_heights] = generate_rivers(rescaled, scale);

    auto final_river = add_circular_trace(rivers, river_heights);
    apply_mask_on_terrain(map, final_river);
}
