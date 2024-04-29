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

enum direction {
    end,
    north,
    east,
    south,
    west,
    undefined
};

image<std::pair<direction, float>> find_paths(image<float>& map) {
    image<std::pair<direction, float>> path_map(map.width(), map.height());

    path_map.for_each_pixel([](auto& m) {
        m = {direction::undefined, std::numeric_limits<float>::infinity()};
    });

    std::uniform_int_distribution<int> dis_noice(0, 10);
    std::random_device rd;
    std::mt19937 gen(rd());

    map.for_each_pixel([&](float& p) {
        if (p > 0) {
            p = p + (dis_noice(gen) * 0.0000009) * 1;
        }
    });

    std::vector<std::vector<std::pair<int, int>>> updated_positions(2);

    for (int y = 0; y < map.height(); y++)  {
        for (int x = 0; x < map.width(); x++)  {
            if (map.at(x, y) == 0.0f) {
                path_map.at(x, y) = {direction::end, 0};
                updated_positions[0].push_back({x, y});
            }
        }
    }

    int current_mode = 0;
    int next_mode = 1;
    while (!updated_positions[current_mode].empty()) {
        std::cout << "found " << updated_positions[current_mode].size() << " updates" << std::endl;
        for (auto [x, y] : updated_positions[current_mode]) {
            auto& [d, current_score] = path_map.at(x, y);
            auto& current_height = map.at(x, y);
            if (auto neighbor_height = map.get(x - 1, y)) {
                auto& [neighbor_direction, neighbor_score] = path_map.at(x - 1, y);
                auto delta = std::max(0.0f, **neighbor_height - current_height);
                if (current_score + delta < neighbor_score) {
                    neighbor_direction = direction::east;
                    neighbor_score = current_score + delta;
                    updated_positions[next_mode].push_back({x - 1, y});
                }
            }
            if (auto neighbor_height = map.get(x + 1, y)) {
                auto& [neighbor_direction, neighbor_score] = path_map.at(x + 1, y);
                auto delta = std::max(0.0f, **neighbor_height - current_height);
                if (current_score + delta < neighbor_score) {
                    neighbor_direction = direction::west;
                    neighbor_score = current_score + delta;
                    updated_positions[next_mode].push_back({x + 1, y});
                }
            }
            if (auto neighbor_height = map.get(x, y - 1)) {
                auto& [neighbor_direction, neighbor_score] = path_map.at(x, y - 1);
                auto delta = std::max(0.0f, **neighbor_height - current_height);
                if (current_score + delta < neighbor_score) {
                    neighbor_direction = direction::south;
                    neighbor_score = current_score + delta;
                    updated_positions[next_mode].push_back({x, y - 1});
                }
            }
            if (auto neighbor_height = map.get(x, y + 1)) {
                auto& [neighbor_direction, neighbor_score] = path_map.at(x, y + 1);
                auto delta = std::max(0.0f, **neighbor_height - current_height);
                if (current_score + delta < neighbor_score) {
                    neighbor_direction = direction::north;
                    neighbor_score = current_score + delta;
                    updated_positions[next_mode].push_back({x, y + 1});
                }
            }
        }
        
        updated_positions[current_mode].clear();
        current_mode = (current_mode + 1) % 2;
        next_mode = (next_mode + 1) % 2;
    }

    return path_map;
}


image<float> generate_scale_circle_2(const int n) {
    image<float> circle(n, n);
    circle.for_each_pixel([&](auto& f, auto x, auto y) {
        auto half = n / 2;
        auto dx = std::abs(static_cast<int>(x) - half);
        auto dy = std::abs(static_cast<int>(y) - half);
        auto distance = std::sqrt(dx * dx + dy * dy);

        if (distance > half) {
            f = 1.0;
        } else {
            f = distance / static_cast<float>(half);
        }
    });

    circle.for_each_pixel([&](auto& f) {
        f = 1.0f - f;
    });

    return circle;
}

void apply_circular_shift(image<float>& target, const image<float>& map, const image<float>& circle, const int x, const int y) {
    auto dx = circle.width() / 2;
    auto dy = circle.height() / 2;
    auto color = map.at(x, y);

    for (auto yi = 0; yi < circle.height(); yi++) {
        for (auto xi = 0; xi < circle.width(); xi++) {
            if (auto p = map.get(x - dx + xi, y - dy + yi)) {
                auto& t = target.at(x - dx + xi, y - dy + yi);
                t = std::min(t, **p * (1.0f - circle.at(xi, yi)) + color * circle.at(xi, yi));
            }
        }
    }
}


void traverse_paths(image<float>& map, const image<std::pair<direction, float>>& path_map) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis_width(300, 2800); //map.width() - 1);
    std::uniform_int_distribution<int> dis_height(300, 2800); //, map.height() - 1);

    auto copy = map;
    image<int> water_level(map.width(), map.height());

    water_level.for_each_pixel([](auto& p) {
        p = 0;
    });

    for (int i = 0; i < 10000; i++) {
        int x, y;
        do {
            x = dis_width(gen);
            y = dis_height(gen);
        } while (map.at(x, y) == 0.0 && map.at(x, y) > 0.75);

        //float height = map.at(x, y);
        while (path_map.at(x, y).first != direction::end) {
            //height = std::min(map.at(x, y), height);
            //map.at(x, y) = height;
            //apply_circular_shift(copy, map, circle, x, y);
            water_level.at(x, y)++;

            auto [direction, score] = path_map.at(x, y);

            if (direction == direction::north) {
                y--;
            }
            if (direction == direction::east) {
                x++;
            }
            if (direction == direction::south) {
                y++;
            }
            if (direction == direction::west) {
                x--;
            }
        }
    }
    std::unordered_map<int, image<float>> circle_stack; 

    water_level.for_each_pixel([&](auto& p, int x, int y) {
        if (p > 0) {
            if (!circle_stack.contains(p)) {
                circle_stack.insert({p, generate_scale_circle_2(4 * p)});
                apply_circular_shift(copy, map, circle_stack.at(p), x, y);
            }

        }
    });

    export_ppm("water.ppm", water_level);

    map = copy;


}


int main() {
    int width = 512;
    int height = 512;
    int scale = 10;


    auto result = generate_base_world(width, height, scale, 0.1, 0.4, 0.1);
    //auto result = *import_ppm<float>("final_map.ppm");
    //    auto path_map = find_paths(result);
    //    traverse_paths(result, path_map);
    //    scale_range(result);
    //export_ppm("final_map_with_rivers.ppm", result);


    ////apply_generated_rivers(width, height, result, scale);
    //std::random_device rd;
    //std::mt19937 gen(rd());



    export_ppm("final_map.ppm", result);

    return 0;
}

