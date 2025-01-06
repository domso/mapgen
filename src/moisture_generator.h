#pragma once

#include <unordered_map>
#include <vector>

#include "image.h"
#include "helper.h"
#include "circle_stack.h"

image<float> generate_moisture(const image<float>& rivers, const image<float>& weights) {
    std::vector<std::pair<size_t, size_t>> river_endpoints;

    rivers.for_each_pixel([&](auto p, auto x, auto y) {
        if (p > 0 && weights.at(x, y) == 0) {
            river_endpoints.push_back({x, y});
        }
    });

    auto [levels, heights] = generate_river_level_and_height(rivers, weights, river_endpoints);
    auto river_moisture = levels.copy_shape();

    circle_stack circles;

    levels.for_each_pixel([&](auto p, int x, int y) {
        int pi = 10 * std::log(p);
        pi = p;
        if (pi > 0) {
            pi = std::min(512, pi);
            apply_circular_add(river_moisture, 1.0, circles.at(pi), x, y);
        }
    });

    return river_moisture;
}

