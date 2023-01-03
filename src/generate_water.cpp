#include "generate_water.h"
#include "helper.h"
#include "config.h"

image<float> simulate_water_flow(const image<float>& terrain) {
    image<float> water(terrain.width(), terrain.height(), 1);

    for (int it = 0; it < config::water::num_simulation; it++) {
        for (int j = 0; j < terrain.height(); j++) {
            for (int i = 0; i < terrain.width(); i++) {
                if (water.at(i, j) > 0) {
                    auto [min_x, min_y] = min_neighbor(terrain, i, j);
                    water.at(min_x, min_y) += 1;
                    water.at(i, j) -= 1;
                }
            }
        }
    }

    return water;
}

int check_entry(const image<float>& water, const int x, const int y) {
    if (water.contains(x, y)) {
        if (water.at(x, y) > 0) {
            return 1;
        }
    }
    
    return 0;
}

int check_bounding_box(const image<float>& water, const int start_x, const int start_y) {
    int found = 0;
    for (int x = 0; x < config::water::filterbox_size; x++) {
        found += check_entry(water, start_x + x, start_y);
        found += check_entry(water, start_x + x, start_y + (config::water::filterbox_size - 1));
    }

    for (int y = 0; y < config::water::filterbox_size; y++) {
        found += check_entry(water, start_x, start_y + y);
        found += check_entry(water, start_x + (config::water::filterbox_size - 1), start_y + y);
    }
    
    return found;
}

void apply_filterbox(image<float>& water) {
    for (int j = 0; j < water.height(); j++) {
        for (int i = 0; i < water.width(); i++) {
            int found = check_bounding_box(water, i, j);

            if (found == 0) {
                for (int y = 0; y < config::water::filterbox_size; y++) {
                    for (int x = 0; x < config::water::filterbox_size; x++) {
                        if (water.contains(i + x, j + y)) {
                            water.at(i + x, j + y) = 0;
                        }
                    }
                }
            }
        }
    }
}

void scale_lines(image<float>& water) {
    image<float> copy(water.width(), water.height(), 0);
    
    for (int j = 0; j < water.height(); j++) {
        for (int i = 0; i < water.width(); i++) {
            if (water.at(i, j) > 0) {
                for (int y = 0; y < config::water::line_scale; y++) {
                    for (int x = 0; x < config::water::line_scale; x++) {
                        if (copy.contains(i + x, j + y)) {
                            copy.at(i + x, j + y) = 1;
                        }
                    }
                }
            }
        }
    }
    
    water = copy;
}

void set_water_height(image<float>& water, const image<float>& terrain) {
    for (int y = 0; y < water.height(); y++) {
        for (int x = 0; x < water.width(); x++) {
            if (water.at(x, y) > 0) {
                water.at(x, y) = terrain.at(x, y);
            }
        }
    }
}

void add_preserving_blur(image<float>& water) {
    image<float> copy = water;
    for (int i = 0; i < config::water::num_blur; i++) {
        add_gaussian_blur(water);
        for (int y = 0; y < water.height(); y++) {
            for (int x = 0; x < water.width(); x++) {
                if (copy.at(x, y) > 0) {
                    water.at(x, y) = copy.at(x, y);
                }
            }
        }
    }   
}

void add_base_level(image<float>& water) {
    water.for_each_pixel([](float& c) {
        if (c < config::water::base_level) {
            c = config::water::base_level;
        }
    });
}

image<float> generate_water(const image<float>& terrain) {
    image<float> water = simulate_water_flow(terrain);
    
    apply_filterbox(water);
    scale_lines(water);
    set_water_height(water, terrain);
    add_preserving_blur(water);
    add_base_level(water);    

    return water;
}
