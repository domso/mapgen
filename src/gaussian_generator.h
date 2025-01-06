#pragma once

#include <random>
#include <iostream>

template<typename T>
class GaussianGenerator {
public:
    GaussianGenerator() {
        std::random_device rd;
        gen = std::mt19937(rd());
        dist = std::normal_distribution<T>(0.10, 0.3);
    }
    
    T generate() {
        T value;
        do {
            value = dist(gen);
        } while (value < 0.0 || value > 1.0);
        
        return value;
    }
private:
    std::mt19937 gen;
    std::normal_distribution<T> dist;
};

