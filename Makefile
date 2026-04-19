# GrumbleQuest — SDL simulator + headless tests (see CMakeLists.txt)
#
#   make build   — configure (if needed) and compile all targets
#   make test    — build tests and run ctest
#   make dev     — build simulator and run it (console stays attached)
#   make clean   — remove the build directory
#
# Override build location: make BUILD_DIR=out/build test

BUILD_DIR ?= build
CMAKE     ?= cmake
NPROC     := $(shell sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 8)

.DEFAULT_GOAL := build

.PHONY: build test dev clean

$(BUILD_DIR)/CMakeCache.txt:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && $(CMAKE) ..

build: $(BUILD_DIR)/CMakeCache.txt
	$(CMAKE) --build $(BUILD_DIR) -j$(NPROC)

test: $(BUILD_DIR)/CMakeCache.txt
	$(CMAKE) --build $(BUILD_DIR) --target grumblequest_tests -j$(NPROC)
	cd $(BUILD_DIR) && ctest --output-on-failure

dev: $(BUILD_DIR)/CMakeCache.txt
	$(CMAKE) --build $(BUILD_DIR) --target grumblequest_sim -j$(NPROC)
	cd $(BUILD_DIR) && ./grumblequest_sim

clean:
	rm -rf $(BUILD_DIR)
