#pragma once
#include "../game/state.h"
#include "../game/world.h"
#include "../game/td_cam.h"

/* Overworld debug overlays — hitboxes, facing arrow, collision panel. */
void render_debug_overworld(const GameState *s, const World *w,
                            const TdCamBasis *cam);

/* FPS counter at top-right. */
void render_debug_fps(void);
