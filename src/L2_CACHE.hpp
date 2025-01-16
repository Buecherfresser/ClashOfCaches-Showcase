#include "MEMORY.hpp"

/*
   L2 module, to manage reading and writing of data
*/

SC_MODULE(L2_CACHE)
{
    // sc_in ports
    //------------------------------------------------------------------------------------------------------------------
    sc_in<bool> clk;
    sc_in<u_int32_t> address;
    sc_in<u_int32_t> data;
    sc_in<bool> write_enabled;
    sc_in<bool> force_read;
    sc_in<bool> enabled;
    //------------------------------------------------------------------------------------------------------------------

    // sc_signals
    //------------------------------------------------------------------------------------------------------------------
    sc_signal<sc_bv<8> *> memory_data_chunk;
    sc_signal<bool> memory_trigger;
    sc_signal<bool> memory_enabled;
    sc_signal<bool> memory_double_line;
    sc_signal<bool> memory_force_read;
    sc_signal<uint32_t> memory_address_helper;
    //------------------------------------------------------------------------------------------------------------------

    // global variables
    //------------------------------------------------------------------------------------------------------------------
    MEMORY memory_module;
    uint32_t cache_lines;
    uint32_t cache_latency;
    uint32_t cache_line_size;
    sc_bv<8> **cache_memory;
    uint32_t *address_memory;
    uint32_t index_bits;
    uint32_t offset_bits;
    sc_bv<8> *helper = new sc_bv<8>[cache_line_size];
    //------------------------------------------------------------------------------------------------------------------

    // sc_out ports
    //------------------------------------------------------------------------------------------------------------------
    sc_out<sc_bv<8> *> result;
    sc_out<bool> trigger;
    sc_out<bool> failure;
    sc_out<bool> cache_hit;
    //------------------------------------------------------------------------------------------------------------------

    SC_CTOR(L2_CACHE);

    L2_CACHE(sc_module_name name, uint32_t cache_lines, uint32_t cache_latency, uint32_t cache_line_size,
             uint32_t memory_latency)
        : sc_module(name),
          memory_trigger(nullptr),
          memory_module("Memory", memory_latency, cache_line_size),
          cache_lines(cache_lines),
          cache_latency(cache_latency),
          cache_line_size(cache_line_size), failure(nullptr)
    {
        memory_module.clk(clk);
        memory_module.address(memory_address_helper);
        memory_module.dataIn(data);
        memory_module.WE(write_enabled);
        memory_module.enable(memory_enabled);
        memory_module.double_cacheline(memory_double_line);
        memory_module.finished(memory_trigger);
        memory_module.dataOut(memory_data_chunk);
        memory_module.force_read(memory_force_read);

        try
        {
            cache_memory = new sc_bv<8> *[cache_lines];
        }
        catch (std::bad_alloc &e)
        {
            std::cerr << "failed to allocate l2_cache_memory: " << "programm terminated" << "\n";
            failure.write(true);
            return;
        }

        // initialize cache_memory with cachelines
        for (uint32_t i = 0; i < cache_lines; i++)
        {
            try
            {
                cache_memory[i] = new sc_bv<8>[cache_line_size];
            }
            catch (std::bad_alloc &e)
            {
                std::cerr << "failed to allocate l2_cache_memory: " << "programm terminated" << "\n";
                failure.write(true);
                return;
            }
        }
        try
        {
            address_memory = new uint32_t[cache_lines];
        }
        catch (std::bad_alloc &e)
        {
            std::cerr << "failed to allocate l2_address_memory: " << "programm terminated" << "\n";
            failure.write(true);
            return;
        }

        // edge case prevention if tag would be 0 but has never been written in the cache
        for (uint32_t i = 0; i < cache_lines; ++i)
        {
            address_memory[i] = UINT_MAX;
        }

        // calc index- and offset-bits
        index_bits = log2(cache_lines);
        offset_bits = log2(cache_line_size);

        SC_THREAD(behavior);
        sensitive << clk.pos();
    }

    ~L2_CACHE()
    {
        for (uint32_t i = 0; i < cache_lines; ++i)
        {
            if (cache_memory[i] != nullptr)
                delete[] (cache_memory[i]);
        }
        delete[] (cache_memory);

        delete[] helper;

        delete[] (address_memory);
    }

    void behavior()
    {

        while (true)
        {
            wait();

            if (enabled.read())
            {
                wait(cache_latency);
                cache_hit.write(false);

                // calculate index,offset and tag
                const u_int32_t index = (address >> offset_bits) & (static_cast<int>(pow(2, index_bits) - 1)) %
                                                                       cache_lines;
                const u_int32_t offset = address & static_cast<int>(pow(2, offset_bits) - 1);
                const u_int32_t tag = address >> (index_bits + offset_bits);

                // calculate values for second cache line if necessary
                const uint32_t bytes_in_first_line = cache_line_size - offset;
                const uint32_t new_adress = address + bytes_in_first_line;
                const uint32_t new_tag = new_adress >> (index_bits + offset_bits);
                const uint32_t new_index = (index + 1) % cache_lines;

                if (!(write_enabled.read()) || force_read.read())
                {
                    // if in cache
                    if (address_memory[index] == tag)
                    {
                        // if 4 bytes in first cacheLine available, read the line and return result
                        if (cache_line_size - offset >= 4)
                        {
                            delete[] (helper);
                            helper = new sc_bv<8>[cache_line_size];
                            std::copy_n(cache_memory[index], cache_line_size, helper);
                            result.write(helper);

                            cache_hit.write(true);
                            trigger.write(!trigger.read());
                        }
                        // if more than one cacheline has to be read
                        else
                        {
                            if (address_memory[new_index] == new_tag)
                            {
                                delete[] (helper);
                                try
                                {
                                    helper = new sc_bv<8>[cache_line_size * 2];
                                }
                                catch (std::bad_alloc &e)
                                {
                                    std::cerr << "failed to allocate l2_helper_memory: " << "programm terminated" << "\n";
                                    failure.write(true);
                                    return;
                                }

                                // fetch data from first line

                                sc_bv<8> *first_line = cache_memory[index];
                                sc_bv<8> *second_line = cache_memory[new_index];

                                std::copy_n(first_line, cache_line_size, helper);
                                std::copy_n(second_line, cache_line_size, helper + cache_line_size);
                                result.write(helper);

                                cache_hit.write(true);
                                trigger.write(!trigger.read());
                                continue;
                            }
                            else
                            {
                                // data not completely in cache return L2_chunk and write new data to the cache

                                // set values to enable Memory
                                memory_address_helper.write(address.read() - offset);
                                memory_double_line.write(true);
                                memory_enabled.write(true);
                                wait(memory_trigger.value_changed_event());
                                memory_enabled.write(false);

                                // fetch the 2 lines required and set them to 0
                                sc_bv<8> *first_line = cache_memory[index];
                                sc_bv<8> *second_line = cache_memory[new_index];

                                for (uint32_t i = 0; i < cache_line_size; ++i)
                                {
                                    first_line[i] = 0;
                                    second_line[i] = 0;
                                }

                                if (first_line == nullptr || second_line == nullptr)
                                {
                                    {
                                        std::cerr << "failed to fetch cache_lines during read of more than one line in: l2_cache"
                                                  << "\n"
                                                  << "programm terminated" << "\n";
                                        failure.write(true);
                                        return;
                                    }
                                }
                                // copy data recieved from Memory to the 2 cache lines
                                std::copy_n(memory_data_chunk.read(), cache_line_size, first_line);
                                std::copy(memory_data_chunk.read() + cache_line_size,
                                          memory_data_chunk.read() + 2 * cache_line_size, second_line);

                                address_memory[index] = tag;
                                address_memory[new_index] = new_tag;
                            }
                        }
                    }
                    else
                    {
                        // address not in cache

                        // request Memory
                        if (cache_line_size - offset < 4)
                        {
                            memory_double_line.write(true);
                        }
                        else
                        {
                            memory_double_line.write(false);
                        }
                        memory_address_helper.write(address.read() - offset);
                        memory_enabled.write(true);

                        wait(memory_trigger.value_changed_event());

                        memory_enabled.write(false);

                        if (memory_double_line.read())
                        {
                            sc_bv<8> *first_line = cache_memory[index];
                            sc_bv<8> *second_line = cache_memory[new_index];

                            for (uint32_t i = 0; i < cache_line_size; ++i)
                            {
                                first_line[i] = 0;
                                second_line[i] = 0;
                            }

                            if (first_line == nullptr || second_line == nullptr)
                            {
                                {
                                    std::cerr << "failed to fetch cache_lines during read of more than one line in: l2_cache"
                                              << "\n"
                                              << "programm terminated" << "\n";
                                    failure.write(true);
                                    return;
                                }
                            }

                            // copy cachelines the returned cachelines from L2
                            std::copy_n(memory_data_chunk.read(), cache_line_size,
                                        first_line);
                            std::copy_n(memory_data_chunk.read() + cache_line_size,
                                        cache_line_size, second_line);

                            address_memory[index] = tag;
                            address_memory[new_index] = new_tag;
                        }
                        else
                        {
                            sc_bv<8> *cache_line = cache_memory[index];
                            for (uint32_t i = 0; i < cache_line_size; ++i)
                            {
                                cache_line[i] = 0;
                            }
                            std::copy_n(memory_data_chunk.read(), cache_line_size, cache_line);

                            address_memory[index] = tag;
                        }
                    }
                    result.write(memory_data_chunk);
                    trigger.write(!trigger.read());
                }
                else
                {
                    // write data to Memory an then the line from the memory to the Cache
                    memory_address_helper.write(address);
                    memory_enabled.write(true);
                    wait(memory_trigger.value_changed_event());
                    memory_enabled.write(false);

                    memory_force_read.write(true);
                    if (cache_line_size - offset < 4)
                    {
                        memory_double_line.write(true);
                    }
                    else
                    {
                        memory_double_line.write(false);
                    }
                    memory_address_helper.write(address.read() - offset);
                    memory_enabled.write(true);
                    wait(memory_trigger.value_changed_event());
                    memory_enabled.write(false);
                    memory_force_read.write(false);

                    // got 2 cache lines back
                    if (memory_double_line.read())
                    {
                        sc_bv<8> *first_line = cache_memory[index];
                        sc_bv<8> *second_line = cache_memory[new_index];

                        for (uint32_t i = 0; i < cache_line_size; ++i)
                        {
                            first_line[i] = 0;
                            second_line[i] = 0;
                        }

                        if (first_line == nullptr || second_line == nullptr)
                        {
                            {
                                std::cerr << "failed to fetch cache_lines during read of more than one line in: l2_cache" << "\n"
                                          << "programm terminated" << "\n";
                                failure.write(true);
                                return;
                            }
                        }
                        std::copy_n(memory_data_chunk.read(), cache_line_size,
                                    first_line);
                        std::copy_n(memory_data_chunk.read() + cache_line_size,
                                    cache_line_size, second_line);

                        address_memory[index] = tag;
                        address_memory[new_index] = new_tag;
                    }

                    // got one cache line back
                    else
                    {
                        sc_bv<8> *cache_line = cache_memory[index];
                        for (uint32_t i = 0; i < cache_line_size; ++i)
                        {
                            cache_line[i] = 0;
                        }
                        std::copy_n(memory_data_chunk.read(), cache_line_size, cache_line);
                        address_memory[index] = tag;
                    }
                    trigger.write(!trigger.read());
                }
            }
        }
    }
};

/*
 * #####################################################################################################################
 * End of L2_CACHE
 */
