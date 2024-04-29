#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <limits>
#include <type_traits>
#include <optional>
#include <streambuf>

#include "image.h"

/**
* Exports the sample to an Portable_Anymap(.pgm)-file
* --> https://de.wikipedia.org/wiki/Portable_Anymap
* @param filename: the output-file
* @return: true on success
*/
template <typename T>
bool export_ppm(const std::string& filename, const image<T>& img) {
    std::ofstream file;
    file.open(filename);

    if (file.is_open()) {
        file << "P5 ";
        file << std::to_string(img.width());
        file << " ";
        file << std::to_string(img.height());
        file << " 255 ";

        img.for_each_pixel([&](const float& p) {
            if (std::is_integral<T>::value) {
                file << (uint8_t)(p / (std::numeric_limits<T>::max() / 255));
            } else {
                file << (uint8_t) (p * 255);
            }
        });

        return file.good();
    }

    return false;
}

template <typename T>
std::optional<image<T>> import_ppm(const std::string& filename) {
    std::ifstream file;
    file.open(filename);

    if (file.is_open()) {
        auto data = std::vector<char>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        
        int num_spaces = 0;
        int offset = 0;

        std::vector<std::string> header_stack(1);

        for (auto c : data) {
            if (c == ' ') {
                num_spaces++;

                header_stack.push_back("");
            } else {
                *header_stack.rbegin() += c;
            }
            offset++;

            if (num_spaces == 4) {
                break;
            }
        }

        if (
            header_stack.size() < 4 ||
            header_stack[0] != "P5" ||
            header_stack[3] != "255"
        ) {
            return std::nullopt;
        }

        image<T> result(std::stoi(header_stack[1]), std::stoi(header_stack[2]));
        result.for_each_pixel([&](T& c, auto x, auto y) {
            auto addr = result.width() * y + x + offset;

            if (addr < data.size()) {
                unsigned char p = data[addr];
                auto s = static_cast<float>(p) / 255;

                if (std::is_integral<T>::value) {
                    c = std::numeric_limits<T>::max() * s;
                } else {
                    c = s;
                }
            }
        });
        
        return result;
    }

    return std::nullopt;
}
