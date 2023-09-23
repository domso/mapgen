#include "generate_terrain.h"

#include <random>
#include "config.h"
#include "helper.h"

void generate_hill(std::mt19937& gen, image<float>& map, const int x, const int y, const int size, const int min, const int max) {
    std::uniform_int_distribution<int> dis(min, max);
    float set_height = dis(gen);
    int fixed_size = size & (~1);
    int half_size = fixed_size / 2;
    bool substract = random_percentage(gen, 50);
    
    for (int j = 0; j <= fixed_size; j++) {
        for (int i = 0; i <= fixed_size; i++) {
            int dest_x = x - half_size + i;
            int dest_y = y - half_size + j;

            if (map.contains(dest_x, dest_y)) {
                float distance = calc_distance(x, y, half_size, i, j);
                if (distance <= half_size) {
                    float offset = (set_height - (set_height / half_size) * distance);
                    if (!substract) {
                        map.at(dest_x, dest_y) += offset;
                    } else {
                        map.at(dest_x, dest_y) -= std::min<float>(map.at(dest_x, dest_y), offset);
                    }
                }
            }
        }
    }
}

void add_erosion(std::mt19937& gen, image<float>& map) {
    image<float> erosion_map(map.width(), map.height(), 1);
    auto copy = map;

    for (int it = 0; it < config::terrain::num_erosion; it++) {
        for (int j = 0; j < map.height(); j++) {
            for (int i = 0; i < map.width(); i++) {
                if (erosion_map.at(i, j) > 0) {
                    auto [min_x, min_y] = min_random_neighbor(gen, map, i, j);
                    
                    if (min_x != i || min_y != j) {
                        erosion_map.at(i, j) -= 1;
                        erosion_map.at(min_x, min_y) += 1;
                        copy.at(i, j) *= config::terrain::erosion_factor;
                        copy.at(min_x, min_y) *= config::terrain::erosion_factor;
                    }
                }
            }
        }
    }

    map = copy;
}

void fade_borders(image<float>& map) {    
    for (int y = 0; y < map.height(); y++) {
        for (int x = 0; x < config::terrain::max_hill_size; x++) {
            double scale = static_cast<double>(x) / static_cast<double>(config::terrain::max_hill_size);
            map.at(x, y) *= scale;
        }
    }
    for (int y = 0; y < config::terrain::max_hill_size; y++) {
        for (int x = 0; x < map.width(); x++) {
            double scale = static_cast<double>(y) / static_cast<double>(config::terrain::max_hill_size);
            map.at(x, y) *= scale;
        }
    }    
    
    for (int y = map.height() - config::terrain::max_hill_size; y < map.height(); y++) {
        for (int x = 0; x < map.width(); x++) {
            double scale = static_cast<double>(map.height() - y) / static_cast<double>(config::terrain::max_hill_size);
            map.at(x, y) *= scale;
        }
    }    
    
    for (int y = 0; y < map.height(); y++) {
        for (int x = map.width() - config::terrain::max_hill_size; x < map.width(); x++) {
            double scale = static_cast<double>(map.width() - x) / static_cast<double>(config::terrain::max_hill_size);
            map.at(x, y) *= scale;
        }
    }    
}

image<float> generate_terrain(const int width, const int height) {
    auto map = generate_unfaded_terrain(width, height);
    fade_borders(map);
    
    return map;
}

image<float> generate_unfaded_terrain(const int width, const int height) {
    image<float> map(width, height);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(config::terrain::min_base_noise, config::terrain::max_base_noise);
    std::uniform_int_distribution<int> dis_width(0, width - 1);
    std::uniform_int_distribution<int> dis_height(0, height - 1);
    std::uniform_int_distribution<int> dis_size(config::terrain::min_hill_size, config::terrain::max_hill_size);


    map.for_each_pixel([&](float& p) {
        p = dis(gen);
    });

    for (int i = 0; i < config::terrain::num_hills; i++) {
        auto s = dis_size(gen);
        
        generate_hill(gen, map, std::min(width - s, std::max(s, dis_width(gen))), std::min(height - s, std::max(s, dis_height(gen))), s, config::terrain::min_hill_height, config::terrain::max_hill_height);
        //generate_hill(gen, map, dis_width(gen), dis_height(gen), s, config::terrain::min_hill_height, config::terrain::max_hill_height);
    }

    scale_range(map);
    for (int i = 0; i < config::terrain::num_pre_blur; i++) {
        add_gaussian_blur(map);
    }

    add_erosion(gen, map);
    
    for (int i = 0; i < config::terrain::num_post_blur; i++) {
        add_gaussian_blur(map);
    }
    scale_range(map);
    
    return map;
}
