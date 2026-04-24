# GrumbleQuest — SDL simulator + headless tests (see CMakeLists.txt)
#
#   make build    — assemble sprites, configure (if needed), and compile all targets
#   make test     — build tests and run ctest
#   make dev      — assemble sprites, build simulator, and run it
#   make sprites  — (re)assemble sprite sheets from assets/sprites/ only
#   make clean    — remove the build directory
#
# Override build location: make BUILD_DIR=out/build test

BUILD_DIR ?= build
CMAKE     ?= cmake
PYTHON    ?= python3
NPROC     := $(shell sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 8)

# Sprite sources — any change here triggers sheet regeneration
SPRITE_SOURCES := $(wildcard assets/sprites/**/*.png) assets/sprites/manifest.json
SPRITE_SHEETS  := $(BUILD_DIR)/assets_iso_tiles.png $(BUILD_DIR)/assets_chars.png $(BUILD_DIR)/assets_items.png $(BUILD_DIR)/assets_ui.png

.DEFAULT_GOAL := build

.PHONY: build test dev sprites editor clean

# ---- Sprite sheet assembly ------------------------------------------------
$(SPRITE_SHEETS): $(SPRITE_SOURCES)
	@mkdir -p $(BUILD_DIR)
	$(PYTHON) tools/gen_spritesheets.py . --out-dir $(BUILD_DIR)

sprites: $(SPRITE_SHEETS)

# ---- CMake configuration --------------------------------------------------
$(BUILD_DIR)/CMakeCache.txt:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && $(CMAKE) ..

# ---- Main targets ---------------------------------------------------------
build: sprites $(BUILD_DIR)/CMakeCache.txt
	$(CMAKE) --build $(BUILD_DIR) -j$(NPROC)

test: sprites $(BUILD_DIR)/CMakeCache.txt
	$(CMAKE) --build $(BUILD_DIR) --target grumblequest_tests -j$(NPROC)
	cd $(BUILD_DIR) && ctest --output-on-failure

dev: sprites $(BUILD_DIR)/CMakeCache.txt
	$(CMAKE) --build $(BUILD_DIR) --target grumblequest_sim -j$(NPROC)
	cd $(BUILD_DIR) && ./grumblequest_sim

editor:
	@echo "Serving at http://localhost:8765/tools/map_editor.html  (Ctrl+C to stop)"
	@( sleep 1 && open http://localhost:8765/tools/map_editor.html ) &
	@$(PYTHON) -m http.server 8765

clean:
	rm -rf $(BUILD_DIR)
