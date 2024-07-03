#pragma once

#include <algorithm>

namespace eng
{

    template<typename It>
    void reverse(It begin, It end)
    {
        --end;
        while (begin < end)
        {
            std::swap(*begin++, *end--);    // Swap elements from each end until iterators intersect in middle
        }
    }

    template<typename It>
    void reverse_strictly_decreasing(It begin, It end, size_t tolerance = 1)    // tolerance 1 for strict ordering
    {
        auto runBegin = begin;
        for (auto runEnd = begin + 1; runBegin != end; ++runEnd)
        {
            // Check for ascending runs and bounds check
            if (*(runEnd - 1) > *runEnd && runEnd != end)
            {
                continue;
            }
            
            // Tolerance specifies what reversals are worth the cost
            if (static_cast<size_t>(runEnd - runBegin) > tolerance)
            {
                reverse(runBegin, runEnd);
            }
            runBegin = runEnd;  // Reset run start; equivalent elements are within tolerance of 1
        }
    }

    template<typename It>
    void insertion_sort(It begin, It end)
    {
        using Type = std::iterator_traits<It>::value_type;

        // Bounds check and early exit
        if (begin >= end || end - begin == 1)
        {
            return;
        }

        // Segment main array into sorted and unsorted segments
        for (auto sorted = begin + 1; sorted != end; ++sorted)
        {
            // Skip already-sorted elements
            if (*(sorted - 1) <= *sorted)
            {
                continue;
            }

            // Move element to insert into temp
            Type temp = std::move(*sorted);
            It scan = sorted;   // scan is always empty

            do
            {
                *scan = std::move(*(scan - 1));
                --scan; // Maintain that scan is always empty
            }
            while (scan != begin && *(scan - 1) > temp);    // Bounds check and stable order check

            *scan = std::move(temp);    // Reinsert back to main
        }
    }

    template<typename InputIt, typename OutputIt>
    void merge(InputIt firstBegin, InputIt firstEnd, InputIt secondBegin, InputIt secondEnd, OutputIt out)
    {
        while (firstBegin != firstEnd)
        {
            if (secondBegin == secondEnd)
            {
                std::move(firstBegin, firstEnd, out);   // Move rest of first run to output
                return;
            }

            // Select element to merge to output with strict ordering in favor of first run
            *out++ = std::move(*firstBegin <= *secondBegin ? *firstBegin++ : *secondBegin++);
        }

        std::move(secondBegin, secondEnd, out); // Move rest of second run to buffer
    }

    template<typename InputIt>
    void stable_sort(InputIt begin, InputIt end)
    {
        using Type = std::iterator_traits<InputIt>::value_type;

        // Bounds check and early exit
        if (begin >= end || end - begin == 1)
        {
            return;
        }

        size_t rangeSize = end - begin;

        // Min-run optimization for more even merges
        size_t min_run = rangeSize;
        constexpr size_t MIN_RUN_THRESHOLD = 10;
        while (min_run >= MIN_RUN_THRESHOLD)
        {
            min_run = (min_run + 1) / 2;
        }

        constexpr size_t REVERSAL_TOLERANCE = 2;    // Tolerance specifies meaningful reversals
        reverse_strictly_decreasing(begin, end, REVERSAL_TOLERANCE);    // Reduce worst-case for insertion sort
        for (size_t i = 0; i < rangeSize; i += min_run) // Use insertion sort for small runs
        {
            insertion_sort(begin + i, begin + std::min(i + min_run, rangeSize));
        }

        // Array was small enough to sort with insertion sort
        if (min_run == rangeSize)
        {
            return;
        }

        // TODO efficiently check if array is already sorted before allocating buffer

        Type* buffer = new Type[rangeSize];

        for (size_t windowSize = min_run; windowSize <= rangeSize; windowSize *= 2)
        {
            // Iterate through window sizes in pairs
            for (size_t i = 0; i + windowSize < rangeSize; i += 2 * windowSize)
            {
                size_t mid = i + windowSize;
                size_t back = std::min(i + 2 * windowSize, rangeSize);

                // Skip already sorted runs
                if (*(begin + mid - 1) <= *(begin + mid))
                {
                    continue;
                }

                // Merge to buffer
                merge(begin + i, begin + mid, begin + mid, begin + back, buffer);

                // Marking next adjacent runs
                size_t j = i + 2 * windowSize;
                mid = j + windowSize;

                // Check if ping-pong merging is applicable
                if (i + 3 * windowSize >= rangeSize || *(begin + mid - 1) <= *(begin + mid))
                {
                    // Move back to original array if we cannot ping-pong merge
                    std::move(buffer, buffer + (back - i), begin + i);
                    continue;
                }

                // Marking continued
                back = std::min(j + 2 * windowSize, rangeSize);

                // Merge next adjacent runs to buffer
                merge(begin + j, begin + mid, begin + mid, begin + back, buffer + (2 * windowSize));

                // Merge runs in buffer back to main
                merge(buffer, buffer + 2 * windowSize, buffer + 2 * windowSize, buffer + (back - i), begin + i);
                
                // Advance i to skip merged runs
                i = j;
            }
        }

        // Free heap-allocated buffers
        delete[] buffer;
        buffer = nullptr;
    }

}