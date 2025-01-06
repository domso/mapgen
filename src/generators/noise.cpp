#include "noise.h"

#include "config.h"
#include "helper.h"

#include "random_generator.h"

namespace mapgen::generators::noise::internal {

void generate_hill(random_generator& rnd, image<float>& map, const int x, const int y, const int size, const int min, const int max) {
    auto dis = rnd.uniform<int>(min, max);
    float set_height = dis.next();
    int fixed_size = size & (~1);
    int half_size = fixed_size / 2;
    bool substract = rnd.roll_percentage(50);
    
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

std::pair<int, int> min_random_neighbor(random_generator& rnd, const image<float>& map, const int x, const int y) {
    std::pair<int, int> result = {x, y};
    float min = map.at(x, y);

    for (int dy = -1; dy < 2; dy++) {
        for (int dx = -1; dx < 2; dx++) {
            if (map.contains(x + dx, y + dy)) {
                if (map.at(x + dx, y + dy) < min && rnd.roll_percentage(50)) {
                    result = {x + dx, y + dy};
                    min = map.at(x + dx, y + dy);
                }
            }

        }
    }
    
    return result;
}

void add_erosion(random_generator& rnd, image<float>& map) {
    image<float> erosion_map(map.width(), map.height(), 1);
    auto copy = map;

    for (int it = 0; it < config::terrain::num_erosion; it++) {
        for (int j = 0; j < map.height(); j++) {
            for (int i = 0; i < map.width(); i++) {
                if (erosion_map.at(i, j) > 0) {
                    auto [min_x, min_y] = min_random_neighbor(rnd, map, i, j);
                    
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

image<float> generate_tile(const int width, const int height) {
    image<float> map(width, height);
    random_generator rnd;

    auto dis = rnd.uniform<int>(config::terrain::min_base_noise, config::terrain::max_base_noise);
    auto dis_width = rnd.uniform<int>(0, width - 1);
    auto dis_height = rnd.uniform<int>(0, height - 1);
    auto dis_size = rnd.uniform<int>(config::terrain::min_hill_size, config::terrain::max_hill_size);

    map.for_each_pixel([&](float& p) {
        p = dis.next();
    });

    for (int i = 0; i < config::terrain::num_hills; i++) {
        auto s = dis_size.next();
        
        //generate_hill(gen, map, std::min(width - s, std::max(s, dis_width(gen))), std::min(height - s, std::max(s, dis_height(gen))), s, config::terrain::min_hill_height, config::terrain::max_hill_height);
        generate_hill(rnd, map, dis_width.next(), dis_height.next(), s, config::terrain::min_hill_height, config::terrain::max_hill_height);
    }

    scale_range(map);
    for (int i = 0; i < config::terrain::num_pre_blur; i++) {
        add_gaussian_blur(map);
    }

    add_erosion(rnd, map);
    
    for (int i = 0; i < config::terrain::num_post_blur; i++) {
        add_gaussian_blur(map);
    }
    scale_range(map);

    fade_borders(map, config::terrain::max_hill_size);
    
    return map;
}

std::vector<image<float>> generate_tiles(const int width, const int height, const int n) {
    std::vector<image<float>> tiles;
    std::vector<std::thread> threads;
    std::mutex mutex;

    for (int i = 0; i < std::min(8, n); i++) {
        std::thread t1([&]() {
            std::unique_lock ul(mutex);
            while (true) {
                if (tiles.size() > n) {
                    return;
                }
                ul.unlock();
                image<float> tile = generate_tile(width, height);
                ul.lock();
                tiles.push_back(std::move(tile));
            }
        });

        threads.push_back(std::move(t1));
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    return tiles;
}

void apply_threshold(image<float>& tile, float threshold) {
    if (threshold == 0) {
        threshold = 0.1;
    }
    tile.for_each_pixel([&](float& m) {
        if (m <= threshold) {
            m = threshold;
        }
    });

    scale_range(tile);
}

image<float> generate_segment(const std::vector<image<float>>& tiles, random_generator& rnd, const int width, const int height) {
    image<float> map(width * 2, height * 2);
    auto dis_width = rnd.uniform<int>(0, width);
    auto dis_height = rnd.uniform<int>(0, height);
    auto dis = rnd.uniform<float>(0, 1.0);

    for (int i = 0; i < 128; i++) {
        auto tile = rnd.roll_in(tiles);
        apply_threshold(tile, rnd.uniform<float>(0.1, 0.7).next());

        auto b = dis.next();
        auto x = dis_width.next();
        auto y = dis_height.next();

        if (auto subregion = map.subregion(x, y, width, height)) {
            subregion->for_each_pixel([&](auto& c, auto x, auto y) {
                c += std::max(0.0f, b * tile.at(x, y));
            });
        }
    }
    scale_range(map);
    return map;
}

}

namespace mapgen::generators::noise {

image<float> generate(const int width, const int height, const int scale, const float fade_factor, const float threshold, const float noise_factor) {
    random_generator rnd;

    image<float> map(width * 2 * scale, height * 2 * scale);

    std::vector<image<float>> segments;
    std::vector<image<float>> tiles = internal::generate_tiles(width, height, 512);

    for (int i = 0; i < scale * scale; i++) {
        segments.push_back(internal::generate_segment(tiles, rnd, width, height));
    }

    int i = 0;
    for (int y = 0; y < scale; y++) {
        for (int x = 0; x < scale; x++) {
            image<float>& tile = segments[i];
            i++;
            if (auto region = map.subregion(
                x * tile.width() - 512 * x,
                y * tile.height() - 512 * y,
                tile.width(),
                tile.height()
            )) {
                (*region).add(tile);
            }
        }
    }
    scale_range(map);

    if (auto result = map.subregion(0, 0, (scale + 1) * width, (scale + 1) * height)) {
        return *result;
    }
    return map;
}

}
