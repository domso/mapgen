#include "water.h"

#include "helper.h"
#include "circle_stack.h"

namespace mapgen::generators::water {

using namespace mapgen::generators::water::internal;

std::pair<image<float>, image<float>> generate(const image<float>& mask, const image<float>& terrain, const std::vector<std::pair<image<float>, image<float>>>& shapes) {
    auto scaled_terrain = terrain.rescale(mask.width(), mask.height());

    auto river_regions = mask_to_border_regions(mask, 8);
    auto river_distribution = generate_river_distribution(mask);

    auto water = river_regions.copy_shape();

    for (int i = 0; i < 8; i++) {
        add_river_to_region(river_regions, i + 1.0f, 1000, scaled_terrain, water, river_distribution);
    }

    filter_non_zero_neighbors(water, scaled_terrain);
    scale_range(river_regions);
    scale_range(water);

    auto [scaled_water, end_points] = upscale_river_map(water, scaled_terrain, terrain.width(), terrain.height());
    auto [river_with_depth, river_scaled_map] = generate_river_depth(scaled_water, terrain, end_points);
    apply_lakes(scaled_water, shapes, river_scaled_map, river_with_depth);

    return {river_with_depth, river_scaled_map};
}


namespace internal {

image<float> add_noise_to_img(const image<float>& src, int factor) {
    auto map = src.copy();
    auto dis_noice = random_generator().uniform<int>(0, 10);

    map.for_each_pixel([&](float& p) {
        if (p > 0) {
            p = p + (dis_noice.next() * 0.09) * 10 * factor;
        }
    });

    return map;
}

std::pair<image<std::pair<direction, float>>, std::pair<int, int>> find_single_path_map(const image<float>& src, const int start_x, const int start_y, const int rnd) {
    auto map = add_noise_to_img(src, rnd);
    image<std::pair<direction, float>> path_map(map.width(), map.height());

    path_map.for_each_pixel([](auto& m) {
        m = {direction::undefined, std::numeric_limits<float>::infinity()};
    });
    path_map.at(start_x, start_y) = {direction::end, 0};

    std::queue<std::pair<int, int>> updated_positions;
    updated_positions.push({start_x, start_y});

    bool found = false;
    int dest_x;
    int dest_y;
    float dest_score = std::numeric_limits<float>::infinity();

    auto check_neighbor = [&](const auto current_height, const auto current_score, const auto x, const auto y, const direction set_direction, const direction current_direction) {
        if (auto neighbor_height = map.get(x, y)) {
            auto& [neighbor_direction, neighbor_score] = path_map.at(x, y);
            auto delta = std::max(0.0f, **neighbor_height - current_height);

            if (current_score + delta < neighbor_score) {
                neighbor_direction = set_direction;
                neighbor_score = current_score + delta;
                if (neighbor_score < dest_score && **neighbor_height > 0) {
                    updated_positions.push({x, y});
                }

                if (**neighbor_height == 0 && neighbor_score < dest_score) {
                    found = true;
                    dest_x = x;
                    dest_y = y;
                    dest_score = neighbor_score;
                }
            }
        }
    };

    while (!updated_positions.empty()) {
        auto [x, y] = updated_positions.front();
        auto& [d, current_score] = path_map.at(x, y);
        auto& current_height = map.at(x, y);
        check_neighbor(current_height, current_score, x - 1, y, direction::east, d);
        check_neighbor(current_height, current_score, x + 1, y, direction::west, d);
        check_neighbor(current_height, current_score, x, y - 1, direction::south, d);
        check_neighbor(current_height, current_score, x, y + 1, direction::north, d);
        
        updated_positions.pop();
    }
    assert(found);

    return {path_map, {dest_x, dest_y}};
}

std::vector<std::pair<int, int>> find_single_path(const image<float>& src, const int start_x, const int start_y, const int rnd) {
    auto [path_map, dest_pos] = find_single_path_map(src, start_x, start_y, rnd);
    auto [dest_x, dest_y] = dest_pos;

    std::vector<std::pair<int, int>> result;
    while (path_map.at(dest_x, dest_y).first != direction::end) {
        result.push_back({dest_x, dest_y});
        auto [direction, score] = path_map.at(dest_x, dest_y);

        if (direction == direction::north) {
            dest_y--;
        }
        if (direction == direction::east) {
            dest_x++;
        }
        if (direction == direction::south) {
            dest_y++;
        }
        if (direction == direction::west) {
            dest_x--;
        }
    }
    result.push_back({dest_x, dest_y});

    return result;
}

int compute_rivers_in_region(const image<float>& region_map, const float region, const int rivers) {
    size_t region_size = 0;
    region_map.for_each_pixel([&](auto& p, auto x, auto y) {
        if (p > 0 && p == region) {
            region_size++;
        }
    });
    float relative_region_size = static_cast<float>(region_size) / region_map.size();
    int rivers_in_region = rivers * relative_region_size;

    return rivers_in_region;
}

std::pair<int, int> generate_river_start_point(uniform_random_number<int>& dis_x, uniform_random_number<int>& dis_y, uniform_random_number<float>& dis_dice, const image<float>& region_map, const image<float>& river_map, const image<float>& wet_map, const float region) {
    bool retry = true;
    int n = 0;
    int x;
    int y;
    while (retry && n < 100000) {
        x = dis_x.next();
        y = dis_y.next();
        auto dice = dis_dice.next();

        auto p = region_map.get(x, y);
        auto r = river_map.get(x, y);
        auto w = wet_map.get(x, y);
        if (p && r && w && **p == region && **r == 0 && **w >= dice) {
            retry = false;
        }

        n++;
    }

    return {x, y};
}

void add_river_to_region(const image<float>& region_map, const float region, const int rivers, const image<float>& weights_map, image<float>& river_map, const image<float>& wet_map) {
    random_generator rnd;
    auto dis_x = rnd.uniform<int>(0, region_map.width());
    auto dis_y = rnd.uniform<int>(0, region_map.height());
    auto dis_dice = rnd.uniform<float>(0, 1.0f);

    auto closed_region = weights_map.copy();
    region_map.for_each_pixel([&](auto& p, auto x, auto y) {
        if (p > 0 && p != region) {
            closed_region.at(x, y) = std::numeric_limits<float>::infinity();
        }
    });

    auto rivers_in_region = compute_rivers_in_region(region_map, region, rivers);

    for (int i = 0; i < rivers_in_region; i++) {
        auto [x, y] = generate_river_start_point(dis_x, dis_y, dis_dice, region_map, river_map, wet_map, region);
        auto track = find_single_path(closed_region, x, y, 3);
        if (i == 0) {
            closed_region.for_each_pixel([&](auto& p, auto x, auto y) {
                if (p < 0.1) {
                    p = 100;
                }
            });
        }

        for (const auto& [pos_x, pos_y] : track) {
            river_map.at(pos_x, pos_y) = region;
            closed_region.at(pos_x, pos_y) = 0;
        }
    }
}

image<float> generate_river_distribution(const image<float>& mask) {
    auto result = mask_to_regions(mask, 10000);
    std::unordered_map<float, float> conversion;

    result.for_each_pixel([&](auto& p) {
        conversion[p] = 0.0f;
    });

    for (auto& [key, value] : conversion) {
        value = random_float(0.0f, 1.0f);
    }

    conversion[0] = 0.0f;
    result.for_each_pixel([&](auto& p) {
        p = conversion[p];
    });

    auto ocean_distance = generate_ocean_distance_map(result);

    result.for_each_pixel([&](auto& p, auto x, auto y) {
        auto d = ocean_distance.at(x, y);
        auto r = static_cast<float>(d) / 100.0;
        r = std::max(0.0, std::log((r * 3.5) + 0.5) + 0.3f);
        p *= r;
    });

    scale_range(result);

    return result;
}

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

void apply_circular_set(image<float>& target, const float color, const image<float>& circle, const int x, const int y) {
    auto dx = circle.width() / 2;
    auto dy = circle.height() / 2;

    for (auto yi = 0; yi < circle.height(); yi++) {
        for (auto xi = 0; xi < circle.width(); xi++) {
            if (auto t = target.get(x - dx + xi, y - dy + yi)) {
                if (circle.at(xi, yi) > 0 && **t == 0) {
                    **t = color;
                }
            }
        }
    }
}

std::pair<image<float>, image<float>> generate_river_depth(const image<float>& rivers, const image<float>& weights, const std::vector<std::pair<size_t, size_t>>& river_endpoints) {
    auto [levels, heights] = generate_river_level_and_height(rivers, weights, river_endpoints);

    circle_stack circles;

    auto copy = weights.copy();
    auto copy2 = weights.copy();

    float max = 0;
    levels.for_each_pixel([&](auto& p, int x, int y) {
        if (p > 0) {
            p = std::log(p);
            max = std::max(max, p);
        }
    });

    levels.for_each_pixel([&](auto& p, int x, int y) {
        if (p > 0) {
            p = 1.0f + (p / max) * 64;
        }
    });

    levels.for_each_pixel([&](auto& p, int x, int y) {
        if (p > 0) {
            apply_circular_set(levels, -1.0f * p, circles.at(p), x, y);
            apply_circular_set(heights, -1.0f * heights.at(x, y), circles.at(p), x, y);
        }
    });

    levels.for_each_pixel([&](auto& p, int x, int y) {
        p = std::abs(p);
    });
    heights.for_each_pixel([&](auto& p, int x, int y) {
        p = std::abs(p);
    });

    levels.for_each_pixel([&](auto p, int x, int y) {
        if (p > 0) {
            copy2.at(x, y) = heights.at(x, y);
            if (copy2.at(x, y) > 0.1) {
                apply_circular_shift(copy, copy2, circles.at(p), x, y);
            }
        }
    });

    levels.for_each_pixel([&](auto& p, int x, int y) {
        if (p > 0) {
            p = 1.0f;
        } else {
            p = 0.0f;
        }
    });

    return {copy, levels};
}

void filter_non_zero_neighbors(image<float>& water, const image<float>& terrain) {
    water.for_each_pixel([&](auto& p, auto x, auto y) {
        auto top = terrain.get(x, y - 1);
        auto bot = terrain.get(x, y + 1);
        auto left = terrain.get(x - 1, y);
        auto right = terrain.get(x + 1, y);

        bool has_non_zero_neighbor = (top && **top > 0) || (bot && **bot > 0) || (left && **left > 0) || (right && **right > 0);

        if (p > 0 && terrain.at(x, y) == 0 && !has_non_zero_neighbor) {
            p = 0.0f;
        }
    });
}

image<std::pair<float, float>> generate_water_scale_cell_position(const image<float>& water, const size_t width, const size_t height) {
    image<std::pair<float, float>> scaled_water_pos(water.width(), water.height());

    water.for_each_pixel([&](auto& p, auto x, auto y) {
        auto scale_x = static_cast<float>(width) / water.width();
        auto scale_y = static_cast<float>(height) / water.height();

        float set_right = random_float(0, 1.0);
        float set_bot = random_float(0, 1.0);

        if (auto right = water.get(x + 1, y)) {
            if (auto bot = water.get(x, y + 1)) {
                if (**right == p && **bot == p) {
                    if (set_right <= 0.5) {
                        set_bot = random_float(0.0, 0.5);
                    } else {
                        set_bot = random_float(0.5, 1.0);
                    }
                }
            }
        }

        if (auto top = scaled_water_pos.get(x, y - 1)) {
            auto [local_right, local_bot] = **top;

            if (water.at(x, y - 1) == p) {
                if (local_bot <= 0.5) {
                    set_right = random_float(0.5, 1.0);
                } else {
                    set_right = random_float(0, 0.5);
                }
            }
        }
        if (auto left = scaled_water_pos.get(x - 1, y)) {
            auto [local_right, local_bot] = **left;

            if (water.at(x - 1, y) == p) {
                if (local_right <= 0.5) {
                    set_bot = random_float(0.5, 1.0);
                } else {
                    set_bot = random_float(0, 0.5);
                }
            }
        }

        scaled_water_pos.at(x, y) = {
            set_right,
            set_bot
        };
    });

    return scaled_water_pos;
}

std::pair<image<float>, std::vector<std::pair<size_t, size_t>>> upscale_river_map(const image<float>& river_map, const image<float>& terrain, const size_t width, const size_t height) {
    image<float> scaled_rivers(width, height);
    auto positions = generate_water_scale_cell_position(river_map, width, height);

    auto check_water_manhatten = [&](auto x, auto y, auto p) {
        if (auto neighbor = river_map.get(x, y)) {
            return (**neighbor == p);
        }

        return false;
    };
    std::vector<std::pair<size_t, size_t>> end_points;
    river_map.for_each_pixel([&](auto& p, auto x, auto y) {
        if (p == 0) {
            return;
        }
        auto scale_x = static_cast<float>(scaled_rivers.width()) / river_map.width();
        auto scale_y = static_cast<float>(scaled_rivers.height()) / river_map.height();

        float top = -1;
        float bot = -1;
        float left = -1;
        float right = -1;

        if (check_water_manhatten(x + 1, y, p)) {
            auto [local_right, local_bot] = positions.at(x, y);
            right = local_right;            
        }
        if (check_water_manhatten(x - 1, y, p)) {
            auto [local_right, local_bot] = positions.at(x - 1, y);
            left = local_right;            
        }
        if (check_water_manhatten(x, y + 1, p)) {
            auto [local_right, local_bot] = positions.at(x, y);
            bot = local_bot;            
        }
        if (check_water_manhatten(x, y - 1, p)) {
            auto [local_right, local_bot] = positions.at(x, y - 1);
            top = local_bot;            
        }

        auto top_x = scale_x * (x + top);
        auto top_y = scale_y * (y);
        auto bot_x = scale_x * (x + bot);
        auto bot_y = scale_y * (y + 1.0f);
        auto right_x = scale_x * (x + 1.0f);
        auto right_y = scale_y * (y + right);
        auto left_x = scale_x * (x);
        auto left_y = scale_y * (y + left);
        auto center_x = scale_x * x;
        auto center_y = scale_y * y;

        int cross = 0;
        if (top >= 0 && bot >= 0) {
            draw_random_manhattan_line(scaled_rivers, top_x, top_y, bot_x, bot_y, p);
            cross++;
        }
        if (left >= 0 && right >= 0) {
            draw_random_manhattan_line(scaled_rivers, left_x, left_y, right_x, right_y, p);
            cross++;
        }
        if (top >= 0 && left >= 0 && cross < 2) {
            draw_random_manhattan_line(scaled_rivers, top_x, top_y, left_x, left_y, p);
        }
        if (top >= 0 && right >= 0 && cross < 2 && left < 0) {
            draw_random_manhattan_line(scaled_rivers, top_x, top_y, right_x, right_y, p);
        }
        if (bot >= 0 && left >= 0 && cross < 2 && top < 0) {
            draw_random_manhattan_line(scaled_rivers, bot_x, bot_y, left_x, left_y, p);
        }
        if (bot >= 0 && right >= 0 && cross < 2 && top < 0 && left < 0) {
            draw_random_manhattan_line(scaled_rivers, bot_x, bot_y, right_x, right_y, p);
        }
        if (top >= 0 && bot < 0 && right < 0 && left < 0) {
            draw_random_manhattan_line(scaled_rivers, top_x, top_y, center_x, center_y, p);
            if (terrain.at(x, y) == 0) {
                end_points.push_back({center_x, center_y});
            }
        }
        if (top < 0 && bot >= 0 && right < 0 && left < 0) {
            draw_random_manhattan_line(scaled_rivers, bot_x, bot_y, center_x, center_y, p);
            if (terrain.at(x, y) == 0) {
                end_points.push_back({center_x, center_y});
            }
        }
        if (top < 0 && bot < 0 && right >= 0 && left < 0) {
            draw_random_manhattan_line(scaled_rivers, right_x, right_y, center_x, center_y, p);
            if (terrain.at(x, y) == 0) {
                end_points.push_back({center_x, center_y});
            }
        }
        if (top < 0 && bot < 0 && right < 0 && left >= 0) {
            draw_random_manhattan_line(scaled_rivers, left_x, left_y, center_x, center_y, p);
            if (terrain.at(x, y) == 0) {
                end_points.push_back({center_x, center_y});
            }
        }
    });

    return {scaled_rivers, end_points};
}

std::vector<std::pair<int, int>> compute_lake_position(const image<float>& rivers) {
    std::vector<std::pair<int, int>> lake_position;
    auto lake_dis = random_generator().uniform<float>(0, 10000.0);
    rivers.for_each_pixel([&](auto p, int x, int y) {
        if (p > 0) {
            auto dice = lake_dis.next();        
            if (dice < 1) {
                lake_position.push_back({x, y});
            }
        }
    });

    return lake_position;
}

void apply_lakes(const image<float>& river_map, const std::vector<std::pair<image<float>, image<float>>>& shapes, image<float>& scaled_river_map, image<float>& terrain) {
    auto lake_position = compute_lake_position(river_map);
    random_generator rnd;
    auto lake_shape_dis = rnd.uniform<int>(0, shapes.size() - 1);
    auto lake_size_dis = rnd.uniform<int>(2, 256);
    for (auto [lake_x, lake_y] : lake_position) {
        auto index = lake_shape_dis.next();
        auto [mask, weights] = shapes[index];
        auto size = std::min(lake_size_dis.next(), static_cast<int>(mask.width()));
        auto lake_shape = mask.rescale(std::max(2, size), std::max(2.0f, (static_cast<float>(mask.height()) / mask.width()) * size));
        lake_shape = resize_and_center(lake_shape, lake_shape.width() + 128, lake_shape.height() + 128);

        auto lake_shape_connector_x = rnd.uniform<int>(0, lake_shape.width() - 1);
        auto lake_shape_connector_y = rnd.uniform<int>(0, lake_shape.height() - 1);
        int connector_x = 0;
        int connector_y = 0;

        auto lake_slope_shape = lake_shape.copy();
        lake_slope_shape.for_each_pixel([&](auto& p) {
            if (p > 0) {
                p = 0.0f;
            } else {
                p = 1.0f;
            }
        });
        auto lake_slope_distance = generate_ocean_distance_map(lake_slope_shape);
        lake_slope_distance.for_each_pixel([&](int p, auto x, auto y) {
            float s;
            if (p > 64) {
                s = 0.0f;
            } else {
                s = 1.0f - p / 64.0f;
            }
             lake_slope_shape.at(x, y) = s;
        });

        int n = 0;
        do {
            connector_x = lake_shape_connector_x.next();
            connector_y = lake_shape_connector_y.next();
            n++;
        } while (lake_shape.at(connector_x, connector_y) == 0 && n < 10000);

        auto top_x = lake_x - connector_x;
        auto top_y = lake_y - connector_y;

        if (auto window = scaled_river_map.subregion(top_x, top_y, lake_shape.width(), lake_shape.height())) {
            window->for_each_pixel([&](auto& p, auto x, auto y) {
                if (lake_shape.at(x, y) > 0) {
                    p = 1.0f;
                }
            });
        }
        if (auto window = terrain.subregion(top_x, top_y, lake_shape.width(), lake_shape.height())) {
            float min_height = 1.0f;
            window->for_each_pixel([&](auto& p, auto x, auto y) {
                if (lake_shape.at(x, y) > 0) {
                    min_height = std::min(min_height, p);
                }
            });
            window->for_each_pixel([&](auto& p, auto x, auto y) {
                p = p * (1.0f - lake_slope_shape.at(x, y)) + min_height * lake_slope_shape.at(x, y);
            });
        }
    }
}
}
}
