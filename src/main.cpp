#include <iostream>
#include <random>

#include "eng/algorithm/stable_sort.h"

#include "tracker.h"
#include "timer.h"

int main()
{
    constexpr int size = 100;
    tracker arr[size];

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

    timer<milliseconds> sortTimer;

    sortTimer.start();

    eng::stable_sort(arr, arr + size);

    sortTimer.stop();

    for (const auto& i : arr)
    {
        std::cout << i << ' ';
    }
    std::cout << '\n';

    std::cout << sortTimer.get_duration().count() << "ms\n";
}