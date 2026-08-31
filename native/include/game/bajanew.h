#ifndef GAME_BAJANEW_H
#define GAME_BAJANEW_H

#include "baja/sim.h"
#include "ng/input.h"
#include "ng/renderer.h"
#include "ng/scene.h"

#define BAJANEW_FIX_COLUMNS 40
#define BAJANEW_FIX_ROWS 32

typedef struct BajanewGame {
    BajaSim sim;
    NgRenderer renderer;
    NgRenderStats last_render;
    NgSceneStats last_scene;
    uint32_t frame;
    uint8_t fix_shadow[BAJANEW_FIX_ROWS][BAJANEW_FIX_COLUMNS];
    uint8_t fix_next[BAJANEW_FIX_ROWS][BAJANEW_FIX_COLUMNS];
    uint8_t initialized;
    uint8_t reserved[3];
} BajanewGame;

void bajanew_game_init(BajanewGame *game);
void bajanew_game_tick(BajanewGame *game, NgPad pad);
void bajanew_game_set_autoplay(BajanewGame *game, uint8_t enabled);

#endif
