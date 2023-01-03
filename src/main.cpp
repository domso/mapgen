#include <vector>
#include <iostream>

#include <random>

#include "export_ppm.h"
#include "generate_terrain.h"
#include "generate_water.h"
#include "merge_map.h"
#include "helper.h"

#include "config.h"

#include "../../cmd/loading_bar.h"

void insert_as_tile(image<float>& map, const int row, const int col, image<float>& terrain) {    
    if (auto region = map.subregion(
        col * terrain.width() - config::terrain::max_hill_size * col, 
        row * terrain.height() - config::terrain::max_hill_size * row,
        terrain.width(),
        terrain.height()
    )) {
        (*region).add(terrain);
    }
}

#include <unordered_map>

std::vector<image<float>> generate_tiles(image<float>& map, image<int>& mask) {
    std::vector<image<float>> result;
    std::unordered_map<int, std::pair<std::pair<int, int>, std::pair<int, int>>> position;

    for (int y = 0; y < mask.height(); y++) {
        for (int x = 0; x < mask.width(); x++) {
            auto d = mask.at(x, y);
            if (d > 0) {
                auto find = position.find(d);
                if (find == position.end()) {
                    position.insert({d, {{x, y}, {x, y}}});
                } else {
                    auto& [key, value] = *find;
                    auto& [start, end] = value;
                    auto& [start_x, start_y] = start;
                    auto& [end_x, end_y] = end;

                    start_x = std::min(start_x, x);
                    start_y = std::min(start_y, y);
                    end_x = std::max(end_x, x);
                    end_y = std::max(end_y, y);
                }
            }
        }
    }

    for (const auto& [key, value] : position) {
        auto& [start, end] = value;
        auto& [start_x, start_y] = start;
        auto& [end_x, end_y] = end;

        if (end_x - start_x > 5 && end_y - start_y > 5) {
            auto blurs = 4;
            auto w = end_x - start_x + 1 + 2 * blurs;
            auto h = end_y - start_y + 1 + 2 * blurs;

            image<float> img(w, h, 0.0);
            for (int y = start_y; y <= end_y; y++) {
                for (int x = start_x; x <= end_x; x++) {
                    if (mask.at(x, y) == key) {
                        img.at(blurs + x - start_x, blurs + y - start_y) = map.at(x, y);
                    }
                }
            }
            for (int i = 0; i < blurs; i++) {
                add_gaussian_blur(img);
            }
            result.push_back(img);
        }
    }

    return result;
}


#include <cmath>
#include <random>

image<float> place_tiles(const size_t width, const size_t height, const std::vector<image<float>>& tiles, const image<float>& background) {
    size_t max_x = width;
    size_t max_y = height;
    image<float> result(width, height, 0.0);
    std::vector<std::pair<int, int>> positions;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis_width(0, max_x - 1);
    std::uniform_int_distribution<int> dis_height(0, max_y - 1);

    size_t current_tile = 0;
    size_t current_placed_tile = 0;

    size_t i = 1;
    cmd::loading_bar loading_bar;
    
    loading_bar.init("Place tiles", tiles.size());
    while (current_tile < tiles.size()) {
        auto current_x = dis_width(gen);
        auto current_y = dis_height(gen);
        auto& tile = tiles[current_tile];

        if (tile.width() < 500 || tile.height() < 500) {
            current_tile++;
            loading_bar.step();
            continue;
        }

        auto current_width = max_x / 1 / (1 + current_placed_tile) - i;
        auto current_height = max_x / 1 / (1 + current_placed_tile) - i;
        std::cout << "(" << current_x << ", " << current_y << ") -- (" << current_width << "," << current_height << ")" << max_x << " " << current_placed_tile << " " << i << std::endl;
        i = std::min(i + 1, max_x / 1 / (1 + current_placed_tile) - 1);

        float scale;
        if (tile.width() > tile.height()) { 
            scale = static_cast<float>(current_width) / static_cast<float>(tile.width());
            current_height = tile.height() * scale;
        } else {
            scale = static_cast<float>(current_height) / static_cast<float>(tile.height());
            current_width = tile.width() * scale;
        }

        if (auto region = result.subregion(current_x, current_y, current_width, current_height)) {
            bool valid = true;
            (*region).check_each_pixel([&](float& p) {
                if (p != 0) {
                    valid = false;
                }
                return valid;
            });

            if (valid) {
                (*region).for_each_pixel([&](float& p, const size_t x, const size_t y) {
                    auto src_x = static_cast<size_t>(x / scale);
                    auto src_y = static_cast<size_t>(y / scale);

                    if (tile.contains(src_x, src_y) && tile.at(src_x, src_y) > 0) {
                        float scaled_data = 0.50 + (tile.at(src_x, src_y) - 0.50) / (1 + current_placed_tile);
                        p = std::max(p, scaled_data);
                    }
                });

                current_tile++;
                current_placed_tile++;
                i = std::min(i, max_x / 1 / (1 + current_placed_tile) - 1);
                loading_bar.step();
                if (current_tile == tiles.size()) {
                    break;
                }
            }
        }

    }
    loading_bar.finalize();
    
    current_tile = 0;
    std::cout << "start filling remaining tiles" << std::endl;

    auto scaled_tiles = tiles;
    for (auto& tile : scaled_tiles) {
        tile.for_each_pixel([](float& p) {
            if (p > 0) {
                p = std::max(0.0, p - 0.5);
            }
        });
    }

    loading_bar.init("Place remaining tiles", scaled_tiles.size());
    while(current_placed_tile > 0 && current_tile < scaled_tiles.size()) {
        auto current_x = dis_width(gen);
        auto current_y = dis_height(gen);
        auto& tile = scaled_tiles[current_tile];

        if (auto region = result.subregion(current_x, current_y, tile.width(), tile.height())) {
            auto [current_min, current_max] = (*region).range();

            if (current_max == 0) {
                continue;
            }
        
            float delta = current_max - current_min; 

            (*region).for_each_pixel([&](float& p, const size_t x, const size_t y) {
                if (tile.at(x, y) > 0) {
                    p += 0.05 * tile.at(x, y);
                }
            });

            current_tile++;
            loading_bar.step();
        }
    }
    loading_bar.finalize();
    scale_range(result);

    auto current_x = 0;
    auto current_y = 0;
    
    auto current_width = max_x;
    auto current_height = max_y;

    float scale;
    if (background.width() > background.height()) { 
        scale = static_cast<float>(current_width) / static_cast<float>(background.width());
        current_height = background.height() * scale;
    } else if (background.width() < background.height()) {
        scale = static_cast<float>(current_height) / static_cast<float>(background.height());
        current_width = background.width() * scale;
    } else {
       scale = static_cast<float>(current_height) / static_cast<float>(background.height());
    }

    if (auto region = result.subregion(current_x, current_y, current_width, current_height)) {
        (*region).for_each_pixel([&](float& p, const size_t x, const size_t y) {
            auto src_x = static_cast<size_t>(x / scale);
            auto src_y = static_cast<size_t>(y / scale);

            if (background.contains(src_x, src_y)) {
                p += 0.05 * background.at(src_x, src_y);
            }
        });
    }

    scale_range(result);

    return result;
}

image<std::pair<float, int>> generate_path(const image<float>& terrain, const size_t start_x, const size_t start_y) {
    image<std::pair<float, int>> result(terrain.width(), terrain.height(), {std::numeric_limits<float>::infinity(), 0});
    int tile_offset = 1;
    result.at(start_x, start_y) = {0.0, 0};

    bool changed = true;
    int min_x = result.width();
    int min_y = result.height();
    int max_x = 0;
    int max_y = 0;

    while (changed) {
        changed = false;

        int old_min_x = min_x;
        int old_min_y = min_y;
        int old_max_x = max_x;
        int old_max_y = max_y;

        for (int y = std::max(0UL, start_y - tile_offset); y < std::min(result.height(), start_y + tile_offset + 1); y++) {
            for (int x = std::max(0UL, start_x - tile_offset); x < std::min(result.width(), start_x + tile_offset + 1); x++) {
                if (
                    y == std::max(0UL, start_y - tile_offset) ||
                    y == std::min(result.height(), start_y + tile_offset + 1) - 1 ||
                    x == std::max(0UL, start_x - tile_offset) ||
                    x == std::min(result.width(), start_x + tile_offset + 1) - 1 ||
                    (old_min_x - 1 <= x && x <= old_max_x + 1) ||
                    (old_min_y - 1 <= y && y <= old_max_y + 1)
                ) {
                    auto delta = terrain.at(x, y);
                    delta = delta * delta * delta * delta;

                    bool update = false;
                    if (delta > 0) {
                        auto [score, step] = result.at(x, y);
                        if (y > 0) {
                            auto [neighbor_score, neighbor_step] = result.at(x, y - 1);

                            if (neighbor_score + delta < score) {
                                score = neighbor_score + delta;
                                step = 1;
                                changed = true;
                                update = true;
                            }
                        }
                        if (x < result.width() - 1) {
                            auto [neighbor_score, neighbor_step] = result.at(x + 1, y);

                            if (neighbor_score + delta < score) {
                                score = neighbor_score + delta;
                                step = 2;
                                changed = true;
                                update = true;
                            }
                        }
                        if (y < result.height() - 1) {
                            auto [neighbor_score, neighbor_step] = result.at(x, y + 1);

                            if (neighbor_score + delta < score) {
                                score = neighbor_score + delta;
                                step = 3;
                                changed = true;
                                update = true;
                            }
                        }
                        if (x > 0) {
                            auto [neighbor_score, neighbor_step] = result.at(x - 1, y);

                            if (neighbor_score + delta < score) {
                                score = neighbor_score + delta;
                                step = 4;
                                changed = true;
                                update = true;
                            }
                        }

                        if (update) {
                            min_x = std::min(min_x, x);
                            min_y = std::min(min_y, y);
                            max_x = std::max(max_x, x);
                            max_y = std::max(max_y, y);

                            result.at(x, y) = {score, step};
                        }
                    }
                }
            }
        }
        std::cout << "(" << min_x << ", " << min_y << ") -- (" << max_x << "," << max_y << ")" << std::endl;
        tile_offset++;
    }

    return result;
}



void export_path_img(image<float>& result, const image<float>& terrain, const image<std::pair<float, int>>& path_map, const int dest_x, const int dest_y) {
    int current_x = dest_x;
    int current_y = dest_y;

    float current_temp = static_cast<float>(dest_y) / result.height();

    bool running = true;
    while (running) {
        const auto& [score, step] = path_map.at(current_x, current_y);
        auto env_temp = static_cast<float>(current_y) / result.height();

        if (env_temp > current_temp) {
            current_temp = current_temp + (env_temp - current_temp) / 0.9;
        } else {
            current_temp = current_temp - (current_temp - env_temp) / 2;
        }
    
        if (terrain.at(current_x, current_y) > 0 && terrain.at(current_x, current_y) < 10.0) {
            result.at(current_x, current_y) = 1.0;
        }
        int margin = 100;

/*
        for (int i = -margin; i < margin; i++) {                                                                                                                                                                         
            for (int j = -margin; j < margin; j++) {
                auto n2 = current_y * width + current_x + j * width + i;
                    
                float quadlen = i * i + j * j;
                float m = static_cast<float>(margin);
                m *= m;

                if (n2 < result.size() && quadlen < m) {
                    float s = (m - quadlen);
                    s = s * s * s * s * s * s * s * s;
                    m = m * m * m * m * m * m * m * m;
                    

                    if (s > 0) {
                        result[n2] = std::max(result[n2], current_temp * (s / (m)));
                        //current_temp * (s / m);
                    }   
                }   
            }   
        }   
*/
        switch (step) {
            case 0: {running = false; break;}
            case 1: {current_y--; break;}
            case 2: {current_x++; break;}
            case 3: {current_y++; break;}
            case 4: {current_x--; break;}
            default: break;
        }
    }
/*
    for (int x = 0; x < width; x++) {
        bool found = true;
        int last = 0;
        for (int y = 0; y < height; y++) {
            if (result[y * width + x] > 0) {
                if (!found) {
                    found = true;
                    last = y;
                } else {
                    float delta = result[y * width + x] - result[last * width + x];
                    float steps = y - last;
                    float delta_per_step = delta / steps;

                    for (int ys = last; ys < y; ys++) {
                        result[ys * width + x] = result[last * width + x] + delta_per_step * (ys - last);
                    }
                    last = y;
                }
            }
        }
        if (found) {
            int y = height - 1;
            float delta = 1.0 - result[last * width + x];
            float steps = y - last;
            float delta_per_step = delta / steps;

            for (int ys = last; ys < y; ys++) {
                result[ys * width + x] = result[last * width + x] + delta_per_step * (ys - last);
            }
        }
    }

    add_gaussian_blur(result, width, height);
    add_gaussian_blur(result, width, height);
    add_gaussian_blur(result, width, height);
    add_gaussian_blur(result, width, height);
*/  
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

float shift_low(const float n) {
    return n * n;
}

image<int> generate_mask(const image<float>& terrain) {
    cmd::loading_bar loading_bar;
    loading_bar.init("Generate base mask", terrain.width() * terrain.height());
    image<int> mask(terrain.width(), terrain.height());
    int s = 1;
    int n = 0;
    terrain.for_each_pixel([&](const float& p, const size_t x, const size_t y) {
        if (p == 0) {
            if (n > 0) {
                s++;
            }
            n = 0;
            mask.at(x, y) = 0;
        } else {
            mask.at(x, y) = s;
            n++;
        }
        loading_bar.step();
    });
    loading_bar.finalize();
    int min;
    bool found = true;
    std::cout << "Start merging tiles" << std::endl;

    n = 0;
    while(found) {
        found = false;

        loading_bar.init("Merge round #" + std::to_string(n), (terrain.width() * (2 * terrain.height()) + terrain.height() * (2 * terrain.width())));
        for (int x = 0; x < terrain.width(); x++) {
            min = 0;
            for (int y = 0; y < terrain.height(); y++) {
                size_t dest_x = x;
                size_t dest_y = y;

                min = std::min(min, mask.at(dest_x, dest_y));

                if (mask.at(dest_x, dest_y) > 0 && min == 0) {
                    min = mask.at(dest_x, dest_y);
                }

                found = found || (mask.at(dest_x, dest_y) != min);

                mask.at(dest_x, dest_y) = min;
            }
            loading_bar.multi_step(terrain.height());
            min = 0;
            for (int y = 0; y < terrain.height(); y++) {
                size_t dest_x = x;
                size_t dest_y = terrain.height() - 1 - y;

                min = std::min(min, mask.at(dest_x, dest_y));

                if (mask.at(dest_x, dest_y) > 0 && min == 0) {
                    min = mask.at(dest_x, dest_y);
                }

                found = found || (mask.at(dest_x, dest_y) != min);
                mask.at(dest_x, dest_y) = min;
            }
            loading_bar.multi_step(terrain.height());
        }
        for (int y = 0; y < terrain.height(); y++) {
            min = 0;
            for (int x = 0; x < terrain.width(); x++) {
                size_t dest_x = x;
                size_t dest_y = y;

                min = std::min(min, mask.at(dest_x, dest_y));

                if (mask.at(dest_x, dest_y) > 0 && min == 0) {
                    min = mask.at(dest_x, dest_y);
                }

                found = found || (mask.at(dest_x, dest_y) != min);
                mask.at(dest_x, dest_y) = min;
            }
            loading_bar.multi_step(terrain.width());
            min = 0;
            for (int x = 0; x < terrain.width(); x++) {
                size_t dest_x = terrain.width() - 1 - x;
                size_t dest_y = y;

                min = std::min(min, mask.at(dest_x, dest_y));

                if (mask.at(dest_x, dest_y) > 0 && min == 0) {
                    min = mask.at(dest_x, dest_y);
                }

                found = found || (mask.at(dest_x, dest_y) != min);
                mask.at(dest_x, dest_y) = min;
            }
            loading_bar.multi_step(terrain.width());
        }
        loading_bar.finalize();
        n++;
        std::cout << std::endl;
    }

    loading_bar.init("Final mask generation", (terrain.width() * (2 * terrain.height()) + terrain.height() * (2 * terrain.width())));
    int incr_mask = 0;
    for (int y = 0; y < terrain.height(); y++) {
        int count = 0;
        int last;
        for (int x = 0; x < terrain.width(); x++) {
            size_t dest_x = x;
            size_t dest_y = y;
            if (mask.at(dest_x, dest_y) > 0) {
                count = incr_mask;
                last = mask.at(dest_x, dest_y);
            } else if (count > 0) {
                mask.at(dest_x, dest_y) = last;
                count--;
            }
        }
        loading_bar.multi_step(terrain.width());
        count = 0;
        for (int x = 0; x < terrain.width(); x++) {
            size_t dest_x = terrain.width() - 1 - x;
            size_t dest_y = y;
            if (mask.at(dest_x, dest_y) > 0) {
                count = incr_mask;
                last = mask.at(dest_x, dest_y);
            } else if (count > 0) {
                mask.at(dest_x, dest_y) = last;
                count--;
            }
        }
        loading_bar.multi_step(terrain.width());
    }
    for (int x = 0; x < terrain.width(); x++) {
        int count = 0;
        int last;
        for (int y = 0; y < terrain.height(); y++) {
            size_t dest_x = x;
            size_t dest_y = y;
            if (mask.at(dest_x, dest_y) > 0) {
                count = incr_mask;
                last = mask.at(dest_x, dest_y);
            } else if (count > 0) {
                mask.at(dest_x, dest_y) = last;
                count--;
            }
        }
        loading_bar.multi_step(terrain.height());
        count = 0;
        for (int y = 0; y < terrain.height(); y++) {
            size_t dest_x = x;
            size_t dest_y = terrain.height() - 1 - y;
            if (mask.at(dest_x, dest_y) > 0) {
                count = incr_mask;
                last = mask.at(dest_x, dest_y);
            } else if (count > 0) {
                mask.at(dest_x, dest_y) = last;
                count--;
            }
        }
        loading_bar.multi_step(terrain.height());
    }
    loading_bar.finalize();

    return mask;
}

#include "terrain_buffer.h"
int main() {
    int width = 512;
    int height = 512;

    int terrain_multiplier = 40;
    image<float> result(width * terrain_multiplier, height * terrain_multiplier);

    terrain_buffer buffer(width, height, 128);
    cmd::loading_bar loading_bar;

    
    loading_bar.init("Generate terrains", terrain_multiplier * terrain_multiplier);
    for (int i = 0; i < terrain_multiplier; i++) {  
        for (int j = 0; j < terrain_multiplier; j++) {            
            image<float> t = buffer.pop();
            insert_as_tile(result, i, j, t);
            loading_bar.step();
        }
    }
    buffer.stop();
    loading_bar.finalize();
    
    auto base_background  = result;
    base_background.for_each_pixel([](float& f) {
        if (f > 0.50) {
            f = 0.5;
        }
    });
    export_ppm("base.ppm", result);
    export_ppm("background.ppm", base_background);

    size_t base_background_min_x = width * terrain_multiplier;
    size_t base_background_max_x = 0;
    size_t base_background_min_y = height * terrain_multiplier;
    size_t base_background_max_y = 0;

    result.for_each_pixel([&](float& p, const size_t x, const size_t y) {
        if (p > 0) {
            base_background_min_x = std::min(base_background_min_x, x);
            base_background_max_x = std::max(base_background_max_x, x);
            base_background_min_y = std::min(base_background_min_y, y);
            base_background_max_y = std::max(base_background_max_y, y);
        }
    });
    
    image<float> cropped_background(base_background_max_x, base_background_max_y);

    if (auto region = base_background.subregion(0, 0, base_background_max_x, base_background_max_y)) {
        (*region).copy_to(cropped_background);
    }

    result.for_each_pixel([](float& f) {
        if (f <= 0.50) {
            f = 0.0;
        }
    });

    auto mask = generate_mask(result);
    auto tiles = generate_tiles(result, mask);
    std::cout << "place tiles" << std::endl;
    auto map = place_tiles(width * terrain_multiplier, height * terrain_multiplier, tiles, cropped_background);
    
    export_ppm("terrain_pre.ppm", map);

    tiles.clear();

    std::random_device rd;
    std::mt19937 gen(rd());
    add_gaussian_blur(map);
    add_gaussian_blur(map);
    add_gaussian_blur(map);
    add_gaussian_blur(map);

    map.for_each_pixel([](float& m) {
        m = m * m;
    });

    scale_range(map);
    map.for_each_pixel([](float& m) {
        m = m * m;
    });
    scale_range(map);

    auto scaled_map = downscale_img(map, width, height);
    std::cout << "save tiles" << std::endl;
    export_ppm("terrain.ppm", map);
    export_ppm("terrain_scaled.ppm", scaled_map);

    image<float> submap(10 * width, 10 * height);
    (*map.subregion(15 * width, 15 * height, 10 * width, 10 * height)).copy_to(submap);

    export_ppm("sub.ppm", submap);

    std::uniform_int_distribution<int> dis_width(0, submap.width() - 1);
    std::uniform_int_distribution<int> dis_height(0, submap.height() - 1);
    std::uniform_int_distribution<int> dis_choice(0, 1000);

    int current_path = 0;

    std::uniform_int_distribution<int> dis_noice(0, 10);
    submap.for_each_pixel([&](float& p) {
        if (p > 0.1) {
            p = p + dis_noice(gen) * 0.1;
        } else {
            p = 0.0;
        }
    });
    
    int dx, dy;
    do {
        dx = dis_width(gen);
        dy = dis_height(gen);
    } while (submap.at(dx, dy) == 0.0);

    auto path = generate_path(submap, dx, dy);

    image<float> path_result(submap.width(), submap.height(), 0);
    loading_bar.init("Generate paths", 1000);

    while (current_path < 1000) {
        auto x = dis_width(gen);
        auto y = dis_height(gen);
        auto dice = dis_choice(gen);
        auto p = submap.at(x, y);
        
        if (p > 0 && p * 1000 > dice) {
            export_path_img(path_result, submap, path, x, y);

            current_path++;
            loading_bar.step();
        }
    }
    loading_bar.finalize();

    export_ppm("path.ppm", path_result);

    return 0;
}
