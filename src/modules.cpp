#include <systemc>
#include "systemc.h"

#include "Simulation_manager.hpp"

// note that this is necessary to call the method from c
extern "C" struct Result run_simulation(
    int cycles,
    unsigned l1CacheLines,
    unsigned l2CacheLines,
    unsigned cacheLineSize,
    unsigned l1CacheLatency,
    unsigned l2CacheLatency,
    unsigned memoryLatency,
    size_t numRequests,
    struct Request requests[],
    const char *tracefile

);

size_t eval_primitiveGateCount(unsigned l1CacheLines, unsigned l2CacheLines, unsigned cacheLineSize)
{
    size_t ret = 0;
    // one bit = 4 Gates
    // =>
    // Storage gates for l1 + l2 cache
    ret += 4 * ((8 * cacheLineSize) * (l1CacheLines + l2CacheLines));

    // Storage to store addresses
    size_t l1tagindex = 32 - (log2(l1CacheLines) + log2(cacheLineSize));
    size_t l2tagindex = 32 - (log2(l2CacheLines) + log2(cacheLineSize));

    ret += 4 * ((l1CacheLines * l1tagindex + l2CacheLines * l2tagindex));

    // gates to get index offset and tag bits of the requested array (the amout of bits are precalculated)
    size_t calc = 0;
    // shift 200 ( -> shift register 4 * 32 = 128 + control logic)
    // AND 32, SUB 150, add 150, Modulo (power of 2) --> AND 32
    // index
    calc += 200 + 32;
    // offset
    calc += 32;
    // tag
    calc += 200;
    // calc amount of bytes in first line
    calc += 150;
    // calc new address (address in CL)
    calc += 150;
    // calc index and cl of the second cl
    calc += 200 + 150 + 32;

    // calc * 2 for L2 cache

    calc = 2 * calc;

    ret += calc;

    return ret;
}

struct Result run_simulation(
    int cycles,
    unsigned l1CacheLines,
    unsigned l2CacheLines,
    unsigned cacheLineSize,
    unsigned l1CacheLatency,
    unsigned l2CacheLatency,
    unsigned memoryLatency,
    size_t numRequests,
    struct Request *requests,
    const char *tracefile)
{
    // create result that is filled from the SimulationManager
    Result result = {.cycles = INT32_MAX, .misses = 0, .hits = 0, .primitiveGateCount = eval_primitiveGateCount(l1CacheLines, l2CacheLines, cacheLineSize)};
    // new clk
    sc_clock clk("clk", 0.1, SC_SEC);
    // new trace file
    sc_trace_file *trace_file = NULL;
    // TODO: setup tracefile
    if (tracefile != NULL)
    {
        trace_file = sc_create_vcd_trace_file(tracefile);
    }

    SIMULATION_MANAGER simulation_manager("simulationManager", l1CacheLines, l2CacheLines,
                                          cacheLineSize,
                                          l1CacheLatency,
                                          l2CacheLatency,
                                          memoryLatency,
                                          numRequests,
                                          requests,
                                          trace_file,
                                          &result);
    simulation_manager.clk(clk);
    if (cycles != INT32_MAX)
    { // if cycles is a specified value
        sc_start((0.1 * cycles), SC_SEC);
    }
    else
    {
        sc_start();
    }

    std::cout.flush();

    return result;
}

// Note that we need this default sc_main implementation.
int sc_main(int argc, char *argv[])
{
    std::cout << "ERROR" << std::endl;
    return 1;
}
