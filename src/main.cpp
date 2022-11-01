#include <vector>
#include <iostream>

#include <random>

#include "export_ppm.h"
#include "generate_terrain.h"
#include "generate_water.h"
#include "merge_map.h"

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

std::vector<std::pair<std::vector<float>, std::pair<int, int>>> generate_tiles(std::vector<int>& mask, const int width, const int height) {
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

        if (end_x - start_x > 10 && end_y - start_y > 10) {
            std::vector<float> img;
            for (int y = start_y; y <= end_y; y++) {
                for (int x = start_x; x <= end_x; x++) {
                    if (mask[y * width + x] == key) {
                        img.push_back(1.0);
                    } else {
                        img.push_back(0.0);
                    }
                }
            }
            result.push_back({img, {end_x - start_x + 1, end_y - start_y + 1}});
        }
        if (end_x - start_x + 1 > 50 && end_y - start_y + 1 > 50) {
            result.push_back({img, {end_x - start_x + 1, end_y - start_y + 1}});
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
    while (true) {
        auto current_x = dis_width(gen);
        auto current_y = dis_height(gen);
        auto& [tile_data, tile_sizes] = tiles[current_tile];
        auto [tile_width, tile_height] = tile_sizes;

        auto current_width = max_x / 1 - i;
        auto current_height = max_x / 1 - i;
        i = std::min(i + 1, max_x / 1 - 1);

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
                        if (src_ptr < tile_data.size()) {
                            result[dest_ptr] = std::max(result[dest_ptr], tile_data[src_ptr]);
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

    return {result, {max_x, max_y}};
}



=======
#include "terrain_buffer.h"
>>>>>>> 7f4cd70d30f40c4edefd14e492df9a828f0122bf
int main() {
    int width = 512;
    int height = 512;

    terrain_buffer buffer(width, height, 128);



    std::vector<float> terrain = generate_terrain(width, height);
//     std::vector<float> terrain2 = generate_terrain(width, height);
//     std::vector<float> terrain3 = generate_terrain(width, height);
//     std::vector<float> terrain4 = generate_terrain(width, height);
//     std::vector<float> terrain5 = generate_terrain(width, height);

     for (auto& f : terrain) {
        if (f > 0.5) {
//            f = 1.0;
        } else {
//            f = 0.0;
        }
     }
    export_ppm("terrain2.ppm", width, height, terrain);
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
    export_ppm("terrain.ppm", width * terrain_multiplier, height * terrain_multiplier, result);
    return 0;
//     
//     float m = 0;
//     for (auto f : result) {
//         m = std::max(f, m);
//     }
//     for (auto& f : result) {
//         f *= (1 / m);
//     }
    
    
     for (auto& f : result) {
        if (f > 0.5) {
        } else {
            f = 0.0;
        }
     }

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
   // std::vector<float> maskImg;
   // for (auto& p : mask) {
    //    maskImg.push_back(static_cast<float>(p) / 10.0);
   // }
    auto tiles = generate_tiles(mask, width * terrain_multiplier, height * terrain_multiplier);
    std::cout << "place tiles" << std::endl;
    auto [map, map_dimensions] = place_tiles(tiles);
    auto [map_width, map_height] = map_dimensions;

    std::cout << "save tiles" << std::endl;
    export_ppm("terrain.ppm", map_width, map_height, map);

    return 0;
}
