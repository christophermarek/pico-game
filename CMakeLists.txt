cmake_minimum_required(VERSION 3.13)

# -----------------------------------------------------------------------
# GrumbleQuest — simulator build (SDL2 + host compiler)
# For the Pico build see pico/CMakeLists.txt (requires pico-sdk)
# -----------------------------------------------------------------------
project(grumblequest C)
set(CMAKE_C_STANDARD 11)

# Warnings — catch bugs early
add_compile_options(-Wall -Wextra -Wno-unused-parameter)

# -----------------------------------------------------------------------
# SDL2
# -----------------------------------------------------------------------
# Homebrew on Apple Silicon puts things in /opt/homebrew
list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
find_package(SDL2 REQUIRED)

# -----------------------------------------------------------------------
# Core game sources (no main, no HAL — linked per target)
# -----------------------------------------------------------------------
set(GAME_CORE_SOURCES
    src/game/state.c
    src/game/world.c
    src/game/player.c
    src/game/skills.c
    src/game/pets.c
    src/game/tick.c
    src/game/save.c
    src/render/renderer.c
    src/render/sprites.c
    src/render/iso_spritesheet.c
    src/render/font.c
    src/ui/hud.c
    src/ui/menu.c
    src/ui/char_create.c
)

# -----------------------------------------------------------------------
# Simulator target
# -----------------------------------------------------------------------
add_executable(grumblequest_sim
    src/main.c
    ${GAME_CORE_SOURCES}
    src/hal_sdl.c
)

target_include_directories(grumblequest_sim PRIVATE src ${CMAKE_SOURCE_DIR}/third_party ${SDL2_INCLUDE_DIRS})
target_link_libraries(grumblequest_sim SDL2::SDL2)

# -----------------------------------------------------------------------
# Headless test runner (stub HAL — no SDL, suitable for CI)
# -----------------------------------------------------------------------
add_executable(grumblequest_tests
    tests/hal_stub.c
    tests/test_runner.c
    ${GAME_CORE_SOURCES}
)
target_include_directories(grumblequest_tests PRIVATE src tests ${CMAKE_SOURCE_DIR}/third_party)

enable_testing()
add_test(NAME grumblequest_tests COMMAND grumblequest_tests)

# -----------------------------------------------------------------------
# Quick build helper:
#   mkdir -p build && cd build && cmake .. && make -j$(nproc) && ./grumblequest_sim
#   ctest --output-on-failure   (or ./grumblequest_tests)
# -----------------------------------------------------------------------
