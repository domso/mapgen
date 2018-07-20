#include "generate_water.h"
#include "helper.h"
#include "config.h"

std::vector<float> simulate_water_flow(const std::vector<float>& terrain, const int width, const int height) {
    std::vector<float> water(width * height, 1);

    for (int it = 0; it < config::water::num_simulation; it++) {
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {
                int current = (j * width) + i;
                if (water[current] > 0) {
                    int min = min_neighbor(terrain, width, height, i, j);
                    water[min] += 1;
                    water[current] -= 1;
                }
            }
        }
    }

    return water;
}

int check_entry(const std::vector<float>& water, const int i) {
    if (i < water.size()) {
        if (water[i] > 0) {
            return 1;
        }
    }
    
    return 0;
}

int check_bounding_box(const std::vector<float>& water, const int width, const int height, const int position) {
    int found = 0;
    for (int x = 0; x < config::water::filterbox_size; x++) {
        found += check_entry(water, position + x);
        found += check_entry(water, position + width * (config::water::filterbox_size - 1) + x);
    }

    for (int y = 0; y < config::water::filterbox_size; y++) {
        found += check_entry(water, position + y * width);
        found += check_entry(water, position + y * width + (config::water::filterbox_size - 1));
    }
    
    return found;
}

void apply_filterbox(std::vector<float>& water, const int width, const int height) {
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            int current = (j * width) + i;
            int found = check_bounding_box(water, width, height, current);

            if (found == 0) {
                for (int y = 0; y < config::water::filterbox_size; y++) {
                    for (int x = 0; x < config::water::filterbox_size; x++) {
                        if (current + y * width + x < water.size()) {
                            water[current + y * width + x] = 0;
                        }
                    }
                }
            }
        }
    }
}

void scale_lines(std::vector<float>& water, const int width, const int height) {
    std::vector<float> copy(width * height, 0);
    
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            int current = (j * width) + i;
            if (water[current] > 0) {
                for (int y = 0; y < config::water::line_scale; y++) {
                    for (int x = 0; x < config::water::line_scale; x++) {
                        if (current + y * width + x < copy.size()) {
                            copy[current + y * width + x] = 1;
                        }
                    }
                }
            }
        }
    }
    
    water = copy;
}

void set_water_height(std::vector<float>& water, const std::vector<float>& terrain, const int width, const int height) {
    for (int i = 0; i < water.size(); i++) {
        if (water[i] > 0) {
            water[i] = terrain[i];// + config::water::height;
        }
    }
}

void add_preserving_blur(std::vector<float>& water, const int width, const int height) {
    std::vector<float> copy = water;
    for (int i = 0; i < config::water::num_blur; i++) {
        add_gaussian_blur(water, width, height);
        for (int i = 0; i < copy.size(); i++) {
            if (copy[i] > 0) {
                water[i] = copy[i];
            }
        }
    }   
}

void add_base_level(std::vector<float>& water) {
    for (auto& c : water) {
        if (c < config::water::base_level) {
            c = config::water::base_level;
        }
    }
}


std::vector<float> generate_water(const std::vector<float>& terrain, const int width, const int height) {
    std::vector<float> water = simulate_water_flow(terrain, width, height);
    
    apply_filterbox(water, width, height);
    scale_lines(water, width, height);
    set_water_height(water, terrain, width, height);
    add_preserving_blur(water, width, height);
    add_base_level(water);    

    return water;
}
