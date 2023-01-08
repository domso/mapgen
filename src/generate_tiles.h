#pragma once

#include <vector>

#include "image.h"

std::vector<image<float>> generate_tiles(const image<float>& map, const image<int>& mask);
