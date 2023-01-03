#include <vector>
#include <iostream>

#include <random>

#include "export_ppm.h"
#include "generate_terrain.h"
#include "generate_water.h"
#include "merge_map.h"
#include "helper.h"

#include "config.h"
void fade_out_terrain(std::vector<float>& terrain, const int width, const int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width / 2; x++) {
            double scale = static_cast<double>(x) / static_cast<double>(width / 2);
            terrain[y * width + x] *= scale;
        }
    }
    for (int y = 0; y < height / 2; y++) {
        for (int x = 0; x < width; x++) {
            double scale = static_cast<double>(y) / static_cast<double>(height / 2);
            terrain[y * width + x] *= scale;
        }
    }    
    
    for (int y = height / 2; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double scale = static_cast<double>(height - y) / static_cast<double>(height / 2);
            terrain[y * width + x] *= scale;
        }
    }    
    
    for (int y = 0; y < height; y++) {
        for (int x = width / 2; x < width; x++) {
            double scale = static_cast<double>(width - x) / static_cast<double>(width / 2);
            terrain[y * width + x] *= scale;
        }
    }
}


void insert_as_tile(std::vector<float>& map, const int rows, const int cols, const int row, const int col, std::vector<float>& terrain, const int width, const int height) {    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {            
            map[
                (row * height + y - config::terrain::max_hill_size * row) * cols * width +
                col * width + x - config::terrain::max_hill_size * col
            ] += terrain[y * width + x];
        }
    }
}



#include <unordered_map>

std::vector<std::pair<std::vector<float>, std::pair<int, int>>> generate_tiles(std::vector<float>& map, std::vector<int>& mask, const int width, const int height) {
    std::vector<std::pair<std::vector<float>, std::pair<int, int>>> result;
    std::unordered_map<int, std::pair<std::pair<int, int>, std::pair<int, int>>> position;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            auto d = mask[y * width + x];
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

            std::vector<float> img(w * h, 0.0);
            for (int y = start_y; y <= end_y; y++) {
                for (int x = start_x; x <= end_x; x++) {
                    if (mask[y * width + x] == key) {
                        img[(blurs + y - start_y) * w + blurs + x - start_x] = map[y * width + x];
                    }
                }
            }
            for (int i = 0; i < blurs; i++) {
                add_gaussian_blur(img, w, h);
            }
            result.push_back({img, {w, h}});
        }
    }

    return result;
}


#include <cmath>
#include <random>

std::pair<std::vector<float>, std::pair<int, int>> place_tiles(std::vector<std::pair<std::vector<float>, std::pair<int, int>>>& tiles) {
    size_t max_x = 20000;
    size_t max_y = 20000;
    std::vector<float> result((max_x) * (max_y));
    std::vector<std::pair<int, int>> positions;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis_width(0, max_x - 1);
    std::uniform_int_distribution<int> dis_height(0, max_y - 1);

    size_t current_tile = 0;

    size_t i = 1;
    while (current_tile < tiles.size()) {
        auto current_x = dis_width(gen);
        auto current_y = dis_height(gen);
        auto& [tile_data, tile_sizes] = tiles[current_tile];
        auto [tile_width, tile_height] = tile_sizes;

        if (tile_width < 500 || tile_height < 500) {
            current_tile++;
            continue;
        }

        auto current_width = max_x / 1 / (1 + current_tile) - i;
        auto current_height = max_x / 1 / (1 + current_tile) - i;
        i = std::min(i + 1, max_x / 1 / (1 + current_tile) - 1);

        float scale;
        if (tile_width > tile_height) { 
            scale = static_cast<float>(current_width) / static_cast<float>(tile_width);
            current_height = tile_height * scale;
        } else {
            scale = static_cast<float>(current_height) / static_cast<float>(tile_height);
            current_width = tile_width * scale;
        }

        if (0 <= current_x && current_x + current_width < max_x && 0 <= current_y && current_y + current_height < max_y) {
            bool valid = true;
            for (auto y = current_y; y < current_y + current_height; y++) {
                for (auto x = current_x; x < current_x + current_width; x++) {
                    auto ptr = y * max_x + x;
                    if (result[ptr] != 0) {
                        valid = false;
                        break;
                    }
                }
            }

            if (valid) {
                for (auto y = current_y; y < current_y + current_height; y++) {
                    for (auto x = current_x; x < current_x + current_width; x++) {
                        auto dest_ptr = y * max_x + x;
                        auto src_ptr = static_cast<size_t>(static_cast<size_t>((y - current_y) / scale) * tile_width + static_cast<size_t>(x - current_x) / scale);
                        if (src_ptr < tile_data.size() && tile_data[src_ptr] > 0) {
                            float scaled_data = 0.50 + (tile_data[src_ptr] - 0.50) / (1 + current_tile);
                            result[dest_ptr] = std::max(result[dest_ptr], scaled_data);
                        }
                    }
                }

                current_tile++;
                if (current_tile == tiles.size()) {
                    break;
                }
            }
        }

    }
    
    current_tile = 0;
    std::cout << "start filling remaining tiles" << std::endl;

    while(current_tile < tiles.size()) {
        auto current_x = dis_width(gen);
        auto current_y = dis_height(gen);
        auto& [tile_data, tile_sizes] = tiles[current_tile];
        auto [current_width, current_height] = tile_sizes;

        if (0 <= current_x && current_x + current_width < max_x && 0 <= current_y && current_y + current_height < max_y) {
            float current_min = 1;
            float current_max = 0;
            for (auto y = current_y; y < current_y + current_height; y++) {
                for (auto x = current_x; x < current_x + current_width; x++) {
                    auto dest_ptr = y * max_x + x;
                    current_min = std::min(current_min, result[dest_ptr]);
                    current_max = std::max(current_max, result[dest_ptr]);
                }
            }
        
                float delta = current_max - current_min; 

                for (auto y = current_y; y < current_y + current_height; y++) {
                    for (auto x = current_x; x < current_x + current_width; x++) {
                        auto dest_ptr = y * max_x + x;
                        auto src_ptr = (y - current_y) * current_width + x - current_x;
                        if (src_ptr < tile_data.size() && tile_data[src_ptr] > 0) {
                            result[dest_ptr] = tile_data[src_ptr];
                        }
                    }
                }

                current_tile++;
        }
    }

    return {result, {max_x, max_y}};
}

std::vector<float> simulate_simple_physics(const std::vector<float>& terrain, const size_t width, const size_t height) {
    std::vector<float> result(width * height, 0.0);
   
    for (int it = 0; it < 512; it++) {
        for (int y = 0; y < height; y++) {
            result[y * width + 0] += 1.0;
        }

    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            auto top = result[(y - 1) * width + x + 0] + terrain[(y - 1) * width + x - 0];
            auto bot = result[(y + 1) * width + x + 0] + terrain[(y + 1) * width + x - 0];
            auto left = result[(y - 0) * width + x - 1] + terrain[(y - 0) * width + x - 1];
            auto right = result[(y - 0) * width + x + 1] + terrain[(y - 0) * width + x + 1];
            auto mid = result[y * width + x] + terrain[y * width + x];

            auto min = std::min(top, std::min(bot, std::min(left, std::min(right, mid))));

            if (top == min) {
                result[(y - 1) * width + x] += result[y * width + x];
                result[y * width + x] = 0;
            }
            if (bot == min) {
                result[(y + 1) * width + x] += result[y * width + x];
                result[y * width + x] = 0;
            }
            if (left == min) {
                result[(y + 0) * width + x - 1] += result[y * width + x];
                result[y * width + x] = 0;
            }
            if (right == min) {
                result[(y + 0) * width + x +1] += result[y * width + x];
                result[y * width + x] = 0;
            }
        }
    }
    }
        


    scale_range(result);
    return result;
}

std::vector<std::pair<float, int>> generate_path(const std::vector<float>& terrain, const int width, const int height, const int start_x, const int start_y, const int dest_x, const int dest_y) {
    std::vector<std::pair<float, int>> result(width * height, {std::numeric_limits<float>::infinity(), 0});
    int tile_offset = 1;
    result[start_y * width + start_x] = {0.0, 0};

    bool changed = true;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis_noice(0, 10);
    
    std::vector<float> noiced_terrain;
    for (auto p : terrain) {
        noiced_terrain.push_back(p + dis_noice(gen) * 0.01);
    }

    while (changed) {
        changed = false;

        for (int y = std::max(0, start_y - tile_offset); y < std::min(height, start_y + tile_offset + 1); y++) {
            for (int x = std::max(0, start_x - tile_offset); x < std::min(width, start_x + tile_offset + 1); x++) {
                auto [score, step] = result[y * width + x];
                auto delta = noiced_terrain[y * width + x] * noiced_terrain[y * width + x] * noiced_terrain[y * width + x] * noiced_terrain[y * width + x];

                if (y > 0) {
                    auto [neighbor_score, neighbor_step] = result[y * width + x - width];

                    if (neighbor_score + delta < score) {
                        score = neighbor_score + delta;
                        step = 1;
                        changed = true;
                    }
                }
                if (x < width - 1) {
                    auto [neighbor_score, neighbor_step] = result[y * width + x + 1];

                    if (neighbor_score + delta < score) {
                        score = neighbor_score + delta;
                        step = 2;
                        changed = true;
                    }
                }
                if (y < height - 1) {
                    auto [neighbor_score, neighbor_step] = result[y * width + x + width];

                    if (neighbor_score + delta < score) {
                        score = neighbor_score + delta;
                        step = 3;
                        changed = true;
                    }
                }
                if (x > 0) {
                    auto [neighbor_score, neighbor_step] = result[y * width + x - 1];

                    if (neighbor_score + delta < score) {
                        score = neighbor_score + delta;
                        step = 4;
                        changed = true;
                    }
                }

                result[y * width + x] = {score, step};
            }
        }
        tile_offset++;
    }

    return result;
}



std::vector<float> export_path_img(const std::vector<float>& terrain, const std::vector<std::pair<float, int>>& path_map, const int width, const int height, const int dest_x, const int dest_y) {
    std::vector<float> result(terrain.size(), 0);

    int current_x = dest_x;
    int current_y = dest_y;

    float current_temp = static_cast<float>(dest_y) / height;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            auto s = static_cast<float>(y) / height;
//            result[y * width + x] = s * s;

        }
    }

    bool running = true;
    while (running) {
        const auto& [score, step] = path_map[current_y * width + current_x];
        auto n = current_y * width + current_x;
        auto env_temp = static_cast<float>(current_y) / height;

        if (env_temp > current_temp) {
            current_temp = current_temp + (env_temp - current_temp) / 0.9;
        } else {
            current_temp = current_temp - (current_temp - env_temp) / 2;
        }

        result[n] = current_temp;
        int margin = 100;

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
*/  

    add_gaussian_blur(result, width, height);
    add_gaussian_blur(result, width, height);
    add_gaussian_blur(result, width, height);
    add_gaussian_blur(result, width, height);
    return result;
}

std::vector<float> merge_paths(const std::vector<float>& path_map, const size_t width, const size_t height) {
    std::cout << "start to merge" << std::endl;
    auto result = path_map;



    return result;
}

std::vector<float> downscale_img(const std::vector<float>& map, const size_t src_width, const size_t src_height, const size_t dest_width, const size_t dest_height) {
    std::vector<float> result(dest_width * dest_height, 0.0);

    for (size_t y = 0; y < dest_height; y++) {
        for (size_t x = 0; x < dest_width; x++) {
            auto vertical_fraction = static_cast<double>(y) / dest_height;
            auto horizontal_fraction = static_cast<double>(x) / dest_width;

            auto src_x = static_cast<size_t>(horizontal_fraction * src_width);
            auto src_y = static_cast<size_t>(vertical_fraction * src_height);

            auto ptr = src_y * src_width + src_x;

            if (ptr < map.size()) {
                result[dest_width * y + x] = map[ptr];
            }
        }
    }

    add_gaussian_blur(result, dest_width, dest_height);
    add_gaussian_blur(result, dest_width, dest_height);
    add_gaussian_blur(result, dest_width, dest_height);
    add_gaussian_blur(result, dest_width, dest_height);

    return result;
}


#include "terrain_buffer.h"
int main() {
    int width = 512;
    int height = 512;

    terrain_buffer buffer(width, height, 128);


/*
    std::vector<float> terrain = generate_terrain(width, height);
     for (auto& f : terrain) {
        if (f > 0.50) {
        } else {
            f = 0.5;
        }
     }
 */  


/*
    std::vector<float> path_result(width * height, 0);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis_width(0, width - 1);
    std::uniform_int_distribution<int> dis_height(0, height - 1);
    for (int i = 0; i < 50; i++) {
        auto path = generate_path(terrain, width, height, dis_width(gen), dis_height(gen), width - 1, height / 2);
        auto path_img = export_path_img(terrain, path, width, height, width - 1, height / 2);

        for (int p = 0; p < path_img.size(); p++) {
            path_result[p] = std::max(path_result[p], path_img[p]);
        }
    }

    for (int i = 0; i < 50; i++) {
        auto path = generate_path(terrain, width, height, 0, i * height / 50, width - 1, i * height / 50);
        auto path_img = export_path_img(terrain, path, width, height, width - 1, i * height / 50);

        for (int p = 0; p < path_img.size(); p++) {
            path_result[p] = std::max(path_result[p], path_img[p]);
        }
    }

    path_result = merge_paths(path_result, width, height);

    export_ppm("path.ppm", width, height, path_result);
    export_ppm("terrain.ppm", width, height, terrain);
*/

//     std::vector<float> terrain2 = generate_terrain(width, height);
//     std::vector<float> terrain3 = generate_terrain(width, height);
//     std::vector<float> terrain4 = generate_terrain(width, height);
//     std::vector<float> terrain5 = generate_terrain(width, height);

     //for (auto& f : terrain) {
        //if (f > 0.50) {
//            f = 1.0;
        //} else {
//            f = 0.0;
        //}
     //}
//     fade_out_terrain(terrain, width, height);
//     fade_out_terrain(terrain2, width, height);
//     fade_out_terrain(terrain3, width, height);
//     fade_out_terrain(terrain4, width, height);
//     fade_out_terrain(terrain5, width, height);
//     
    int terrain_multiplier = 40;
    std::vector<float> result(width * height * terrain_multiplier * terrain_multiplier);
    
    for (int i = 0; i < terrain_multiplier; i++) {  
        for (int j = 0; j < terrain_multiplier; j++) {            
            std::vector<float> t = buffer.pop();
            insert_as_tile(result, terrain_multiplier, terrain_multiplier, i, j, t, width, height);
        }
    }
    buffer.stop();
//     
//     float m = 0;
//     for (auto f : result) {
//         m = std::max(f, m);
//     }
//     for (auto& f : result) {
//         f *= (1 / m);
//     }
    
    
     for (auto& f : result) {
        if (f > 0.50) {
        } else {
            f = 0.0;
        }
     }
        std::cout << "save tile map" << std::endl;
        export_ppm("tile_map.ppm", width * terrain_multiplier, height * terrain_multiplier, result);

    std::vector<int> mask(width * height * terrain_multiplier * terrain_multiplier);
    int s = 1;
    int n = 0;
    for (int y = 0; y < height * terrain_multiplier; y++) {
        for (int x = 0; x < width * terrain_multiplier; x++) {
            if (result[y * width * terrain_multiplier + x] == 0) {
                if (n > 0) {
                    s++;
                }
                n = 0;
                mask[y * width * terrain_multiplier + x] = 0;
            } else {
                mask[y * width * terrain_multiplier + x] = s;
                n++;
            }
        }
    }
    int min;
    bool found = true;

    while(found) {
        std::cout << "Next round of merge" << std::endl;
        found = false;

        for (int x = 0; x < width * terrain_multiplier; x++) {
            min = 0;
            for (int y = 0; y < height * terrain_multiplier; y++) {
                size_t ptr = y * width * terrain_multiplier + x;

                min = std::min(min, mask[ptr]);

                if (mask[ptr] > 0 && min == 0) {
                    min = mask[ptr];
                }

                found = found || (mask[ptr] != min);
                mask[ptr] = min;
            }
            for (int y = 0; y < height * terrain_multiplier; y++) {
                size_t ptr = (height * terrain_multiplier - 1 - y) * width * terrain_multiplier + x;

                min = std::min(min, mask[ptr]);

                if (mask[ptr] > 0 && min == 0) {
                    min = mask[ptr];
                }

                found = found || (mask[ptr] != min);
                mask[ptr] = min;
            }
        }
        for (int y = 0; y < height * terrain_multiplier; y++) {
            min = 0;
            for (int x = 0; x < width * terrain_multiplier; x++) {
                size_t ptr = y * width * terrain_multiplier + x;

                min = std::min(min, mask[ptr]);

                if (mask[ptr] > 0 && min == 0) {
                    min = mask[ptr];
                }

                found = found || (mask[ptr] != min);
                mask[ptr] = min;
            }
            for (int x = 0; x < width * terrain_multiplier; x++) {
                size_t ptr = y * width * terrain_multiplier + (width * terrain_multiplier - 1 - x);

                min = std::min(min, mask[ptr]);

                if (mask[ptr] > 0 && min == 0) {
                    min = mask[ptr];
                }

                found = found || (mask[ptr] != min);
                mask[ptr] = min;
            }
        }
    }
    int incr_mask = 0;
    for (int y = 0; y < height * terrain_multiplier; y++) {
        int count = 0;
        int last;
        for (int x = 0; x < width * terrain_multiplier; x++) {
            size_t ptr = y * width * terrain_multiplier + x;
            if (mask[ptr] > 0) {
                count = incr_mask;
                last = mask[ptr];
            } else if (count > 0) {
                mask[ptr] = last;
                count--;
            }
        }
        count = 0;
        for (int x = 0; x < width * terrain_multiplier; x++) {
            size_t ptr = y * width * terrain_multiplier + (width * terrain_multiplier - 1 - x);
            if (mask[ptr] > 0) {
                count = incr_mask;
                last = mask[ptr];
            } else if (count > 0) {
                mask[ptr] = last;
                count--;
            }
        }
    }
    for (int x = 0; x < width * terrain_multiplier; x++) {
        int count = 0;
        int last;
        for (int y = 0; y < height * terrain_multiplier; y++) {
            size_t ptr = y * width * terrain_multiplier + x;
            if (mask[ptr] > 0) {
                count = incr_mask;
                last = mask[ptr];
            } else if (count > 0) {
                mask[ptr] = last;
                count--;
            }
        }
        count = 0;
        for (int y = 0; y < height * terrain_multiplier; y++) {
            size_t ptr = (height * terrain_multiplier - 1 - y) * width * terrain_multiplier + x;
            if (mask[ptr] > 0) {
                count = incr_mask;
                last = mask[ptr];
            } else if (count > 0) {
                mask[ptr] = last;
                count--;
            }
        }
    }
    std::cout << "incr complete" << std::endl;
    min;
    found = true;

   // std::vector<float> mask_img;
   // for (auto& p : mask) {
    //    mask_img.push_back(static_cast<float>(p) / 10.0);
   // }
    auto tiles = generate_tiles(result, mask, width * terrain_multiplier, height * terrain_multiplier);
    std::cout << "place tiles" << std::endl;
    auto [map, map_dimensions] = place_tiles(tiles);
    auto [map_width, map_height] = map_dimensions;
        export_ppm("terrain_pre.ppm", map_width, map_height, map);

    for (auto& m : map) {
        if (m > 0.50) {
            //m = m - 0.50;
        }
    }
    add_gaussian_blur(map, map_width, map_height);
    add_gaussian_blur(map, map_width, map_height);
    add_gaussian_blur(map, map_width, map_height);
    add_gaussian_blur(map, map_width, map_height);
    for (auto& m : map) {
        m = m * m;
    }
    scale_range(map);
    for (auto& m : map) {
        m = m * m;
    }
    scale_range(map);

    auto scaled_map = downscale_img(map, map_width, map_height, width, height);
        for (auto& p : scaled_map) {
            if (p < 0.05) {
                p = 5.0;
            }
        }

        std::cout << "save tiles" << std::endl;
        export_ppm("terrain.ppm", map_width, map_height, map);
        export_ppm("terrain_scaled.ppm", width, height, scaled_map);

    std::vector<float> path_result(width * height, 0);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis_width(0, width - 1);
    std::uniform_int_distribution<int> dis_height(0, height - 1);

    for (int i = 0; i < 4; i++) {
        auto waypoint_x = dis_width(gen);
        auto waypoint_y = dis_height(gen);
        auto start_y = dis_height(gen);
        auto end_y = dis_height(gen);

        {
            auto path = generate_path(scaled_map, width, height, 0, start_y, waypoint_x, waypoint_y);
            auto path_img = export_path_img(scaled_map, path, width, height, waypoint_x, waypoint_y);

            for (int p = 0; p < path_img.size(); p++) {
                path_result[p] = std::max(path_result[p], path_img[p]);
            }
        }
        {
            auto path = generate_path(scaled_map, width, height, waypoint_x, waypoint_y, width - 1, end_y);
            auto path_img = export_path_img(scaled_map, path, width, height, width - 1, end_y);

            for (int p = 0; p < path_img.size(); p++) {
                path_result[p] = std::max(path_result[p], path_img[p]);
            }
        }
    }

    path_result = merge_paths(path_result, width, height);

    export_ppm("path.ppm", width, height, path_result);


    return 0;
}
