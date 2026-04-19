#pragma once
#include <stdint.h>
#include <string.h>

/* Move categories */
#define CAT_PHYSICAL 0
#define CAT_SPECIAL  1
#define CAT_STATUS   2

/* Effect IDs */
#define EFF_NONE      0
#define EFF_ATK_DOWN  1
#define EFF_DEF_DOWN  2
#define EFF_SPD_DOWN  3
#define EFF_BURN      4
#define EFF_PAR       5
#define EFF_HEAL      6
#define EFF_ATK_UP    7
#define EFF_DEF_UP    8
#define EFF_SPD_UP    9

typedef struct {
    char     name[16];
    char     type[12];
    uint8_t  power;
    uint8_t  acc;      /* 0-100 */
    uint8_t  pp;
    uint8_t  category; /* CAT_* */
    uint8_t  effect_id;
} Move;

/* 32 moves, index 0 = "None" placeholder */
static const Move MOVES[32] = {
    /* 0 */ {"None",        "normal",   0,   0,  0, CAT_STATUS,   EFF_NONE},
    /* 1 */ {"Tackle",      "normal",  40, 100, 35, CAT_PHYSICAL, EFF_NONE},
    /* 2 */ {"Shadow Bite", "dark",    50,  95, 25, CAT_PHYSICAL, EFF_NONE},
    /* 3 */ {"Dark Pulse",  "dark",    65,  90, 20, CAT_SPECIAL,  EFF_NONE},
    /* 4 */ {"Night Slash", "dark",    70,  95, 20, CAT_PHYSICAL, EFF_NONE},
    /* 5 */ {"Abyss Wave",  "dark",    90,  85, 10, CAT_SPECIAL,  EFF_NONE},
    /* 6 */ {"Vine Whip",   "grass",   45, 100, 25, CAT_PHYSICAL, EFF_NONE},
    /* 7 */ {"Leaf Blade",  "grass",   65,  95, 20, CAT_PHYSICAL, EFF_NONE},
    /* 8 */ {"Solar Beam",  "grass",   90,  85, 10, CAT_SPECIAL,  EFF_NONE},
    /* 9 */ {"Petal Dance", "grass",   80,  90, 15, CAT_SPECIAL,  EFF_NONE},
    /*10 */ {"Ember",       "fire",    40, 100, 25, CAT_SPECIAL,  EFF_BURN},
    /*11 */ {"Flame Charge","fire",    50,  95, 20, CAT_PHYSICAL, EFF_SPD_UP},
    /*12 */ {"Flamethrower","fire",    75,  90, 15, CAT_SPECIAL,  EFF_NONE},
    /*13 */ {"Fire Blast",  "fire",    95,  80, 10, CAT_SPECIAL,  EFF_BURN},
    /*14 */ {"Thunder Jolt","electric",45,  95, 25, CAT_SPECIAL,  EFF_NONE},
    /*15 */ {"Static Bolt", "electric",60,  90, 20, CAT_SPECIAL,  EFF_PAR},
    /*16 */ {"Volt Crash",  "electric",80,  85, 10, CAT_SPECIAL,  EFF_PAR},
    /*17 */ {"Plasma Storm","electric",90,  80, 10, CAT_SPECIAL,  EFF_NONE},
    /*18 */ {"Air Slash",   "air",     60,  90, 20, CAT_SPECIAL,  EFF_NONE},
    /*19 */ {"Gust",        "air",     40, 100, 35, CAT_SPECIAL,  EFF_NONE},
    /*20 */ {"Hurricane",   "air",     90,  80, 10, CAT_SPECIAL,  EFF_NONE},
    /*21 */ {"Tail Wind",   "air",      0, 100, 15, CAT_STATUS,   EFF_SPD_UP},
    /*22 */ {"Scratch",     "normal",  40, 100, 35, CAT_PHYSICAL, EFF_NONE},
    /*23 */ {"Growl",       "normal",   0, 100, 40, CAT_STATUS,   EFF_ATK_DOWN},
    /*24 */ {"Leer",        "normal",   0, 100, 30, CAT_STATUS,   EFF_DEF_DOWN},
    /*25 */ {"Slam",        "normal",  65,  90, 20, CAT_PHYSICAL, EFF_NONE},
    /*26 */ {"Hyper Slam",  "normal",  80,  85, 15, CAT_PHYSICAL, EFF_NONE},
    /*27 */ {"Recover",     "normal",   0, 100, 10, CAT_STATUS,   EFF_HEAL},
    /*28 */ {"Iron Defense","normal",   0, 100, 15, CAT_STATUS,   EFF_DEF_UP},
    /*29 */ {"Swords Dance","normal",   0, 100, 20, CAT_STATUS,   EFF_ATK_UP},
    /*30 */ {"Hex",         "dark",    55,  95, 15, CAT_SPECIAL,  EFF_NONE},
    /*31 */ {"Shadow Force","dark",    80,  90, 10, CAT_PHYSICAL, EFF_NONE},
};

static inline const Move *get_move(uint8_t id) {
    if (id >= 32) id = 0;
    return &MOVES[id];
}
