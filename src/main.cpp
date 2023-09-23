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

template<typename Tcall>
void draw_line(image<float>& img, int start_x, int start_y, int end_x, int end_y, const Tcall& call) {
    // Calculate the delta values for x and y
    int dx = abs(end_x - start_x);
    int dy = abs(end_y - start_y);

    // Determine the direction of movement in x and y
    int sx = (start_x < end_x) ? 1 : -1;
    int sy = (start_y < end_y) ? 1 : -1;

    // Initialize error term
    int err = dx - dy;

    // Initialize current position
    int x = start_x;
    int y = start_y;

    // Loop to plot the points
    while (true) {

        // Check if we've reached the end point
        if (x == end_x && y == end_y) {
            break;
        }

        // Calculate the next position
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }

        // Plot the current point
        call(img.at(x, y), x, y);
    }
}

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
                draw_line(result, scale * prev_x, scale * prev_y, scale * current_x, scale * current_y, [](auto& f, auto x, auto y) {f += 1.0;});


                draw_line(heights, scale * prev_x, scale * prev_y, scale * current_x, scale * current_y, [&](auto& f, auto x, auto y) {
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

    //for (int i = 0; i < 32; i++) {
    //    float m = 3.0;
    //    lut[7 * 32 + i] = 5 * 32 + m * i;
    //}


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
/*
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
    */
}

std::pair<int, int> update_positions_on_map(const image<float>& map, const image<float>& rivers, const int x, const int y) {
    auto current_x = x;
    auto current_y = y;

    float current_min = std::numeric_limits<float>::infinity();

    for (int dy = -1; dy < 2; dy++) {
        auto dx = 0;
        if (dx != dy && map.contains(current_x + dx, current_y + dy)) {
            current_min = std::min(current_min, map.at(current_x + dx, current_y + dy) * 256 + rivers.at(current_x + dx, current_y + dy));
            if (map.at(current_x + dx, current_y + dy) == 0) {
                return {current_x + dx, current_y + dy};
            }
        }
    }

    for (int dx = -1; dx < 2; dx++) {
        auto dy = 0;
        if (dx != dy && map.contains(current_x + dx, current_y + dy)) {
            current_min = std::min(current_min, map.at(current_x + dx, current_y + dy) * 256 + rivers.at(current_x + dx, current_y + dy));
            if (map.at(current_x + dx, current_y + dy) == 0) {
                return {current_x + dx, current_y + dy};
            }
        }
    }
    for (int dy = -1; dy < 2; dy++) {
        auto dx = 0;
        if (dx != dy && map.contains(current_x + dx, current_y + dy)) {
            auto val = map.at(current_x + dx, current_y + dy) * 256 + rivers.at(current_x + dx, current_y + dy);
            if (current_min == val) {
                return {current_x + dx, current_y + dy};
            }
        }
    }
    for (int dx = -1; dx < 2; dx++) {
        auto dy = 0;
        if (dx != dy && map.contains(current_x + dx, current_y + dy)) {
            auto val = map.at(current_x + dx, current_y + dy) * 256 + rivers.at(current_x + dx, current_y + dy);
            if (current_min == val) {
                return {current_x + dx, current_y + dy};
            }
        }
    }

    return {current_x, current_y};
}

image<float> generate_rivers2(const image<float>& map) {
    image<float> result(map.width(), map.height());
    image<float> log(map.width(), map.height());
    image<float> rain(map.width(), map.height());

    auto copy = map;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis_width(0, map.width() - 1);
    std::uniform_int_distribution<int> dis_height(0, map.height() - 1);
    std::uniform_int_distribution<int> dis_noice(0, 10);


    rain.for_each_pixel([&](float& p) {
        if (dis_noice(gen) == 9) {
            p = 0.1;
        }
    });
    copy.for_each_pixel([&](float& p) {
        if (p > 0) {
            //p = p + (0.01 + dis_noice(gen) * 0.009) * 0.5;
        }
    });

    for (int i = 0; i < 1000; i++) {
        log.for_each_pixel([&](float& p, auto x, auto y) {
            p += 1.0;
        });
        image<float> newLog(map.width(), map.height());
        log.for_each_pixel([&](float& p, auto x, auto y) {
            if (map.at(x, y) > 0) {
                auto [next_x, next_y] = update_positions_on_map(copy, log, x, y);

                auto v = p;
                p = 0;
                if (map.at(next_x, next_y) > 0) {
                    newLog.at(next_x, next_y) += v;
                }
            }
        });
        log = newLog;

        result.for_each_pixel([&](float& p, auto x, auto y) {
            p = std::max(p, log.at(x, y));
        });
    }


    return log;


/*
    image<float> result(map.width(), map.height());
    auto copy = map;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis_width(0, map.width() - 1);
    std::uniform_int_distribution<int> dis_height(0, map.height() - 1);
    std::uniform_int_distribution<int> dis_noice(0, 10);


    copy.for_each_pixel([&](float& p) {
        if (p > 0) {
            p = p + 0.01 + dis_noice(gen) * 0.009;
        }
    });
    
    for (int i = 0; i < 1; i++) {
        int current_x, current_y;

        do {
            current_x = dis_width(gen);
            current_y = dis_height(gen);
        } while (copy.at(current_x, current_y) == 0 && result.at(current_x, current_y) == 0);

        image<float> new_river(map.width(), map.height());
        while (copy.at(current_x, current_y) > 0) {
            new_river.at(current_x, current_y)++;

            auto [next_x, next_y] = update_positions_on_map(copy, new_river, current_x, current_y);

            copy.at(next_x, next_y) = std::min(copy.at(next_x, next_y), copy.at(current_x, current_y));

            current_x = next_x;
            current_y = next_y;
        }

        result.for_each_pixel([&](auto& f, auto x, auto y) {
            f += new_river.at(x, y);
        });
    }
    
    scale_range(result);

    return result;
    */
}



int main() {
    int width = 512;
    int height = 512;
    auto base_map = generate_unfaded_terrain(width, height);

    export_ppm("blab.ppm", base_map);

    base_map.for_each_pixel([&](auto& p, auto x, auto y) {
        auto dx = std::abs(static_cast<int>(x) - width / 2);
        auto dy = std::abs(static_cast<int>(y) - height / 2);
        auto distance = std::sqrt(dx * dx + dy * dy);

        auto start = std::min(width, height) * 0.4;
        auto end = std::min(width, height);
        auto border = end - start;

        if (distance > start) {
            auto scale = 1.0 - (distance - start) / border;
            p *= scale;            
        }
    });

    std::vector<int> histogram(256);

    base_map.for_each_pixel([&](auto& p) {
        if (p < 1.0) {
            histogram[p * 256]++;            
        }
    });
    int threshold = 255;
    int sum = 0;
    float factor = 0.3;
    while (width * height * factor > sum && threshold >= 0) {
        sum += histogram[threshold];
        threshold--;
    }


    base_map.for_each_pixel([&](auto& p) {
        if (p * 256 < threshold) {
            p = 0;
        }
    });


    reshape_hills_on_map(base_map);
    add_gaussian_blur(base_map);
    add_gaussian_blur(base_map);
    add_gaussian_blur(base_map);
    add_gaussian_blur(base_map);

    int scale = 20;

    auto larger_map = base_map.rescale(scale * width, scale * height);
    export_ppm("blub.ppm", base_map);
    export_ppm("bluub.ppm", larger_map);


        auto result = generate_multi_terrain(width, height, 2 * scale, 2 * scale);
        export_ppm("full.ppm", result);

        add_gaussian_blur(larger_map);
        add_gaussian_blur(larger_map);
        scale_range(larger_map);

        result.for_each_pixel([&](auto& p, auto x, auto y) {
            if (larger_map.contains(x, y)) {
                auto s = larger_map.at(x, y);

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

        //result.for_each_pixel([](auto& p) {
        //    if (p < 0.3) {
        //        p = 0;
        //    } else {
        //        p = (p - 0.3) * 0.9 + 0.1;
        //    }
        //});
        //scale_range(result);


        scale_range(result);

        //export_ppm("path.ppm", generate_rivers2(result));


        export_ppm("full2.ppm", result);

/*
    image<std::pair<char, char>> pos_map(scale * width, scale * height);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis_width(0, scale);

    pos_map.for_each_pixel([&](auto& f) {
        auto& [x, y] = f;
        x = dis_width(gen);
        y = dis_width(gen);
    });
    */

    //export_ppm("path.ppm", rivers);
    //rivers.for_each_pixel([&](auto f, auto x, auto y) {
    //    if (x > 0 && y > 0 && x < rivers.width() - 1 && y < rivers.height() - 1 && f > 0) {
    //        if (rivers.at(x + 1, y + 1) > 0) {
    //            rivers.at(x, y + 1) = rivers.at(x, y);
    //        }
    //    }
    //});
    //rivers.for_each_pixel([&](auto f, auto x, auto y) {
    //    if (x > 0 && y > 0 && x < rivers.width() - 1 && y < rivers.height() - 1 && f > 0) {
    //        if (rivers.at(x + 1, y - 1) > 0) {
    //            rivers.at(x, y - 1) = rivers.at(x, y);
    //        }
    //    }
    //});
    //export_ppm("path3.ppm", rivers);
    //return 0;

    //rivers.for_each_pixel([&](auto f, auto x, auto y) {
    //    if (x > 0 && y > 0 && x < rivers.width() - 1 && y < rivers.height() - 1 && f > 0) {
    //        auto [dx, dy] = pos_map.at(x, y);
    //        larger_river.at(scale * x + dx, scale * y + dy) = f;

    //        if (rivers.at(x, y + 1) > 0) {
    //            auto t = rivers.at(x, y + 1);
    //            auto [edx, edy] = pos_map.at(x, y + 1);

    //            draw_line(larger_river, scale * x + dx, scale * y + dy, scale * (x) + edx, scale * (y + 1) + edy, std::min(t, f));
    //        }

    //        if (rivers.at(x + 1, y) > 0) {
    //            auto t = rivers.at(x + 1, y);
    //            auto [edx, edy] = pos_map.at(x + 1, y);

    //            draw_line(larger_river, scale * x + dx, scale * y + dy, scale * (x + 1) + edx, scale * y + edy, std::min(t, f));
    //        }
    //    }
    //});
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis_noice(0, 10);

    result.for_each_pixel([&](float& p) {
        if (p > 0) {
            p = p + p * (dis_noice(gen) * 0.01);
        }
    });

    scale_range(result);

    auto rescaled_result = result.rescale(width, height);
    export_ppm("rescaled.ppm", rescaled_result);

    auto [rivers, river_heights] = generate_rivers(rescaled_result);
    export_ppm("heights.ppm", river_heights);

    export_ppm("path2.ppm", rivers);


    image<float> final_river(scale * width, scale * height);
    
    {
    image<float> scaleCircle(512, 512);
    scaleCircle.for_each_pixel([&](auto& f, auto x, auto y) {
        auto dx = std::abs(static_cast<int>(x) - 256);
        auto dy = std::abs(static_cast<int>(y) - 256);
        auto distance = std::sqrt(dx * dx + dy * dy);

        if (distance > 256) {
            f = 1.0f;
        } else if (distance < 128) {
            f = 0.0f;
        } else {
            f = (distance - 128.0f) / 128.0f;
        }
    });

    image<float> circle(256, 256);
    circle.for_each_pixel([&](auto& f, auto x, auto y) {
        auto dx = std::abs(static_cast<int>(x) - 128);
        auto dy = std::abs(static_cast<int>(y) - 128);
        auto distance = std::sqrt(dx * dx + dy * dy);

        if (distance > 128) {
            f = 0;
        } else {
            f = 1.0;
        }
    });
    std::vector<image<float>> circles;
    std::vector<image<float>> scaleCircles;

    for (int i = 0; i < 255; i++) {
        circles.push_back(circle.rescale(i + 1, i + 1));
        scaleCircles.push_back(scaleCircle.rescale((i + 1) * 2, (i + 1) * 2));
    }

    rivers.for_each_pixel([&](auto& f, auto x, auto y) {
        if (f > 0) {
            auto val = std::min(255.0F, f * 256) / 2;
            auto src = circles[val];
            src.for_each_pixel([&](auto& f) {
                f *= river_heights.at(x, y);
            });
            if (auto dest = final_river.subregion(x - src.width() / 2, y - src.height() / 2, src.width(), src.height())) {
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



    //int found = 0;
    //int pos;
    //float found_col;

    //final_river.for_each_pixel([&](auto& f, auto x, auto y) {
    //    if (larger_map.at(x, y) == 0 || x == 0) {
    //        found = 0;
    //        return;
    //    }
    //    
    //    switch (found) {
    //        case 0 : {
    //            if (f > 0) {
    //                pos = x;
    //                found_col = f;
    //                found++;
    //            }

    //            break;
    //        }
    //        case 1 : {
    //            if (f > 0) {
    //                pos = x;
    //                found_col = f;
    //            } else {
    //                found++;
    //            }

    //            break;
    //        }
    //        case 2 : {
    //            if (f > 0) {
    //                for (int i = pos + 1; i < x; i++) {
    //                    auto distance = x - pos;
    //                    auto delta = f - found_col;
    //                    auto rel = i - pos + 1;

    //                    auto s = found_col + (delta / distance) * rel;

    //                    final_river.at(i, y) = s;
    //                }
    //                found = 1;
    //                pos = x;
    //                found_col = f;
    //            }

    //            break;
    //        }
    //        default: break;
    //    }

    //});



    }

    image<float> scale_map(scale * width, scale * height, 1.0);
    image<float> river_map(scale * width, scale * height, 1.0);

    int thisSize = 128;
    image<float> circle(thisSize, thisSize);
    circle.for_each_pixel([&](auto& f, auto x, auto y) {
        auto half = thisSize / 2;
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

    result.for_each_pixel([&](auto&, int x, int y) {
        if (!final_river.contains(x, y) || !scale_map.contains(x, y) || !river_map.contains(x, y)) {
            return;
        }

        auto r = final_river.at(x, y);

        if ((y % 1000) == 0 && x == 0) {
            std::cout << y << std::endl;
        }

        if (r > 0) {
            circle.for_each_pixel([&](auto f, auto cx, auto cy) {
                auto dest_x = x - circle.width() / 2 + cx;
                auto dest_y = y - circle.height() / 2 + cy;

                if (scale_map.contains(dest_x, dest_y)) {
                    float old = scale_map.at(dest_x, dest_y);
                    float next = f;
                    scale_map.at(dest_x, dest_y) = std::min(old, next);
                }
                if (river_map.contains(dest_x, dest_y)) {
                    if (f < 1.0) {
                        float old = river_map.at(dest_x, dest_y);
                        float next = r;//(1.0 - r) * f + r;
                        river_map.at(dest_x, dest_y) = std::min(old, next);
                    }
                }
            });
        }
    });
    result.for_each_pixel([&](auto& f, int x, int y) {
        if (!river_map.contains(x, y)) {
            return;
        }
        f = f * scale_map.at(x, y) +  river_map.at(x, y) * (1.0f - scale_map.at(x, y));
        //float old = f;
        //float next = river_map.at(x, y);

        //f = std::min(old, next);

        //float r = river_map.at(x, y);
        //float rev = 1.0f - f;

        //float d = rev * scale_map.at(x, y);

        //f = r - d;


        //f = f * scale_map.at(x, y) + (1.0f - scale_map.at(x, y));





    });

    final_river.for_each_pixel([&](auto& f, auto x, auto y) {
        if (f > 0 && result.contains(x, y)) {
            result.at(x, y) = std::min(f, result.at(x, y));
        }
    });
    export_ppm("final_river.ppm", final_river);
    export_ppm("final_map.ppm", result);


    return 0;
}

