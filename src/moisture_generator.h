#pragma once

#include <unordered_map>
#include <vector>

#include "image.h"
#include "helper.h"
#include "circle_stack.h"

image<float> generate_moisture(const image<float>& rivers, const image<float>& weights);

void apply_circular_add(image<float>& target, const float color, const image<float>& circle, const int x, const int y);
