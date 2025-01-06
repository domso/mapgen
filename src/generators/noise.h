#pragma once

#include <vector>
#include <random>
#include <thread>
#include <mutex>

#include "image.h"
#include "random_generator.h"

namespace mapgen::generators::noise {

image<float> generate(const int width, const int height, const int scale, const float fade_factor, const float threshold, const float noise_factor);

namespace internal {

void generate_hill(random_generator& rnd, image<float>& map, const int x, const int y, const int size, const int min, const int max);
std::pair<int, int> min_random_neighbor(random_generator& rnd, const image<float>& map, const int x, const int y);
void add_erosion(random_generator& rnd, image<float>& map);
image<float> generate_tile(const int width, const int height);
std::vector<image<float>> generate_tiles(const int width, const int height, const int n);

void apply_threshold(image<float>& tile, float threshold);
image<float> generate_segment(const std::vector<image<float>>& tiles, random_generator& rnd, const int width, const int height);

}

}
