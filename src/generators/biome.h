#pragma once

#include "image.h"
#include "helper.h"


namespace mapgen::generators::biome {

struct biome_maps {
    image<float> cells;
    image<float> temperature;
    image<float> moisture;
    image<float> altitude;
};

biome_maps generate(const image<float>& weights, const image<float>& rivers);

namespace internal {

image<float> generate_cell_map(const image<float>& cells, const image<float>& data);

}
}

