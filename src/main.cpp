#include <vector>
#include <iostream>

#include <random>

#include "export_ppm.h"
#include "circle_stack.h"
#include "helper.h"

#include "config.h"

#include "../../cmd/loading_bar.h"


#include <cassert>
#include <cmath>
#include <random>

#include "altitude_generator.h"
#include "wetness_generator.h"

#include "generators/shapes.h"
#include "generators/terrain.h"

#include <random>
#include <algorithm>


#include <vector>
#include <set>
#include <random>
#include <algorithm>

#include <vector>
#include <set>
#include <random>
#include <queue>
#include <algorithm>

#include "generators/water.h"







int main() {
    int width = 512;
    int height = 512;
    int scale = 20;
    //auto asd = generate_base_world(width, height, scale, 0.1, 0.4, 0.1);

    //export_ppm("region2.ppm", asd);
    //return 0;


    auto result = *import_ppm<float>("region2.ppm");
    auto shapes = mapgen::generators::shapes::generate(result);
    auto [mask, terrain] = mapgen::generators::terrain::generate(shapes, result, width, height);
    auto [a, b] = mapgen::generators::water::generate(mask, terrain, shapes);

    export_ppm("rwd.ppm", a);
    export_ppm("blub_r.ppm", b);

    return 0;
}
