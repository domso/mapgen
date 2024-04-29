#include "generate_rivers.h"

#include <random>

#include "helper.h"

bool update_paths_from_neighbors(image<std::pair<float, int>>& positions, const size_t src_x, const size_t src_y, const size_t dest_x, const size_t dest_y, const float delta, const int delta_step) {
    auto [score, step] = positions.at(src_x, src_y);
    auto [neighbor_score, neighbor_step] = positions.at(dest_x, dest_y);

    if (neighbor_score + delta < score) {
        score = neighbor_score + delta;
        step = delta_step;
        positions.at(src_x, src_y) = {score, step};
        
        return true;
    }

    return false;
}

bool update_path_position(const image<float>& terrain, image<std::pair<float, int>>& positions, const size_t x, const size_t y) {
    auto delta = terrain.at(x, y);
    delta = delta * delta * delta * delta;

    bool update = false;
    if (delta >= 0) {
        if (y > 0 && x > 0) {
            update = update_paths_from_neighbors(positions, x, y, x - 1, y - 1, delta, 1) || update;
        }
        if (y > 0) {
            update = update_paths_from_neighbors(positions, x, y, x, y - 1, delta, 2) || update;
        }
        if (y > 0 && x < positions.width() - 1) {
            update = update_paths_from_neighbors(positions, x, y, x + 1, y - 1, delta, 3) || update;
        }
        if (x < positions.width() - 1) {
            update = update_paths_from_neighbors(positions, x, y, x + 1, y, delta, 4) || update;
        }
        if (y < positions.height() - 1 && x < positions.width() - 1) {
            update = update_paths_from_neighbors(positions, x, y, x + 1, y + 1, delta, 5) || update;
        }
        if (y < positions.height() - 1) {
            update = update_paths_from_neighbors(positions, x, y, x, y + 1, delta, 6) || update;
        }
        if (y < positions.height() - 1 && x > 0) {
            update = update_paths_from_neighbors(positions, x, y, x - 1, y + 1, delta, 7) || update;
        }
        if (x > 0) {
            update = update_paths_from_neighbors(positions, x, y, x - 1, y, delta, 8) || update;
        }
    }

    return update;
}


image<std::pair<float, int>> generate_path(const image<float>& terrain, size_t start_x, size_t start_y) {
    image<std::pair<float, int>> result(terrain.width(), terrain.height(), {std::numeric_limits<float>::infinity(), 0});
    size_t tile_offset = 1;
    image<char> change_map(terrain.width() / 128, terrain.height() / 128);
    change_map.for_each_pixel([](char& b) {
        b = 0;
    });

    result.at(start_x, start_y) = {0.0, 0};
    change_map.at(start_x / 128, start_y / 128) = 1;

    bool changed = true;
    int last_number_of_change = change_map.width() * change_map.height();

    while (changed) {
        changed = false;

        change_map.for_each_pixel([&](char& b, const int tx, const int ty) {
            while (b > 0) {
                b = 0;
                for (int y = 0; y < 128; y++) {
                    for (int x = 0; x < 128; x++) {
                        if (update_path_position(terrain, result, tx * 128 + x, ty * 128 + y)) {
                            b = 1;
                        }
                    }
                }

                if (b > 0) {
                    changed = true;
                    if (tx > 0) {
                        change_map.at(tx - 1, ty) = 1;
                    }
                    if (tx < change_map.width() - 1) {
                        change_map.at(tx + 1, ty) = 1;
                    }
                    if (ty > 0) {
                        change_map.at(tx, ty - 1) = 1;
                    }
                    if (ty < change_map.height() - 1) {
                        change_map.at(tx, ty + 1) = 1;
                    }
                }
            }
        });
        int n = 0;
        change_map.for_each_pixel([&](char& b) {
            if (b > 0) {
                n++;
            }
        });

        if (n < last_number_of_change) {
            last_number_of_change = n;
        }

        tile_offset++;
    }

    return result;
}


void export_path_img(image<float>& result, image<float>& heights, const image<float>& terrain, const image<std::pair<float, int>>& path_map, const int dest_x, const int dest_y, const int scale) {
    int current_x = dest_x;
    int current_y = dest_y;
    int prev_x;
    int prev_y;

    bool running = true;
    bool skip_first = true;
    float current_height;

    while (running) {
        const auto& [score, step] = path_map.at(current_x, current_y);

        if (terrain.at(current_x, current_y) > 0.0) { 
            if (skip_first) {
                skip_first = false;
                current_height = terrain.at(current_x, current_y);
            } else if (result.at(current_x, current_y) >= 0.0) {
                bool skip_first_draw = true;
                result.draw_line(scale * prev_x, scale * prev_y, scale * current_x, scale * current_y, [&](auto& f, auto x, auto y) {
                    if (skip_first_draw) {
                        skip_first_draw = false;
                        return;
                    }

                    f += 1.0;
                });

                skip_first_draw = true;
                heights.draw_line(scale * prev_x, scale * prev_y, scale * current_x, scale * current_y, [&](auto& f, auto x, auto y) {
                    if (skip_first_draw) {
                        skip_first_draw = false;
                        return;
                    }

                    current_height = std::min(current_height, terrain.at(x / scale, y / scale));
                    if (f == 0) {
                        f = current_height;
                    } else {
                        f = std::min(f, current_height);
                    }
                });
            } else {
                return;
            }

            prev_x = current_x;
            prev_y = current_y;
        }
        switch (step) {
            case 0: {running = false; break;}
            case 1: {current_y--; current_x--; break;}
            case 2: {current_y--; break;}
            case 3: {current_y--; current_x++; break;}
            case 4: {current_x++; break;}
            case 5: {current_y++; current_x++; break;}
            case 6: {current_y++; break;}
            case 7: {current_y++; current_x--; break;}
            case 8: {current_x--; break;}
            default: break;
        }
    }
}

std::pair<image<std::pair<float, int>>, image<float>> generate_path_to_random_position(const image<float>& map) {
    auto copy = map;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis_width(0, map.width() - 1);
    std::uniform_int_distribution<int> dis_height(0, map.height() - 1);
    std::uniform_int_distribution<int> dis_noice(0, 10);

    int dx, dy;
    do {
        dx = dis_width(gen);
        dy = dis_height(gen);
    } while (copy.at(dx, dy) > 0.1);

    copy.for_each_pixel([&](float& p) {
        if (p > 0) {
            p = p + (0.01 + dis_noice(gen) * 0.009) * 1;
        }
    });

    add_gaussian_blur(copy);
    add_gaussian_blur(copy);
    add_gaussian_blur(copy);
    add_gaussian_blur(copy);

    auto path = generate_path(copy, dx, dy);

    return {path, copy};
}

std::pair<image<float>, image<float>> draw_rivers_by_path(const image<std::pair<float, int>>& path, const image<float>& map, const size_t n, const int scale) {
    image<float> rivers(scale * map.width(), scale * map.height(), 0.0);
    image<float> heights(scale * map.width(), scale * map.height(), 0.0);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis_choice(0, n);
    std::uniform_int_distribution<int> dis_width(0, map.width() - 1);
    std::uniform_int_distribution<int> dis_height(0, map.height() - 1);

    int current_path = 0;
    while (current_path < n) {
        auto x = dis_width(gen);
        auto y = dis_height(gen);
        auto dice = dis_choice(gen);
        auto p = map.at(x, y);
        
        if (p > 0 && p * n > dice) {
            export_path_img(rivers, heights, map, path, x, y, scale);

            current_path++;
        }
    }

    return {rivers, heights};
}

std::tuple<image<float>, image<float>, image<float>> add_rivers_on_map(const image<float>& map, const size_t n, const int scale) {
    image<float> hatches(map.width(), map.height(), 0.0);

    auto [path, noised_map] = generate_path_to_random_position(map);

    path.for_each_pixel([&](std::pair<float, int>& p, int x, int y) {
        if (noised_map.at(x, y) > 0.1) {
            hatches.at(x, y) = 0.25 * p.second;
        }
    });

    auto [rivers, heights] = draw_rivers_by_path(path, noised_map, n, scale);

    return {rivers, hatches, heights};
}

std::tuple<image<float>, image<float>, image<float>> generate_rivers(const image<float>& map, const int scale) {
    auto [rivers, hatches, heights] = add_rivers_on_map(map, map.width() * map.height() * 0.01, scale);

    hatches.for_each_pixel([&](float& p, int x, int y) {
        p = map.at(x, y) + p;
    });

    scale_range(rivers);
    scale_range(hatches);
    scale_range(heights);

    return {rivers, hatches, heights};
}

