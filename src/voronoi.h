#pragma once

#include <iostream>
#include <cassert>
#include <set>
#include <algorithm>

#include "image.h"

class voronoi_generator {
public:
    bool contains(const float x, const float y) {
        return m_sample_points.contains({x, y});
    }
    void add(const float x, const float y) {
        if (m_sample_points.contains({x, y})) {
            return;
        }
        m_sample_points.insert({x, y});
        m_samples.push_back({.p = {x, y}});
    }

    image<float> generate(const int width, const int height) {
        image<float> result(width, height);

        std::sort(m_samples.begin(), m_samples.end(), [&](const auto& lhs, const auto& rhs) {
            return lhs.p.x < rhs.p.x || (lhs.p.x == rhs.p.x && lhs.p.y < rhs.p.y);
        });

        //m_samples.clear();
        //m_samples.push_back({.p = {49, 94}});
        //m_samples.push_back({.p = {80, 170}});
        //m_samples.push_back({.p = {88, 75}});
        //m_samples.push_back({.p = {116, 133}});
        for (int i = 0; i < m_samples.size(); i++) {
            m_samples[i].soi.inequations.push_back({.a_x = 1, .a_y = 0, .t = 0});
            m_samples[i].soi.inequations.push_back({.a_x = 0, .a_y = 1, .t = 0});
            m_samples[i].soi.inequations.push_back({.a_x = -1, .a_y = 0, .t = static_cast<float>(width)});
            m_samples[i].soi.inequations.push_back({.a_x = 0, .a_y = -1, .t = static_cast<float>(height)});
        }

        for (int i = 0; i < m_samples.size(); i++) {
            for (int j = i + 1; j < m_samples.size(); j++) {
                if (m_samples[i].check_and_update_range(m_samples[j], width, height)) {
                    m_samples[i].create_edge_if_needed(m_samples[j]);
                } else {
                    auto dx = m_samples[j].p.x - m_samples[i].p.x;

                    if (dx / 2 >= m_samples[i].scan_radius) {
                        break;
                    }
                }
            }
        }

        for (int i = 0; i < m_samples.size(); i++) {
            auto c = i + 1; ////m_samples[i].size(result);
            m_samples[i].paint_on_as(result, c, c);
        }

        return result;
    }

private:
    struct point {
        float x;
        float y;
    };

    static float distance(const point& a, const point& b) {
       float dx = b.x - a.x;
       float dy = b.y - a.y;
       return sqrt(dx*dx + dy*dy);
    }

    static float diameter_to_edge(const float d) {
       return d / sqrt(2.0f);
    }

    struct inequation {
        float a_x;
        float a_y;
        float t;

        bool satisfies(const point& p) const {
            return a_x * p.x + a_y * p.y + t >= 0;
        }
        bool loose_satisfies(const point& p) const {
            return a_x * p.x + a_y * p.y + t > -4;
        }
        float evaluate_on_x(const float x) const {
            if (a_y == 1) {
                return -a_x * x - t;
            }
            if (a_y == -1) {
                return a_x * x + t;
            }
            return -1;
        }
        float evaluate_on_y(const float y) const {
            assert(a_y != 0);
            if (a_y == 1) {
                return (y + t) / (-a_x);
            }
            if (a_y == -1) {
                return (y - t) / a_x;
            }
            return -1;
        }
        inequation invert() const {
            inequation copy = *this;
            copy.a_x *= -1;
            copy.a_y *= -1;
            copy.t *= -1;
            return copy;
        }
        point compute_crosspoint(const inequation& e2) const {
            const inequation& e1 = *this;
            bool e1_is_vertical = std::abs(e1.a_y) == 0;
            bool e1_is_horizontal = std::abs(e1.a_x) == 0;
            bool e2_is_vertical = std::abs(e2.a_y) == 0;
            bool e2_is_horizontal = std::abs(e2.a_x) == 0;

            if (e1_is_vertical && e2_is_vertical) {
                return {-1, -1};
            }
            if (e1_is_vertical && !e2_is_vertical) {
                point r;
                r.x = std::abs(e1.t);
                r.y = e2.evaluate_on_x(r.x);
                return r;
            }
            if (!e1_is_vertical && e2_is_vertical) {
                point r;
                r.x = std::abs(e2.t);
                r.y = e1.evaluate_on_x(r.x);
                return r;
            }
            if (e1_is_horizontal && e2_is_horizontal) {
                return {-1, -1};
            }
            if (e1_is_horizontal && !e2_is_horizontal) {
                point r;
                r.y = std::abs(e1.t);
                r.x = e2.evaluate_on_y(r.y);
                return r;
            }
            if (!e1_is_horizontal && e2_is_horizontal) {
                point r;
                r.y = std::abs(e2.t);
                r.x = e1.evaluate_on_y(r.y);
                return r;
            }
            if (e1_is_vertical && e2_is_horizontal) {
                point r;
                r.x = std::abs(e1.t);
                r.y = std::abs(e2.t);
                return r;
            }
            if (e1_is_horizontal && e2_is_vertical) {
                point r;
                r.x = std::abs(e2.t);
                r.y = std::abs(e1.t);
                return r;
            }

            auto normalized_e1 = e1;
            auto normalized_e2 = e2;

            if (normalized_e1.a_y > 0) {
                normalized_e1 = normalized_e1.invert();
            }
            if (normalized_e2.a_y > 0) {
                normalized_e2 = normalized_e2.invert();
            }

            point r;
            r.x = (normalized_e2.t - normalized_e1.t) / (normalized_e1.a_x - normalized_e2.a_x);
            r.y = normalized_e1.evaluate_on_x(r.x);

            return r;
        }
        bool are_parallel_and_more_strict(const inequation& e2) const {
            const inequation& e1 = *this;
            bool e1_is_vertical = e1.a_y == 0;
            bool e1_is_horizontal = e1.a_x == 0;
            bool e2_is_vertical = e2.a_y == 0;
            bool e2_is_horizontal = e2.a_x == 0;

            if (e1_is_vertical && e2_is_vertical) {
                if (e1.a_x > 0 && e2.a_x > 0) {
                    return e2.t > e1.t;
                }
                if (e1.a_x < 0 && e2.a_x < 0) {
                    return e2.t < e1.t;
                }
            }
            if (e1_is_horizontal && e2_is_horizontal) {
                if (e1.a_y > 0 && e2.a_y > 0) {
                    return e2.t > e1.t;
                }
                if (e1.a_y < 0 && e2.a_y < 0) {
                    return e2.t < e1.t;
                }
            }

            return false;
        }
    };

    struct system_of_inequations {
        std::vector<inequation> inequations;

        bool cut_space_by(const inequation& e) const {
            if (inequations.empty()) {
                return true;
            }

            if (std::abs(e.a_x) == 0) {
                //horizontal
                for (int i = 0; i < inequations.size(); i++) {
                    if (std::abs(inequations[i].a_x) != 0) {
                        continue;
                    }

                    if (e.a_y > 0 && inequations[i].a_y > 0 && e.t > inequations[i].t) {
                        return false;
                    }
                    if (e.a_y < 0 && inequations[i].a_y < 0 && e.t > inequations[i].t) {
                        return false;
                    }
                    if (e.a_y > 0 && inequations[i].a_y < 0 && (-e.t) > inequations[i].t) {
                        return false;
                    }
                    if (e.a_y < 0 && inequations[i].a_y > 0 && e.t < (-inequations[i].t)) {
                        return false;
                    }
                }
            }
            if (std::abs(e.a_y) == 0) {
                //vertical
                for (int i = 0; i < inequations.size(); i++) {
                    if (std::abs(inequations[i].a_y) != 0) {
                        continue;
                    }

                    if (e.a_x > 0 && inequations[i].a_x > 0 && e.t > inequations[i].t) {
                        return false;
                    }
                    if (e.a_x < 0 && inequations[i].a_x < 0 && e.t > inequations[i].t) {
                        return false;
                    }
                    if (e.a_x > 0 && inequations[i].a_x < 0 && (-e.t) > inequations[i].t) {
                        return false;
                    }
                    if (e.a_x < 0 && inequations[i].a_x > 0 && e.t < (-inequations[i].t)) {
                        return false;
                    }
                }
            }

            for (int i = 0; i < inequations.size(); i++) {
                if (std::abs(e.a_x) == 0 && std::abs(inequations[i].a_x) == 0) {
                    continue;
                }
                if (std::abs(e.a_y) == 0 && std::abs(inequations[i].a_y) == 0) {
                    continue;
                }
                auto cp = e.compute_crosspoint(inequations[i]);

                bool success = true;
                for (int j = 0; j < inequations.size(); j++) {
                    if (i != j) {
                        if (!inequations[j].loose_satisfies(cp)) {
                            success = false;
                        }
                    }
                }
                if (success) {
                    return true;
                }
            }

            return false;
        }
        bool satisfies(const point& p) const {
            for (const auto& e : inequations) {
                if (!e.satisfies(p)) {
                    return false;
                }
            }

            return true;
        }

    };


    struct sample {
        system_of_inequations soi;
        point p;
        float scan_radius = std::numeric_limits<float>::max();

        void paint_on_as(image<float>& img, const float color, const float mark) {
            for (int y = std::max(0.0f, p.y - scan_radius); y <= std::min(static_cast<float>(img.height() - 1), p.y + scan_radius); y++) {
                for (int x = std::max(0.0f, p.x - scan_radius); x <= std::min(static_cast<float>(img.width() - 1), p.x + scan_radius); x++) {
                    if (soi.satisfies({static_cast<float>(x), static_cast<float>(y)})) {
                        if (auto p = img.get(x, y)) {
                            **p = color;
                        }
                    }
                }
            }

            img.at(p.x, p.y) = mark;
        }
        size_t size(image<float>& img) const {
            size_t result = 0;
            for (int y = std::max(0.0f, p.y - scan_radius); y <= std::min(static_cast<float>(img.height() - 1), p.y + scan_radius); y++) {
                for (int x = std::max(0.0f, p.x - scan_radius); x <= std::min(static_cast<float>(img.width() - 1), p.x + scan_radius); x++) {
                    if (soi.satisfies({static_cast<float>(x), static_cast<float>(y)})) {
                        result++;
                    }
                }
            }

            return result;
        }

        inequation separation_line(const point& p2) const {
            const auto& p1 = p;
            point mid = {.x = (p1.x + p2.x) / 2.0f, .y = (p1.y + p2.y) / 2.0f};

            auto dx = mid.x - p1.x;
            auto dy = mid.y - p1.y;

            assert(dx != 0 || dy != 0);

            if (dx == 0) {
                inequation result = {.a_x = 0, .a_y = -1, .t = mid.y};
                if (dy > 0) {
                    return result;
                } else {
                    return result.invert();
                }
            }

            if (dy == 0) {
                inequation result = {.a_x = -1, .a_y = 0, .t = mid.x};
                if (dx > 0) {
                    return result;
                } else {
                    return result.invert();
                }
            }

            auto m = -dx / dy;
            auto t = mid.y - m * mid.x;

            inequation result = {.a_x = m, .a_y = -1, .t = t};
            if (dy > 0) {
                return result;
            } else {
                return result.invert();
            }
        }

        bool check_and_update_range(const sample& other, const int width, const int height) {
            auto d = distance(p, other.p);
            auto e = diameter_to_edge(d) / 2;

            //std::cout << "\t" << e << std::endl;

            if (e >= scan_radius) {
                return false;
            }

            inequation left = {.a_x = 1, .a_y = 0, .t = -(p.x - e)};
            inequation right = {.a_x = -1, .a_y = 0, .t = p.x + e};
            inequation top = {.a_x = 0, .a_y = -1, .t = p.y + e};
            inequation bot = {.a_x = 0, .a_y = 1, .t = -(p.y - e)};

            if ( 
                (std::abs(left.t) >= 0 && soi.cut_space_by(left)) ||
                (std::abs(right.t) < width && soi.cut_space_by(right)) ||
                (std::abs(top.t) < height && soi.cut_space_by(top)) ||
                (std::abs(bot.t) >= 0 && soi.cut_space_by(bot))
            ) {
                //std::cout << "\t" << (std::abs(left.t) >= 0 && soi.cut_space_by(left)) << " ";
                //std::cout << (std::abs(right.t) < width && soi.cut_space_by(right)) << " ";
                //std::cout << (std::abs(top.t) < height && soi.cut_space_by(top)) << " ";
                //std::cout << (std::abs(bot.t) >= 0 && soi.cut_space_by(bot)) << std::endl;
                return true;
            } else {
                scan_radius = e;

                return false;
            }
        }

        void create_edge_if_needed(sample& other) {
            auto s1_s2 = separation_line(other.p);
            auto s2_s1 = s1_s2.invert();

            if (soi.cut_space_by(s1_s2)) {
                soi.inequations.push_back(s1_s2);
            }
            if (other.soi.cut_space_by(s2_s1)) {
                other.soi.inequations.push_back(s2_s1);
            }
        }
    };

   std::vector<sample> m_samples; 
   std::set<std::pair<float, float>> m_sample_points;
};
