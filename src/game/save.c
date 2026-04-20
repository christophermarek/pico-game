#include "save.h"
#include "hal.h"
#include <string.h>

#define SAVE_MAGIC   0x4735513Eu  /* 'G5Q1' as a uint32 */
#define SAVE_VERSION 7u           /* bump whenever GameState layout changes */

typedef struct {
    uint32_t  magic;
    uint8_t   version;
    uint8_t   _pad[3];
    GameState state;
} SaveFile;

bool save_write(const GameState *s) {
    SaveFile sf;
    sf.magic   = SAVE_MAGIC;
    sf.version = SAVE_VERSION;
    memset(sf._pad, 0, sizeof(sf._pad));
    memcpy(&sf.state, s, sizeof(GameState));
    return hal_save(&sf, sizeof(sf));
}

bool save_read(GameState *s) {
    SaveFile sf;
    if (!hal_load(&sf, sizeof(sf)))            return false;
    if (sf.magic   != SAVE_MAGIC)              return false;
    if (sf.version != SAVE_VERSION)            return false;
    memcpy(s, &sf.state, sizeof(GameState));
    return true;
}
