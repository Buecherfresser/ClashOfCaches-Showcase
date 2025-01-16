# ---------------------------------------
# CONFIGURATION BEGIN
# ---------------------------------------

# entry point for the program and target name
C_SRCS = src/main.c
CPP_SRCS = src/modules.cpp

# Object files
C_OBJS = $(C_SRCS:.c=.o)
CPP_OBJS = $(CPP_SRCS:.cpp=.o)

# assignment task file
HEADERS :=

# target name
TARGET := createCache

SCPATH = ../systemc

# Additional flags for the compiler
CXXFLAGS := -std=c++14  -I$(SCPATH)/include -L$(SCPATH)/lib -lsystemc -lm


# ---------------------------------------
# CONFIGURATION END
# ---------------------------------------

# Determine if clang or gcc is available
CXX := $(shell command -v g++ || command -v clang++)
ifeq ($(strip $(CXX)),)
    $(error Neither clang++ nor g++ is available. Exiting.)
endif

CC := $(shell command -v gcc || command -v clang)
ifeq ($(strip $(CC)),)
    $(error Neither clang nor g is available. Exiting.)
endif

# Add rpath except for MacOS
UNAME_S := $(shell uname -s)

ifneq ($(UNAME_S), Darwin)
	CXXFLAGS += -Wl,-rpath=$(SCPATH)/lib
else
	CXXFLAGS += -Wl,-rpath $(SCPATH)/lib
endif



# Default to release build for both app and library
all: debug

# Rule to compile .c files to .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to compile .cpp files to .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Debug build
debug: CXXFLAGS += -g -Wall
debug: $(TARGET)

# Release build
release: CXXFLAGS += -O2
release: $(TARGET)

#sanitizer: CXXFLAGS += -g -Wall -fsanitize=address
#sanitizer: $(TARGET)

# Rule to link object files to executable
$(TARGET): $(C_OBJS) $(CPP_OBJS)
	$(CXX) $(CXXFLAGS) $(C_OBJS) $(CPP_OBJS) $(LDFLAGS) -o $(TARGET)

# clean up
clean:
	rm -f $(TARGET)
	rm -rf src/*.o

rebuild:
	$(MAKE) clean
	$(MAKE) all
.PHONY: all debug release clean rebuild