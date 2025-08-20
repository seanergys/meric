# MERIC Extension Library Tests

This directory contains unit tests for the MERIC Extension Library, focusing on the reserved memory manager and main API functionality.

## Prerequisites

1. **GoogleTest Framework**: Clone GoogleTest into this directory:
   ```bash
   git clone https://github.com/google/googletest.git
   ```

2. **Build Dependencies**: Ensure the main MERIC library is built:
   ```bash
   cd ../../..
   make  # or ./waf build
   ```

## Building and Running Tests

### Quick Start
```bash
# Build and run all tests
make test

# Build and run specific tests
make reserved_memory_manager_test
make meric_ext_test
```

### Memory Checking
```bash
# Run tests with Valgrind for memory leak detection
make test-valgrind
```

### Available Targets
- `make all` - Build all test executables
- `make test` - Build and run all tests
- `make test-valgrind` - Run tests with memory checking
- `make clean` - Clean build artifacts
- `make clean-all` - Clean everything including GoogleTest
- `make help` - Show all available targets

## Test Structure

### ReservedMemoryManager Tests (`reserved_memory_manager_test.cpp`)
Tests the core memory management functionality:
- Page creation and management
- Slot allocation and reuse
- Memory pool operations
- Page removal logic
- Error handling

### MERIC Extension API Tests (`meric_ext_test.cpp`)
Tests the main library API:
- Initialization with different domain configurations
- Energy measurement reading
- Reserved vs dynamic memory modes
- Utility functions and macros
- Error conditions

## Test Configuration

Tests use a simple template configuration with:
- Domain 0 (A64FX) enabled with 5 counters
- Small memory pools for faster testing
- Proper setup/teardown for isolation

## Notes

- Tests are designed to work even if specific hardware domains are not available
- Memory allocation tests focus on the reserved memory manager logic
- Error conditions are tested to ensure graceful failure handling
- All tests use GoogleTest framework for assertions and test structure
