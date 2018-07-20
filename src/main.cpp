#include <vector>
#include <iostream>

#include <random>

#include "export_ppm.h"
#include "generate_terrain.h"
#include "generate_water.h"
#include "merge_map.h"

int main() {
    int width = 512;
    int height = 512;

    std::vector<float> terrain = generate_terrain(width, height);
    std::vector<float> water = generate_water(terrain, width, height);

    export_ppm("terrain.ppm", width, height, terrain);
    export_ppm("water.ppm", width, height, water);

    return 0;
}
