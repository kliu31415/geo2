#include <profileapi.h>

namespace geo2 {

class Timer
{
    //we use LARGE_INTEGER.QuadPart directly, which assumes a 64-bit system
    static_assert(sizeof(size_t) == 8);

    LARGE_INTEGER v;
public:
    Timer()
    {}
    void start()
    {
        QueryPerformanceCounter(&v);
    }
    uint64_t elapsed_ns()
    {
        LARGE_INTEGER v2;
        QueryPerformanceCounter(&v2);

        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        return ((uint64_t)(1e9) * (v2.QuadPart - v.QuadPart)) / freq.QuadPart;
    }
    uint64_t elapsed_us()
    {
        return elapsed_ns() / 1000;
    }
};

}
