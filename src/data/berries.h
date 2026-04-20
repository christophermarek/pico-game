#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    char    id[12];
    char    name[16];
    int8_t  hp_restore;
} Berry;

#define BERRY_COUNT 8

static const Berry BERRY_TABLE[BERRY_COUNT] = {
    /* id          name            hp  */
    {"oran",   "Oran Berry",    10},
    {"sitrus", "Sitrus Berry",  25},
    {"pecha",  "Pecha Berry",    0},
    {"rawst",  "Rawst Berry",    5},
    {"leppa",  "Leppa Berry",    0},
    {"wiki",   "Wiki Berry",    15},
    {"mago",   "Mago Berry",    15},
    {"lum",    "Lum Berry",     10},
};

static inline const Berry *get_berry(const char *berry_id) {
    for (int i = 0; i < BERRY_COUNT; i++) {
        if (strcmp(BERRY_TABLE[i].id, berry_id) == 0)
            return &BERRY_TABLE[i];
    }
    return NULL;
}

static inline const Berry *get_berry_by_idx(int idx) {
    if (idx < 0 || idx >= BERRY_COUNT) return NULL;
    return &BERRY_TABLE[idx];
}

