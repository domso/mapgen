#pragma once

namespace config {
    struct terrain {
        constexpr static const int min_base_noise = 20;
        constexpr static const int max_base_noise = 25;
        constexpr static const int num_hills = 1000;
        constexpr static const int min_hill_size = 4;
        constexpr static const int max_hill_size = 200;
        constexpr static const int min_hill_height = 2;
        constexpr static const int max_hill_height = 3;
        constexpr static const int num_pre_blur = 100;
        constexpr static const int num_post_blur = 20;

        constexpr static const int num_erosion = 2;
        constexpr static const float erosion_factor = 0.9;
    };
    
    struct water {
        constexpr static const int num_simulation = 100;
        constexpr static const int filterbox_size = 20;
        constexpr static const int line_scale = 6;
        constexpr static const float height = 0.000;
        constexpr static const int num_blur = 100;
        constexpr static const float base_level = 0.25;
    };
}
