#pragma once

#include "image.h"
#include "helper.h"

#include "temperature_generator.h"
#include "moisture_generator.h"

struct biome_maps {
    image<float> cells;
    image<float> temperature;
    image<float> moisture;
    image<float> altitude;
};

image<float> generate_cell_map(const image<float>& cells, const image<float>& data);
biome_maps generate_biomes(const image<float>& weights, const image<float>& rivers);
