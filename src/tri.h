
template<typename Tx, typename Ty>
struct point_2d {
    Tx x;
    Ty y;
};

bool check_line(const point_2d<int, int> point, const point_2d<int, int> line_start, const point_2d<int, int> line_end, const bool inclusive) {
    int delta_x = line_end.x - line_start.x;
    int delta_y = line_end.y - line_start.y;
    float m = static_cast<float>(delta_y) / static_cast<float>(delta_x);

    if (inclusive) {
        return std::abs(line_start.y + (point.x - line_start.x) * m - point.y) <= 1.0;    
    } else {
        return line_start.y + (point.x - line_start.x) * m >= point.y;
    }
}



int map_on_edge(const point_2d<int, int> point, const int border, const point_2d<int, int> start) {
    int delta_y = point.y - start.y;
    int delta_x = point.x - start.x;
    float m = static_cast<float>(delta_y) / static_cast<float>(delta_x);
 
    return point.y + (border - point.x) * m;
}

float compute_edge_color(const int edge, const float top_c, const float center_c, const int top_y, const int center_y) {
    int delta_y = top_y - center_y;
    float delta_c = top_c - center_c;
    float m = delta_c / delta_y;

    return center_c + m * (edge - center_y);
}

float compute_line_color(const point_2d<int, int> point, const point_2d<int, int> start, const point_2d<int, int> end, const float start_c, const float end_c) {
    int delta_x = end.x - start.x;
    int delta_y = end.y - start.y;
    float line_length = std::sqrt(static_cast<float>(delta_x * delta_x + delta_y * delta_y));

    delta_x = point.x - start.x;
    delta_y = point.y - start.y;
    float position_length = std::sqrt(static_cast<float>(delta_x * delta_x + delta_y * delta_y));
    
    float delta_c = end_c - start_c;

    return start_c + delta_c * (position_length / line_length);
}


image<float> compute_right_edge_triangle(const int width, const int height, const point_2d<int, int> left, const point_2d<int, int> right, const point_2d<int, int> top, const float base_color, const float left_color, const float right_color, const float top_color) {
    image<float> gradient_image(width, height);

    gradient_image.for_each_pixel([&](float& p, int x, int y) {
        if (
            (check_line({x, y}, left, right, true) || check_line({x, y}, left, top, true)) || 
            (check_line({x, y}, left, right, false) && !check_line({x, y}, left, top, false))
        ) {
            int edge_y = std::max(std::min(right.y, map_on_edge({x, y}, width - 1, left)), top.y);
            float edge_color = compute_edge_color(edge_y, top_color, right_color, top.y, right.y);
            p = compute_line_color({x, y}, left, {width - 1, edge_y}, left_color, edge_color);
        } else {
            p = 0.0;
        }
    });

    return gradient_image;
}

void add_top_left_triangle(image<float>& img, point_2d<int, int> left, point_2d<int, int> right, point_2d<int, int> top, const float left_color, const float right_color, const float top_color) {
    if (true || right.y > top.y) {
        int right_len = right.y - top.y;
        int left_len = 256 + (right.y - left.y);

        auto rel_left = left;
        auto rel_right = right;
        auto rel_top = top;
        
        int min = std::min(rel_top.y, std::min(rel_right.y, rel_left.y));
        rel_left.y -= min;
        rel_right.y -= min;
        rel_top.y -= min;
        int max = std::max(rel_top.y, std::max(rel_right.y, rel_left.y));
        
        auto triangle = compute_right_edge_triangle(img.width() / 2, max, rel_left, rel_right, rel_top, 0.0, left_color, right_color, top_color);

        if (auto region = img.subregion(left.x, std::min(left.y, top.y), triangle.width(), triangle.height())) {
            (*region).mix_max(triangle);
        }
    }
}

void add_bot_left_triangle(image<float>& img, point_2d<int, int> left, point_2d<int, int> right, point_2d<int, int> top, const float left_color, const float right_color, const float top_color) {
    if (true || right.y > top.y) {
        int right_len = right.y - top.y;
        int left_len = 256 + (right.y - left.y);

        auto rel_left = left;
        auto rel_right = right;
        auto rel_top = top;
        
        int min = std::min(rel_top.y, std::min(rel_right.y, rel_left.y));
        rel_left.y -= min;
        rel_right.y -= min;
        rel_top.y -= min;
        int max = std::max(rel_top.y, std::max(rel_right.y, rel_left.y));

        rel_left.y = max - rel_left.y;
        rel_right.y = max - rel_right.y;
        rel_top.y = max - rel_top.y;
        
        auto triangle = compute_right_edge_triangle(img.width() / 2, max, rel_left, rel_right, rel_top, 0.0, left_color, top_color, right_color);
        triangle.mirror_horizontal();

        if (auto region = img.subregion(left.x, std::min(right.y, left.y), triangle.width(), triangle.height())) {
            (*region).mix_max(triangle);
        }
    }
}

void add_top_right_triangle(image<float>& img, point_2d<int, int> left, point_2d<int, int> right, point_2d<int, int> top, const float left_color, const float right_color, const float top_color) {
    if (right.y > top.y) {
        int right_len = right.y - top.y;
        int left_len = 256 + (right.y - left.y);

        auto rel_left = left;
        auto rel_right = right;
        auto rel_top = top;

        std::swap(rel_left.x, rel_right.x);
        rel_left.x -= img.width() / 2;
        rel_right.x -= img.width() / 2;
        rel_top.x = rel_right.x;
        
        int min = std::min(rel_top.y, std::min(rel_right.y, rel_left.y));
        rel_left.y -= min;
        rel_right.y -= min;
        rel_top.y -= min;
        int max = std::max(rel_top.y, std::max(rel_right.y, rel_left.y));
        
        auto triangle = compute_right_edge_triangle(img.width() / 2, max, rel_left, rel_right, rel_top, 0.0, left_color, right_color, top_color);
        triangle.mirror_vertical();

        if (auto region = img.subregion(top.x, std::min(top.y, left.y), triangle.width(), triangle.height())) {
            (*region).mix_max(triangle);
        }
    }
}
void add_bot_right_triangle(image<float>& img, point_2d<int, int> left, point_2d<int, int> right, point_2d<int, int> top, const float left_color, const float right_color, const float top_color) {
    if (true || right.y > top.y) {
        int right_len = right.y - top.y;
        int left_len = 256 + (right.y - left.y);

        auto rel_left = left;
        auto rel_right = right;
        auto rel_top = top;

        std::swap(rel_left.x, rel_right.x);
        rel_left.x -= img.width() / 2;
        rel_right.x -= img.width() / 2;
        rel_top.x = rel_right.x;
        
        int min = std::min(rel_top.y, std::min(rel_right.y, rel_left.y));
        rel_left.y -= min;
        rel_right.y -= min;
        rel_top.y -= min;
        int max = std::max(rel_top.y, std::max(rel_right.y, rel_left.y));

        rel_left.y = max - rel_left.y;
        rel_right.y = max - rel_right.y;
        rel_top.y = max - rel_top.y;
        
        auto triangle = compute_right_edge_triangle(img.width() / 2, max, rel_left, rel_right, rel_top, 0.0, left_color, top_color, right_color);
        triangle.mirror_vertical();
        triangle.mirror_horizontal();

        if (auto region = img.subregion(top.x, std::min(right.y, left.y), triangle.width(), triangle.height())) {
            (*region).mix_max(triangle);
        }
    }
}

image<float> generate_isometric_tile(const int scale, const int top_height, const int right_height, const int bottom_height, const int left_height) {
    point_2d<int, int> top = {2 * scale, 0};
    point_2d<int, int> bot = {2 * scale, 2 * scale - 1};
    point_2d<int, int> left = {0, scale};
    point_2d<int, int> right = {4 * scale - 1, scale};
    
    top.y -= top_height;
    bot.y -= bottom_height;
    left.y -= left_height;
    right.y -= right_height;

    int min = std::min(top.y, std::min(bot.y, std::min(left.y, right.y)));
    top.y -= min;
    bot.y -= min;
    left.y -= min;
    right.y -= min;
    int max = std::max(top.y, std::max(bot.y, std::max(left.y, right.y)));
    point_2d<int, int> center = {2 * scale, (bot.y + top.y) / 2};


    image<float> result(4 * scale, max);
    auto top_color =  static_cast<float>(top_height) / 1000;
    auto bot_color =  static_cast<float>(bottom_height) / 1000;
    auto left_color = static_cast<float>(left_height) / 1000;
    auto right_color =  static_cast<float>(right_height) / 1000;
    auto center_color = (top_color + bot_color) / 2;

    //top_color = 1.0;
    //center_color = 1.0;
    //left_color = 1.0;
    add_top_left_triangle(result, left, center, top, left_color, center_color, top_color);
    //top_color = 0.75;
    //center_color = 0.75;
    //right_color = 0.75;
    add_top_right_triangle(result, right, center, top, right_color, center_color, top_color);
    //bot_color = 0.5;
    //center_color = 0.5;
    //left_color = 0.5;
    add_bot_left_triangle(result, left, center, bot, left_color, center_color, bot_color);
    //bot_color = 0.25;
    //center_color = 0.25;
    //right_color = 0.25;
    add_bot_right_triangle(result, right, center, bot, right_color, center_color, bot_color);

    return result;
}

template<typename T>
class stripe_image {
public:
    void init(const image<T>& img) {
        m_width = img.width();
        m_height = img.height();
        m_lines.resize(img.height());
        m_offsets.resize(img.height());

        for (size_t y = 0; y < img.height(); y++) {
            init_line(img, y);
        }
    }
    image<T> export_img() const {
        image<T> result(m_width, m_height);

        result.for_each_pixel([&](float& p, const size_t x, const size_t y) {
            if (m_offsets[y] <= x && x < m_offsets[y] + m_lines[y].size()) {
                p = m_lines[y][x - m_offsets[y]];
            } else {
                p = 0.0;
            }
        });

        return result;        
    }
private:
    void init_line(const image<T>& img, const size_t y) {
        size_t min_x = get_min_x(img, y);
        size_t max_x = get_max_x(img, y);

        for (size_t x = min_x; x < max_x; x++) {
            m_lines[y].push_back(img.at(x, y));
        }        
        m_offsets[y] = min_x;
    }
    size_t get_min_x(const image<T>& img, const size_t y) {
        for (size_t x = 0; x < img.width(); x++) {
            if (img.at(x, y) > 0) {
                return x;
            }
        }

        return img.width();
    }
    size_t get_max_x(const image<T>& img, const size_t y) {
        for (size_t x = 0; x < img.width(); x++) {
            if (img.at(img.width() - 1 - x, y) > 0) {
                return img.width() - 1 - x + 1;
            }
        }

        return 0;
    }

    size_t m_width;
    size_t m_height;
    std::vector<size_t> m_offsets;
    std::vector<std::vector<T>> m_lines;
};

int main() {
    auto terrain = generate_terrain(512, 512);
    int scale = 5;


    terrain.for_each_pixel([](float& p) {
        //p = 0.0;
    });

    std::vector<std::pair<point_2d<int, int>, stripe_image<float>>> stripes;

    int global_min_x = std::numeric_limits<int>::max();
    int global_min_y = std::numeric_limits<int>::max();
    int global_max_x = 0;
    int global_max_y = 0;
    int height_scale = 1000;

    for (int y = 0; y < terrain.height() - 2; y++) {
        std::vector<std::pair<point_2d<int, int>, image<float>>> tiles;
        int min_x = std::numeric_limits<int>::max();
        int min_y = std::numeric_limits<int>::max();
        int max_x = 0;
        int max_y = 0;


        for (int x = 0; x < terrain.width() - 2; x++) {
            image<float> tile = generate_isometric_tile(scale, height_scale * terrain.at(x, y), height_scale * terrain.at(x + 1, y), height_scale * terrain.at(x + 1, y + 1), height_scale * terrain.at(x, y + 1));

            auto x_position = 2 * scale * x - 2 * scale * y - x + y;
            auto y_position = scale * x + scale * y - x - y;

            auto top_y = y_position - 2 * scale -  terrain.at(x, y) * height_scale;
            auto right_y = y_position - scale - terrain.at(x + 1, y) * height_scale;
            auto bot_y = y_position - terrain.at(x + 1, y + 1) * height_scale;
            auto left_y = y_position - scale - terrain.at(x, y + 1) * height_scale;

            int max = std::max(top_y, std::max(bot_y, std::max(left_y, right_y)));

            min_x = std::min(min_x, x_position);
            min_y = std::min(min_y, max - static_cast<int>(tile.height()));
            max_x = std::max(max_x, x_position);
            max_y = std::max(max_y, max - static_cast<int>(tile.height()));

            global_min_x = std::min(global_min_x, x_position);
            global_min_y = std::min(global_min_y, max - static_cast<int>(tile.height()));
            global_max_x = std::max(global_max_x, x_position);
            global_max_y = std::max(global_max_y, max - static_cast<int>(tile.height()));

            tiles.push_back({{x_position, static_cast<int>(max - tile.height())}, std::move(tile)});
        }
       
        image<float> map(max_x - min_x + 1, max_y - min_y + 1);

        for (int x = 0; x < terrain.width() - 2; x++) {
            auto& [pos, tile] = tiles[x];
            if (auto region = map.subregion(pos.x - min_x, pos.y - min_y, tile.width(), tile.height())) {
                tile.for_each_pixel([&](float& p, const int x, const int y) {
                    if (p > 0) {
                        (*region).at(x, y) = p;
                    }
                });
            }
        }

        stripe_image<float> stripe;
        stripe.init(map);

        stripes.push_back({{min_x, min_y}, std::move(stripe)});
    }
    image<float> map(global_max_x - global_min_x + 1, global_max_y - global_min_y + 1);
    for (int y = 0; y < terrain.height() - 2; y++) {
        auto& [pos, stripe] = stripes[y];
        auto img = stripe.export_img();
        if (auto region = map.subregion(pos.x - global_min_x, pos.y - global_min_y, img.width(), img.height())) {
            img.for_each_pixel([&](float& p, const int x, const int y) {
                if (p > 0) {
                    (*region).at(x, y) = p;
                }
            });
        }
    }


    export_ppm("tri.ppm", map);
}
