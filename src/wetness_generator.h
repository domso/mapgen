#pragma once

#include <map>
#include <queue>

#include "image.h"
#include "helper.h"
#include "gaussian_generator.h"

image<int> generate_ocean_distance_map(const image<float>& mask) {
    image<int> result(mask.width(), mask.height());

    std::queue<std::pair<int, int>> remaining;

    mask.for_each_pixel([&](auto p, int x, int y) {
        if (p == 0) {
            result.at(x, y) = 0;
            remaining.push({x, y});
        } else {
            result.at(x, y) = mask.width() * mask.height();
        }
    });

    auto check_pos = [&](const auto current, const auto x, const auto y) {
        if (auto p = result.get(x, y)) {
            if (current + 1 < **p) {
                **p = current + 1;
                remaining.push({x, y});
            }
        }
    };

    while (!remaining.empty()) {
        auto [x, y] = remaining.front();
        auto current = result.at(x, y);
        check_pos(current, x + 1, y);
        check_pos(current, x - 1, y);
        check_pos(current, x, y + 1);
        check_pos(current, x, y - 1);
        remaining.pop();
    }

    return result; 
}

image<float> generate_wetness(const image<float>& mask) {
    //auto result = mask.copy();
    auto result = mask_to_regions(mask, 10000);
    std::unordered_map<float, float> conversion;

    result.for_each_pixel([&](auto& p) {
        conversion[p] = 0.0f;
    });

    for (auto& [key, value] : conversion) {
        value = random_float(0.0f, 1.0f);
    }

    conversion[0] = 0.0f;
    result.for_each_pixel([&](auto& p) {
        p = conversion[p];
    });

    auto ocean_distance = generate_ocean_distance_map(result);

    result.for_each_pixel([&](auto& p, auto x, auto y) {
        auto d = ocean_distance.at(x, y);
        auto r = static_cast<float>(d) / 100.0;
        r = std::max(0.0, std::log((r * 3.5) + 0.5) + 0.3f);
        p *= r;
    });

    //add_gaussian_blur(result);
    scale_range(result);

    return result;
}

