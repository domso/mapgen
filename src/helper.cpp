#include "helper.h"

bool random_percentage(std::mt19937& gen, const int percent) {
    std::uniform_int_distribution<int> dis(1, 100);
    return dis(gen) <= percent;
}

float calc_distance(const int x, const int y, const int halfSize, const int i, const int j) {
    int diffX = x - (x - halfSize + i);
    int diffY = y - (y - halfSize + j);

    return std::sqrt(diffX * diffX + diffY * diffY);
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

int min_random_neighbor(std::mt19937& gen, const std::vector<float>& map, const int width, const int height, const int x, const int y) {
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

int min_neighbor(const std::vector<float>& map, const int width, const int height, const int x, const int y) {
    int result = y * width + x;
    float min = map[result];
    
    for (int offset = 0; offset < 9; offset++) {
        int index = (y - 1 + offset / 3) * width + (x - 1 + offset % 3);
        if (index >= 0 && index < map.size()) {            
            if (map[index] < min) {
                result = index;
                min = map[index];
            }
        }
    }

    return result;
}

