#include <systemc>
#include "systemc.h"

#include "structs.h"

extern "C" struct Result run_simulation(
    int cycles,
    unsigned l1CacheLines,
    unsigned l2CacheLines,
    unsigned cacheLineSize,
    unsigned l1CacheLatency,
    unsigned l2CacheLatency,
    unsigned memoryLatency,
    size_t numRequests,
    struct Request requests[], // init array in main
    // struct Request requests[numRequests],
    const char *tracefile);

struct Result run_simulation(
    int cycles,
    unsigned l1CacheLines,
    unsigned l2CacheLines,
    unsigned cacheLineSize,
    unsigned l1CacheLatency,
    unsigned l2CacheLatency,
    unsigned memoryLatency,
    size_t numRequests,
    struct Request requests[], // init array in main
    // struct Request requests[numRequests],
    const char *tracefile)
{
    if (tracefile != nullptr)
    {
        std::cout << "DEBUG: modules.cpp: Tracefile is not empty.\n";
        sc_trace_file *file1 = sc_create_vcd_trace_file(tracefile);
        sc_trace(file1, NULL, "platzhalter");
    }
    struct Result result;
    result.cycles = 0;
    result.hits = 0;
    result.misses = 0;
    result.primitiveGateCount = 0;
    return result;
}
// Note that we need this default sc_main implementation.
int sc_main(int argc, char *argv[])
{
    std::cout << "ERROR" << std::endl;
    return 1;
}