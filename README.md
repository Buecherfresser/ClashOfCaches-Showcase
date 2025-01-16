# ClashOfCaches

This project was developed for the GRA (Grundlagen der Rechnerarchitektur) course in SoSe 2024. It implements a cache simulation system using SystemC and C++, with a C-based framework program. The simulation allows investigation of cache performance by configuring various parameters such as cache size, line count, and latency. It supports a two-level cache hierarchy (L1 and L2) with direct mapping and inclusive caching strategy, using LRU (Least Recently Used) as replacement policy.

## Prerequisites

- C++ compiler (g++ or clang++)
- SystemC library

## Setup

1. Place the SystemC library in a folder named `systemc` one level above the project directory
2. The SystemC path should be: `../systemc`

## Building and Running

```bash
# Build the debug version
make debug

# Build the release version
make release

# Run the program
./createCache

# Clean build files
make clean
