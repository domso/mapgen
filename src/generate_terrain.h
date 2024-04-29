#pragma once

#include <vector>
#include <random>

#include "image.h"

void add_erosion(std::mt19937& gen, image<float>& map);
void fade_borders(image<float>& map);
image<float> generate_terrain(const int width, const int height);
image<float> generate_unfaded_terrain(const int width, const int height);
