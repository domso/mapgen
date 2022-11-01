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
    }

    return result;
}

#include "terrain_buffer.h"
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
    std::cout << tiles.size() << std::endl;
//    for (int i = 0; i < std::min(tiles.size(), 1000UL); i++) {
//        auto& [map, size] = tiles[i];
//        auto& [w, h] = size;
//        export_ppm("blub" + std::to_string(i) + ".ppm", w, h, map);
//    }

//    export_ppm("terrain.ppm", width * terrain_multiplier, height * terrain_multiplier, maskImg);
//    export_ppm("terrain2.ppm", width * terrain_multiplier, height * terrain_multiplier, result);

    return 0;
}
