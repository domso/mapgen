#include <vector>
#include <iostream>

#include <random>

#include "export_ppm.h"
#include "generate_terrain.h"
#include "generate_multi_terrain.h"
#include "generate_world.h"
#include "circle_stack.h"
#include "helper.h"

#include "config.h"

#include "../../cmd/loading_bar.h"


#include <cassert>
#include <cmath>
#include <random>

#include "altitude_generator.h"
#include "wetness_generator.h"

void invert_image(image<float>& img) {
    img.for_each_pixel([](auto& p) {
        p = 1.0f - p;
    });
}

image<float> center_reformat_image(const image<float>& img, const size_t width, const size_t height) {
    image<float> result(width, height);

    auto dx = width - img.width();
    auto dy = height - img.height();

    result.for_each_pixel([&](auto& p, auto x, auto y) {
        auto sx = x - dx / 2;
        auto sy = y - dy / 2;

        if (auto s = img.get(sx, sy)) {
            p = **s;
        }
    });

    return result;
}

void draw_manhattan_line(image<float>& img, const int start_x, const int start_y, const int end_x, const int end_y, const float color) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0, 1.0);

    int x = start_x;
    int y = start_y;

    // Calculate total distances to move
    const int dx = std::abs(end_x - start_x);
    const int dy = std::abs(end_y - start_y);

    // Determine direction of movement
    const int sx = (start_x < end_x) ? 1 : -1;
    const int sy = (start_y < end_y) ? 1 : -1;

    // Draw first pixel
    if (img.contains(x, y)) {
        img.at(x, y) = color;
    }

    // Keep track of error to decide whether to move horizontally or vertically
    float error = 0.0f;
    const float slope = (dx == 0) ? std::numeric_limits<float>::infinity()
                                 : static_cast<float>(dy) / static_cast<float>(dx);

    while (x != end_x || y != end_y) {
        if (x == end_x) {
            // Only vertical movement remaining
            y += sy;
        }
        else if (y == end_y) {
            // Only horizontal movement remaining
            x += sx;
        }
        else {
            // Decide whether to move horizontally or vertically
            const float error_if_horizontal = std::abs(slope - (static_cast<float>(std::abs(y - start_y)) /
                                                              static_cast<float>(std::abs(x + sx - start_x))));
            const float error_if_vertical = std::abs(slope - (static_cast<float>(std::abs(y + sy - start_y)) /
                                                            static_cast<float>(std::abs(x - start_x))));

            auto dice = dis(gen);
            if (dice < 0.3) {
                x += sx;
            } else if (dice < 0.6) {
                y += sy;
            } else {
                if (error_if_horizontal < error_if_vertical) {
                    x += sx;
                } else {
                    y += sy;
                }
            }
        }

        if (img.contains(x, y)) {
            img.at(x, y) = color;
        }
    }
}

void remove_redundant_line(image<float>& img) {
    img.for_each_pixel([&](auto& p, auto x, auto y) {
        auto N = (y > 0) ? img.at(x, y - 1) > 0.0f : false;
        auto S = (y < img.height() - 1) ? img.at(x, y + 1) > 0.0f : false;
        auto W = (x > 0) ? img.at(x - 1, y) > 0.0f : false;
        auto E = (x < img.width() - 1) ? img.at(x + 1, y) > 0.0f : false;

        auto NW = (x > 0 && y > 0) ? img.at(x - 1, y - 1) > 0.0f : false;
        auto NE = (x < img.width() - 1 && y > 0) ? img.at(x + 1, y - 1) > 0.0f : false;
        auto SW = (x > 0 && y < img.height() - 1) ? img.at(x - 1, y + 1) > 0.0f : false;
        auto SE = (x < img.width() - 1 && y < img.height() - 1) ? img.at(x + 1, y + 1) > 0.0f : false;

        int count = N + S + W + E + NW + NE + SW + SE;

        auto required =
            (N && W && !NW && count < 7) ||
            (N && E && !NE && count < 7) ||
            (N && S && (!NW || !W || !SW) && (!NE || !E || !SE)) ||
            (E && W && (!NW || !N || !NE) && (!SW || !S || !SE)) ||
            (S && E && !SE && count < 7) ||
            (S && W && !SW && count < 7);
        if (!required) {
            p = 0.0f;
        }
    });
}


void apply_color_segmentation_at(image<float>& img, const float t) {
    std::vector<int> histogram(256, 0);

    int count = img.width() * img.height();
    img.for_each_pixel([&](auto& p) {
        assert(p >= 0);
        int v = static_cast<int>(p * 256);
        v = std::min(255, v);
        histogram[v]++;
    });
    int black_count = t * count;
    float threshold = 0;
    
    int sum = 0;
    for (int i = 0; i < 256; i++) {
        sum += histogram[i];        

        if (sum >= black_count) {
            threshold = static_cast<float>(i) / 256.0f;
            break;
        }
    }
    
    img.for_each_pixel([&](auto& p) {
        if (p >= threshold) {
            p = 1.0f;
        } else {
            p = 0.0f;
        }
    });
}

#include <vector>
#include <queue>
#include <utility>

// Iterative flood fill function using a queue
void flood_fill(auto& img, int start_x, int start_y, float new_color, std::vector<std::vector<bool>>& visited) {
    std::queue<std::pair<int, int>> q;
    q.push({start_x, start_y});
    visited[start_y][start_x] = true;
    img.at(start_x, start_y) = new_color;

    // Define the 8 directions: horizontal, vertical, and diagonal neighbors
    const int dx[] = {-1, -1, -1,  0, 0,  1, 1, 1};
    const int dy[] = {-1,  0,  1, -1, 1, -1, 0, 1};

    while (!q.empty()) {
        int x = q.front().first;
        int y = q.front().second;
        q.pop();

        // Check all 8 neighboring pixels
        for (int i = 0; i < 8; i++) {
            int new_x = x + dx[i];
            int new_y = y + dy[i];

            // Check boundaries and if pixel is white and not visited
            if (new_x >= 0 && new_x < img.width() && new_y >= 0 && new_y < img.height() && !visited[new_y][new_x] && img.at(new_x, new_y) == 1.0f) {
                visited[new_y][new_x] = true;
                img.at(new_x, new_y) = new_color;
                q.push({new_x, new_y});
            }
        }
    }
}

// Main function to label connected components
void label_connected_components(auto& img) {
    std::vector<std::vector<bool>> visited(img.height(), std::vector<bool>(img.width(), false));
    float current_color = 1.0f; // Start with a gray color, increment for each region

    img.for_each_pixel([&](auto p, auto x, auto y) {
        if (!visited[y][x] && p == 1.0f) {  // If pixel is white and not visited
            flood_fill(img, x, y, current_color, visited);
            current_color += 1.0f;  // Increment color for next region
        }
    });
}

std::vector<std::pair<image<float>, image<float>>> extract_components(const auto& img, const auto& src) {
    std::vector<std::pair<image<float>, image<float>>> result;
    std::unordered_map<float, std::tuple<size_t, size_t, size_t, size_t>> comps;

    img.for_each_pixel([&](const auto& p, auto x, auto y) {
        auto it = comps.find(p);
        if (it != comps.end()) {
            auto& [min_x, min_y, max_x, max_y] = it->second;
            min_x = std::min(min_x, x);
            min_y = std::min(min_y, y);
            max_x = std::max(max_x, x);
            max_y = std::max(max_y, y);
        } else {
            if (p > 0) {
                comps.insert({p, {x, y, x, y}});
            }
        }
    });

    for (const auto& [s, pos] : comps) {
        auto& [min_x, min_y, max_x, max_y] = pos;
        auto width = max_x - min_x + 1;
        auto height = max_y - min_y + 1;
        
        if (width * height > 25) {
            if (auto mask_tile = img.subregion(min_x, min_y, width, height)) {
                if (auto src_tile = src.subregion(min_x, min_y, width, height)) {
                    auto mask = mask_tile->copy();
                    auto weights = src_tile->copy();

                    mask.for_each_pixel([&](auto& p, auto x, auto y) {
                        if (s != p) {
                            p = 0;
                            weights.at(x, y) = 0;
                        } else {
                            p = 1.0;
                        }
                    });
                    result.push_back({mask, weights});
                }
            }
        }
    }

    std::sort(result.begin(), result.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first.width() * lhs.first.height() < rhs.first.width() * rhs.first.height();
    });

    return result;
}

void shuffle_shape_library(std::vector<image<float>>& imgs) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    std::vector<std::pair<float, image<float>>> scores;
    for (const auto& img : imgs) {
        float size = img.width() * img.height();
        scores.push_back({dist(gen) * size, img});
    }

    std::sort(scores.begin(), scores.end(), [](const auto& a, const auto& b) {
        return a.first > b.first; 
    });

    imgs.clear();
    for (const auto& [score, img] : scores) {
        imgs.push_back(img);
    }
}

size_t non_zero_count(const image<float>& img) {
    size_t result = 0;
    img.for_each_pixel([&](auto& p) {
        result += (p > 0);
    });
    return result;
}


image<float> combine_until(const std::vector<image<float>>& imgs, const int width, const int height) {
    image<float> result(width, height);
    int total_non_zeros = 0;
    int non_zero_threshold = width * height * 0.5;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    size_t main_island_index = imgs.size() * 0.90 + imgs.size() * 0.10 * dist(gen);
    main_island_index = std::min(imgs.size() - 1, main_island_index);
    main_island_index = imgs.size() - 1;

    auto main_island = imgs[main_island_index].copy();

    main_island = main_island.rescale(width * 0.75, height * 0.75);

    int x = result.width() / 2 - main_island.width() / 2;
    int y = result.height() / 2 - main_island.height() / 2;

    if (auto tile = result.subregion(x, y, main_island.width(), main_island.height())) {
        main_island.copy_to(*tile);
    }
    
    for (const auto& tile : imgs) {
        int new_non_zeros = non_zero_count(tile);
        if (total_non_zeros + new_non_zeros < non_zero_threshold) {
            bool retry = true;
            int n = 0;
            while (retry && n < 100) {
                auto x = static_cast<int>(dist(gen) * (width - 1 - tile.width()));
                auto y = static_cast<int>(dist(gen) * (height - 1 - tile.height()));

                if (auto sr = result.subregion(x, y, tile.width(), tile.height())) {
                    if (non_zero_count(*sr) == 0) {
                        retry = false;
                        total_non_zeros += new_non_zeros;
                        tile.copy_to(*sr);
                    }
                }
                n++;
            }
        }
    }

    scale_range(result);
    return result;
}

#include <random>
#include <algorithm>


#include <vector>
#include <set>
#include <random>
#include <algorithm>

#include <vector>
#include <set>
#include <random>
#include <queue>
#include <algorithm>

struct Point { int x, y; };

// Helper function to find a non-border pixel in a component
Point find_interior_point(const auto& img, int label, int min_x, int min_y, int max_x, int max_y) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::vector<Point> interior_points;

    for(int y = min_y + 1; y < max_y; y++) {
        for(int x = min_x + 1; x < max_x; x++) {
            if(static_cast<int>(img.at(x, y)) == label) {
                bool is_interior = true;
                for(int dy = -1; dy <= 1 && is_interior; dy++) {
                    for(int dx = -1; dx <= 1; dx++) {
                        if(static_cast<int>(img.at(x + dx, y + dy)) != label) {
                            is_interior = false;
                            break;
                        }
                    }
                }
                if(is_interior) interior_points.push_back({x, y});
            }
        }
    }

    if(interior_points.empty()) {
        // Fallback to any pixel of the component if no interior found
        for(int y = min_y; y <= max_y; y++) {
            for(int x = min_x; x <= max_x; x++) {
                if(static_cast<int>(img.at(x, y)) == label) {
                    return {x, y};
                }
            }
        }
    }

    std::uniform_int_distribution<> dis(0, interior_points.size() - 1);
    return interior_points[dis(gen)];
}

// Compute distances using BFS
void compute_distances(const auto& img, Point seed, int label,
                      std::vector<std::vector<int>>& distances,
                      int min_x, int min_y, int width, int height) {
    std::queue<Point> q;
    distances[seed.y - min_y][seed.x - min_x] = 0;
    q.push(seed);

    const int dx[] = {-1, 0, 1, 0};
    const int dy[] = {0, -1, 0, 1};

    while(!q.empty()) {
        Point p = q.front();
        q.pop();

        for(int i = 0; i < 4; i++) {
            int nx = p.x + dx[i];
            int ny = p.y + dy[i];

            if(nx >= min_x && nx < min_x + width &&
               ny >= min_y && ny < min_y + height &&
               static_cast<int>(img.at(nx, ny)) == label &&
               distances[ny - min_y][nx - min_x] == -1) {
                distances[ny - min_y][nx - min_x] = distances[p.y - min_y][p.x - min_x] + 1;
                q.push({nx, ny});
            }
        }
    }
}

auto apply_fade_extension(auto& img) {
    image<float> distances(img.width(), img.height());

    img.for_each_pixel([&](auto p, auto x, auto y) {
        if (p > 0) {
            distances.at(x, y) = 0.0f;
        } else {
            distances.at(x, y) = img.width() + img.height();
        }
    });

    bool update = true;
    auto update_call = [&](const float p, const float s) {
        if (s + 1 < p) {
            update = true;
            return s + 1;
        } else {
            return p;
        }
    };
    while (update) {
        update = false;
        distances.for_each_pixel([&](auto& p, auto x, auto y) {
            if (p != 0) {
                if (auto top = distances.get(x, y - 1)) {
                    p = update_call(p, **top);
                }
                if (auto bot = distances.get(x, y + 1)) {
                    p = update_call(p, **bot);
                }
                if (auto left = distances.get(x - 1, y)) {
                    p = update_call(p, **left);
                }
                if (auto right = distances.get(x + 1, y)) {
                    p = update_call(p, **right);
                }
            }
        });
    }

    scale_range(distances);

    invert_image(distances);

    img = distances;
}

auto extract_complete_components(auto& img, const auto& src) {
    // 1. Choose random 512x512 window
    std::random_device rd;
    std::mt19937 gen(rd());

    // 2. Collect all labels in the random window
    std::set<int> labels;
    bool retry = true;

    while (retry) {
        labels.clear();
        retry = false;

        std::uniform_int_distribution<> dis_x(512, img.width() - 512);
        std::uniform_int_distribution<> dis_y(512, img.height() - 512);

        int start_x = dis_x(gen);
        int start_y = dis_y(gen);

        int non_zeros = 0;
        int totals = 0;

        for(int y = start_y; y < start_y + 512 && y < img.height(); ++y) {
            for(int x = start_x; x < start_x + 512 && x < img.width(); ++x) {
                float val = img.at(x, y);
                if(val > 0.0f) {  // Ignore background (0)
                    labels.insert(static_cast<int>(val));
                    non_zeros++;
                }
                totals++;
            }
        }

        retry = non_zeros*3 < totals;
        std::cout << non_zeros << " " << totals << std::endl;
    }

    labels.erase(1);

    // 3. Find bounding box for all components with these labels
    int min_x = img.width(), min_y = img.height();
    int max_x = 0, max_y = 0;
    bool first = true;

    // Update labels set with all labels in the expanded region
    std::set<int> expanded_labels;

    img.for_each_pixel([&](auto p, auto x, auto y) {
        if(p > 0.0f && labels.count(static_cast<int>(p))) {
            min_x = std::min(min_x, static_cast<int>(x));
            min_y = std::min(min_y, static_cast<int>(y));
            max_x = std::max(max_x, static_cast<int>(x));
            max_y = std::max(max_y, static_cast<int>(y));
            first = false;
        }
    });

    if(first) {  // No components found
        return *img.subregion(0, 0, 1, 1);  // Return minimal region
    }


{
    // Make bounding box rectangular by making width equal to height
    int width = max_x - min_x + 1;
    int height = max_y - min_y + 1;
    int max_size = std::max(width, height);

    // Adjust bounds while ensuring we don't exceed image dimensions
    int x_adjustment = (max_size - width) / 2;
    int y_adjustment = (max_size - height) / 2;

    min_x = std::max(0, min_x - x_adjustment);
    min_y = std::max(0, min_y - y_adjustment);

    max_x = min_x + max_size - 1;
    max_y = min_y + max_size - 1;

    // Ensure we don't exceed image boundaries
    if(max_x >= img.width()) {
        int overflow = max_x - (img.width() - 1);
        max_x = img.width() - 1;
        min_x = std::max(0, min_x - overflow);
    }

    if(max_y >= img.height()) {
        int overflow = max_y - (img.height() - 1);
        max_y = img.height() - 1;
        min_y = std::max(0, min_y - overflow);
    }
}


    // Get all labels in the expanded bounding box
    for(int y = min_y; y <= max_y; ++y) {
        for(int x = min_x; x <= max_x; ++x) {
            float val = img.at(x, y);
            if(val > 0.0f) {
                expanded_labels.insert(static_cast<int>(val));
            }
        }
    }
    expanded_labels.erase(1);

    // 4. Find complete components within this bounding box
    std::set<int> complete_labels;

    // For each label in the expanded region, check if it's complete
    for(int label : expanded_labels) {
        bool is_complete = true;
        bool found = false;

        // Check the borders of the bounding box
        // Top and bottom borders
        for(int x = min_x; x <= max_x && is_complete; ++x) {
            if(x < 0 || x >= img.width()) continue;
            if(min_y > 0 && static_cast<int>(img.at(x, min_y - 1)) == label) {
                is_complete = false;
            }
            if(max_y + 1 < img.height() && static_cast<int>(img.at(x, max_y + 1)) == label) {
                is_complete = false;
            }
        }

        // Left and right borders
        for(int y = min_y; y <= max_y && is_complete; ++y) {
            if(y < 0 || y >= img.height()) continue;
            if(min_x > 0 && static_cast<int>(img.at(min_x - 1, y)) == label) {
                is_complete = false;
            }
            if(max_x + 1 < img.width() && static_cast<int>(img.at(max_x + 1, y)) == label) {
                is_complete = false;
            }
            for(int x = min_x; x <= max_x; ++x) {
                if(static_cast<int>(img.at(x, y)) == label) {
                    found = true;
                }
            }
        }

        if(is_complete && found) {
            complete_labels.insert(label);
        }
    }

    // 5. Create new image with only complete components
    int width = max_x - min_x + 1;
    int height = max_y - min_y + 1;
    auto result = img.subregion(min_x, min_y, width, height)->copy();
    result.for_each_pixel([&](auto& p, auto x, auto y) {
        int label = static_cast<int>(p);
        if(label == 0 || !complete_labels.count(label)) {
            p = 0.0f;
        } else {
            p = 1.0f;
        }
    });

    auto src_window = src.subregion(min_x, min_y, width, height)->copy();
    //invert_image(src_window);
    ////apply_fade_extension(result);

    //result.for_each_pixel([&](auto& p, auto x, auto y) {
    //    p = src_window.at(x, y);

    //    p = p * p;
    //    p = p * p;
    //    p = p * p;
    //    p = 1.0 / (1.0 + std::exp(-0.5 * (p * 12 - 6)));
    //});

    return result;
}



enum direction {
    end,
    north,
    east,
    south,
    west,
    undefined
};


image<std::pair<direction, float>> find_paths(const image<float>& src) {
    auto map = src.copy();
    image<std::pair<direction, float>> path_map(map.width(), map.height());

    path_map.for_each_pixel([](auto& m) {
        m = {direction::undefined, std::numeric_limits<float>::infinity()};
    });

    std::uniform_int_distribution<int> dis_noice(0, 10);
    std::random_device rd;
    std::mt19937 gen(rd());

    map.for_each_pixel([&](float& p) {
        if (p < 0.01) {
            p = 0;
        }
    });

    map.for_each_pixel([&](float& p) {
        if (p > 0) {
            p = p + (dis_noice(gen) * 0.09) * 1;
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

std::vector<std::pair<int, int>> find_single_path(const image<float>& src, const int start_x, const int start_y, const int rnd) {
    auto map = src.copy();
    image<std::pair<direction, float>> path_map(map.width(), map.height());

    path_map.for_each_pixel([](auto& m) {
        m = {direction::undefined, std::numeric_limits<float>::infinity()};
    });

    std::uniform_int_distribution<int> dis_noice(0, 10);
    std::random_device rd;
    std::mt19937 gen(rd());

    map.for_each_pixel([&](float& p) {
        if (p > 0) {
            p = p + (dis_noice(gen) * 0.09) * 10 * rnd;
        }
    });

    //export_ppm("check.ppm", map);
    //map.for_each_pixel([&](float& p) {
    //    for (int i = 0; i < rnd; i++) {
    //        p = 1.0f - p;
    //        p = p * p;
    //        p = 1.0f - p;
    //    }
    //});
    //export_ppm("check_a.ppm", map);


    std::vector<std::vector<std::pair<int, int>>> updated_positions(2);

    path_map.at(start_x, start_y) = {direction::end, 0};
    updated_positions[0].push_back({start_x, start_y});

    int current_mode = 0;
    int next_mode = 1;

    bool found = false;
    int dest_x;
    int dest_y;
    float dest_score = std::numeric_limits<float>::infinity();

    auto check_neighbor = [&](const auto current_height, const auto current_score, const auto x, const auto y, const direction set_direction, const direction current_direction) {
        if (auto neighbor_height = map.get(x, y)) {
            auto& [neighbor_direction, neighbor_score] = path_map.at(x, y);
            auto delta = std::max(0.0f, **neighbor_height - current_height);

            if (set_direction != current_direction) {
                //delta += 0.1;
            }

            if (current_score + delta < neighbor_score) {
                neighbor_direction = set_direction;
                neighbor_score = current_score + delta;
                if (neighbor_score < dest_score && **neighbor_height > 0) {
                    updated_positions[next_mode].push_back({x, y});
                }

                if (**neighbor_height == 0 && neighbor_score < dest_score) {
                    found = true;
                    dest_x = x;
                    dest_y = y;
                    dest_score = neighbor_score;

                    std::cout << neighbor_score << " < " << dest_score << std::endl;
                }
            }
        }
    };

    while (!updated_positions[current_mode].empty()) {
        //std::cout << "found " << updated_positions[current_mode].size() << " updates" << std::endl;
        for (auto [x, y] : updated_positions[current_mode]) {
            auto& [d, current_score] = path_map.at(x, y);
            auto& current_height = map.at(x, y);
            check_neighbor(current_height, current_score, x - 1, y, direction::east, d);
            check_neighbor(current_height, current_score, x + 1, y, direction::west, d);
            check_neighbor(current_height, current_score, x, y - 1, direction::south, d);
            check_neighbor(current_height, current_score, x, y + 1, direction::north, d);

            //if (found) {
            //    break;
            //}
        }
        //if (found) {
        //    break;
        //}
        
        updated_positions[current_mode].clear();
        current_mode = (current_mode + 1) % 2;
        next_mode = (next_mode + 1) % 2;
    }
    if (!found) {
        return {};
    }

    std::cout << dest_x << std::endl;
    std::cout << dest_y << std::endl;

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

void apply_circular_add(image<float>& target, const float color, const image<float>& circle, const int x, const int y) {
    auto dx = circle.width() / 2;
    auto dy = circle.height() / 2;

    for (auto yi = 0; yi < circle.height(); yi++) {
        for (auto xi = 0; xi < circle.width(); xi++) {
            if (auto t = target.get(x - dx + xi, y - dy + yi)) {
                **t = std::max(**t, color * circle.at(xi, yi));
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


void traverse_paths(image<float>& map, const image<std::pair<direction, float>>& path_map) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis_width(0, map.width() - 1);
    std::uniform_int_distribution<int> dis_height(0, map.height() - 1);

    scale_range(map);

    auto copy = map.copy();
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

        float height = map.at(x, y);
        while (path_map.at(x, y).first != direction::end) {
            height = std::min(map.at(x, y), height);
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

    circle_stack circles;

    water_level.for_each_pixel([&](auto& p, int x, int y) {
        if (p > 0) {
            apply_circular_shift(copy, map, circles.at(4 * p), x, y);
        }
    });

    //scale_range(water_level);

    export_ppm("water.ppm", water_level);
    export_ppm("water_src.ppm", map);

    map = copy;


}

void draw_curly_split_line(image<float>& image, const float p, const int y, const int margin) {
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<float> dis(0, 1.0);

    auto current_y = y - margin + dis(gen) * 2 *  margin;
        if (current_y < 0) {
            current_y = 0;
        }
        if (current_y > image.height() - 1) {
            current_y = image.height() - 1;
        }


    for (int x = 0; x < image.width(); x++) {
        image.at(x, current_y) = p;

        auto delta = y - current_y;
        auto relative_delta = static_cast<float>(std::abs(delta)) / margin;
        relative_delta = 0.5;
        auto r = dis(gen);

        if (r < relative_delta) {
            // move to base line
            if (y < current_y) {
                current_y--;
            } else {
                current_y++;
            }
        } else {
            // move from base line
            if (y < current_y) {
                current_y++;
            } else {
                current_y--;
            }
        }
        if (current_y < y - margin) {
            current_y = y - margin;
        }
        if (current_y > y + margin) {
            current_y = y + margin;
        }

        if (current_y < 0) {
            current_y = 0;
        }
        if (current_y > image.height() - 1) {
            current_y = image.height() - 1;
        }
    }
}

void fill_split_lines(image<float>& image, const float d) {
    //image.for_each_pixel([&](auto& p, auto x, auto y) {
    //    if (p == 0) {
    //        if (y == 0) {
    //            p = d;
    //        } else {
    //            p = image.at(x, y - 1);
    //        }
    //    }
    //});
    for (int x = 0; x < image.width(); x++) {
        int start = 0;
        for (int y = 0; y < image.height(); y++) {
            if (image.at(x, y) != 0 || y == image.height() - 1) {
                auto delta = image.at(x, y) - image.at(x, start);
                if (y == image.height() - 1) {
                    delta = 1.0f - image.at(x, start);
                }
                auto dy = delta / (y - start + 1);
                for (int i = 0; i < y - start; i++) {
                    image.at(x, start + i) = image.at(x, start) + i * dy; 
                }
                start = y;
            }
        }
    }
}

#include <cmath>

double map_value(double value) {
    return value * value;
    if (value <= 0.8) {
        // Logarithmische Funktion für den Bereich [0, 0.8]
        return std::log(value + 1) / std::log(1.8);
    } else {
        // Exponentielle Funktion für den Bereich (0.8, 1]
        return std::pow(1.8, value - 0.8);
    }
}


#include "voronoi.h"

#include <random>
#include <vector>
#include <cmath>

class TerrainGenerator {
private:
    std::mt19937 rng;

    // Noise function for more natural-looking terrain
    float perlin(float x, float y, int seed) {
        // Simplified perlin-like noise for demonstration
        float val = sin(x * 0.1f + seed) * cos(y * 0.1f + seed);
        val += 0.5f * sin(x * 0.2f + seed + 0.5f) * cos(y * 0.2f + seed + 0.5f);
        return val * 0.5f + 0.5f;
    }

public:
    TerrainGenerator(int seed) : rng(seed) {}
    voronoi_generator v;
    auto generate(int width, int height) {
        // Generate initial points for continents
        int num_continents = 5;
        std::uniform_real_distribution<float> dist_x(width * 0.2f, width * 0.8f);
        std::uniform_real_distribution<float> dist_y(height * 0.2f, height * 0.8f);

        // Place continent centers
        std::vector<std::pair<float, float>> continent_centers;
        for (int i = 0; i < num_continents; i++) {
            float x = dist_x(rng);
            float y = dist_y(rng);
            continent_centers.push_back({x, y});

            // Add the main continent point
            v.add(x, y);

            // Add surrounding points for the continent
            int points_per_continent = 20;
            std::normal_distribution<float> local_dist(0, width * 0.1f);

            for (int j = 0; j < points_per_continent; j++) {
                float local_x = x + local_dist(rng);
                float local_y = y + local_dist(rng);
                v.add(local_x, local_y);
            }
        }

        // Add some random islands
        int num_islands = 15;
        for (int i = 0; i < num_islands; i++) {
            v.add(dist_x(rng), dist_y(rng));
        }

        // Generate the Voronoi diagram
        auto voronoi = v.generate(width, height);


        return voronoi;
    }
};

image<float> create_mountain_feature(const image<float>& weights, const image<float>& noise) {
    auto map = weights.copy();
    auto result = noise.copy();

    map.for_each_pixel([&](auto& p, auto x, auto y) {
        p = p * p;
        p = p * p;
        //p = p * p;
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

image<float> mask_to_border_regions(const image<float>& image, const int regions) {
    voronoi_generator v;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis_x(0, image.width());
    std::uniform_int_distribution<int> dis_y(0, image.height());

    for (int i = 0; i < regions; i++) {
        bool retry = true;
        int n = 0;

        while (retry && n < 1000000) {
            auto x = dis_x(gen);
            auto y = dis_y(gen);

            if (!v.contains(x, y) && image.contains(x, y) && image.at(x, y) == 0) {
                retry = false;
                v.add(x, y);                
            }
            n++;
        }
    }
    auto result = v.generate(image.width(), image.height());

    result.for_each_pixel([&](auto& p, auto x, auto y) {
        if (image.at(x, y) == 0) {
            p = 0;
        }
    });

    return result;
}

void add_river_to_region(const image<float>& region_map, const float region, const int rivers, const image<float>& weights_map, image<float>& river_map, const int rnd, const image<float>& wet_map, const std::unordered_map<float, std::tuple<float, int, int>>& water_zone_tiles) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis_x(0, region_map.width());
    std::uniform_int_distribution<int> dis_y(0, region_map.height());
    std::uniform_real_distribution<float> dis_dice(0, 1.0f);

    auto closed_region = weights_map.copy();
    region_map.for_each_pixel([&](auto& p, auto x, auto y) {
        if (p > 0 && p != region) {
            closed_region.at(x, y) = std::numeric_limits<float>::infinity();
        }
    });

    size_t region_size = 0;
    region_map.for_each_pixel([&](auto& p, auto x, auto y) {
        if (p > 0 && p == region) {
            region_size++;
        }
    });
    float relative_region_size = static_cast<float>(region_size) / region_map.size();
    int rivers_in_region = rivers * relative_region_size;
    std::cout << rivers_in_region << std::endl;

    for (int i = 0; i < rivers_in_region; i++) {
        bool retry = true;
        int n = 0;
        int x;
        int y;
        while (retry && n < 100000) {
            x = dis_x(gen);
            y = dis_y(gen);
            auto dice = dis_dice(gen);

            auto p = region_map.get(x, y);
            auto r = river_map.get(x, y);
            auto w = wet_map.get(x, y);
            if (p && r && w && **p == region && **r == 0 && **w >= dice) {
                retry = false;
            }

            n++;
        }
        if (n == 100000) {
            return;
        }

        auto track = find_single_path(closed_region, x, y, rnd);
        if (i == 0) {
            closed_region.for_each_pixel([&](auto& p, auto x, auto y) {
                if (p < 0.1) {
                    p = 100;//
                }
            });
        }

        for (const auto& [pos_x, pos_y] : track) {
            river_map.at(pos_x, pos_y) = region;
            closed_region.at(pos_x, pos_y) = 0;
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

    std::cout << " start scaping" << std::endl;
    
    levels.for_each_pixel([&](auto p, int x, int y) {
        if (p > 0) {
            copy2.at(x, y) = heights.at(x, y);
            if (copy2.at(x, y) > 0.1) {
                apply_circular_shift(copy, copy2, circles.at(p), x, y);
            }
        }
    });

    std::cout << " stop scaping" << std::endl;

    levels.for_each_pixel([&](auto& p, int x, int y) {
        if (p > 0) {
            p = 1.0f;
        } else {
            p = 0.0f;
        }
    });

    export_ppm("heights.ppm", heights);

    return {copy, levels};
}


int main() {
    int width = 512;
    int height = 512;
    int scale = 20;

    auto result = *import_ppm<float>("region.ppm");
    //result.for_each_pixel([&](auto& p, auto x, auto y) {
    //    if (x >= result.width() / 2 + 2000) {
    //        p = 0.0;
    //    } else if (x >= result.width() / 2 + 1000) {
    //        auto dx = x - (result.width() / 2 + 1000);

    //        if (p < 0.001f * dx) {
    //            p = 0.0f;
    //        }
    //    } else if (x >= result.width() / 2 - 1000 + 1000) {
    //        auto dx = x - (result.width() / 2 - 1000 + 1000);

    //        if (p < 1.0 - 0.001f * dx) {
    //            p = 0.0f;
    //        }
    //    } else {
    //        p = 0;
    //    }
    //    //if (y >= result.width() / 2 + 1000) {
    //    //    p = 0.0;
    //    //} else if (y >= result.width() / 2) {
    //    //    auto dx = y - (result.width() / 2);

    //    //    if (p < 0.001f * dx) {
    //    //        p = 0.0f;
    //    //    }
    //    //} else if (y >= result.width() / 2 - 1000) {
    //    //    auto dx = y - (result.width() / 2 - 1000);

    //    //    if (p < 1.0 - 0.001f * dx) {
    //    //        p = 0.0f;
    //    //    }
    //    //} else {
    //    //    p = 0;
    //    //}
    //});
    //export_ppm("asd.ppm", result);
    //return 0;


    auto segment = result.copy();
    //apply_color_segmentation_at(segment, 0.5);
    segment.for_each_pixel([](auto& p) {
         if (p > 0.5) {
             p = 1;
         } else {
             p = 0;
         }
    });
    //invert_image(segment);
    label_connected_components(segment);
    auto tiles = extract_components(segment, result);

    //shuffle_shape_library(tiles);
    //auto csl = combine_until(tiles, 1024, 1024);
    //export_ppm("csl.ppm", csl);

    auto [mask, weights] = *(tiles.rbegin() + 0);

    mask = center_reformat_image(mask, width, height);
    weights = center_reformat_image(weights, width, height);
    
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

    auto blub = create_mountain_feature(weights, result);
    blub.for_each_pixel([&](auto& p) {
        if (p < 0.000001f) {
            p = 0.0f;
        }
    });


    export_ppm("blub.ppm", blub);
    auto idk = mask_to_regions(mask, 20);
    scale_range(idk);
    export_ppm("idk.ppm", idk);

    auto blub_r = mask_to_border_regions(mask, 8);

    image<float> water(blub_r.width(), blub_r.height());

    auto blub_weights = blub.rescale(mask.width(), mask.height());


    auto blub_region_blab = blub_weights.copy();
    quantify_image_pre_zero(blub_region_blab, 5);
    export_ppm("blab.ppm", blub_region_blab);


    /////////////////////////
    auto water_zones = mask.copy();
    invert_image(water_zones);
    label_connected_components(water_zones);
    std::unordered_map<float, float> conversion;
    water_zones.for_each_pixel([&](auto& p, auto x, auto y) {
        conversion[p] = p;
    });
    water_zones.for_each_pixel([&](auto& p, auto x, auto y) {
        if (x == 0 || y == 0 || x == water_zones.width() - 1 || y == water_zones.height() - 1) {
            conversion[p] = 0.0f;
        }
    });
    water_zones.for_each_pixel([&](auto& p, auto x, auto y) {
        p = conversion[p];
    });
    std::unordered_map<float, std::tuple<float, int, int>> water_zone_tiles;
    water_zones.for_each_pixel([&](auto& p, auto x, auto y) {
        if (p > 0.0) {
            auto& [tile, set_x, set_y] = water_zone_tiles[p];

            if (blub_weights.at(x, y) < blub_weights.at(set_x, set_y) || water_zones.at(set_x, set_y) != p) {
                set_x = x;
                set_y = y;
                tile = blub_r.at(x, y);
            }
        }
    });

    scale_range(water_zones);
    //auto tiles = extract_components(segment, result);
    export_ppm("water_zones.ppm", water_zones);
    /////////////////////////



    //auto blub_weights = weights.copy();
    //scale_range(blub_weights);
    //auto blub_scale = *result.subregion(result.width() / 2 - blub_weights.width() / 2, result.height() / 2 - blub_weights.height() / 2, blub_weights.width(), blub_weights.height());

    blub_weights.for_each_pixel([&](auto& p, auto x, auto y) {
        //if (p < 0.01) {
        //    p = 0.0;
        //}
        //if (mask.at(x, y) == 0) {
        //    p = 0.0f;
        //}
    //    if (p > 0) {
    //        p = p * p;
    //        p = p * p;
    //        p = 1.0 / (1.0 + std::exp(-0.5 * (p * 12 - 6)));

    //        auto s = blub_scale.at(x, y);
    //        p = (p + s * p) / 2;
    //    }
    });
    //scale_range(blub_weights);

    auto wet_map = generate_wetness(mask);
    export_ppm("wet_map.ppm", wet_map);

    add_river_to_region(blub_r, 1.0, 1000, blub_weights, water, 3, wet_map, water_zone_tiles);
    add_river_to_region(blub_r, 2.0, 1000, blub_weights, water, 3, wet_map, water_zone_tiles);
    add_river_to_region(blub_r, 3.0, 1000, blub_weights, water, 3, wet_map, water_zone_tiles);
    add_river_to_region(blub_r, 4.0, 1000, blub_weights, water, 3, wet_map, water_zone_tiles);
    add_river_to_region(blub_r, 5.0, 1000, blub_weights, water, 3, wet_map, water_zone_tiles);
    add_river_to_region(blub_r, 6.0, 1000, blub_weights, water, 3, wet_map, water_zone_tiles);
    add_river_to_region(blub_r, 7.0, 1000, blub_weights, water, 3, wet_map, water_zone_tiles);
    add_river_to_region(blub_r, 8.0, 1000, blub_weights, water, 3, wet_map, water_zone_tiles);

    water.for_each_pixel([&](auto& p, auto x, auto y) {
        auto top = blub_weights.get(x, y - 1);
        auto bot = blub_weights.get(x, y + 1);
        auto left = blub_weights.get(x - 1, y);
        auto right = blub_weights.get(x + 1, y);

        bool has_non_zero_neighbor = (top && **top > 0) || (bot && **bot > 0) || (left && **left > 0) || (right && **right > 0);

        if (p > 0 && blub_weights.at(x, y) == 0 && !has_non_zero_neighbor) {
            p = 0.0f;
        }
    });

    scale_range(blub_r);
    scale_range(water);

    auto moisture_map = generate_moisture(water, blub_weights);
    scale_range(moisture_map);
    export_ppm("moisture_map.ppm", moisture_map);



    auto scaled_water = blub.copy_shape();
    image<std::pair<float, float>> scaled_water_pos(water.width(), water.height());

    water.for_each_pixel([&](auto& p, auto x, auto y) {
        auto scale_x = static_cast<float>(scaled_water.width()) / water.width();
        auto scale_y = static_cast<float>(scaled_water.height()) / water.height();

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
            //scale_x * (static_cast<float>(x) + 0.50f),// + random_float(-0.4, 0.4)),
            //scale_y * (static_cast<float>(y) + 0.50f)// + random_float(-0.4, 0.4))
        };

        //scaled_water.at(scaled_water_pos.at(x, y).first, scaled_water_pos.at(x, y).second) = p;
    });
    auto check_water_manhatten = [&](auto x, auto y, auto p) {
        if (auto neighbor = water.get(x, y)) {
            return (**neighbor == p);
        }

        return false;
    };
    export_ppm("pre_water.ppm", water);
    //remove_redundant_line(water);
    std::vector<std::pair<size_t, size_t>> end_points;
    water.for_each_pixel([&](auto& p, auto x, auto y) {
        if (p == 0) {
            return;
        }
        auto scale_x = static_cast<float>(scaled_water.width()) / water.width();
        auto scale_y = static_cast<float>(scaled_water.height()) / water.height();

        float top = -1;
        float bot = -1;
        float left = -1;
        float right = -1;

        if (check_water_manhatten(x + 1, y, p)) {
            auto [local_right, local_bot] = scaled_water_pos.at(x, y);
            right = local_right;            
        }
        if (check_water_manhatten(x - 1, y, p)) {
            auto [local_right, local_bot] = scaled_water_pos.at(x - 1, y);
            left = local_right;            
        }
        if (check_water_manhatten(x, y + 1, p)) {
            auto [local_right, local_bot] = scaled_water_pos.at(x, y);
            bot = local_bot;            
        }
        if (check_water_manhatten(x, y - 1, p)) {
            auto [local_right, local_bot] = scaled_water_pos.at(x, y - 1);
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
            draw_manhattan_line(scaled_water, top_x, top_y, bot_x, bot_y, p);
            cross++;
        }
        if (left >= 0 && right >= 0) {
            draw_manhattan_line(scaled_water, left_x, left_y, right_x, right_y, p);
            cross++;
        }
        if (top >= 0 && left >= 0 && cross < 2) {
            draw_manhattan_line(scaled_water, top_x, top_y, left_x, left_y, p);
        }
        if (top >= 0 && right >= 0 && cross < 2 && left < 0) {
            draw_manhattan_line(scaled_water, top_x, top_y, right_x, right_y, p);
        }
        if (bot >= 0 && left >= 0 && cross < 2 && top < 0) {
            draw_manhattan_line(scaled_water, bot_x, bot_y, left_x, left_y, p);
        }
        if (bot >= 0 && right >= 0 && cross < 2 && top < 0 && left < 0) {
            draw_manhattan_line(scaled_water, bot_x, bot_y, right_x, right_y, p);
        }
        if (top >= 0 && bot < 0 && right < 0 && left < 0) {
            draw_manhattan_line(scaled_water, top_x, top_y, center_x, center_y, p);
            if (blub_weights.at(x, y) == 0) {
                end_points.push_back({center_x, center_y});
            }
        }
        if (top < 0 && bot >= 0 && right < 0 && left < 0) {
            draw_manhattan_line(scaled_water, bot_x, bot_y, center_x, center_y, p);
            if (blub_weights.at(x, y) == 0) {
                end_points.push_back({center_x, center_y});
            }
        }
        if (top < 0 && bot < 0 && right >= 0 && left < 0) {
            draw_manhattan_line(scaled_water, right_x, right_y, center_x, center_y, p);
            if (blub_weights.at(x, y) == 0) {
                end_points.push_back({center_x, center_y});
            }
        }
        if (top < 0 && bot < 0 && right < 0 && left >= 0) {
            draw_manhattan_line(scaled_water, left_x, left_y, center_x, center_y, p);
            if (blub_weights.at(x, y) == 0) {
                end_points.push_back({center_x, center_y});
            }
        }
    });

    std::cout << "finished manhatten stuff" << std::endl;
    for (auto [x, y] : end_points) {
        std::cout << x << ", " << y << std::endl;
        assert(scaled_water.at(x, y) > 0);
    }

    auto [river_with_depth, river_scaled_map] = generate_river_depth(scaled_water, blub, end_points);
    blub = river_with_depth;

    std::vector<std::pair<int, int>> lake_position;
    std::random_device lake_rd;
    std::mt19937 lake_gen(lake_rd());
    std::uniform_real_distribution<float> lake_dis(0, 10000.0);
    scaled_water.for_each_pixel([&](auto p, int x, int y) {
        if (p > 0) {
            auto dice = lake_dis(lake_gen);        
            if (dice < 1) {
                lake_position.push_back({x, y});
            }
        }
    });

    std::cout << lake_position.size() << std::endl;

    std::uniform_int_distribution<int> lake_shape_dis(0, tiles.size() - 1);
    std::uniform_int_distribution<int> lake_size_dis(2, 256);
    for (auto [lake_x, lake_y] : lake_position) {
        auto index = lake_shape_dis(lake_gen);
        auto [mask, weights] = tiles[index];
        auto size = std::min(lake_size_dis(lake_gen), static_cast<int>(mask.width()));
        auto lake_shape = mask.rescale(std::max(2, size), std::max(2.0f, (static_cast<float>(mask.height()) / mask.width()) * size));
        lake_shape = center_reformat_image(lake_shape, lake_shape.width() + 128, lake_shape.height() + 128);

        std::uniform_int_distribution<int> lake_shape_connector_x(0, lake_shape.width() - 1);
        std::uniform_int_distribution<int> lake_shape_connector_y(0, lake_shape.height() - 1);
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


        std::cout << "place lake at " << lake_x << ", " << lake_y << " with size " << size << std::endl;

        int n = 0;
        do {
            connector_x = lake_shape_connector_x(lake_gen);
            connector_y = lake_shape_connector_y(lake_gen);
            n++;
        } while (lake_shape.at(connector_x, connector_y) == 0 && n < 10000);

        auto top_x = lake_x - connector_x;
        auto top_y = lake_y - connector_y;

        if (auto window = river_scaled_map.subregion(top_x, top_y, lake_shape.width(), lake_shape.height())) {
            window->for_each_pixel([&](auto& p, auto x, auto y) {
                if (lake_shape.at(x, y) > 0) {
                    p = 1.0f;
                }
            });
        }
        if (auto window = blub.subregion(top_x, top_y, lake_shape.width(), lake_shape.height())) {
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

    export_ppm("rwd.ppm", blub);


    export_ppm("blub_r.ppm", blub_r);
    export_ppm("water.ppm", water);
    export_ppm("scaled_water.ppm", scaled_water);
    export_ppm("river_scaled_map.ppm", river_scaled_map);
    export_ppm("weights.ppm", weights);
    export_ppm("blub_weights.ppm", blub_weights);



    return 0;

/*
    altitude.for_each_pixel([&](auto& p, auto x, auto y) {
        p = p * p;
        //p = p * p;
        ////p = p * p;
        p = 1.0 / (1.0 + std::exp(-0.5 * (p * 12 - 6)));
    });

    scale_range(altitude);
    for (int i = 0; i < 25; i++) {
        //add_gaussian_blur(altitude);
    }
    scale_range(altitude);

    auto path_map = find_paths(altitude);
    traverse_paths(altitude, path_map);
    scale_range(altitude);

    altitude = altitude.rescale(result.width(), result.height());

    result.for_each_pixel([&](auto& p, auto x, auto y) {
        auto s = altitude.at(x, y);

        if (s > 0) {
            p = (s + s * p) / 2;
        } else {
            p = 0;
        }
    });

    scale_range(result);

    export_ppm("what_is_this.ppm", result);



    scale_range(blub_r);
    scale_range(water);


    export_ppm("blub_r.ppm", blub_r);
    export_ppm("water.ppm", water);
    export_ppm("weights.ppm", weights);


    return 0;


    auto map = extract_complete_components(segment, result);
    export_ppm("1.ppm", map);
    return 0;

    scale_range(result);
    scale_range(map);

    for (int i = 0; i < 25; i++) {
        add_gaussian_blur(map);
    }
    //for (int i = 0; i < 4; i++) {
    //    map.for_each_pixel([](auto& p) {
    //        p = p * p;
    //    });
    //    add_gaussian_blur(map);
    //}
    scale_range(map);

{
    auto path_map = find_paths(map);
    traverse_paths(map, path_map);
    scale_range(map);
}

    export_ppm("final_map.ppm", map);
    return 0;

    map = map.rescale(result.width(), result.height());

    result.for_each_pixel([&](auto& p, auto x, auto y) {
        auto s = map.at(x, y);

        if (s > 0) {
            p = (s + s * p) / 2;
        } else {
            p = 0;
        }
    });

    scale_range(result);

    export_ppm("what_is_this.ppm", result);
    export_ppm("what_is_this2.ppm", map);

    return 0;
    */

/*
    image<float> result(width, height);

    draw_curly_split_line(result, 0.1, 64,  32);
    draw_curly_split_line(result, 0.2, 128, 32);
    draw_curly_split_line(result, 0.3, 192, 32);
    draw_curly_split_line(result, 0.5, 256, 32);
    draw_curly_split_line(result, 0.7, 320, 32);
    draw_curly_split_line(result, 0.8, 384, 32);
    draw_curly_split_line(result, 0.9, 448, 32);

    fill_split_lines(result, 0);
    add_gaussian_blur(result);
    add_gaussian_blur(result);
    add_gaussian_blur(result);
    add_gaussian_blur(result);
    add_gaussian_blur(result);
    scale_range(result);

    image<float> terrain = generate_terrain(width, height);

    add_gaussian_blur(terrain);
    add_gaussian_blur(terrain);

    terrain.for_each_pixel([](auto& p) {
        //p = p * p;
        if (p < 0.30) {
            p = 0;
        } else {
            p -= 0.30;
        }
    });

    scale_range(terrain);
    terrain.for_each_pixel([](auto& p) {
        p = p * p;
    });
    int min_x = std::numeric_limits<int>::max();
    int min_y = std::numeric_limits<int>::max();
    int max_x = std::numeric_limits<int>::min();
    int max_y = std::numeric_limits<int>::min();

    terrain.for_each_pixel([&](auto& p, int x, int y) {
        if (p > 0.01) {
            min_x = std::min(min_x, x);
            min_y = std::min(min_y, y);
            max_x = std::max(max_x, x);
            max_y = std::max(max_y, y);
        }
    });
    terrain = *terrain.subregion(min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
    terrain = terrain.rescale(result.width(), result.height());
    result.for_each_pixel([&](auto& p, auto x, auto y) {
        //p = (p + terrain.at(x, y)) / 2.0;
        p *= (1.0f - terrain.at(x, y));
        if (terrain.at(x, y) == 0) {
            p = 0;
        }
    });
    scale_range(result);






    export_ppm("region.ppm", result);

    return 0;
    */


    //auto result2 = generate_base_world(width, height, scale, 0.1, 0.4, 0.1);
    //export_ppm("region.ppm", result2);
    //return 0;

    image<float> terrain = generate_unfaded_terrain(width, height);
    //auto result = *import_ppm<float>("region.ppm");

    //add_gaussian_blur(terrain);
    //add_gaussian_blur(terrain);

    //terrain.for_each_pixel([](auto& p) {
    //    //p = p * p;
    //    if (p < 0.30) {
    //        p = 0;
    //    } else {
    //        p -= 0.30;
    //    }
    //});

    //scale_range(terrain);
    //int min_x = std::numeric_limits<int>::max();
    //int min_y = std::numeric_limits<int>::max();
    //int max_x = std::numeric_limits<int>::min();
    //int max_y = std::numeric_limits<int>::min();

    //terrain.for_each_pixel([&](auto& p, int x, int y) {
    //    if (p > 0) {
    //        min_x = std::min(min_x, x);
    //        min_y = std::min(min_y, y);
    //        max_x = std::max(max_x, x);
    //        max_y = std::max(max_y, y);
    //    }
    //});
    //terrain = *terrain.subregion(min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
    //terrain.for_each_pixel([](auto& p) {
    //    p = p * p;
    //});
    //scale_range(terrain);
    export_ppm("what_is_this.ppm", terrain);
    //terrain = terrain.rescale(result.width(), result.height());

/*
    result.for_each_pixel([&](auto& p, auto x, auto y) {
        if (terrain.contains(x, y)) {
            auto s = terrain.at(x, y);

            if (s > 0) {
                p = (s + s * p) / 2;
            } else {
                p = 0;
            }
        }
    });
    */

    //scale_range(result);
    //export_ppm("what_is_this.ppm", result);

/*
    auto path_map = find_paths(result);
    traverse_paths(result, path_map);
    scale_range(result);

    export_ppm("final_map.ppm", result);
    */

    return 0;
}
