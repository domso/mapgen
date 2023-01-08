#include "generate_tiles.h"
#include <unordered_map>

#include "helper.h"

struct bounding_box {
    int start_x;
    int start_y;
    int end_x;
    int end_y;
};

std::unordered_map<int, bounding_box> find_bounding_boxes(const image<int>& mask) {
    std::unordered_map<int, bounding_box> position;

    for (int y = 0; y < mask.height(); y++) {
        for (int x = 0; x < mask.width(); x++) {
            auto d = mask.at(x, y);
            if (d > 0) {
                auto find = position.find(d);
                if (find == position.end()) {
                    bounding_box box;
                    box.start_x = x;
                    box.start_y = y;
                    box.end_x = x;
                    box.end_y = y;
                    position.insert({d, box});
                } else {
                    auto& [key, box] = *find;

                    box.start_x = std::min(box.start_x, x);
                    box.start_y = std::min(box.start_y, y);
                    box.end_x = std::max(box.end_x, x);
                    box.end_y = std::max(box.end_y, y);
                }
            }
        }
    }

    return position;
}

image<float> extract_tile_by_bounding_box(const image<float>& map, const image<int>& mask, const bounding_box& box, const int key) {
    auto blurs = 4;
    auto w = box.end_x - box.start_x + 1 + 2 * blurs;
    auto h = box.end_y - box.start_y + 1 + 2 * blurs;
    image<float> tile(w, h, 0.0);

    for (int y = box.start_y; y <= box.end_y; y++) {
        for (int x = box.start_x; x <= box.end_x; x++) {
            if (mask.at(x, y) == key) {
                tile.at(blurs + x - box.start_x, blurs + y - box.start_y) = map.at(x, y);
            }
        }
    }
    for (int i = 0; i < blurs; i++) {
        add_gaussian_blur(tile);
    }
    
    return tile;
}

std::vector<image<float>> generate_tiles(const image<float>& map, const image<int>& mask) {
    std::vector<image<float>> result;
    auto bounding_boxes = find_bounding_boxes(mask);

    for (const auto& [key, box] : bounding_boxes) {
        if (box.end_x - box.start_x > 5 && box.end_y - box.start_y > 5) {
            auto tile = extract_tile_by_bounding_box(map, mask, box, key);
            result.push_back(tile);
        }
    }

    return result;
}
