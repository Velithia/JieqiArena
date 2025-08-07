# Makefile for the Jieqi League Tool

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++23 -Wall -Wextra -O2 -I.
LDFLAGS = -static

# Name of the final executable
TARGET = jieqi_arena

# Automatically find all C++ source files
SOURCES = main.cpp types.cpp logger.cpp piece_pool.cpp engine_process.cpp engine.cpp time_manager.cpp game.cpp protocol.cpp move_validator.cpp
# Generate object file names from source file names
OBJECTS = $(SOURCES:.cpp=.o)

# Default target: build the executable
all: $(TARGET)

# Rule to link the executable
# We now use LDFLAGS for the linker-specific flags.
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Rule to compile a .cpp file into a .o file
# CXXFLAGS are for the compiler.
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Target to clean up build files
clean:
	rm -f $(OBJECTS) $(TARGET)

# Phony targets
.PHONY: all clean