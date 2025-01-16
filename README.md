# ClashOfCaches

This project is part of the Hardware Design (GRA) course at Technical University of Munich (TUM) in Summer Semester 2024. It implements a cache simulation system using SystemC with C++ and a framework program in C to investigate cache performance.

The simulation allows configuration of various cache parameters including cache line size, number of cache lines, and latencies for L1 cache, L2 cache, and main memory. The caches are implemented as direct-mapped and inclusive, using LRU (Least Recently Used) as the replacement strategy.

The framework program processes memory access traces from CSV files and simulates the cache behavior, providing detailed statistics about cache hits, misses, and cycle counts.

This project requires SystemC to build and run.

## Prerequisites

- C++ compiler (g++ or clang++)
- C compiler (gcc or clang)
- SystemC library
- Make

## SystemC Setup

1. Place your SystemC installation in a directory parallel to this project:
```
parent_directory/
├── systemc/
└── ClashOfCaches/
```

The default path for SystemC is set to `../../systemc` in the Makefile. If your SystemC is installed elsewhere, update the `SCPATH` variable in `src/Makefile`.

## Building the Project

Navigate to the `src` directory:
```bash
cd src
```

### Build Options

- Debug build (default):
```bash
make debug
```

- Release build:
```bash
make release
```

- Clean build files:
```bash
make clean
```

## Project Structure

The project consists of:
- C source files: `main.c`, `csv_parsing.c`
- C++ source files: `modules.cpp`
- Header files: `structs.hpp`

## Platform-Specific Notes

The build system automatically detects your platform and adjusts the linking flags accordingly. Special handling is in place for macOS (Darwin) systems. 