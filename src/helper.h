#pragma once

#include <random>
#include <vector>

#include "image.h"
#include "voronoi.h"

bool random_percentage(std::mt19937& gen, const int percent);

float calc_distance(const int x, const int y, const int half_size, const int i, const int j);

void add_gaussian_blur(image<float>& img);

void scale_range(image<float>& img);

std::pair<int, int> min_random_neighbor(std::mt19937& gen, const image<float>& map, const int x, const int y);

std::pair<int, int> min_neighbor(const image<float>& map, const int x, const int y);

image<float> downscale_img(const image<float>& src, const size_t dest_width, const size_t dest_height);

image<float> extract_non_zero_region(const image<float>& img);

image<float> extract_background(const image<float>& img, const float threshold);

std::vector<int> generate_grayscale_histogram(const image<float>& img);

void apply_relative_threshold(image<float>& img, const float factor);

void apply_circular_fade_out(image<float>& img, const float factor);

void apply_relative_noise(image<float>& img, const float factor);

image<float> mask_to_regions(const image<float>& image, const int regions);

int random_integer(const int min, const int max);
float random_float(const float min, const float bound);

void quantify_image_pre_zero(image<float>& image, const int levels);

std::pair<image<float>, image<float>> generate_river_level_and_height(const image<float>& rivers, const image<float>& weights, const std::vector<std::pair<size_t, size_t>>& river_endpoints);
