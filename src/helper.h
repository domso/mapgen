#pragma once

#include <random>
#include <vector>

#include "image.h"

bool random_percentage(std::mt19937& gen, const int percent);

float calc_distance(const int x, const int y, const int half_size, const int i, const int j);

void add_gaussian_blur(image<float>& img);

void scale_range(image<float>& img);

std::pair<int, int> min_random_neighbor(std::mt19937& gen, const image<float>& map, const int x, const int y);

std::pair<int, int> min_neighbor(const image<float>& map, const int x, const int y);


