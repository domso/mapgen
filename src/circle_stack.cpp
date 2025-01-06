#include "circle_stack.h"

#include <cmath>

image<float> circle_stack::at(const int n) {
    auto it = m_lookup.find(n);

    if (it != m_lookup.end()) {
        return it->second;
    }

    auto circle = generate_scale_circle(n);
    m_lookup.insert({n, circle});

    return circle;
}

image<float> circle_stack::generate_scale_circle(const int n) const {
    image<float> circle(n, n);
    circle.for_each_pixel([&](auto& f, auto x, auto y) {
        auto half = n / 2;
        auto dx = std::abs(static_cast<int>(x) - half);
        auto dy = std::abs(static_cast<int>(y) - half);
        auto distance = std::sqrt(dx * dx + dy * dy);

        if (distance > half) {
            f = 1.0;
        } else {
            f = distance / static_cast<float>(half);
        }
    });

    circle.for_each_pixel([&](auto& f) {
        f = 1.0f - f;
    });

    return circle;
}
