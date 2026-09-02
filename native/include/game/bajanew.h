#ifndef GAME_BAJANEW_H
#define GAME_BAJANEW_H

#include "baja/sim.h"
#include "game/bajanew_assets.h"
#include "ng/input.h"
#include "ng/pool.h"
#include "ng/renderer.h"
#include "ng/strip.h"

#define BAJANEW_FIX_COLUMNS 40
#define BAJANEW_FIX_ROWS 32

/* The scene costs more than one 60 Hz field to draw, so the cartridge holds a
 * steady thirty frames a second and advances the simulation twice per drawn
 * frame.  Gameplay therefore still runs at the sixty steps a second everything
 * is tuned for, instead of quietly running at half pace whenever the renderer
 * misses a field. */
#define BAJANEW_FIELDS_PER_FRAME 2
#define BAJANEW_SIM_STEPS_PER_FRAME 2

#define BAJANEW_POOL_SLOTS (NG_HW_SPRITE_CAPACITY - BAJANEW_STRIP_SLOTS)
/* World objects a frame can hold before the nearest ones win. */
#define BAJANEW_DRAW_ITEMS 24

/* One object waiting to be placed, nearest first. */
typedef struct BajanewDrawItem {
    const NgSpriteFrame *frame;
    int16_t x;
    int16_t y;
    uint16_t depth;
    uint8_t zoom_x;
    uint8_t zoom_y;
    uint8_t flags;
    uint8_t palette;
} BajanewDrawItem;

/* The renderer never writes gameplay state.  Everything below is presentation
 * derived from the immutable simulation snapshot. */
typedef struct BajanewGame {
    BajaSim sim;
    /* Kept for its scanline accounting; every sprite is placed directly. */
    NgRenderer renderer;
    NgRenderStats last_render;
    NgSpritePool pool;
    NgSpriteCache pool_cache[BAJANEW_POOL_SLOTS];
    BajanewDrawItem items[BAJANEW_DRAW_ITEMS];
    uint16_t item_count;
    /* The backdrop and the road own fixed hardware sprites beneath everything
     * the renderer places, so a frame costs them only the words that moved. */
    NgStripLayer sky;
    NgStripLayer ground;
    NgStripLayer road[BAJA_ROAD_BANDS];
    uint16_t strip_table[BAJANEW_STRIP_WORDS];
    uint8_t road_phase[BAJA_ROAD_BANDS];
    uint16_t strip_words;
    uint16_t road_slots;
    uint16_t scenery_cursor;
    /* A contact or crash call-out stays up long enough to be read. */
    uint8_t message_kind;
    uint8_t message_timer;
    /* Sound: what the driver was last told, and effects waiting to be sent. */
    uint8_t audio_engine_step;
    uint8_t audio_music;
    uint8_t audio_pending;
    uint8_t audio_phase;
    uint16_t audio_air_timer;
    uint32_t frame;
    int16_t sky_pan;
    int16_t ground_pan;
    /* Aligned so a row can be cleared and compared four cells at a time; the
     * HUD leaves most of every row untouched. */
    /* Cells are 12-bit tile indices: the big numerals live past 255. */
    uint16_t fix_shadow[BAJANEW_FIX_ROWS][BAJANEW_FIX_COLUMNS] __attribute__((aligned(4)));
    uint16_t fix_next[BAJANEW_FIX_ROWS][BAJANEW_FIX_COLUMNS] __attribute__((aligned(4)));
    BajaFp progress_scale;
    uint32_t fix_dirty;
    uint32_t fix_written;
    uint16_t best_frames;
    uint8_t initialized;
    uint8_t reserved[3];
} BajanewGame;

void bajanew_game_init(BajanewGame *game);
void bajanew_game_tick(BajanewGame *game, NgPad pad);
/* The next byte for the sound driver, or zero.  Called once per frame after
 * the tick; effects first, then music changes, then engine pitch. */
uint8_t bajanew_game_audio(BajanewGame *game);
void bajanew_game_set_autoplay(BajanewGame *game, uint8_t enabled);

#endif
