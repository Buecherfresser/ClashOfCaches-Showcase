#include <errno.h>
#include <stdio.h>
#include <getopt.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "csv_parsing.c"

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
    fprintf(stderr, "\n%s", help_msg);
}

int check_valid_tracefile(char *path_tf)
{
    if (path_tf == NULL)
    {
        fprintf(stderr, "Filepath is null \n");
        return 0;
    }

    if (path_tf[0] == '\0')
    {
        fprintf(stderr, "Filepath is empty \n");
        return 0;
    }

    char *stop = strrchr(path_tf, '/');

    if (stop == NULL)
    {
        return 1;
    }

    size_t len = stop - path_tf;

    char dir_path[len + 1];

    memcpy(dir_path, path_tf, len);
    dir_path[len] = '\0';

    struct stat info;

    // get path info
    if (stat(dir_path, &info) != 0)
    {
        // some err
        fprintf(stderr, "some err in stat() \n");
        return 0;
    }

    // check if ISDIR
    if (S_ISDIR(info.st_mode))
    {
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    unsigned long cycles = ULONG_MAX;
    unsigned long cache_line_size = 4;
    unsigned long l1_lines = 8;
    unsigned long l2_lines = 32;
    unsigned long l1_latency = 5;
    unsigned long l2_latency = 20;
    unsigned long memory_latency = 400;
    char *path_trace_file = NULL;
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
            int64_t tmp = 0;
            switch (ch)
            {
            case 'c':
                // cycles = strtol(optarg, &endptr, 10);
                errno = 0;
                cycles = strtol(optarg, &endptr, 10);
                if (errno == EINVAL)
                {
                    fprintf(stderr, "failed during read of Cycles:  no parsable number found.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                if (errno == ERANGE)
                {
                    fprintf(stderr, "failed during read of Cycles:  value was out of range.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                if (optarg[0] == '-')
                {
                    fprintf(stderr, "Invalid amount of Cycles, out of bounds ([1,ULONG_MAX]).\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                // cycles = (unsigned long) tmp;

                if (*endptr != '\0')
                {
                    // || errno ka warum die dass wollen TODO
                    fprintf(stderr, "Invalid amount of cycles.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                break;
            case 'h':
                print_help(progname);
                return EXIT_SUCCESS;
                ;
                break;
            case CACHELINESIZE:
                errno = 0;
                cache_line_size = strtol(optarg, &endptr, 10);

                if (errno == EINVAL)
                {
                    fprintf(stderr, "failed during read of Cache-line-size: no parsable number found.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                if (errno == ERANGE)
                {
                    fprintf(stderr, "failed during read of Cache-line-size: value was out of range.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                if (optarg[0] == '-' || cache_line_size < 4)
                {
                    fprintf(stderr, "Invalid Cache-line-size, out of bounds ([4,ULONG_MAX]).\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                if (*endptr != '\0')
                {
                    // || errno ka warum die dass wollen TODO
                    fprintf(stderr, "Invalid Cache-line-size.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                break;

            case L1LINES:
                errno = 0;
                l1_lines = strtol(optarg, &endptr, 10);

                if (optarg[0] == '-' || l1_lines < 2)
                {
                    fprintf(stderr, "Invalid amount of l1-lines, out of bounds ([2,ULONG_MAX]).\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                if (errno == EINVAL)
                {
                    fprintf(stderr, "failed during read of l1-lines: no parsable number found.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                if (errno == ERANGE)
                {
                    fprintf(stderr, "failed during read of l1-lines: value was out of range.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                if (*endptr != '\0')
                {
                    fprintf(stderr, "Invalid amount of l1-lines.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                break;

            case L2LINES:
                errno = 0;
                l2_lines = strtol(optarg, &endptr, 10);

                if (optarg[0] == '-' || l2_lines < 2)
                {
                    fprintf(stderr, "Invalid amount of l2-lines, out of bounds ([2,ULONG_MAX]).\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }

                if (errno == EINVAL)
                {
                    fprintf(stderr, "failed during read of l2-lines: no parsable number found.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                if (errno == ERANGE)
                {
                    fprintf(stderr, "failed during read of l2-lines: value was out of range.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                if (*endptr != '\0')
                {
                    fprintf(stderr, "Invalid amount of L2 cache lines.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                break;

            case L1LATENCY:
                l1_latency = strtol(optarg, &endptr, 10);
                if (optarg[0] == '-' || l1_latency < 1)
                {
                    fprintf(stderr, "Invalid l1-latency, out of bounds ([1,ULONG_MAX]).\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                if (errno == EINVAL)
                {
                    fprintf(stderr, "failed during read of l1-latency: no parsable number found.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                if (errno == ERANGE)
                {
                    fprintf(stderr, "failed during read of l1-latency: value was out of range.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }

                if (*endptr != '\0')
                {
                    fprintf(stderr, "Invalid latency for l1 cache.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                break;

            case L2LATENCY:
                l2_latency = strtol(optarg, &endptr, 10);
                if (optarg[0] == '-' || l2_latency < 1)
                {
                    fprintf(stderr, "Invalid l2-latency, out of bounds ([1,ULONG_MAX]).\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                if (errno == EINVAL)
                {
                    fprintf(stderr, "failed during read of l2-latency: no parsable number found.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                if (errno == ERANGE)
                {
                    fprintf(stderr, "failed during read of l2-latency: value was out of range.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }

                if (*endptr != '\0')
                {
                    fprintf(stderr, "Invalid latency for l2 cache.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                break;
            case MEMORYLATENCY:
                memory_latency = strtol(optarg, &endptr, 10);
                if (optarg[0] == '-' || memory_latency < 1)
                {
                    fprintf(stderr, "Invalid Memory-latency, out of bounds ([1,ULONG_MAX]).\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                if (errno == EINVAL)
                {
                    fprintf(stderr, "failed during read of Memory-latency: no parsable number found.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                if (errno == ERANGE)
                {
                    fprintf(stderr, "failed during read of Memory-latency: value was out of range.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }

                if (*endptr != '\0')
                {
                    // || errno ka warum die dass wollen TODO
                    fprintf(stderr, "Invalid latency for Memory.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }

                break;
            case TF:
                path_trace_file = optarg;
                if (check_valid_tracefile(path_trace_file) == 0)
                {
                    fprintf(stderr, "Invalid trace_file path.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                break;
            }
        }
        else
        {
            // invalid parameter detection
            if (l1_lines > l2_lines)
            {
                fprintf(stderr, "%s\n%s%u\n%s%u\n\n", "Invalid Cache sizes: l1-lines is not <= l2-lines.",
                        "l1-lines : ", l1_lines, "l2-lines : ", l2_lines);
                print_usage(progname);

                return EXIT_FAILURE;
            }

            // else: not a valid argument or positional argument

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

    // some cleanup
    free(requestst);

    return 0;
}
