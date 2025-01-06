#pragma once

#include "image.h"

class circle_stack {
public:
    image<float> at(const int n);
private:
    image<float> generate_scale_circle(const int n) const;
    std::unordered_map<int, image<float>> m_lookup; 
};
