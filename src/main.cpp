#include <vector>
#include <iostream>

#include <random>

#include "export_ppm.h"
#include "generate_terrain.h"
#include "generate_multi_terrain.h"
#include "generate_world.h"
#include "helper.h"

#include "config.h"

#include "../../cmd/loading_bar.h"


#include <cassert>
#include <cmath>
#include <random>


int main() {
    int width = 512;
    int height = 512;
    int scale = 20;

    auto result = generate_base_world(width, height, scale, 0.4, 0.3, 0.1);
    apply_generated_rivers(width, height, result, scale);

    export_ppm("final_map.ppm", result);

    return 0;
}

