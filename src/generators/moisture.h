#pragma once

#include <unordered_map>
#include <vector>

#include "image.h"
#include "helper.h"
#include "circle_stack.h"

namespace mapgen::generators::moisture {

image<float> generate(const image<float>& rivers, const image<float>& weights);

namespace internal {

void apply_circular_add(image<float>& target, const float color, const image<float>& circle, const int x, const int y);

}
}
