# GrumbleQuest — SDL simulator + headless tests (see CMakeLists.txt)
#
#   make build      — assemble sprites, configure (if needed), and compile all targets
#   make test       — build tests and run ctest
#   make dev        — assemble sprites, build simulator, and run it
#   make sprites    — (re)assemble sprite sheets from assets/sprites/ only
#   make web        — build WebAssembly target into web/build/ (needs Emscripten)
#   make web-serve  — build web target then serve at http://localhost:8000
#   make clean      — remove the build directory
#
# Override build location: make BUILD_DIR=out/build test

BUILD_DIR ?= build
WEB_BUILD_DIR ?= web/build
CMAKE     ?= cmake
PYTHON    ?= python3
NPROC     := $(shell sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 8)
WEB_PORT  ?= 8000

# Sprite sources — any change here triggers sheet regeneration
SPRITE_SOURCES := $(wildcard assets/sprites/**/*.png) assets/sprites/manifest.json
SPRITE_SHEETS  := $(BUILD_DIR)/assets_iso_tiles.png $(BUILD_DIR)/assets_chars.png $(BUILD_DIR)/assets_items.png

.DEFAULT_GOAL := build

.PHONY: build test dev sprites editor clean web web-serve web-clean

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

# ---- Web / WebAssembly build ---------------------------------------------
# Kept isolated: separate build tree (web/build/), separate CMakeLists
# (web/CMakeLists.txt), separate HAL implementation (src/hal_web.c).
# Nothing here touches the desktop or Pico builds.

define EMCC_MISSING_MSG
error: emcc (Emscripten) not found on PATH.

Install the Emscripten SDK, then activate it in this shell:

  git clone https://github.com/emscripten-core/emsdk.git ~/emsdk
  cd ~/emsdk && ./emsdk install latest && ./emsdk activate latest
  source ~/emsdk/emsdk_env.sh

Verify with:  emcc --version

Then rerun:   make web
endef
export EMCC_MISSING_MSG

web: sprites
	@command -v emcc >/dev/null 2>&1 || { echo "$$EMCC_MISSING_MSG" >&2; exit 1; }
	@mkdir -p $(WEB_BUILD_DIR)
	cd $(WEB_BUILD_DIR) && emcmake cmake ..
	cd $(WEB_BUILD_DIR) && emmake $(MAKE) -j$(NPROC)
	@echo ""
	@echo "Built: $(WEB_BUILD_DIR)/grumblequest.html"
	@echo "Run:   make web-serve   (then open http://localhost:$(WEB_PORT)/grumblequest.html)"

web-serve: web
	@echo "Serving $(WEB_BUILD_DIR) at http://localhost:$(WEB_PORT)/grumblequest.html"
	@echo "On your phone, use http://<this-machine-LAN-IP>:$(WEB_PORT)/grumblequest.html"
	@echo "Ctrl+C to stop."
	@cd $(WEB_BUILD_DIR) && $(PYTHON) -m http.server $(WEB_PORT)

web-clean:
	rm -rf $(WEB_BUILD_DIR)
