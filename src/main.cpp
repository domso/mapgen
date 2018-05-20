
#include <vector>
#include <iostream>

#include "export_ppm.h"
#include "generate_map.h"

int main() {    
    int width = 512;
    int height = 512;
    
    std::vector<float> map = generate_map(width, height);    
    export_ppm("export.ppm", width, height, map);        
    
    return 0;
}
