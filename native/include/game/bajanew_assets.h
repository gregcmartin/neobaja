#ifndef GAME_BAJANEW_ASSETS_H
#define GAME_BAJANEW_ASSETS_H

#include "assets.h"
#include "baja/sim.h"
#include "ng/asset.h"

#define BAJANEW_ROAD_PHASES 2
#define BAJANEW_RIVAL_LODS 4

/* A sprite plus the world width its frame spans, so one projection sizes
 * every object in the scene from the same metres-per-pixel scale. */
typedef struct BajanewSpriteDef {
    const NgSpriteFrame *frame;
    uint16_t world_width_q8;
} BajanewSpriteDef;

extern const NgSpriteFrame *const bajanew_road_frames[BAJA_ROAD_BANDS][BAJANEW_ROAD_PHASES];
extern const uint8_t bajanew_road_authored_height[BAJA_ROAD_BANDS];
extern const BajanewSpriteDef bajanew_scenery[BAJA_SCENERY_KINDS];
extern const BajanewSpriteDef bajanew_rival[2][BAJANEW_RIVAL_LODS];
extern const uint8_t bajanew_rival_lod_width[BAJANEW_RIVAL_LODS];
extern const BajanewSpriteDef bajanew_player_def;
extern const BajanewSpriteDef bajanew_dust_def;

#endif
