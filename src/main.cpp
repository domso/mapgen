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
        if (y > 0) {
            update = update_paths_from_neighbors(positions, x, y, x, y - 1, delta, 1) || update;
        }
        if (x < positions.width() - 1) {
            update = update_paths_from_neighbors(positions, x, y, x + 1, y, delta, 2) || update;
        }
        if (y < positions.height() - 1) {
            update = update_paths_from_neighbors(positions, x, y, x, y + 1, delta, 3) || update;
        }
        if (x > 0) {
            update = update_paths_from_neighbors(positions, x, y, x - 1, y, delta, 4) || update;
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
int margin = 100;

void export_path_img(image<float>& result, const image<float>& terrain, const image<std::pair<float, int>>& path_map, const int dest_x, const int dest_y) {
    int current_x = dest_x;
    int current_y = dest_y;

    float current_temp = static_cast<float>(dest_y) / result.height();

    bool running = true;
    bool skip_first = true;

    while (running) {
        const auto& [score, step] = path_map.at(current_x, current_y);
        auto env_temp = static_cast<float>(current_y) / result.height();

        if (env_temp > current_temp) {
            current_temp = current_temp + (env_temp - current_temp) / 0.9;
        } else {
            current_temp = current_temp - (current_temp - env_temp) / 2;
        }
    
        if (terrain.at(current_x, current_y) > 0.1) { 
            if (skip_first) {
                skip_first = false;
            } else if (result.at(current_x, current_y) >= 0.0) {
                result.at(current_x, current_y) = 1.0;
            } else {
                return;
            }
        }
        switch (step) {
            case 0: {running = false; break;}
            case 1: {current_y--; break;}
            case 2: {current_x++; break;}
            case 3: {current_y++; break;}
            case 4: {current_x--; break;}
            default: break;
        }
    }
}


void reshape_hills_on_map(image<float>& map) {
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
}



int add_rivers_on_map(image<float>& path_result, image<float>& hatch_result, const image<float>& map, std::mt19937& gen, const size_t n) {
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
        p = p + 0.1 + dis_noice(gen) * 0.2;
    });
    export_ppm("blub.ppm", copy);
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
            export_path_img(path_result, copy, path, x, y);

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

image<float> generate_rivers(const image<float>& map) {
    cmd::loading_bar loading_bar;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis_width(0, map.width() - 1);
    std::uniform_int_distribution<int> dis_height(0, map.height() - 1);
    image<float> rivers(map.width(), map.height(), 0.0);
    image<float> hatches(map.width(), map.height(), 0.0);

    add_rivers_on_map(rivers, hatches, map, gen, map.width() * map.height() * 0.01);
/*
    for (int j = 0; j < 7; j++) {
        for (int i = 0; i < 7; i++) {
            if (auto rivers_region = rivers.subregion(2560 * i, 2560 * j, 2560, 2560)) {
                auto map_region = map.subregion(2560 * i, 2560 * j, 2560, 2560);
                auto hatch_region = hatches.subregion(2560 * i, 2560 * j, 2560, 2560);
                rivers_region->for_each_pixel([](float& p) {
                    if (p > 0.0) {
                        p = -p;
                    }
                });
                auto placed_rivers = add_rivers_on_map(*rivers_region, *hatch_region, *map_region, gen, 2560 * 2560 * 0.01);
            }
        }
    }
    rivers.for_each_pixel([](float& p) {
        p = std::abs(p);
    });
    */
    scale_range(rivers);

    add_gaussian_blur(hatches);
    hatches.for_each_pixel([&](float& p, int x, int y) {
        p = map.at(x, y) + p;
    });
    scale_range(hatches);
    export_ppm("blub_hatch.ppm", hatches);

    return rivers;

    size_t num_placed_rivers = 0;
    loading_bar.init("Generate rivers", 10000);
    while (num_placed_rivers < 10000) {
        int x = dis_width(gen);
        int y = dis_height(gen);
        int width = map.width() * 0.1 + dis_width(gen);
        int height = map.height() * 0.1 + dis_height(gen);

        if (width > 0 && height > 0) {
            if (auto map_region = map.subregion(x, y, width, height)) {
                if (auto rivers_region = rivers.subregion(x, y, width, height)) {
                    auto hatch_region = hatches.subregion(x, y, width, height);
                    float scale = 0.0001;
                    size_t n = width * height * scale;
                    auto [min, max] = map_region->range();

                    if (min == 0 && max > 0) {
                        rivers_region->for_each_pixel([](float& p) {
                            p = -std::abs(p);
                        });

                        auto placed_rivers = add_rivers_on_map(*rivers_region, *hatch_region, *map_region, gen, n);
                        num_placed_rivers += placed_rivers;
                        loading_bar.multi_step(placed_rivers);
                    }
                }
            }
        }
    }

    rivers.for_each_pixel([](float& p) {
        p = std::abs(p);
    });
    scale_range(rivers);

    loading_bar.finalize();

    return rivers;
}


int main() {
    int width = 512;
    int height = 512;
    image<float> map(40 * width, 40 * height);

    {
        auto result = generate_multi_terrain(width, height, 40, 40);
        auto cropped_background = extract_background(result, 0.5);
        result.apply_lower_threshold(0.5, 0.0);

        auto mask = generate_mask(result);

        auto tiles = generate_tiles(result, mask);
        map = generate_placement(result.width(), result.height(), tiles, cropped_background);
        
        export_ppm("terrain_pre.ppm", map);
        reshape_hills_on_map(map);

        auto scaled_map = downscale_img(map, width, height);
        export_ppm("terrain_scaled.ppm", scaled_map);
    }

    auto path_result = generate_rivers(map);

    export_ppm("path.ppm", path_result);

    return 0;
}
