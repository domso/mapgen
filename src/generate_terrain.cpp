#include "generate_terrain.h"

#include <random>
#include "config.h"
#include "helper.h"

bool check_boundaries(const int x, const int y, const int halfSize, const int width, const int height, const int i, const int j) {
    return
        (x - halfSize + i) >= 0 &&
        (x - halfSize + i) < width &&
        (y - halfSize + j) >= 0 &&
        (y - halfSize + j) < height;
}

void generate_hill(std::mt19937& gen, std::vector<float>& map, const int width, const int height, const int x, const int y, const int size, const int min, const int max) {
    std::uniform_int_distribution<int> dis(min, max);
    float setHeight = dis(gen);
    int fixedSize = size & (~1);
    int halfSize = fixedSize / 2;
    bool substract = random_percentage(gen, 50);
    
    for (int j = 0; j <= fixedSize; j++) {
        for (int i = 0; i <= fixedSize; i++) {
            if (check_boundaries(x, y, halfSize, width, height, i, j)) {
                float distance = calc_distance(x, y, halfSize, i, j);
                if (distance <= halfSize) {
                    int index = (y - halfSize + j) * width + (x - halfSize + i);
                    float offset = (setHeight - (setHeight / halfSize) * distance);
                    if (!substract) {
                        map[index] += offset;
                    } else {
                        map[index] -= std::min<float>(map[index], offset);
                    }
                }
            }
        }
    }
}

void add_erosion(std::mt19937& gen, std::vector<float>& map, const int width, const int height) {
    std::vector<float> erosionMap(width * height, 1);
    std::vector<float> copy = map;

    for (int it = 0; it < config::terrain::num_erosion; it++) {
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {
                int current = (j * width) + i;
                if (erosionMap[current] > 0) {
                    int min = min_random_neighbor(gen, map, width, height, i, j);
                    
                    if (min != current) {
                        erosionMap[current] -= 1;
                        erosionMap[min] += 1;
                        copy[current] *= config::terrain::erosion_factor;
                        copy[min] *= config::terrain::erosion_factor;
                    }
                }
            }
        }
    }

    map = copy;
}

void fade_borders(std::vector<float>& map, const int width, const int height) {    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < config::terrain::max_hill_size; x++) {
            double scale = static_cast<double>(x) / static_cast<double>(config::terrain::max_hill_size);
            map[y * width + x] *= scale;
        }
    }
    for (int y = 0; y < config::terrain::max_hill_size; y++) {
        for (int x = 0; x < width; x++) {
            double scale = static_cast<double>(y) / static_cast<double>(config::terrain::max_hill_size);
            map[y * width + x] *= scale;
        }
    }    
    
    for (int y = height - config::terrain::max_hill_size; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double scale = static_cast<double>(height - y) / static_cast<double>(config::terrain::max_hill_size);
            map[y * width + x] *= scale;
        }
    }    
    
    for (int y = 0; y < height; y++) {
        for (int x = width - config::terrain::max_hill_size; x < width; x++) {
            double scale = static_cast<double>(width - x) / static_cast<double>(config::terrain::max_hill_size);
            map[y * width + x] *= scale;
        }
    }    
}

std::vector<float> generate_terrain(const int width, const int height) {
    std::vector<float> map(width * height);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(config::terrain::min_base_noise, config::terrain::max_base_noise);
    std::uniform_int_distribution<int> disWidth(0, width - 1);
    std::uniform_int_distribution<int> disHeight(0, height - 1);
    std::uniform_int_distribution<int> disSize(config::terrain::min_hill_size, config::terrain::max_hill_size);

    for (auto& i : map) {
        i = dis(gen);
    }

    for (int i = 0; i < config::terrain::num_hills; i++) {
        auto s = disSize(gen);
        
        generate_hill(gen, map, width, height, std::min(width - s, std::max(s, disWidth(gen))), std::min(height - s, std::max(s, disHeight(gen))), s, config::terrain::min_hill_height, config::terrain::max_hill_height);
    }

    scale_range(map);
    for (int i = 0; i < config::terrain::num_pre_blur; i++) {
        add_gaussian_blur(map, width, height);
    }

    add_erosion(gen, map, width, height);
    
    for (int i = 0; i < config::terrain::num_post_blur; i++) {
        add_gaussian_blur(map, width, height);
    }
    scale_range(map);
    fade_borders(map, width, height);
    
    return map;
}




