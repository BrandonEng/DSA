#pragma once

#include <chrono>

using std::chrono::microseconds;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::minutes;
using std::chrono::hours;

template<typename TimeUnit>
struct timer
{
    using clock = std::chrono::high_resolution_clock;


    void start()
    {
        mStartTime = clock::now();
    }
    void stop()
    {
        mEndTime = clock::now();
    }

    auto get_duration() const
    {
        return std::chrono::duration_cast<TimeUnit>(mEndTime - mStartTime);
    }

private:
    clock::time_point mStartTime;
    clock::time_point mEndTime;
};