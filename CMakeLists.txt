cmake_minimum_required(VERSION 3.13)

# GrumbleQuest — SDL2 simulator + headless test runner.
# Pico hardware build lives in pico/CMakeLists.txt.
project(grumblequest C)
set(CMAKE_C_STANDARD 11)

add_compile_options(-Wall -Wextra -Wno-unused-parameter)

# Homebrew on Apple Silicon lives under /opt/homebrew.
list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
find_package(SDL2 REQUIRED)

set(GAME_CORE_SOURCES
    src/game/state.c
    src/game/items.c
    src/game/actions.c
    src/game/recipes.c
    src/game/npcs.c
    src/game/structures.c
    src/game/world.c
    src/game/player.c
    src/game/skills.c
    src/game/tick.c
    src/game/save.c
    src/render/renderer.c
    src/render/render_debug.c
    src/render/iso_spritesheet.c
    src/render/font.c
    src/ui/hud.c
    src/ui/menu.c
    src/ui/dialog.c
    src/ui/shop.c
    src/ui/craft.c
)

add_executable(grumblequest_sim
    src/main.c
    ${GAME_CORE_SOURCES}
    src/hal_sdl.c
)
target_include_directories(grumblequest_sim PRIVATE
    src ${CMAKE_SOURCE_DIR}/third_party ${SDL2_INCLUDE_DIRS})
target_link_libraries(grumblequest_sim SDL2::SDL2)

add_executable(grumblequest_tests
    tests/hal_stub.c
    tests/test_runner.c
    ${GAME_CORE_SOURCES}
)
target_include_directories(grumblequest_tests PRIVATE
    src tests ${CMAKE_SOURCE_DIR}/third_party)

enable_testing()
add_test(NAME grumblequest_tests COMMAND grumblequest_tests)
