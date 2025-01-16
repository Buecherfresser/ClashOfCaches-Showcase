// #ifndef EXPANDED_METHOD_H
#define EXPANDED_METHOD_H

/*print help msg*/
void print_help(const char *progname);

/*print usage msg*/
void print_usage(const char *progname);

const char *usage_msg =

    "Usage: ./createCache  [options] <request_file_name> \n \n"

    "Options:\n"
    "   -c <number>, --cycles <number>          Specify the amount of cycles the program should simulate.\n"
    "   -h                                      Show help message and exit\n"
    "   --cacheline-size <number>               Set the size of a cacheline (in byte)\n"
    "   --l1-lines <number>                     Set the lines of the l1-Cache to <number> lines (has to be <= l2-lines)\n"
    "   --l2-lines <number>                     Set the lines of the l2-Cache to <number> lines (has to be >= l1-lines)\n"
    "   --l1-latency <number>                   Set the latency ot the l1-Cache to <number> cycles\n"
    "   --l2-latency <number>                   Set the latency ot the l2-Cache to <number> cycles\n"
    "   --memory-latency <number>               Set the latency ot the memory to <number> cycles\n"
    "   --tf=<filename>                         Creates a Tracefile, that traces each signal of the program\n";
;

const char *help_msg =

    "Usage: ./createCache  [options] <request_file_name> \n \n"
    "Positional arguments:\n"
    "   request_file_name                       filename of the input csv-file, that contains the requests .\n"
    "\n"
    "Optional arguments:\n"
    "   -c <number>, --cycles <number>          Specify the amount of cycles the program should simulate.\n"
    "   -h                                      Show help message and exit\n"
    "   --cacheline-size <number>               Set the size of a cacheline (in byte)\n"
    "   --l1-lines <number>                     Set the lines of the l1-Cache to <number> lines (has to be <= l2-lines)\n"
    "   --l2-lines <number>                     Set the lines of the l2-Cache to <number> lines (has to be >= l1-lines)\n"
    "   --l1-latency <number>                   Set the latency ot the l1-Cache to <number> cycles\n"
    "   --l2-latency <number>                   Set the latency ot the l2-Cache to <number> cycles\n"
    "   --memory-latency <number>               Set the latency ot the memory to <number> cycles\n"
    "   --tf=<filename>                         Creates a Tracefile, that traces each signal of the program\n"
    "\n"
;

// #endif
