# Makefile for MERIC Extension Library Tests
# Author: Osman Yasal <osman.yasal@vsb.cz>

ifeq ($(shell echo ${CXX} | awk '{n=split($$1,0A,"/"); print A[n]}'),icpc)
	OPENMP=-qopenmp
else ifeq ($(shell echo ${CXX} | awk '{n=split($$1,A,"/"); print A[n]}'),FCC)
	OPENMP=-Kopenmp
else
	OPENMP=-fopenmp
endif


################################################################################
MERIC=-L../lib -I../include 
#HDEEM=-lhdeem -lfreeipmi
#X86ADAPT=-lx86_adapt
#CPUFREQ=--lcpufreq
#CPUFREQ=-L/lib64/ -lcpupower
#PAPI=-lpapi
#RESTCLIENT= -lrestclient-cpp
#NUMA=-lnuma
NVML=-lnvidia-ml -DHAVE_NVML_H
#ESMI=-le_smi64
#LIBMSR=-lmsr -L${LIBMSR_PATH}/lib -I${LIBMSR_PATH}/include
################################################################################

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -O0
INCLUDES = -I../../../include -I.. -I../basis -Igoogletest/googletest/include
LDFLAGS = -L../../../lib -Lgoogletest/build/lib
LIBS = -lgtest -lgtest_main ../../../lib/libmeric_ext.a -lpthread $(MERIC) $(OPENMP) $(HDEEM) $(X86ADAPT) $(CPUFREQ) $(PAPI) $(RESTCLIENT) $(NUMA) $(NVML) $(ESMI) $(LIBMSR) $(CXX_FLAGS)

# Directories
SRC_DIR = ..
TEST_DIR = .
GTEST_DIR = googletest
BUILD_DIR = build

# Source files
TEST_SOURCES = $(wildcard *_test.cpp)

# Test executables
TEST_TARGETS = $(patsubst %_test.cpp,$(BUILD_DIR)/%_test,$(TEST_SOURCES))

# Default target
all: setup $(TEST_TARGETS)

# Setup build directory
setup:
	@mkdir -p $(BUILD_DIR)

# Build GoogleTest if needed
gtest:
	@if [ ! -d "$(GTEST_DIR)" ]; then \
		echo "GoogleTest not found. Please clone it:"; \
		echo "git clone https://github.com/google/googletest.git"; \
		exit 1; \
	fi
	@if [ ! -f "$(GTEST_DIR)/build/lib/libgtest.a" ]; then \
		echo "Building GoogleTest..."; \
		mkdir -p $(GTEST_DIR)/build; \
		cd $(GTEST_DIR)/build && cmake .. && make; \
	fi

# Compile test source files
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Link test executables (using pre-built libmeric_ext.a)
$(BUILD_DIR)/%_test: $(BUILD_DIR)/%_test.o | gtest
	$(CXX) $(CXXFLAGS) $< $(LDFLAGS) $(LIBS) -o $@

# Individual test targets
reserved_memory_manager_test: $(BUILD_DIR)/reserved_memory_manager_test
	@echo "Running Reserved Memory Manager tests..."
	$(BUILD_DIR)/reserved_memory_manager_test

meric_ext_test: $(BUILD_DIR)/meric_ext_test
	@echo "Running MERIC Extension tests..."
	$(BUILD_DIR)/meric_ext_test

# Run all tests
test: $(TEST_TARGETS)
	@echo "Running all tests..."
	@for test in $(TEST_TARGETS); do \
		echo "Running $$test..."; \
		$$test || exit 1; \
	done
	@echo "All tests passed!"

# Run tests with valgrind for memory checking
test-valgrind: $(TEST_TARGETS)
	@echo "Running tests with Valgrind..."
	@for test in $(TEST_TARGETS); do \
		echo "Running $$test with Valgrind..."; \
		valgrind --leak-check=full --error-exitcode=1 $$test || exit 1; \
	done
	@echo "All tests passed with Valgrind!"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	@echo "Build directory cleaned."

# Clean everything including GoogleTest
clean-all: clean
	rm -rf $(GTEST_DIR)/build
	@echo "All build artifacts cleaned."

# Help target
help:
	@echo "Available targets:"
	@echo "  all                    - Build all tests"
	@echo "  test                   - Run all tests"
	@echo "  test-valgrind          - Run tests with memory checking"
	@echo "  reserved_memory_manager_test - Build and run reserved memory manager tests"
	@echo "  meric_ext_test         - Build and run MERIC extension tests"
	@echo "  gtest                  - Build GoogleTest framework"
	@echo "  clean                  - Clean build artifacts"
	@echo "  clean-all              - Clean all including GoogleTest"
	@echo "  help                   - Show this help message"

# Declare phony targets
.PHONY: all setup gtest test test-valgrind clean clean-all help reserved_memory_manager_test meric_ext_test

# Dependencies (regenerate with `make depend` if you add this target)
-include $(BUILD_DIR)/*.d