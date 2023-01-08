#pragma once

#include "image.h"

image<float> generate_placement(const size_t width, const size_t height, const std::vector<image<float>>& tiles, const image<float>& background);
