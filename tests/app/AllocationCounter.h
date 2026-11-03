#pragma once

#include <atomic>

namespace algo::test
{
    // Counts the calls to the global operator new while counting is on. allocation_counter.cpp replaces operator new.
    std::atomic<bool>& countingAllocations ();
    std::atomic<long>& allocationCount ();

    struct ScopedAllocationCount
    {
        ScopedAllocationCount ()
        {
            allocationCount ()     = 0;
            countingAllocations () = true;
        }
        ~ScopedAllocationCount ()
        {
            countingAllocations () = false;
        }
        long count () const
        {
            return allocationCount ().load ();
        }
    };
}
