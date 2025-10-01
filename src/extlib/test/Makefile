# Makefile for MERIC Extension Library Tests
# Author: Osman Yasal <osman.yasal@vsb.cz>

# Library linker flags
meric_ext=-lmeric_ext
gtest=-lgtest -lgtest_main -lpthread
#hdeem=-lhdeem -lfreeipmi
#x86_adapt=-lx86_adapt
#nvml=-lnvidia-ml
#libmsr=-lmsr

# OpenMP flag
openmp_flag=-fopenmp

# Compiler settings
CXX=g++
CXXFLAGS=-std=c++17 -O0 -Wall $(openmp_flag)
INCLUDE=-I../../../include
LDFLAGS=-L../../../lib
LDLIBS=$(meric_ext) $(gtest) $(hdeem) $(x86_adapt) $(nvml) $(libmsr)

# Build directory
build_dir=build

# Test source files
test_sources=$(wildcard *_test.cpp)

# Test executables
test_targets=$(patsubst %_test.cpp,$(build_dir)/%_test,$(test_sources))

# Build all tests (default target)
all: $(test_targets)
.PHONY: all

$(build_dir)/%_test: %_test.cpp
	@mkdir -p $(build_dir)
	$(CXX) $(CXXFLAGS) $< $(INCLUDE) $(LDFLAGS) $(LDLIBS) -o $@

# Run ReservedMemoryManager tests
reserved_memory_manager_test: $(build_dir)/reserved_memory_manager_test
	@echo "Running ReservedMemoryManager tests..."
	$(build_dir)/reserved_memory_manager_test

# Run MERIC Extension API tests
meric_ext_test: $(build_dir)/meric_ext_test
	@echo "Running MERIC Extension API tests..."
	$(build_dir)/meric_ext_test

# Run all tests
test: $(test_targets)
	@echo "Running all tests..."
	@for test in $(test_targets); do \
		echo "Running $$test..."; \
		$$test || exit 1; \
	done
	@echo "All tests passed!"
.PHONY: test

# Run all tests with Valgrind for memory leak detection
test-valgrind: $(test_targets)
	@echo "Running all tests with Valgrind..."
	@for test in $(test_targets); do \
		echo "Running $$test with Valgrind..."; \
		valgrind --leak-check=full --error-exitcode=1 $$test || exit 1; \
	done
	@echo "All tests passed with Valgrind!"
.PHONY: test-valgrind

# Clean target
clean:
	rm -rf $(build_dir)
	@echo "Build directory cleaned"
.PHONY: clean

# Help target
help:
	@echo "Available targets:"
	@echo "  all                          - Build all tests (default target)"
	@echo "  reserved_memory_manager_test - Build and run ReservedMemoryManager tests"
	@echo "  meric_ext_test               - Build and run MERIC Extension API tests"
	@echo "  test                         - Build and run all tests"
	@echo "  test-valgrind                - Build and run all tests with Valgrind for memory leak detection"
	@echo "  clean                        - Clean build directory"
	@echo "  help                         - Show this help message"
.PHONY: help
