#include "image.h"
#include "../../cmd/loading_bar.h"

image<int> generate_base_mask(const image<float>& terrain) {
    cmd::loading_bar loading_bar;
    loading_bar.init("Generate base mask", terrain.width() * terrain.height());
    image<int> mask(terrain.width(), terrain.height());
    int s = 1;
    int n = 0;
    terrain.for_each_pixel([&](const float& p, const size_t x, const size_t y) {
        if (p == 0) {
            if (n > 0) {
                s++;
            }
            n = 0;
            mask.at(x, y) = 0;
        } else {
            mask.at(x, y) = s;
            n++;
        }
        loading_bar.step();
    });
    loading_bar.finalize();

    return mask;
}

void apply_tile_segmentation(image<int>& mask) {
    cmd::loading_bar loading_bar;
    bool found = true;
    int round_count = 0;
    int min = 0;
    
    auto update_mask_at_position = [&](const size_t x, const size_t y) {
        min = std::min(min, mask.at(x, y));

        if (mask.at(x, y) > 0 && min == 0) {
            min = mask.at(x, y);
        }

        found = found || (mask.at(x, y) != min);

        mask.at(x, y) = min;
    };

    while(found) {
        found = false;

        loading_bar.init("Segmentation round #" + std::to_string(round_count), (mask.width() * (2 * mask.height()) + mask.height() * (2 * mask.width())));
        for (int x = 0; x < mask.width(); x++) {
            min = 0;
            for (int y = 0; y < mask.height(); y++) {
                update_mask_at_position(x, y);
            }
            loading_bar.multi_step(mask.height());
            min = 0;
            for (int y = 0; y < mask.height(); y++) {
                update_mask_at_position(x, mask.height() - 1 - y);
            }
            loading_bar.multi_step(mask.height());
        }
        for (int y = 0; y < mask.height(); y++) {
            min = 0;
            for (int x = 0; x < mask.width(); x++) {
                update_mask_at_position(x, y);
            }
            loading_bar.multi_step(mask.width());
            min = 0;
            for (int x = 0; x < mask.width(); x++) {
                update_mask_at_position(mask.width() - 1 - x, y);
            }
            loading_bar.multi_step(mask.width());
        }
        loading_bar.finalize();
        round_count++;
    }
}

void shrink_tile_ids(image<int>& mask) {
    cmd::loading_bar loading_bar;
    loading_bar.init("Shrink mask", (mask.width() * (2 * mask.height()) + mask.height() * (2 * mask.width())));
    int incr_mask = 0;
    int count = 0;
    int last = 0;

    auto update_mask_at_position = [&](const size_t x, const size_t y) {
        if (mask.at(x, y) > 0) {
            count = incr_mask;
            last = mask.at(x, y);
        } else if (count > 0) {
            mask.at(x, y) = last;
            count--;
        }
    };

    for (int y = 0; y < mask.height(); y++) {
        count = 0;
        last = 0;
        for (int x = 0; x < mask.width(); x++) {
            update_mask_at_position(x, y);
        }
        loading_bar.multi_step(mask.width());
        count = 0;
        for (int x = 0; x < mask.width(); x++) {
            update_mask_at_position(mask.width() - 1 - x, y);
        }
        loading_bar.multi_step(mask.width());
    }
    for (int x = 0; x < mask.width(); x++) {
        int count = 0;
        int last = 0;
        for (int y = 0; y < mask.height(); y++) {
            update_mask_at_position(x, y);
        }
        loading_bar.multi_step(mask.height());
        count = 0;
        for (int y = 0; y < mask.height(); y++) {
            update_mask_at_position(x, mask.height() - 1 - y);
        }
        loading_bar.multi_step(mask.height());
    }
    loading_bar.finalize();
}

image<int> generate_mask(const image<float>& terrain) {
    auto mask = generate_base_mask(terrain);
    apply_tile_segmentation(mask);
    shrink_tile_ids(mask);

    return mask;
}
