#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <limits>
#include <type_traits>


/**
* Exports the sample to an Portable_Anymap(.pgm)-file
* --> https://de.wikipedia.org/wiki/Portable_Anymap
* @param filename: the output-file
* @return: true on success
*/
template <typename T>
bool export_ppm(const std::string& filename, const int width, const int height, const std::vector<T>& image) {
    std::ofstream file;
    file.open(filename);

    if (file.is_open()) {
        file << "P5 ";
        file << std::to_string(width);
        file << " ";
        file << std::to_string(height);
        file << " 255 ";

        for (auto p : image) {
            if (std::is_integral<T>::value) {
                file << (uint8_t)(p / (std::numeric_limits<T>::max() / 255));
            } else {
                file << (uint8_t) (p * 255);
            }
        }

        return file.good();
    }

    return false;
}
