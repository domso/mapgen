#pragma once

#include <vector>
#include <tuple>

#include "image.h"

bool update_paths_from_neighbors(image<std::pair<float, int>>& positions, const size_t src_x, const size_t src_y, const size_t dest_x, const size_t dest_y, const float delta, const int delta_step);

bool update_path_position(const image<float>& terrain, image<std::pair<float, int>>& positions, const size_t x, const size_t y);

image<std::pair<float, int>> generate_path(const image<float>& terrain, size_t start_x, size_t start_y);

void export_path_img(image<float>& result, image<float>& heights, const image<float>& terrain, const image<std::pair<float, int>>& path_map, const int dest_x, const int dest_y, const int scale);

std::pair<image<std::pair<float, int>>, image<float>> generate_path_to_random_position(const image<float>& map);

std::pair<image<float>, image<float>> draw_rivers_by_path(const image<std::pair<float, int>>& path, const image<float>& map, const int n, const int scale);

std::tuple<image<float>, image<float>, image<float>> add_rivers_on_map(const image<float>& map, const size_t n, const int scale);

std::tuple<image<float>, image<float>, image<float>> generate_rivers(const image<float>& map, const int scale);

