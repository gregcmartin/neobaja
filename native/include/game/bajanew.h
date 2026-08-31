#ifndef GAME_BAJANEW_H
#define GAME_BAJANEW_H

#include "baja/sim.h"
#include "game/bajanew_assets.h"
#include "ng/input.h"
#include "ng/renderer.h"

#define BAJANEW_FIX_COLUMNS 40
#define BAJANEW_FIX_ROWS 32

/* The scene costs more than one 60 Hz field to draw, so the cartridge holds a
 * steady thirty frames a second and advances the simulation twice per drawn
 * frame.  Gameplay therefore still runs at the sixty steps a second everything
 * is tuned for, instead of quietly running at half pace whenever the renderer
 * misses a field. */
#define BAJANEW_FIELDS_PER_FRAME 2
#define BAJANEW_SIM_STEPS_PER_FRAME 2

/* The renderer never writes gameplay state.  Everything below is presentation
 * derived from the immutable simulation snapshot. */
typedef struct BajanewGame {
    BajaSim sim;
    NgRenderer renderer;
    NgRenderStats last_render;
    uint32_t frame;
    int16_t sky_pan;
    /* Aligned so a row can be cleared and compared four cells at a time; the
     * HUD leaves most of every row untouched. */
    uint8_t fix_shadow[BAJANEW_FIX_ROWS][BAJANEW_FIX_COLUMNS] __attribute__((aligned(4)));
    uint8_t fix_next[BAJANEW_FIX_ROWS][BAJANEW_FIX_COLUMNS] __attribute__((aligned(4)));
    BajaFp progress_scale;
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
