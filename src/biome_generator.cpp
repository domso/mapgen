#include "biome_generator.h"

image<float> generate_cell_map(const image<float>& cells, const image<float>& data) {
    auto result = cells.copy_shape();

    std::unordered_map<float, std::pair<float, int>> region_stats;
    std::unordered_map<float, float> conversion;

    cells.for_each_pixel([&](auto& p, auto x, auto y) {
        if (p > 0) {
            auto& [sum, count] = region_stats[p];
            count++;
            sum += data.at(x, y);
        }
    });

    for (auto [key, value] : region_stats) {
        auto [sum, count] = value;
        auto avg = sum / count;
        conversion[key] = avg;
    }

    cells.for_each_pixel([&](auto& p, auto x, auto y) {
        if (p > 0) {
            result.at(x, y) = conversion[p];
        }
    });

    return result;
}

biome_maps generate_biomes(const image<float>& weights, const image<float>& rivers) {
    auto cells = mask_to_regions(weights, 1024);
    
    return {
        .cells = cells,
        .temperature = generate_cell_map(cells, generate_temperature(weights)),
        .moisture = generate_cell_map(cells, generate_moisture(rivers, weights)),
        .altitude = generate_cell_map(cells, weights)
    };
}
