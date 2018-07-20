#pragma once

#include <random>
#include <vector>

#include "helper.h"

bool random_percentage(std::mt19937& gen, const int percent);

float calc_distance(const int x, const int y, const int halfSize, const int i, const int j);

void add_gaussian_blur(std::vector<float>& map, const int width, const int height);

void scale_range(std::vector<float>& map);

int min_random_neighbor(std::mt19937& gen, const std::vector<float>& map, const int width, const int height, const int x, const int y);

int min_neighbor(const std::vector<float>& map, const int width, const int height, const int x, const int y);


