#include "generate_map.h"

#include <random>

struct config {
    constexpr static const int min_base_noise = 20;
    constexpr static const int max_base_noise = 25;
    constexpr static const int num_hills = 1000;
    constexpr static const int min_hill_size = 4;
    constexpr static const int max_hill_size = 200;
    constexpr static const int min_hill_height = 2;
    constexpr static const int max_hill_height = 3;
    constexpr static const int num_pre_blur = 20;
    constexpr static const int num_post_blur = 20;
    
    constexpr static const int num_erosion = 2;
    constexpr static const float erosion_factor = 0.9;
};

bool random_percentage(std::mt19937& gen, const int percent) {
    std::uniform_int_distribution<int> dis(1, 100);
    return dis(gen) <= percent;
}

bool check_boundaries(const int x, const int y, const int halfSize, const int width, const int height, const int i, const int j) {
    return
        (x - halfSize + i) >= 0 &&
        (x - halfSize + i) < width &&
        (y - halfSize + j) >= 0 &&
        (y - halfSize + j) < height;
}

float calc_distance(const int x, const int y, const int halfSize, const int i, const int j) {
    int diffX = x - (x - halfSize + i);
    int diffY = y - (y - halfSize + j);

    return std::sqrt(diffX * diffX + diffY * diffY);
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

void add_gaussian_blur(std::vector<float>& map, const int width, const int height) {
    std::vector<float> copy = map;
    std::vector<int> kernel = {1,2,1,2,4,2,1,2,1};

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            float tmp = 0;
            float scale = 0;
            
            for (int offset = 0; offset < 9; offset++) {
                int index = (j - 1 + offset / 3) * width + (i - 1 + offset % 3);
                if (index >= 0 && index < map.size()) {
                    tmp += map[index] * kernel[offset];
                    scale += kernel[offset];                    
                }
            }

            copy[j * width + i] = tmp / scale;
        }
    }

    map = copy;
}

void scale_range(std::vector<float>& map) {
    float min = MAXFLOAT;
    float max = 0;

    for (auto f : map) {
        min = std::min<float>(min, f);
        max = std::max<float>(max, f);
    }

    for (auto& f : map) {
        f = (f - min) * (1.0 / (max - min));
    }
}

int min_neighbor(std::mt19937& gen, std::vector<float>& map, const int width, const int height, const int x, const int y) {
    int result = y * width + x;
    float min = map[result];
    
    for (int offset = 0; offset < 9; offset++) {
        int index = (y - 1 + offset / 3) * width + (x - 1 + offset % 3);
        if (index >= 0 && index < map.size()) {            
            if (map[index] < min && random_percentage(gen, 50)) {
                result = index;
                min = map[index];
            }
        }
    }

    return result;
}

void add_erosion(std::mt19937& gen, std::vector<float>& map, const int width, const int height) {
    std::vector<float> erosionMap(width * height, 1);
    std::vector<float> copy = map;

    for (int it = 0; it < config::num_erosion; it++) {
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {
                int current = (j * width) + i;
                if (erosionMap[current] > 0) {
                    int min = min_neighbor(gen, map, width, height, i, j);
                    
                    if (min != current) {
                        erosionMap[current] -= 1;
                        erosionMap[min] += 1;
                        copy[current] *= config::erosion_factor;
                        copy[min] *= config::erosion_factor;
                    }
                }
            }
        }
    }

    map = copy;
}

std::vector<float> generate_map(const int width, const int height) {
    std::vector<float> map(width * height);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(config::min_base_noise, config::max_base_noise);
    std::uniform_int_distribution<int> disWidth(0, width - 1);
    std::uniform_int_distribution<int> disHeight(0, height - 1);
    std::uniform_int_distribution<int> disSize(config::min_hill_size, config::max_hill_size);

    for (auto& i : map) {
        i = dis(gen);
    }

    for (int i = 0; i < config::num_hills; i++) {
        generate_hill(gen, map, width, height, disWidth(gen), disHeight(gen), disSize(gen), config::min_hill_height, config::max_hill_height);
    }

    scale_range(map);
    for (int i = 0; i < config::num_pre_blur; i++) {
        add_gaussian_blur(map, width, height);
    }

    add_erosion(gen, map, width, height);
    
    for (int i = 0; i < config::num_post_blur; i++) {
        add_gaussian_blur(map, width, height);
    }
    scale_range(map);
    return map;
}

