# CMake is the single build definition. `make` includes the desktop application;
# `make headless` builds the computational tools without Qt.
CMAKE ?= cmake
BUILD_DIR ?= build
BUILD_TYPE ?= Release
JOBS ?= 2

.PHONY: all configure gui headless test clean
all: configure
	$(CMAKE) --build $(BUILD_DIR) --config $(BUILD_TYPE) --parallel $(JOBS)

configure:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DMFG_BUILD_GUI=ON

gui: all

headless:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DMFG_BUILD_GUI=OFF
	$(CMAKE) --build $(BUILD_DIR) --config $(BUILD_TYPE) --parallel $(JOBS)

test: all
	ctest --test-dir $(BUILD_DIR) -C $(BUILD_TYPE) --output-on-failure

clean:
	$(CMAKE) --build $(BUILD_DIR) --target clean
