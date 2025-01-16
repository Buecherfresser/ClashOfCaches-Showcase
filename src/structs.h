// #include <cstdint> // check wether needed for uint32_t
// #include <cstddef> // needed for size_t
#include <stdint.h>

struct Request
{
    uint32_t addr;
    uint32_t data;
    int we;
};

struct Result
{
    size_t cycles;
    size_t misses;
    size_t hits;
    size_t primitiveGateCount;
};