#include "terrain.h"

#include "circle_stack.h"

namespace mapgen::generators::terrain {

std::pair<image<float>, image<float>> generate(const std::vector<std::pair<image<float>, image<float>>>& shapes, const image<float>& noise, const int width, const int height) {
    assert(!shapes.empty());
    
    auto [mask, weights] = *shapes.rbegin();

    mask = resize_and_center(mask, width, height);
    weights = resize_and_center(weights, width, height);
    
    mask.for_each_pixel([&](auto& p) {
        if (p < 0.000001f) {
            p = 0.0f;
        }
    });
    weights.for_each_pixel([&](auto& p) {
        if (p < 0.000001f) {
            p = 0.0f;
        }
    });

    auto terrain = internal::create_terrain(weights, noise);
    terrain.for_each_pixel([&](auto& p) {
        if (p < 0.000001f) {
            p = 0.0f;
        }
    });

    return {mask, terrain};
}

namespace internal {

void apply_circular_shift(image<float>& target, const image<float>& map, const image<float>& circle, const int x, const int y) {
    auto dx = circle.width() / 2;
    auto dy = circle.height() / 2;
    auto color = map.at(x, y);

    for (auto yi = 0; yi < circle.height(); yi++) {
        for (auto xi = 0; xi < circle.width(); xi++) {
            if (auto p = map.get(x - dx + xi, y - dy + yi)) {
                if (auto t = target.get(x - dx + xi, y - dy + yi)) {
                    **t = std::min(**t, **p * (1.0f - circle.at(xi, yi)) + color * circle.at(xi, yi));
                }
            }
        }
    }
}

image<std::pair<direction, float>> find_paths(const image<float>& src) {
    auto map = src.copy();
    image<std::pair<direction, float>> path_map(map.width(), map.height());

    path_map.for_each_pixel([](auto& m) {
        m = {direction::undefined, std::numeric_limits<float>::infinity()};
    });

    auto noise = random_generator().uniform<int>(0, 10) ;

    map.for_each_pixel([&](float& p) {
        if (p < 0.01) {
            p = 0;
        }
    });

    map.for_each_pixel([&](float& p) {
        if (p > 0) {
            p = p + (noise.next() * 0.09) * 1;
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

void traverse_paths(image<float>& map, const image<std::pair<direction, float>>& path_map) {
    random_generator rnd;

    auto dis_width = rnd.uniform<int>(0, map.width() - 1);
    auto dis_height = rnd.uniform<int>(0, map.height() - 1);

    scale_range(map);

    auto copy = map.copy();
    image<int> water_level(map.width(), map.height());

    water_level.for_each_pixel([](auto& p) {
        p = 0;
    });

    for (int i = 0; i < 10000; i++) {
        int x, y;
        do {
            x = dis_width.next();
            y = dis_height.next();
        } while (map.at(x, y) == 0.0 && map.at(x, y) > 0.75);

        while (path_map.at(x, y).first != direction::end) {
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

    circle_stack circles;

    water_level.for_each_pixel([&](auto& p, int x, int y) {
        if (p > 0) {
            apply_circular_shift(copy, map, circles.at(4 * p), x, y);
        }
    });

    map = copy;
}

image<float> create_terrain(const image<float>& weights, const image<float>& noise) {
    auto map = weights.copy();
    auto result = noise.copy();

    map.for_each_pixel([&](auto& p, auto x, auto y) {
        p = p * p;
        p = p * p;
        p = 1.0 / (1.0 + std::exp(-0.5 * (p * 12 - 6)));
    });

    scale_range(map);
    for (int i = 0; i < 25; i++) {
        add_gaussian_blur(map);
    }
    scale_range(map);

    auto path_map = find_paths(map);
    traverse_paths(map, path_map);
    scale_range(map);

    auto iv = map.copy();
    iv.for_each_pixel([&](auto& p) {
        if (p < 0.01) {
            p = 1.0f;
        } else {
            p = 0.0f;
        }
    });

    auto distance = generate_ocean_distance_map(iv);
    image<float> coast_map(distance.width(), distance.height());
    coast_map.for_each_pixel([&](auto& p, auto x, auto y) {
        auto d = distance.at(x, y);

        if (d > 20) {
            p = 1.0f;
        } else {
            p = d / 20.0f;
        }
    });

    map = map.rescale(result.width(), result.height());
    coast_map = coast_map.rescale(result.width(), result.height());
    scale_range(map);
    scale_range(coast_map);

    scale_range(result);

    result.for_each_pixel([&](auto& p, auto x, auto y) {
        if (p < 0.01) {
            p = 0.0f;
        }
        auto s = map.at(x, y);

        if (s > 0 && p > 0) {
            p = (s + s * p) / 2;
        } else {
            p = 0;
        }
    });
    scale_range(result);

    result.for_each_pixel([&](auto& p, auto x, auto y) {
        auto c = coast_map.at(x, y);
        if (0 < c && c < 1.0f) {
            if (noise.at(x, y) > c) {
                p = std::max(p, 0.01f);
            }
        }
    });


    scale_range(result);

    return result;
}

}
}
