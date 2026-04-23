#include "save.h"
#include "hal.h"
#include <string.h>

#define SAVE_MAGIC   0x4735513Eu
#define SAVE_VERSION 8u   /* bump when SaveData layout changes */

/*
 * Persistence-only slice of GameState. Transient state (debug telemetry,
 * message log, menu cursor, skilling progress, frame counter) deliberately
 * stays out — they have no meaning across sessions and bloating the save
 * with them makes version bumps more frequent.
 */
typedef struct {
    PlayerTD td;
    uint8_t  td_cam_bearing;

    int16_t  hp, max_hp;
    uint8_t  energy;

    Skill    skills[SKILL_COUNT];
    InvSlot  inv[INV_SLOTS];

    uint32_t tick_count;
    uint16_t day;
    uint32_t total_steps;

    bool     debug_mode;
} SaveData;

typedef struct {
    uint32_t magic;
    uint8_t  version;
    uint8_t  _pad[3];
    SaveData data;
} SaveFile;

bool save_write(const GameState *s) {
    SaveFile sf;
    sf.magic   = SAVE_MAGIC;
    sf.version = SAVE_VERSION;
    memset(sf._pad, 0, sizeof(sf._pad));

    sf.data.td              = s->td;
    sf.data.td.walk_frame   = 0.0f;  /* animation phase is transient */
    sf.data.td_cam_bearing  = s->td_cam_bearing;
    sf.data.hp              = s->hp;
    sf.data.max_hp          = s->max_hp;
    sf.data.energy          = s->energy;
    memcpy(sf.data.skills, s->skills, sizeof(sf.data.skills));
    memcpy(sf.data.inv,    s->inv,    sizeof(sf.data.inv));
    sf.data.tick_count      = s->tick_count;
    sf.data.day             = s->day;
    sf.data.total_steps     = s->total_steps;
    sf.data.debug_mode      = s->debug_mode;

    return hal_save(&sf, sizeof(sf));
}

bool save_read(GameState *s) {
    SaveFile sf;
    if (!hal_load(&sf, sizeof(sf)))  return false;
    if (sf.magic   != SAVE_MAGIC)    return false;
    if (sf.version != SAVE_VERSION)  return false;

    s->td             = sf.data.td;
    s->td_cam_bearing = sf.data.td_cam_bearing;
    s->hp             = sf.data.hp;
    s->max_hp         = sf.data.max_hp;
    s->energy         = sf.data.energy;
    memcpy(s->skills, sf.data.skills, sizeof(s->skills));
    memcpy(s->inv,    sf.data.inv,    sizeof(s->inv));
    s->tick_count     = sf.data.tick_count;
    s->day            = sf.data.day;
    s->total_steps    = sf.data.total_steps;
    s->debug_mode     = sf.data.debug_mode;
    return true;
}
