#include <iostream>
#include <random>
#include <vector>

#include "eng/algorithm/stable_sort.h"
#include "eng/container/vector.h"

#include "tracker.h"
#include "timer.h"

int main()
{
    constexpr int size = 1000;
    size_t arr[size];

    std::mt19937 engine{ std::random_device{ }() };
    std::uniform_int_distribution<size_t> dist{ 1, size };

    for (int i = 0; i < size; ++i)
    {
        arr[i] = dist(engine);
    }

    for (const auto& i : arr)
    {
        std::cout << i << ' ';
    }
    std::cout << '\n';

    timer<microseconds> sortTimer;

    sortTimer.start();

    eng::stable_sort(arr, arr + size);

    sortTimer.stop();

    for (const auto& i : arr)
    {
        std::cout << i << ' ';
    }
    std::cout << '\n';

    std::cout << sortTimer.get_duration().count() << "us\n";

    eng::vector<tracker> vec = { 10, 20, 30, 40, 50 };
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(4);
    vec.push_back(5);
    vec.push_back(6);
    vec.push_back(7);
    vec.push_back(8);

    eng::vector<tracker> vec2;
    vec2 = vec;
    tracker* raw = vec2.data();
    *raw = 0;
    
    for (auto& i : vec2)
    {
        std::cout << i << ' ';
    }
    std::cout << '\n';
}