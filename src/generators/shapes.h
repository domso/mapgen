#pragma once

#include <vector>

#include "image.h"

namespace mapgen::generators::shapes {

std::vector<std::pair<image<float>, image<float>>> generate(const image<float>& base);

namespace internal {

void label(auto& img);
std::vector<std::pair<image<float>, image<float>>> extract(const auto& img, const auto& src);
void flood_fill(auto& img, const int start_x, const int start_y, const float new_color, std::vector<std::vector<bool>>& visited);

}
}
