#pragma once

#include <vector>
#include <cassert>

#include "helper.h"
#include "image.h"
#include "random_generator.h"

namespace mapgen::generators::terrain {

std::pair<image<float>, image<float>> generate(const std::vector<std::pair<image<float>, image<float>>>& shapes, const image<float>& noise, const int width, const int height);

namespace internal {

void apply_circular_shift(image<float>& target, const image<float>& map, const image<float>& circle, const int x, const int y);

enum direction {
    end,
    north,
    east,
    south,
    west,
    undefined
};

image<std::pair<direction, float>> find_paths(const image<float>& src);
void traverse_paths(image<float>& map, const image<std::pair<direction, float>>& path_map);
image<float> create_terrain(const image<float>& weights, const image<float>& noise);

}
}
