#ifndef GAME_BAJANEW_ASSETS_H
#define GAME_BAJANEW_ASSETS_H

#include "assets.h"
#include "baja/sim.h"
#include "bajanew_assets_config.h"
#include "ng/asset.h"

#define BAJANEW_ROAD_PHASES 2
#define BAJANEW_RIVAL_LODS 4

/* A sprite plus the world width its frame spans, so one projection sizes
 * every object in the scene from the same metres-per-pixel scale. */
typedef struct BajanewSpriteDef {
    const NgSpriteFrame *frame;
    uint16_t world_width_q8;
} BajanewSpriteDef;

/* One road band's strip: the sheet, how many tile columns it holds, how many
 * hardware sprites its on-screen window needs, its tile rows, the palette
 * slot it owns and the pixel height it was authored at. */
typedef struct BajanewStripDef {
    const NgSpriteFrame *frame;
    uint16_t strip_columns;
    uint8_t window_columns;
    uint8_t rows;
    uint8_t palette;
    uint8_t authored_height;
} BajanewStripDef;

extern const BajanewStripDef bajanew_road_strip[BAJA_ROAD_BANDS];
/* Each band's palette in both surface phases, already hazed for its depth. */
extern const uint16_t bajanew_road_palette[BAJA_ROAD_BANDS][BAJANEW_ROAD_PHASES][16];
/* Strip column at which the backdrop layers sit when the road runs straight,
 * and how far each may pan with a bend. */
extern const int16_t bajanew_backdrop_origin_x;
extern const int16_t bajanew_ground_y;
#define BAJANEW_MAP_POINTS 64
extern const uint8_t bajanew_map_points[BAJANEW_MAP_POINTS + 1][2];
extern const int16_t bajanew_sky_pan;
extern const int16_t bajanew_ground_pan;
extern const BajanewSpriteDef bajanew_scenery[BAJA_SCENERY_KINDS];
extern const BajanewSpriteDef bajanew_scenery_far[BAJA_SCENERY_KINDS];
extern const BajanewSpriteDef bajanew_rival[2][BAJANEW_RIVAL_LODS];
extern const uint8_t bajanew_rival_lod_width[BAJANEW_RIVAL_LODS];
extern const BajanewSpriteDef bajanew_player_def;
extern const BajanewSpriteDef bajanew_dust_def;
/* The dust bank behind a truck at speed, three sizes as an animation. */
extern const BajanewSpriteDef bajanew_dust_wide_def[3];

#endif
