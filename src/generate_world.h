#pragma once

#include "image.h"

void reshape_hills_on_map(image<float>& map);

image<float> add_noise_with_terrains(const image<float>& map, const int width, const int height, const int scale);

std::vector<image<float>> generate_circle_stack(const int n);

image<float> add_circular_trace(const image<float>& map, const image<float>& color);

image<float> generate_scale_circle(const int n);

void apply_mask_on_terrain(image<float>& map, const image<float>& mask);

image<float> generate_base_world(const int width, const int height, const int scale, const float fade_factor, const float threshold, const float noise_factor);

void apply_generated_rivers(const int width, const int height, image<float>& map, const int scale);
