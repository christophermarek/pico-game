#pragma once
#include "state.h"
#include "world.h"
#include "hal.h"

void player_update_td(GameState *s, const Input *inp, World *w);
void player_do_action(GameState *s, World *w);
void player_stop_action(GameState *s);

/* Call after td_cam_bearing changes — keeps td.dir (world facing) stable
 * and pivots the sprite (screen_dir) so the character visually turns with
 * the view. */
void player_camera_rotated(GameState *s);

bool player_collide_who(const World *w, float x, float y,
                        int *out_tx, int *out_ty, char *out_kind);

/* Probe the tile that player_do_action would act on right now, without
 * starting an action. Returns true for a live actionable node, false
 * otherwise (including depleted). Used by debug overlay. */
bool player_peek_action_target(const GameState *s, const World *w,
                               int *out_tx, int *out_ty);
