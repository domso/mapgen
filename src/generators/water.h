#pragma once

#include <vector>
#include <queue>
#include <unordered_map>

#include "image.h"
#include "random_generator.h"

namespace mapgen::generators::water {

std::pair<image<float>, image<float>> generate(const image<float>& mask, const image<float>& terrain, const std::vector<std::pair<image<float>, image<float>>>& shapes);

namespace internal {

enum direction {
    end,
    north,
    east,
    south,
    west,
    undefined
};

image<float> add_noise_to_img(const image<float>& src, int factor);

std::pair<image<std::pair<direction, float>>, std::pair<int, int>> find_single_path_map(const image<float>& src, const int start_x, const int start_y, const int rnd);

std::vector<std::pair<int, int>> find_single_path(const image<float>& src, const int start_x, const int start_y, const int rnd);

int compute_rivers_in_region(const image<float>& region_map, const float region, const int rivers);

std::pair<int, int> generate_river_start_point(uniform_random_number<int>& dis_x, uniform_random_number<int>& dis_y, uniform_random_number<float>& dis_dice, const image<float>& region_map, const image<float>& river_map, const image<float>& wet_map, const float region);

void add_river_to_region(const image<float>& region_map, const float region, const int rivers, const image<float>& weights_map, image<float>& river_map, const image<float>& wet_map);

image<float> generate_river_distribution(const image<float>& mask);

void apply_circular_shift(image<float>& target, const image<float>& map, const image<float>& circle, const int x, const int y);

void apply_circular_set(image<float>& target, const float color, const image<float>& circle, const int x, const int y);

std::pair<image<float>, image<float>> generate_river_depth(const image<float>& rivers, const image<float>& weights, const std::vector<std::pair<size_t, size_t>>& river_endpoints);

void filter_non_zero_neighbors(image<float>& water, const image<float>& terrain);

image<std::pair<float, float>> generate_water_scale_cell_position(const image<float>& water, const size_t width, const size_t height);

std::pair<image<float>, std::vector<std::pair<size_t, size_t>>> upscale_river_map(const image<float>& river_map, const image<float>& terrain, const size_t width, const size_t height);

std::vector<std::pair<int, int>> compute_lake_position(const image<float>& rivers);

void apply_lakes(const image<float>& river_map, const std::vector<std::pair<image<float>, image<float>>>& shapes, image<float>& scaled_river_map, image<float>& terrain);

}
}
