#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include "structs.h"
#include "main.h"
#include "csv_parsing.h"

// Definitions for command line arguments
#define CACHELINESIZE 1000
#define L1LINES 1001
#define L2LINES 1002
#define L1LATENCY 1003
#define L2LATENCY 1004
#define MEMORYLATENCY 1005
#define TF 1006

extern struct Result run_simulation(
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

// Declare options for long command line arguments, used in getopt_long
struct option long_options[] = {
    {"cycles", required_argument, 0, 'c'},
    {"cacheline-size", required_argument, 0, CACHELINESIZE},
    {"l1-lines", required_argument, 0, L1LINES},
    {"l2-lines", required_argument, 0, L2LINES},
    {"l1-latency", required_argument, 0, L1LATENCY},
    {"l2-latency", required_argument, 0, L2LATENCY},
    {"memory-latency", required_argument, 0, MEMORYLATENCY},
    {"tf", optional_argument, 0, TF},
    {"help", optional_argument, 0, 'h'},
    // null termination as specified in the manual for getopt_long
    {0, 0, 0, 0}};

// Optstring for getopt_long
// Character 'c' (cycles) with an parameter and 'h' (help) without one
const char *optstring = "c:h";

// TODO Checken ob sterr richtig ist
void print_usage(const char *progname)
{
    fprintf(stderr, usage_msg, progname, progname, progname);
}

void print_help(const char *progname)
{
    print_usage(progname);
    fprintf(stderr, "\n%s", help_msg);
}

int main(int argc, char *argv[])
{
    unsigned long cycles = 0;
    unsigned long cache_line_size = 0;
    unsigned long l1_lines = 0;
    unsigned long l2_lines = 0;
    unsigned long l1_latency = 0;
    unsigned long l2_latency = 0;
    unsigned long memory_latency = 0;
    char *path_trace_file;
    char *data_path;
    const char *progname = argv[0];
    unsigned long numRequests = 10;

    if (argc == 1)
    {
        print_usage(progname);
        return EXIT_FAILURE;
    }
    int ch;
    char *endptr = "test";
    // TODO add values to optional parameters
    // Command line parsing
    while (optind < argc)
    {
        // 5th arg: If the longindex field is not NULL, then the integer pointed to by it will be set to the index of the long option relative to longopts.
        if ((ch = getopt_long(argc, argv, optstring, long_options, NULL)) != -1)
        {
            switch (ch)
            {
            case 'c':
                cycles = strtol(optarg, &endptr, 10);
                if (*endptr != '\0')
                { // || errno ka warum die dass wollen TODO
                    fprintf(stderr, "Invalid amount of cycles.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                break;
            case CACHELINESIZE:
                cache_line_size = strtol(optarg, &endptr, 10);
                if (*endptr != '\0')
                { // || errno ka warum die dass wollen TODO
                    fprintf(stderr, "Invalid size for cache line.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                break;
            case L1LINES:
                l1_lines = strtol(optarg, &endptr, 10);
                if (*endptr != '\0')
                { // || errno ka warum die dass wollen TODO
                    fprintf(stderr, "Invalid amount of L1 cache lines.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                break;
            case L2LINES:
                l2_lines = strtol(optarg, &endptr, 10);
                if (*endptr != '\0')
                { // || errno ka warum die dass wollen TODO
                    fprintf(stderr, "Invalid amount of L2 cache lines.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                break;
            case L1LATENCY:
                l1_latency = strtol(optarg, &endptr, 10);
                if (*endptr != '\0')
                { // || errno ka warum die dass wollen TODO
                    fprintf(stderr, "Invalid latency for l1 cache.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                break;
            case L2LATENCY:
                l2_latency = strtol(optarg, &endptr, 10);
                if (*endptr != '\0')
                { // || errno ka warum die dass wollen TODO
                    fprintf(stderr, "Invalid latency for l2 cache.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                break;
            case MEMORYLATENCY:
                memory_latency = strtol(optarg, &endptr, 10);
                if (*endptr != '\0')
                { // || errno ka warum die dass wollen TODO
                    fprintf(stderr, "Invalid latency for memory.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                break;
            case TF:
                path_trace_file = optarg;
                // TODO check path
                break;
            }
        }
        // else: not a valid argument or positional argument
        else
        {
            // argument is at last position
            if (optind == argc - 1)
            {
                data_path = argv[optind];
                optind++;
            }
            else
            {
                print_usage(progname);
                return EXIT_FAILURE;
            }
        }
    }
    struct Request *requestst;

    requestst = readCSV(data_path, progname, &numRequests);

    if (requestst == NULL)
    {
        fprintf(stderr, "Error while reading csv file\n");
        return 1;
    }
    // Initialize Array of Requests with the specified size
    // struct Request requests[numRequests]; TODO
    struct Result result = run_simulation(cycles,
                                          l1_lines,
                                          l2_lines,
                                          cache_line_size,
                                          l1_latency,
                                          l2_latency,
                                          memory_latency,
                                          numRequests,
                                          requestst,
                                          path_trace_file);
    printf("Arguments: \n"
           "Cycles: %lu\n"
           "L1-Lines: %lu\n"
           "L2-Lines: %lu\n"
           "cache_line_size: %lu\n"
           "L1-Latency: %lu\n"
           "L2-Latency: %lu\n"
           "Memory-Latency: %lu\n"
           "Number of Requests: %lu\n"
           "Path for trace file: %s\n"
           "Path for data csv file: %s\n\n",
           cycles, l1_lines, l2_lines, cache_line_size, l1_latency, l2_latency,
           memory_latency, numRequests, path_trace_file, data_path);
    printf("Simulation result:\n"
           "Cycles: %zu\n"
           "Hits: %zu\n"
           "Misses: %zu\n"
           "PrimitiveGateCount: %zu\n",
           result.cycles, result.hits, result.misses, result.primitiveGateCount);

    // struct Result result = run_simulation(cycles, l1_lines, l2_lines, cache_line_size, l1_latency, l2_latency, memory_latency, numRequests, requests, path_trace_file);
    // printf("Simulation result: \nCycles: %i\nHits: %i\nMisses: %i\nPrimitiveGateCount: %i\n", result.cycles, result.hits, result.misses, result.primitiveGateCount);
    return 0;
}
