#include "AllocationCounter.h"

#include <cstdlib>
#include <new>

namespace algo::test
{
    std::atomic<bool>& countingAllocations ()
    {
        static std::atomic<bool> counting { false };
        return counting;
    }

    std::atomic<long>& allocationCount ()
    {
        static std::atomic<long> count { 0 };
        return count;
    }
}

namespace
{
    void* allocate (std::size_t size)
    {
        if (algo::test::countingAllocations ().load (std::memory_order_relaxed))
            ++algo::test::allocationCount ();
        if (void* p = std::malloc (size == 0 ? 1 : size))
            return p;
        throw std::bad_alloc ();
    }
}

void* operator new (std::size_t size)
{
    return allocate (size);
}

void* operator new[] (std::size_t size)
{
    return allocate (size);
}

void* operator new (std::size_t size, const std::nothrow_t&) noexcept
{
    try
    {
        return allocate (size);
    }
    catch (...)
    {
        return nullptr;
    }
}

void* operator new[] (std::size_t size, const std::nothrow_t&) noexcept
{
    try
    {
        return allocate (size);
    }
    catch (...)
    {
        return nullptr;
    }
}

void operator delete (void* p) noexcept
{
    std::free (p);
}

void operator delete[] (void* p) noexcept
{
    std::free (p);
}

void operator delete (void* p, std::size_t) noexcept
{
    std::free (p);
}

void operator delete[] (void* p, std::size_t) noexcept
{
    std::free (p);
}
