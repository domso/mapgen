#include <vector>
#include <iostream>

#include "export_ppm.h"
#include "helper.h"
#include "config.h"

#include "generators/noise.h"
#include "generators/shapes.h"
#include "generators/terrain.h"
#include "generators/water.h"
#include "generators/biome.h"

int main() {
    int width = 512;
    int height = 512;
    int scale = 20;
    auto asd = mapgen::generators::noise::generate(width, height, scale, 0.1, 0.4, 0.1);

    export_ppm("region2.ppm", asd);
    //return 0;


    auto result = *import_ppm<float>("region2.ppm");
    auto shapes = mapgen::generators::shapes::generate(result);
    auto [mask, terrain] = mapgen::generators::terrain::generate(shapes, result, width, height);
    auto [a, b] = mapgen::generators::water::generate(mask, terrain, shapes);

    auto biomes = mapgen::generators::biome::generate(terrain.rescale(width, height), b.rescale(width, height));

    export_ppm("rwd.ppm", a);
    export_ppm("blub_r.ppm", b);
    export_ppm("cells.ppm", biomes.cells);
    export_ppm("temperature.ppm", biomes.temperature);
    export_ppm("moisture.ppm", biomes.moisture);
    export_ppm("altitude.ppm", biomes.altitude);

    return 0;
}
