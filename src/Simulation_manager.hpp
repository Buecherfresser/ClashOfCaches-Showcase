#include <systemc>
#include "systemc.h"

#include "structs.h"
#include "L1_CACHE.hpp"

using namespace sc_core;

// sc_module for managing the requests
SC_MODULE(SIMULATION_MANAGER)
{
    unsigned l1CacheLines;
    unsigned l2CacheLines;
    unsigned cacheLineSize;
    unsigned l1CacheLatency;
    unsigned l2CacheLatency;
    unsigned memoryLatency;
    size_t numRequests;
    struct Request *requests;
    sc_trace_file *tracefile;
    struct Result *result;

    unsigned pc;
    unsigned old_pc;
    unsigned long long cycleCounter;

    sc_in<bool> clk;

    sc_signal<bool> enable, we, hit, finished, failure, cleanup;
    sc_signal<sc_bv<8> *> data_out;
    sc_signal<u_int32_t> address_in, data_in;

    unsigned counter;
    unsigned old_counter;

    L1_CACHE l1;

    SC_CTOR(SIMULATION_MANAGER);

    SIMULATION_MANAGER(
        sc_module_name n,
        unsigned l1Lines,
        unsigned l2Lines,
        unsigned LineSize,
        unsigned l1Latency,
        unsigned l2Latency,
        unsigned memLatency,
        size_t numReq,
        struct Request *req,
        sc_trace_file *tracef,
        struct Result *res) : sc_module(n), l1CacheLines(l1Lines), l2CacheLines(l2Lines),
                              cacheLineSize(LineSize), l1CacheLatency(l1Latency),
                              l2CacheLatency(l2Latency), memoryLatency(memLatency),
                              numRequests(numReq), requests(req), tracefile(tracef), result(res),
                              l1("L1", l1CacheLines, l1CacheLatency, cacheLineSize, l2CacheLatency, l2CacheLines, memoryLatency)
    {
        if (tracefile != NULL)
        {
            // set tracefile
            sc_trace(tracefile, counter, "request_counter");
            sc_trace(tracefile, address_in, "request_address");
            sc_trace(tracefile, data_in, "request_data");
        }

        counter = 0;
        old_counter = 0;

        // -------  SETUP L1_CACHE  --------
        // input
        l1.clk(clk);
        l1.address(address_in);
        l1.data(data_in);
        l1.write_enabled(we);

        // output
        l1.trigger(finished);
        l1.result(data_out);
        l1.failure(failure);
        l1.cache_hit(hit);
        cycleCounter = 0;

        cleanup.write(false);

        SC_THREAD(beh_new);
        sensitive << clk;

        SC_THREAD(beh_wb);
        sensitive << finished;

        SC_THREAD(count_cycles)
        sensitive << clk.pos();

        // setup first request, otherwise the first request is 0
        if (numRequests >= 1)
        {
            address_in = requests[0].addr;
            we = requests[0].we;
            data_in = requests[0].data;
        }
    }
    void count_cycles()
    {
        while (true)
        {
            wait();
            cycleCounter++;
        }
    }

    void beh_new()
    {
        while (true)
        {
            wait();
            while (old_counter == counter)
            {
                wait();
            }
            if (counter == numRequests)
            {
                result->cycles = cycleCounter;
                sc_stop();
                return;
            }

            Request _request = requests[counter];

            old_counter = counter;

            // set input of L1
            address_in = _request.addr;
            we = _request.we;
            if (_request.we)
            {

                data_in = _request.data;
            }
            else
            {
                data_in = 0;
            }
        }
    }

    void beh_wb()
    {
        while (true)
        {
            wait(finished.value_changed_event());

            if (failure.read())
            {
                // failure

                printf("\n Some Error in Simulation --> Abort Simulation \n");

                result->cycles = cycleCounter;
                sc_stop();
                return;
            }

            if (!we)
            { // miss and hit count when reading
                if (hit)
                {
                    result->hits++;
                }
                else
                {
                    result->misses++;
                }
            }

            if (!we)
            {
                // gets the 32 bit from the L1 and updates request
                requests[counter].data = convert_Data_Chunk();
            }

            counter++;

            if (counter == numRequests) // Requests-finished
            {
                result->cycles = cycleCounter;
                sc_stop();
            }
        }
    }

    u_int32_t convert_Data_Chunk() // convert Data_Chunk to uint_32
    {
        u_int32_t index = address_in.read() % cacheLineSize;
        index = index + 3;

        sc_bv<32> bv;

        for (int i = 0; i < 4; i++)
        {
            bv.range((i * 8) + 7, i * 8) = data_out.read()[index - i];
        }
        u_int32_t ret = bv.to_uint();
        return ret;
    }
};