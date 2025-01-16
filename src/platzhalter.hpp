#ifndef MODULES_HPP
#define MODULES_HPP

#include <systemc>
using namespace sc_core;

// TODO mark run_simulation as "C"-Extern
// extern "C" int run_simulation(int seconds);
SC_MODULE(ADDER)
{
    sc_in<bool> clk;
    sc_out<int> sum;

    SC_CTOR(ADDER)
    {
        SC_THREAD(add);
        sensitive << clk.pos();
    }

    void add()
    {
        while (true)
        {
            wait();
            sum->write(sum->read() + 1);
        }
    }
};

#endif