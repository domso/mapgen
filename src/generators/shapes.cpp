#include "shapes.h"

#include <queue>
#include <utility>
#include <algorithm>

namespace mapgen::generators::shapes {

std::vector<std::pair<image<float>, image<float>>> generate(const image<float>& base) {
    auto copy = base;
    copy.for_each_pixel([](auto& p) {
         if (p > 0.5) {
             p = 1;
         } else {
             p = 0;
         }
    });
    internal::label(copy);

    return internal::extract(copy, base);
}


namespace internal {

void label(auto& img) {
    std::vector<std::vector<bool>> visited(img.height(), std::vector<bool>(img.width(), false));
    float current_color = 1.0f; // Start with a gray color, increment for each region

    img.for_each_pixel([&](auto p, auto x, auto y) {
        if (!visited[y][x] && p == 1.0f) {  // If pixel is white and not visited
            flood_fill(img, x, y, current_color, visited);
            current_color += 1.0f;  // Increment color for next region
        }
    });
}

std::vector<std::pair<image<float>, image<float>>> extract(const auto& img, const auto& src) {
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

void flood_fill(auto& img, const int start_x, const int start_y, const float new_color, std::vector<std::vector<bool>>& visited) {
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

}
}
