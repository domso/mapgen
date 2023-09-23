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

image<float> downscale_img(const image<float>& src, const size_t dest_width, const size_t dest_height);

image<float> extract_non_zero_region(const image<float>& img);

image<float> extract_background(const image<float>& img, const float threshold);

std::vector<int> generate_grayscale_histogram(const image<float>& img);

void apply_relative_threshold(image<float>& img, const float factor);

void apply_circular_fade_out(image<float>& img, const float factor);

void apply_relative_noise(image<float>& img, const float factor);
