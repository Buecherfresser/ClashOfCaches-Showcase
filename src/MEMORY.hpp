#include <systemc>
#include "systemc.h"
#include <unordered_map>
#include <bitset>
using namespace sc_core;

SC_MODULE(MEMORY)
{
    sc_in<bool> clk;
    sc_in<u_int32_t> address;
    sc_in<u_int32_t> dataIn;
    sc_in<bool> WE;
    sc_in<bool> enable;
    sc_in<bool> double_cacheline;
    sc_in<bool> force_read;

    sc_out<bool> finished;
    // TODO schlechter Stil, befülle mit Werten
    sc_out<sc_bv<8> *> dataOut;
    sc_bv<8> *bytes = nullptr;

    std::unordered_map<uint32_t, uint8_t> memory;
    uint32_t cache_line_size;
    uint32_t cache_line_size_out;
    uint32_t latency;

    SC_CTOR(MEMORY);

    MEMORY(sc_module_name name, const uint32_t latency, const uint32_t cache_line_size)
        : sc_module(name), cache_line_size(cache_line_size), latency(latency)
    {
        SC_THREAD(behaviour);
        sensitive << clk.pos();
    }

    void behaviour()
    {
        while (true)
        {
            wait();
            if (enable.read())
            {
                wait(latency);
                if (WE.read() && !force_read)
                {
                    memory[address.read() + 3] = dataIn.read() & 0xFF;
                    memory[address.read() + 2] = (dataIn.read() >> 8) & 0xFF;
                    memory[address.read() + 1] = (dataIn.read() >> 16) & 0xFF;
                    memory[address.read()] = (dataIn.read() >> 24) & 0xFF;
                    finished.write(!finished);
                }
                else
                {
                    if (double_cacheline.read())
                    {
                        cache_line_size_out = cache_line_size * 2;
                    }
                    else
                    {
                        cache_line_size_out = cache_line_size;
                    }
                    if (bytes != nullptr)
                    {
                        delete[] (bytes);
                    }
                    bytes = new sc_bv<8>[cache_line_size_out];
                    for (uint32_t i = 0; i < cache_line_size_out; ++i)
                    {
                        bytes[i] = 0;
                    }

                    for (uint32_t i = 0; i < cache_line_size_out; ++i)
                    {
                        auto it = memory.find(address + i);
                        if (it != memory.end())
                        {
                            bytes[i] = (unsigned char)it->second;
                        }
                    }
                    dataOut.write(bytes);
                    finished.write(!finished);
                }
            }
        }
    }
};
