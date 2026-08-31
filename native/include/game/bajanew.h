#ifndef GAME_BAJANEW_H
#define GAME_BAJANEW_H

#include "baja/sim.h"
#include "game/bajanew_assets.h"
#include "ng/input.h"
#include "ng/renderer.h"

#define BAJANEW_FIX_COLUMNS 40
#define BAJANEW_FIX_ROWS 32

/* The renderer never writes gameplay state.  Everything below is presentation
 * derived from the immutable simulation snapshot. */
typedef struct BajanewGame {
    BajaSim sim;
    NgRenderer renderer;
    NgRenderStats last_render;
    uint32_t frame;
    int16_t sky_pan;
    uint8_t fix_shadow[BAJANEW_FIX_ROWS][BAJANEW_FIX_COLUMNS];
    uint8_t fix_next[BAJANEW_FIX_ROWS][BAJANEW_FIX_COLUMNS];
    uint32_t fix_dirty;
    uint32_t fix_written;
    uint16_t best_frames;
    uint8_t initialized;
    uint8_t reserved[3];
} BajanewGame;

void bajanew_game_init(BajanewGame *game);
void bajanew_game_tick(BajanewGame *game, NgPad pad);
void bajanew_game_set_autoplay(BajanewGame *game, uint8_t enabled);

#endif
