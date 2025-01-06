#pragma once

#include <random>
#include <memory>
#include <type_traits>


template<typename T>
struct universal_uniform_distribution {
    std::false_type dis;
};

template<typename T>
    requires std::is_integral<T>::value
struct universal_uniform_distribution<T> {
    universal_uniform_distribution(const T& min, const T& max) : dis(min, max) {}
    std::uniform_int_distribution<T> dis;
};

template<typename T>
    requires std::is_floating_point<T>::value
struct universal_uniform_distribution<T> {
    universal_uniform_distribution(const T& min, const T& max) : dis(min, max) {}
    std::uniform_real_distribution<T> dis;
};

struct random_device_wrapper {
    random_device_wrapper() : gen(rd()) {}
    std::random_device rd;
    std::mt19937 gen;
};

template<typename T>
class uniform_random_number {
public:
    uniform_random_number(const T& min, const T& max, std::shared_ptr<random_device_wrapper>& wrapper) : m_dis(min, max), m_wrapper(wrapper) {}
    const T& current() {
        return m_current;
    }
    const T& next() {
        m_current = m_dis.dis(m_wrapper->gen);
        return m_current;
    }
private:
    T m_current;
    universal_uniform_distribution<T> m_dis;
    std::shared_ptr<random_device_wrapper> m_wrapper;
};

class random_generator {
public:
    template<typename T>
    auto uniform(const T& min, const T& max) {
        return uniform_random_number(min, max, m_wrapper);
    }
    bool roll_percentage(const int percent) {
        auto dis = uniform<int>(1, 100);
        return dis.next() <= percent;
    }
    template<typename T>
    const auto& roll_in(const T& container) {
        auto dis = uniform<size_t>(0, container.size() - 1);
        return container.at(dis.next());
    }
    template<typename T>
    auto& roll_in(T& container) {
        auto dis = uniform<size_t>(0, container.size() - 1);
        return container.at(dis.next());
    }
private:
    std::shared_ptr<random_device_wrapper> m_wrapper = std::make_shared<random_device_wrapper>();
};
