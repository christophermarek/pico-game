#include "game_loop.h"
#include "hal.h"
#ifndef PICO_BUILD
#include <stdio.h>
#include <stdlib.h>
#endif

int main(void) {
    if (!game_boot()) {
#ifndef PICO_BUILD
        fprintf(stderr, "fatal: map file not found (assets/maps/map.bin)\n");
        hal_deinit();
        return 1;
#else
        while (1) {}  /* halt — no map on device */
#endif
    }

    while (1) game_step();

    hal_deinit();
    return 0;
}
