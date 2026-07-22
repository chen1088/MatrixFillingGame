CXX ?= g++
CXXFLAGS ?= -O3 -std=c++20 -Wall -Wextra -Wpedantic -Wconversion
CPPFLAGS ?= -Iinclude
BUILD_DIR ?= build

CORE_SOURCES := src/model.cpp src/cnf.cpp src/search.cpp
HEADERS := include/mfg/model.hpp include/mfg/cnf.hpp include/mfg/search.hpp

.PHONY: all test clean

all: $(BUILD_DIR)/mfg_search

$(BUILD_DIR)/mfg_search: $(CORE_SOURCES) src/main.cpp $(HEADERS)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CORE_SOURCES) src/main.cpp -o $@

$(BUILD_DIR)/mfg_tests: $(CORE_SOURCES) tests/test_main.cpp $(HEADERS)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CORE_SOURCES) tests/test_main.cpp -o $@

test: $(BUILD_DIR)/mfg_tests
	./$(BUILD_DIR)/mfg_tests

clean:
	rm -rf $(BUILD_DIR)
