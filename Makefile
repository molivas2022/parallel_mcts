# Compiler settings
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O3 -march=native -flto -fopenmp

# Executable name
TARGET = main

# Directories
OBJDIR = obj

# Files
SRCS = $(wildcard *.cpp)

# Pattern substitution: replaces .cpp with obj/.o and obj/.d
OBJS = $(patsubst %.cpp, $(OBJDIR)/%.o, $(SRCS))
DEPS = $(patsubst %.cpp, $(OBJDIR)/%.d, $(SRCS))

# Default target
all: $(TARGET)

# Linking the final executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compiling individual source files
# @mkdir -p $(@D) ensures the target directory exists before compiling
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# Run the simulation
run: $(TARGET)
	./$(TARGET)

# Clean up build artifacts
clean:
	rm -rf $(OBJDIR) $(TARGET)

# Include the auto-generated dependency files
-include $(DEPS)

# Mark targets that don't represent physical files
.PHONY: all clean run