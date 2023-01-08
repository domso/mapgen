#include "generate_multi_terrain.h"

#include "config.h"
#include "terrain_buffer.h"

#include "../../cmd/loading_bar.h"

void insert_as_tile(image<float>& map, const int col, const int row, image<float>& terrain) {    
    if (auto region = map.subregion(
        col * terrain.width() - config::terrain::max_hill_size * col, 
        row * terrain.height() - config::terrain::max_hill_size * row,
        terrain.width(),
        terrain.height()
    )) {
        (*region).add(terrain);
    }
}

image<float> generate_multi_terrain(const size_t width, const size_t height, const size_t columns, const size_t rows) {
    image<float> result(width * columns, height * rows);

    terrain_buffer buffer(width, height, 128);
    cmd::loading_bar loading_bar;
    
    loading_bar.init("Generate terrains", rows * columns);
    for (int y = 0; y < rows; y++) {  
        for (int x = 0; x < columns; x++) {            
            image<float> t = buffer.pop();
            insert_as_tile(result, x, y, t);
            loading_bar.step();
        }
    }
    buffer.stop();
    loading_bar.finalize();

    return result;
}
