#include "generate_placement.h"

#include <vector>
#include <random>
#include <tuple>
#include "image.h"
#include "helper.h"

#include "../../cmd/loading_bar.h"

std::tuple<size_t, size_t, float> compute_scaled_sizes(const image<float>& tile, const size_t dest_width, const size_t dest_height) {
    size_t width = dest_width;
    size_t height = dest_height;
    float scale;

    if (tile.width() > tile.height()) { 
        scale = static_cast<float>(width) / static_cast<float>(tile.width());
        height = tile.height() * scale;
    } else {
        scale = static_cast<float>(height) / static_cast<float>(tile.height());
        width = tile.width() * scale;
    }
    
    return {width, height, scale};
}
 
bool place_tile_on_map(image<float>& map, const image<float>& tile, const size_t x, const size_t y, const size_t width, const size_t height, const float scale, const int num_placement) {
    if (auto region = map.subregion(x, y, width, height)) {
        bool valid = true;
        (*region).check_each_pixel([&](float& p) {
            if (p != 0) {
                valid = false;
            }
            return valid;
        });

        if (valid) {
            (*region).for_each_pixel([&](float& p, const size_t rx, const size_t ry) {
                auto src_x = static_cast<size_t>(rx / scale);
                auto src_y = static_cast<size_t>(ry / scale);

                if (tile.contains(src_x, src_y) && tile.at(src_x, src_y) > 0) {
                    float scaled_data = 0.50 + (tile.at(src_x, src_y) - 0.50) / (1 + num_placement);
                    p = std::max(p, scaled_data);
                }
            });

            return true;
        }
    }

    return false;
}

void place_tiles_on_map(image<float>& map, const std::vector<image<float>>& tiles, std::mt19937& gen) {
    std::uniform_int_distribution<int> dis_width(0, map.width() - 1);
    std::uniform_int_distribution<int> dis_height(0, map.height() - 1);

    size_t current_tile = 0;
    size_t current_placed_tile = 0;
    size_t offset = 1;

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

        auto [current_width, current_height, scale] = compute_scaled_sizes(
            tile,
            map.width() / (1 + current_placed_tile) - offset,
            map.width() / (1 + current_placed_tile) - offset
        );
        offset = std::min(offset + 1, map.width() / (1 + current_placed_tile) - 1);

        if (place_tile_on_map(map, tile, current_x, current_y, current_width, current_height, scale, current_placed_tile)) {
            current_tile++;
            current_placed_tile++;
            offset = std::min(offset, map.width() / (1 + current_placed_tile) - 1);
            loading_bar.step();
            if (current_tile == tiles.size()) {
                break;
            }
        }
    }
    loading_bar.finalize();
}

std::vector<image<float>> normalize_tiles(const std::vector<image<float>>& tiles) {
    auto result = tiles;
    for (auto& tile : result) {
        tile.for_each_pixel([](float& p) {
            if (p > 0) {
                p = std::max(0.0, p - 0.5);
            }
        });
    }
    
    return result;
}

void place_noice_tiles_on_map(image<float>& map, const std::vector<image<float>>& tiles, std::mt19937& gen) {
    std::uniform_int_distribution<int> dis_width(0, map.width() - 1);
    std::uniform_int_distribution<int> dis_height(0, map.height() - 1);
    auto normalized_tiles = normalize_tiles(tiles);
    size_t current_tile = 0;
    cmd::loading_bar loading_bar;

    int early_terminate = 1000000;
    loading_bar.init("Place noice tiles", normalized_tiles.size());
    while(current_tile < normalized_tiles.size()) {
        auto current_x = dis_width(gen);
        auto current_y = dis_height(gen);
        auto& tile = normalized_tiles[current_tile];

        auto check_x = current_x + tile.width() / 2;
        auto check_y = current_y + tile.height() / 2;

        if (map.contains(check_x, check_y) && map.at(check_x, check_y) != 0) {
            if (auto region = map.subregion(current_x, current_y, tile.width(), tile.height())) {
                (*region).for_each_pixel([&](float& p, const size_t x, const size_t y) {
                    if (tile.at(x, y) > 0) {
                        p = std::abs(p) + 0.05 * tile.at(x, y);
                        p = -p;
                    }
                });

                current_tile++;
                early_terminate = 1000000;
                loading_bar.step();
            }
        } else {
            early_terminate--;
            if (early_terminate == 0) {
                early_terminate = 1000000;
                current_tile++;
                loading_bar.step();
            }
        }
    }

    map.for_each_pixel([](float& p) {
        p = std::abs(p);
    });

    scale_range(map);

    loading_bar.finalize();
}

void place_background(image<float>& map, const image<float>& background) {
    auto [width, height, scale] = compute_scaled_sizes(background, map.width(), map.height());

    if (auto region = map.subregion(0, 0, width, height)) {
        (*region).for_each_pixel([&](float& p, const size_t x, const size_t y) {
            auto src_x = static_cast<size_t>(x / scale);
            auto src_y = static_cast<size_t>(y / scale);

            if (background.contains(src_x, src_y)) {
                p += 0.05 * background.at(src_x, src_y);
            }
        });
    }

    scale_range(map);
}

image<float> generate_placement(const size_t width, const size_t height, const std::vector<image<float>>& tiles, const image<float>& background) {
    image<float> result(width, height, 0.0);

    std::random_device rd;
    std::mt19937 gen(rd());

    place_tiles_on_map(result, tiles, gen);
    place_noice_tiles_on_map(result, tiles, gen);
    place_background(result, background);

    return result;
}
