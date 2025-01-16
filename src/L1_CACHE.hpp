#include "L2_CACHE.hpp"

/*
   L1 module, to manage reading and writing of data
*/

SC_MODULE(L1_CACHE)
{
    // sc_in ports
    //------------------------------------------------------------------------------------------------------------------
    sc_in<bool> clk;
    sc_in<u_int32_t> address;
    sc_in<u_int32_t> data;
    sc_in<bool> write_enabled;
    //------------------------------------------------------------------------------------------------------------------

    // sc_signals
    //------------------------------------------------------------------------------------------------------------------
    sc_signal<u_int32_t> test;
    sc_signal<bool> l2_enabled;
    sc_signal<sc_bv<8> *> l2_data_chunk;
    sc_signal<bool> l2_finished, l2_force_read;
    sc_signal<bool> l2_failed;
    sc_signal<bool> l2_cache_hit;
    //------------------------------------------------------------------------------------------------------------------

    // global variables
    //------------------------------------------------------------------------------------------------------------------
    L2_CACHE l2_cache_module;
    const uint32_t cache_lines;
    const uint32_t cache_latency;
    const uint32_t cache_line_size;
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

    SC_CTOR(L1_CACHE);

    L1_CACHE(sc_module_name name, uint32_t cache_lines, uint32_t cache_latency, uint32_t cache_line_size,
             uint32_t cache_latency2, uint32_t cache_lines2, uint32_t memory_latency)
        : sc_module(name),
          l2_cache_module("L2",
                          cache_lines2,
                          cache_latency2,
                          cache_line_size,
                          memory_latency),
          cache_lines(cache_lines),
          cache_latency(cache_latency),
          cache_line_size(cache_line_size)
    {
        l2_cache_module.clk(clk);
        l2_cache_module.address(address);
        l2_cache_module.data(data);
        l2_cache_module.write_enabled(write_enabled);
        l2_cache_module.result(l2_data_chunk);
        l2_cache_module.trigger(l2_finished);
        l2_cache_module.enabled(l2_enabled);
        l2_cache_module.force_read(l2_force_read);
        l2_cache_module.failure(l2_failed);
        l2_cache_module.cache_hit(l2_cache_hit);

        
        l2_data_chunk.write(nullptr);

        try
        {
            cache_memory = new sc_bv<8> *[cache_lines];
        }
        catch (std::bad_alloc& e)
        {
            std::cerr << "failed to allocate l1_cache_memory: " << "programm terminated" << "\n";
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
            catch (std::bad_alloc& e)
            {
                std::cerr << "failed to allocate l1_cache_memory: " << "programm terminated" << "\n";
                failure.write(true);
                return;
            }
        }
        try
        {
            address_memory = new uint32_t[cache_lines];
        }
        catch (std::bad_alloc& e)
        {
            std::cerr << "failed to allocate l1_address_memory: " << "programm terminated" << "\n";
            failure.write(true);
            return;
        }

        for (uint32_t i = 0; i < cache_lines; ++i)
        {
            address_memory[i] = UINT32_MAX;
        }

        // calc index- and offset-bits
        index_bits = log2(cache_lines);
        offset_bits = log2(cache_line_size);

        SC_THREAD(behavior);
        sensitive << clk.pos();
    }

    ~L1_CACHE()
    {

        for (uint32_t i = 0; i < cache_lines; ++i)
        {
            delete[] (cache_memory[i]);
        }
        delete[] (cache_memory);

        delete[] (address_memory);

        delete[] helper;
    }

    void behavior()
    {

        while (true)
        {

            wait(cache_latency);
            cache_hit.write(false);

            //  calculate index,offset and tag
            const u_int32_t index = (address >> offset_bits) & (static_cast<int>(pow(2, index_bits) - 1)) % cache_lines;
            const u_int32_t offset = address & (static_cast<int>(pow(2, offset_bits) - 1));
            const u_int32_t tag = address >> (index_bits + offset_bits);

            // calculate values for second line if necessary
            const uint32_t bytes_in_first_line = cache_line_size - offset;
            const uint32_t new_address = address + bytes_in_first_line;
            const uint32_t new_tag = new_address >> (index_bits + offset_bits);
            const uint32_t new_index = (index + 1) % cache_lines;

            

            if (!write_enabled.read())
            {
                // if in cache
                if (address_memory[index] == tag)
                {
                    // if 4 byte in first cacheLine avaiable, read the Line and return result
                    if (cache_line_size - offset >= 4)
                    {
                        delete[] (helper);
                        helper = new sc_bv<8>[cache_line_size];
                        std::copy_n(cache_memory[index], cache_line_size, helper);
                        result.write(helper);

                        cache_hit.write(true);
                        trigger.write(!trigger.read());
                    }
                    // if more than one cacheLine has to be read
                    else
                    {
                        if (address_memory[new_index] == new_tag)
                        {
                            delete[] (helper);
                            try
                            {
                                helper = new sc_bv<8>[cache_line_size * 2];
                            }
                            catch (std::bad_alloc& e)
                            {
                                std::cerr << "failed to allocate l1_helper_memory: " << "programm terminated" << "\n";
                                failure.write(true);
                                return;
                            }

                            // fetch data from cache

                            sc_bv<8> *first_line = cache_memory[index];
                            sc_bv<8> *second_line = cache_memory[new_index];

                            std::copy_n(first_line, cache_line_size, helper);
                            std::copy_n(second_line, cache_line_size, helper);
                            result.write(helper);

                            cache_hit.write(true);
                            trigger.write(!trigger.read());
                            continue;
                        }
                        else
                        {
                            // data not completely in cache return L2_chunk and write new data to the cache
                            
                            //enable Memory
                            l2_enabled.write(true);
                            wait(l2_finished.value_changed_event());
                            l2_enabled.write(false);

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
                                    std::cerr << "failed to fetch cache_lines during read of more than one line in: l1_cache"
                                              << "\n"
                                              << "programm terminated" << "\n";
                                    failure.write(true);
                                    return;
                                }
                            }

                            std::copy_n(l2_data_chunk.read(), cache_line_size, first_line);
                            std::copy_n(l2_data_chunk.read() + cache_line_size, cache_line_size, second_line);

                            address_memory[index] = tag;
                            address_memory[new_index] = new_tag;
                            cache_hit.write(l2_cache_hit);
                            result.write(l2_data_chunk);
                            trigger.write(!trigger.read());
                            continue;
                        }
                    }
                }
                else
                {
                    //  adress not in cache
                    //  wait for L2 cache to finish
                    l2_enabled.write(true);
                    wait(l2_finished.value_changed_event());
                    l2_enabled.write(false);

                    if (cache_line_size - offset < 4)
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
                                std::cerr << "failed to fetch cache_lines during read of more than one line in: l1_cache"
                                          << "\n"
                                          << "programm terminated" << "\n";
                                failure.write(true);
                                return;
                            }
                        }

                        // copy 2 cachelines from L2 cache to L1 cache
                        std::copy_n(l2_data_chunk.read(), cache_line_size, first_line);
                        std::copy(l2_data_chunk.read() + cache_line_size,
                                  l2_data_chunk.read() + 2 * cache_line_size, second_line);

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
                        std::copy_n(l2_data_chunk.read(), cache_line_size, cache_line);
                        address_memory[index] = tag;
                    }
                    cache_hit.write(l2_cache_hit);
                    result.write(l2_data_chunk);
                    trigger.write(!trigger.read());
                }
            }
            else
            {
                //  write data to the Cache

                // send write request to the L2_Cache
                l2_enabled.write(true);
                wait(l2_finished.value_changed_event());

                // Update Cache with new values from Memory
                l2_enabled.write(false);

                l2_force_read.write(true);
                l2_enabled.write(true);
                wait(l2_finished.value_changed_event());
                l2_enabled.write(false);
                l2_force_read.write(false);
                
                if (cache_line_size - offset < 4)
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
                            std::cerr << "failed to fetch cache_lines during read of more than one line in: l1_cache"
                                      << "\n"
                                      << "programm terminated" << "\n";
                            failure.write(true);
                            return;
                        }
                    }
                    std::copy_n(l2_data_chunk.read(), cache_line_size,
                                first_line);
                    std::copy_n(l2_data_chunk.read() + cache_line_size,
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
                    std::copy_n(l2_data_chunk.read(), cache_line_size, cache_line);
                    address_memory[index] = tag;
                }
                trigger.write(!trigger);
            }
        }
    }
};

/*
 * #####################################################################################################################
 * End of L1_CACHE
 */
