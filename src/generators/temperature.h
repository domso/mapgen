#pragma once

#include <unordered_map>

#include "image.h"
#include "helper.h"

namespace mapgen::generators::temperature {

image<float> generate(const image<float>& map);

}
