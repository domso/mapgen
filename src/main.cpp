#include <vector>
#include <iostream>

#include <random>

#include "export_ppm.h"
#include "generate_terrain.h"
#include "generate_water.h"
#include "generate_mask.h"
#include "generate_placement.h"
#include "generate_multi_terrain.h"
#include "generate_tiles.h"
#include "merge_map.h"
#include "helper.h"

#include "config.h"

#include "../../cmd/loading_bar.h"




#include <cassert>
#include <cmath>
#include <random>


bool update_paths_from_neighbors(image<std::pair<float, int>>& positions, const size_t src_x, const size_t src_y, const size_t dest_x, const size_t dest_y, const float delta, const int delta_step) {
    auto [score, step] = positions.at(src_x, src_y);
    auto [neighbor_score, neighbor_step] = positions.at(dest_x, dest_y);

    if (neighbor_score + delta < score) {
        score = neighbor_score + delta;
        step = delta_step;
        positions.at(src_x, src_y) = {score, step};
        
        return true;
    }

    return false;
}

bool update_path_position(const image<float>& terrain, image<std::pair<float, int>>& positions, const size_t x, const size_t y) {
    auto delta = terrain.at(x, y);
    delta = delta * delta * delta * delta;

    bool update = false;
    if (delta >= 0) {
        if (y > 0 && x > 0) {
            update = update_paths_from_neighbors(positions, x, y, x - 1, y - 1, delta, 1) || update;
        }
        if (y > 0) {
            update = update_paths_from_neighbors(positions, x, y, x, y - 1, delta, 2) || update;
        }
        if (y > 0 && x < positions.width() - 1) {
            update = update_paths_from_neighbors(positions, x, y, x + 1, y - 1, delta, 3) || update;
        }
        if (x < positions.width() - 1) {
            update = update_paths_from_neighbors(positions, x, y, x + 1, y, delta, 4) || update;
        }
        if (y < positions.height() - 1 && x < positions.width() - 1) {
            update = update_paths_from_neighbors(positions, x, y, x + 1, y + 1, delta, 5) || update;
        }
        if (y < positions.height() - 1) {
            update = update_paths_from_neighbors(positions, x, y, x, y + 1, delta, 6) || update;
        }
        if (y < positions.height() - 1 && x > 0) {
            update = update_paths_from_neighbors(positions, x, y, x - 1, y + 1, delta, 7) || update;
        }
        if (x > 0) {
            update = update_paths_from_neighbors(positions, x, y, x - 1, y, delta, 8) || update;
        }
    }

    return update;
}


image<std::pair<float, int>> generate_path(const image<float>& terrain, size_t start_x, size_t start_y) {
    cmd::loading_bar loading_bar;
    image<std::pair<float, int>> result(terrain.width(), terrain.height(), {std::numeric_limits<float>::infinity(), 0});
    size_t tile_offset = 1;
    image<char> change_map(terrain.width() / 128, terrain.height() / 128);
    change_map.for_each_pixel([](char& b) {
        b = 0;
    });

    result.at(start_x, start_y) = {0.0, 0};
    change_map.at(start_x / 128, start_y / 128) = 1;

    bool changed = true;
    loading_bar.init("Generate paths", change_map.width() * change_map.height());
    int last_number_of_change = change_map.width() * change_map.height();

    while (changed) {
        changed = false;

        change_map.for_each_pixel([&](char& b, const int tx, const int ty) {
            while (b > 0) {
                b = 0;
                for (int y = 0; y < 128; y++) {
                    for (int x = 0; x < 128; x++) {
                        if (update_path_position(terrain, result, tx * 128 + x, ty * 128 + y)) {
                            b = 1;
                        }
                    }
                }

                if (b > 0) {
                    changed = true;
                    if (tx > 0) {
                        change_map.at(tx - 1, ty) = 1;
                    }
                    if (tx < change_map.width() - 1) {
                        change_map.at(tx + 1, ty) = 1;
                    }
                    if (ty > 0) {
                        change_map.at(tx, ty - 1) = 1;
                    }
                    if (ty < change_map.height() - 1) {
                        change_map.at(tx, ty + 1) = 1;
                    }
                }
            }
        });
        int n = 0;
        change_map.for_each_pixel([&](char& b) {
            if (b > 0) {
                n++;
            }
        });
        std::cout << n << std::endl;
        if (n < last_number_of_change) {
            loading_bar.multi_step(last_number_of_change - n);
            last_number_of_change = n;
        }

        tile_offset++;
    }
    loading_bar.finalize();

    return result;
}


void export_path_img(image<float>& result, image<float>& heights, const image<float>& terrain, const image<std::pair<float, int>>& path_map, const int dest_x, const int dest_y, const int scale) {
    int current_x = dest_x;
    int current_y = dest_y;
    int prev_x;
    int prev_y;

    float current_temp = static_cast<float>(dest_y) / result.height();

    bool running = true;
    bool skip_first = true;
    float current_height;

    while (running) {
        const auto& [score, step] = path_map.at(current_x, current_y);

        if (terrain.at(current_x, current_y) > 0.0) { 
            if (skip_first) {
                skip_first = false;
                current_height = terrain.at(current_x, current_y);
            } else if (result.at(current_x, current_y) >= 0.0) {
                bool skip_first_draw = true;
                result.draw_line(scale * prev_x, scale * prev_y, scale * current_x, scale * current_y, [&](auto& f, auto x, auto y) {
                    if (skip_first_draw) {
                        skip_first_draw = false;
                        return;
                    }

                    f += 1.0;
                });

                skip_first_draw = true;
                heights.draw_line(scale * prev_x, scale * prev_y, scale * current_x, scale * current_y, [&](auto& f, auto x, auto y) {
                    if (skip_first_draw) {
                        skip_first_draw = false;
                        return;
                    }

                    current_height = std::min(current_height, terrain.at(x / scale, y / scale));
                    if (f == 0) {
                        f = current_height;
                    } else {
                        f = std::min(f, current_height);
                    }
                });
            } else {
                return;
            }

            prev_x = current_x;
            prev_y = current_y;
        }
        switch (step) {
            case 0: {running = false; break;}
            case 1: {current_y--; current_x--; break;}
            case 2: {current_y--; break;}
            case 3: {current_y--; current_x++; break;}
            case 4: {current_x++; break;}
            case 5: {current_y++; current_x++; break;}
            case 6: {current_y++; break;}
            case 7: {current_y++; current_x--; break;}
            case 8: {current_x--; break;}
            default: break;
        }
    }
}


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



int add_rivers_on_map(image<float>& path_result, image<float>& hatch_result, image<float>& height_result, const image<float>& map, std::mt19937& gen, const size_t n, const int scale) {
    std::uniform_int_distribution<int> dis_width(0, map.width() - 1);
    std::uniform_int_distribution<int> dis_height(0, map.height() - 1);
    std::uniform_int_distribution<int> dis_choice(0, n);
    std::uniform_int_distribution<int> dis_noice(0, 10);

    auto copy = map;

    
    int dx, dy;
    int early_terminate = 100000;
    do {
        dx = dis_width(gen);
        dy = dis_height(gen);
        early_terminate--;
    } while (copy.at(dx, dy) > 0.1 && early_terminate > 0);

    if (early_terminate == 0) {
        std::cout << "early" << std::endl;
        return 0;
    }

    copy.for_each_pixel([&](float& p) {
        if (p > 0) {
            p = p + (0.01 + dis_noice(gen) * 0.009) * 1;
        }
    });
    std::cout << "generate path" << std::endl;
    auto path = generate_path(copy, dx, dy);
    std::cout << "complete" << std::endl;

    path.for_each_pixel([&](std::pair<float, int>& p, int x, int y) {
        if (copy.at(x, y) > 0.1) {
            hatch_result.at(x, y) = 0.25 * p.second;
        }
    });

    add_gaussian_blur(hatch_result);

    
   
    cmd::loading_bar loading_bar;

    early_terminate = 100000;
    int current_path = 0;
    loading_bar.init("Draw paths", n);
    while (current_path < n && early_terminate > 0) {
        auto x = dis_width(gen);
        auto y = dis_height(gen);
        auto dice = dis_choice(gen);
        auto p = copy.at(x, y);
        
        if (p > 0 && p * n > dice) {
            export_path_img(path_result, height_result, map, path, x, y, scale);

            current_path++;
            loading_bar.step();
            early_terminate = 100000;
        } else {
            early_terminate--;
        }
    }
    loading_bar.finalize();

    return current_path;
}

std::pair<image<float>, image<float>> generate_rivers(const image<float>& map) {
    cmd::loading_bar loading_bar;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis_width(0, map.width() - 1);
    std::uniform_int_distribution<int> dis_height(0, map.height() - 1);
    int scale = 20;
    image<float> rivers(scale * map.width(), scale * map.height(), 0.0);
    image<float> hatches(map.width(), map.height(), 0.0);
    image<float> heights(scale * map.width(), scale * map.height(), 0.0);


    add_rivers_on_map(rivers, hatches, heights, map, gen, map.width() * map.height() * 0.01, scale);

    scale_range(rivers);

    add_gaussian_blur(hatches);
    hatches.for_each_pixel([&](float& p, int x, int y) {
        p = map.at(x, y) + p;
    });
    scale_range(hatches);
    export_ppm("blub_hatch.ppm", hatches);

    return {rivers, heights};
}

std::vector<int> generate_grayscale_histogram(const image<float>& img) {
    std::vector<int> histogram(256);

    img.for_each_pixel([&](auto& p) {
        if (p < 1.0) {
            histogram[p * 256]++;            
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

int main() {
    int width = 512;
    int height = 512;
    auto base_map = generate_unfaded_terrain(width, height);

    apply_circular_fade_out(base_map, 0.4);
    apply_relative_threshold(base_map, 0.3);

    reshape_hills_on_map(base_map);

    int scale = 20;

    auto larger_map = base_map.rescale(scale * width, scale * height);

    auto result = add_noise_with_terrains(larger_map, width, height, scale);

    apply_relative_noise(result, 0.01);

    auto rescaled_result = result.rescale(width, height);
    auto [rivers, river_heights] = generate_rivers(rescaled_result);

    auto final_river = add_circular_trace(rivers, river_heights);
    apply_mask_on_terrain(result, final_river);

    export_ppm("final_map.ppm", result);

    return 0;
}

