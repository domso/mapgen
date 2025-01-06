#include "temperature_generator.h"

image<float> generate_temperature(const image<float>& map) {
    auto result = map.copy_shape();

    result.for_each_pixel([&](auto& p, auto x, auto y) {
        auto r = static_cast<float>(y) / map.height();
        p = 1.0 / (1.0 + std::exp(-0.5 * (r * 12 - 6)));
    });

    auto regions = mask_to_regions(map, 1024);
    std::unordered_map<float, float> conversion;

    regions.for_each_pixel([&](auto& p) {
        conversion[p] = 0.0f;
    });

    for (auto& [key, value] : conversion) {
        value = random_float(0.99f, 1.01f);
    }

    conversion[0] = 1.0f;
    regions.for_each_pixel([&](auto& p) {
        p = conversion[p];
    });

    result.for_each_pixel([&](auto& p, auto x, auto y) {
        p = p * regions.at(x, y) * (1.0f - map.at(x, y));
    });

    scale_range(result);

    return result;
}
