# compiler settings
CXX = g++
N_SIZE ?= 11	# default N_SIZE
CXXFLAGS = -std=c++20 -Wall -Wextra -O3 -march=native -flto -fopenmp -DN_SIZE=$(N_SIZE)

# executable
TARGET = main

# directories
OBJDIR = obj

# files
SRCS = $(wildcard *.cpp)

# pattern substitution
OBJS = $(patsubst %.cpp, $(OBJDIR)/%.o, $(SRCS))
DEPS = $(patsubst %.cpp, $(OBJDIR)/%.d, $(SRCS))

# default target
all: $(TARGET)

# linking
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# compiling individual source files
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# run the simulation
run: $(TARGET)
	./$(TARGET)

# clean up build artifacts
clean:
	rm -rf $(OBJDIR) $(TARGET)

# include the dependency files
-include $(DEPS)

.PHONY: all clean run