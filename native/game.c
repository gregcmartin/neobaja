#include "game/bajanew.h"

#include "ng/platform.h"

/* ----------------------------------------------------------- presentation --
 * The player rides a fixed screen berth: its sprite frame spans the same world
 * width as a rival's, at the depth that makes the two match when they touch.
 */
#define PLAYER_GROUND_Y 213
#define PLAYER_SCALE_Q8 11029
#define PLAYER_CENTER_X 160

#define BACKDROP_SPAN 640
#define BACKDROP_MARGIN ((BACKDROP_SPAN - BAJA_SCREEN_WIDTH) / 2)
/* Rows 0..27 are the visible 224 lines; the FIX map's last four rows are off
 * screen and are never touched. */
#define FIX_VISIBLE_ROWS 28
#define FIX_CELLS (FIX_VISIBLE_ROWS * BAJANEW_FIX_COLUMNS)

/* Draw order: sky, ground, then every band front-to-back with room between
 * consecutive bands for the objects standing on that stretch of road. */
#define PRIORITY_BACKDROP 0
#define PRIORITY_BAND(b) ((uint8_t)(8U + (uint8_t)(b) * 4U))
#define PRIORITY_ON_BAND(b) ((uint8_t)(10U + (uint8_t)(b) * 4U))
#define PRIORITY_DUST 200
#define PRIORITY_PLAYER 204

/* Scenery on screen at once.  The hardware sprite table is the hard limit and
 * the road already claims most of it, so the field is capped rather than left
 * to drop columns at the worst possible moment. */
#define SCENERY_BUDGET 3

#define SHADE_AMBER 0x80

enum {
    POSE_NEUTRAL = 0,
    POSE_LEFT,
    POSE_RIGHT,
    POSE_AIR,
    POSE_SETTLED
};

/* Development instrument: a debugger or MAME script can raise this to peel
 * layers off the frame and find what the 68000 is actually spending time on.
 * Zero is the shipping scene and nothing in the game ever writes it. */
volatile uint8_t bajanew_render_level = 0;

static uint8_t map_input(NgPad pad)
{
    uint8_t input = 0;
    if ((pad.held & NG_INPUT_LEFT) != 0U) input |= BAJA_INPUT_LEFT;
    if ((pad.held & NG_INPUT_RIGHT) != 0U) input |= BAJA_INPUT_RIGHT;
    if ((pad.held & NG_INPUT_A) != 0U) input |= BAJA_INPUT_THROTTLE;
    if ((pad.held & NG_INPUT_B) != 0U) input |= BAJA_INPUT_BRAKE;
    if ((pad.held & NG_INPUT_START) != 0U) input |= BAJA_INPUT_START;
    return input;
}

/* ------------------------------------------------------------- FIX layer -- */

/* Only the rows the HUD actually writes are cleared and compared.  Walking
 * all 1120 cells twice a frame cost the 68000 a measurable slice of its
 * budget for rows that are blank all game. */
static void clear_next(BajanewGame *game)
{
    uint32_t dirty = game->fix_dirty;
    uint8_t row;
    for (row = 0; row < FIX_VISIBLE_ROWS; ++row) {
        if ((dirty & (1UL << row)) != 0UL) {
            uint8_t *cells = &game->fix_next[row][0];
            uint8_t column;
            for (column = 0; column < BAJANEW_FIX_COLUMNS; ++column) {
                cells[column] = (uint8_t)' ';
            }
        }
    }
    game->fix_written = 0;
}

/* Text helpers resolve the row once and then walk it.  Indexing a two
 * dimensional FIX buffer per character costs the 68000 a 16-bit multiply
 * every time, and the HUD writes a few hundred characters a frame. */
static uint8_t *fix_row(BajanewGame *game, int16_t row)
{
    if (row < 0 || row >= FIX_VISIBLE_ROWS) return 0;
    game->fix_written |= (uint32_t)1UL << row;
    return &game->fix_next[row][0];
}

static void put_text(BajanewGame *game, int16_t column, int16_t row,
                     uint8_t shade, const char *text)
{
    uint8_t *cells = fix_row(game, row);
    if (cells == 0) return;
    while (*text != '\0' && column < BAJANEW_FIX_COLUMNS) {
        if (column >= 0) cells[column] = (uint8_t)((uint8_t)*text + shade);
        ++column;
        ++text;
    }
}

static void put_uint(BajanewGame *game, int16_t column, int16_t row, uint8_t shade,
                     uint32_t value, uint8_t digits, uint8_t pad_zero)
{
    uint8_t *cells = fix_row(game, row);
    uint8_t index;
    if (cells == 0) return;
    for (index = 0; index < digits; ++index) {
        int16_t at = (int16_t)(column + (int16_t)(digits - index - 1U));
        uint8_t glyph = (uint8_t)('0' + (uint8_t)(value % 10U));
        if (!pad_zero && index > 0U && value == 0U) glyph = (uint8_t)' ';
        if (at >= 0 && at < BAJANEW_FIX_COLUMNS) {
            cells[at] = (uint8_t)(glyph + shade);
        }
        value /= 10U;
    }
}

static void put_bar(BajanewGame *game, int16_t column, int16_t row,
                    uint8_t cells_wide, uint8_t filled_eighths)
{
    uint8_t *cells = fix_row(game, row);
    uint8_t i;
    if (cells == 0) return;
    for (i = 0; i < cells_wide; ++i) {
        uint8_t remaining = filled_eighths > (uint8_t)(i * 8U) ?
                            (uint8_t)(filled_eighths - (uint8_t)(i * 8U)) : 0U;
        uint8_t glyph = 0x80U;
        int16_t at = (int16_t)(column + (int16_t)i);
        if (remaining >= 8U) glyph = 0x84U;
        else if (remaining >= 6U) glyph = 0x83U;
        else if (remaining >= 4U) glyph = 0x82U;
        else if (remaining >= 2U) glyph = 0x81U;
        if (at >= 0 && at < BAJANEW_FIX_COLUMNS) cells[at] = glyph;
    }
}

/* A flat walk with running row/column counters.  The nested version indexed a
 * two dimensional array 1280 times a frame and cost the 68000 a full frame. */
static void flush_fix(BajanewGame *game)
{
    uint32_t dirty = game->fix_dirty | game->fix_written;
    uint8_t row;

    for (row = 0; row < FIX_VISIBLE_ROWS; ++row) {
        uint8_t *shadow;
        const uint8_t *next;
        uint8_t column;
        if ((dirty & (1UL << row)) == 0UL) continue;
        shadow = &game->fix_shadow[row][0];
        next = &game->fix_next[row][0];
        for (column = 0; column < BAJANEW_FIX_COLUMNS; ++column) {
            if (shadow[column] != next[column]) {
                shadow[column] = next[column];
                ng_fix_putc(column, row, 0, (char)next[column]);
            }
        }
    }
    game->fix_dirty = game->fix_written;
}

/* ---------------------------------------------------------------- sprites -- */

static void submit(NgRenderer *renderer, const NgSpriteFrame *frame,
                   int16_t x, int16_t y, uint8_t priority,
                   uint8_t zoom_x, uint8_t zoom_y, uint8_t flags)
{
    NgRenderOptions options;
    options.x = x;
    options.y = y;
    options.priority = priority;
    options.palette_override = 0xffU;
    options.flags = flags;
    options.zoom_x = zoom_x;
    options.zoom_y = zoom_y;
    options.reserved = 0;
    (void)ng_renderer_submit_ex(renderer, frame, &options);
}

/* Size a world object from the shared projection scale and draw it standing on
 * the road surface.  Choosing a nearer LOD keeps the shrink factor mild, which
 * is what stops a distant vehicle turning into sparkling noise. */
/* 65536 / n for the tile counts a sprite frame can have, so sizing an object
 * costs multiplies instead of the 68000's software division. */
static const uint16_t reciprocal_tiles[9] = {
    0, 65535, 32768, 21845, 16384, 13107, 10923, 9362, 8192
};

static void submit_world_sprite(BajanewGame *game, const BajanewSpriteDef *def,
                                const BajaObjectProjection *projection,
                                uint8_t priority, uint8_t flags)
{
    uint32_t pixels;
    uint16_t width_tiles;
    uint16_t height_tiles;
    int32_t zoom_x;
    int32_t zoom_y;

    if (def->frame == 0) return;
    width_tiles = def->frame->width_tiles;
    height_tiles = def->frame->height_tiles;
    if (width_tiles == 0U || width_tiles > 8U || height_tiles == 0U) return;
    (void)height_tiles;
    pixels = ((uint32_t)def->world_width_q8 * (uint32_t)projection->scale_q8) >> 16;
    if (pixels == 0U) return;
    if (pixels > 2048U) pixels = 2048U;

    zoom_x = (int32_t)((pixels * reciprocal_tiles[width_tiles]) >> 16) - 1;
    if (zoom_x < 0) zoom_x = 0;
    if (zoom_x > 15) zoom_x = 15;
    /* Uniform scale: shrinking each tile column to zoom_x+1 pixels means the
     * rows must shrink by the same fraction, and that fraction is the whole of
     * zoom_y regardless of how many tile rows the frame has. */
    zoom_y = ((zoom_x + 1) << 4) - 1;

    submit(&game->renderer, def->frame, projection->screen_x, projection->ground_y,
           priority, (uint8_t)zoom_x, (uint8_t)zoom_y, flags);
}

static const BajanewSpriteDef *rival_lod(uint8_t livery, uint16_t pixels)
{
    uint8_t lod = BAJANEW_RIVAL_LODS - 1U;
    uint8_t i;
    for (i = 0; i < BAJANEW_RIVAL_LODS; ++i) {
        if (bajanew_rival_lod_width[i] >= pixels) lod = i;
    }
    return &bajanew_rival[livery][lod];
}

static void draw_backdrop(BajanewGame *game, const BajaRoadBand *bands)
{
    /* The horizon leans against the bend the road is taking, which is the only
     * parallax cue available above the funnel. */
    int16_t target = (int16_t)((BAJA_SCREEN_CENTER - bands[0].center_x) / 2);
    if (target > BACKDROP_MARGIN) target = BACKDROP_MARGIN;
    if (target < -BACKDROP_MARGIN) target = -BACKDROP_MARGIN;
    game->sky_pan += (int16_t)((target - game->sky_pan) / 8);
    submit(&game->renderer, &ng_asset_backdrop_frames[0],
           (int16_t)(-BACKDROP_MARGIN + game->sky_pan), 0,
           PRIORITY_BACKDROP, 0x0fU, 0xffU, 0U);
}

static void draw_road(BajanewGame *game, const BajaRoadBand *bands)
{
    /* Levels 16 and up draw only the first N bands, so the sprite budget can
     * be measured against real hardware instead of guessed at. */
    uint8_t limit = bajanew_render_level >= 16U ?
                    (uint8_t)(bajanew_render_level - 16U) : BAJA_ROAD_BANDS;
    uint8_t b;
    if (limit > BAJA_ROAD_BANDS) limit = BAJA_ROAD_BANDS;
    for (b = 0; b < limit; ++b) {
        const BajaRoadBand *band = &bands[b];
        uint16_t authored;
        int32_t zoom_y;
        if (!band->visible || band->height == 0U) continue;
        authored = bajanew_road_authored_height[b];
        zoom_y = (((int32_t)band->height * 256) / authored) - 1;
        if (zoom_y < 0) zoom_y = 0;
        if (zoom_y > 255) zoom_y = 255;
        submit(&game->renderer, bajanew_road_frames[b][band->phase],
               band->center_x, band->top_y, PRIORITY_BAND(b),
               0x0fU, (uint8_t)zoom_y, 0U);
    }
}

static void draw_scenery(BajanewGame *game, const BajaView *view)
{
    const BajaFp near_edge = game->sim.player_s;
    const BajaFp far_edge = game->sim.player_s + baja_fp_from_int(260);
    uint8_t drawn = 0;
    uint16_t i;
    /* The course list is sorted by distance, so a cheap window finds the few
     * items in view without projecting the whole field every frame. */
    for (i = 0; i < BAJA_SCENERY_COUNT; ++i) {
        const BajaScenery *item = &game->sim.scenery[i];
        BajaObjectProjection projection;
        if (item->s < near_edge) continue;
        if (item->s > far_edge) break;
        if (drawn >= SCENERY_BUDGET) break;
        baja_project_object_in(&game->sim, view, item->s, item->e, &projection);
        if (!projection.visible) continue;
        submit_world_sprite(game, &bajanew_scenery[item->kind], &projection,
                            PRIORITY_ON_BAND(projection.band),
                            (item->e < 0) ? NG_RENDER_FLIP_X : 0U);
        ++drawn;
    }
}

static void draw_rivals(BajanewGame *game, const BajaView *view)
{
    uint8_t i;
    for (i = 0; i < BAJA_RIVAL_COUNT; ++i) {
        const BajaRival *rival = &game->sim.rivals[i];
        BajaObjectProjection projection;
        const BajanewSpriteDef *def;
        uint32_t pixels;
        if (!rival->active) continue;
        baja_project_object_in(&game->sim, view, rival->s, rival->e, &projection);
        if (!projection.visible) continue;
        pixels = ((uint32_t)bajanew_rival[0][0].world_width_q8 *
                  (uint32_t)projection.scale_q8) >> 16;
        if (pixels > 255U) pixels = 255U;
        def = rival_lod((uint8_t)(rival->profile & 1U), (uint16_t)pixels);
        submit_world_sprite(game, def, &projection,
                            PRIORITY_ON_BAND(projection.band), 0U);
    }
}

static void draw_player(BajanewGame *game)
{
    const BajaSim *sim = &game->sim;
    int32_t lean = (sim->steer * 12) / BAJA_FP_ONE;
    int32_t lift = (int32_t)((sim->bounce * 24) / BAJA_FP_ONE);
    int16_t x = (int16_t)(PLAYER_CENTER_X + lean);
    int16_t y = (int16_t)(PLAYER_GROUND_Y - lift);
    uint8_t pose = POSE_NEUTRAL;
    BajaObjectProjection projection;

    if (sim->bounce < -(BAJA_FP_ONE / 5)) pose = POSE_AIR;
    else if (sim->steer < -(BAJA_FP_ONE / 3)) pose = POSE_LEFT;
    else if (sim->steer > (BAJA_FP_ONE / 3)) pose = POSE_RIGHT;
    else if (sim->speed < BAJA_FP_ONE / 8) pose = POSE_SETTLED;

    projection.screen_x = x;
    projection.ground_y = y;
    projection.scale_q8 = PLAYER_SCALE_Q8;
    projection.depth = 4;
    projection.band = BAJA_ROAD_BANDS - 1U;
    projection.visible = 1;

    if (sim->dust_event != 0U) {
        uint8_t frame = (uint8_t)((game->frame >> 2) % 3U);
        submit(&game->renderer, &ng_asset_dust_frames[frame],
               (int16_t)(x - 36), (int16_t)(y + 2), PRIORITY_DUST,
               0x0fU, 0xffU, 0U);
        submit(&game->renderer, &ng_asset_dust_frames[(frame + 1U) % 3U],
               (int16_t)(x + 36), (int16_t)(y + 2), PRIORITY_DUST,
               0x0fU, 0xffU, NG_RENDER_FLIP_X);
    }

    {
        BajanewSpriteDef def = bajanew_player_def;
        def.frame = &ng_asset_player_frames[pose];
        submit_world_sprite(game, &def, &projection, PRIORITY_PLAYER, 0U);
    }
}

/* ------------------------------------------------------------------- HUD -- */

static void draw_hud(BajanewGame *game)
{
    const BajaSim *sim = &game->sim;
    uint32_t hundredths = (sim->race_frames * 100U) / 60U;
    uint32_t seconds = hundredths / 100U;
    uint32_t speed = (uint32_t)((sim->speed * 216) / BAJA_FP_ONE);
    uint32_t progress = (uint32_t)(((int64_t)sim->player_s * 100) /
                                   sim->track.total_length);
    uint32_t revs;

    put_text(game, 1, 1, 0, "POSITION");
    put_uint(game, 2, 2, SHADE_AMBER, sim->position, 1, 1);
    put_text(game, 3, 2, 0, "/");
    put_uint(game, 4, 2, 0, BAJA_RIVAL_COUNT + 1U, 1, 1);
    put_text(game, 1, 4, 0, "LEG");
    put_uint(game, 1, 5, 0, progress, 3, 0);
    put_text(game, 4, 5, 0, "%");

    put_text(game, 17, 1, 0, "TIME");
    put_uint(game, 15, 2, SHADE_AMBER, seconds / 60U, 1, 1);
    put_text(game, 16, 2, SHADE_AMBER, "'");
    put_uint(game, 17, 2, SHADE_AMBER, seconds % 60U, 2, 1);
    put_text(game, 19, 2, SHADE_AMBER, "\"");
    put_uint(game, 20, 2, SHADE_AMBER, hundredths % 100U, 2, 1);

    put_text(game, 31, 1, 0, "BEST LEG");
    if (game->best_frames != 0U) {
        uint32_t best = (game->best_frames * 100U) / 60U;
        put_uint(game, 32, 2, 0, best / 6000U, 1, 1);
        put_text(game, 33, 2, 0, "'");
        put_uint(game, 34, 2, 0, (best / 100U) % 60U, 2, 1);
        put_text(game, 36, 2, 0, "\"");
        put_uint(game, 37, 2, 0, best % 100U, 2, 1);
    } else {
        put_text(game, 32, 2, 0, "-'--\"--");
    }

    put_text(game, 1, 24, 0, "SPEED");
    put_uint(game, 1, 25, SHADE_AMBER, speed, 3, 0);
    put_text(game, 5, 25, 0, "KMH");
    put_text(game, 1, 27, 0, "GEAR");
    put_uint(game, 6, 27, SHADE_AMBER, sim->gear, 1, 1);
    revs = (uint32_t)((sim->speed * 8) / BAJA_FP_ONE);
    if (revs > 6U) revs = 6U;
    put_bar(game, 1, 26, 6, (uint8_t)(revs * 8U + 4U));

    put_text(game, 26, 24, 0, "ENSENADA");
    put_text(game, 24, 25, SHADE_AMBER, "PACIFIC RUN");
    put_bar(game, 24, 27, 12, (uint8_t)((progress * 96U) / 100U));

    if (sim->surface == BAJA_SURFACE_DIRT) put_text(game, 17, 22, SHADE_AMBER, "OFF ROAD");
    else if (sim->surface == BAJA_SURFACE_SHOULDER) put_text(game, 18, 22, 0, "EDGE");
    if (sim->collision_event != 0U) put_text(game, 17, 20, SHADE_AMBER, "CONTACT");
}

/* ----------------------------------------------------------------- scene -- */

static void draw_race(BajanewGame *game, uint8_t with_actors)
{
    BajaRoadBand bands[BAJA_ROAD_BANDS];
    BajaView view;
    uint8_t level = bajanew_render_level;
    if (level >= 16U) level = 0U;
    if (level >= 5U) return;
    baja_view_init(&game->sim, &view);
    (void)baja_project_bands(&game->sim, bands);
    if (level >= 4U) return;
    draw_backdrop(game, bands);
    if (level >= 3U) return;
    draw_road(game, bands);
    if (level >= 2U) return;
    draw_scenery(game, &view);
    if (level >= 1U) return;
    if (with_actors) {
        draw_rivals(game, &view);
        draw_player(game);
    }
}

static void draw_frame(BajanewGame *game)
{
    const BajaSim *sim = &game->sim;
    uint8_t level = bajanew_render_level;
    if (level >= 16U) level = 0U;
    if (level >= 7U) return;
    clear_next(game);
    ng_renderer_begin(&game->renderer);
    if (level >= 6U) {
        game->last_render = ng_renderer_flush(&game->renderer);
        flush_fix(game);
        return;
    }

    switch ((BajaPhase)sim->phase) {
    case BAJA_PHASE_SPLASH:
        /* The developer mark stands alone for its five seconds and ignores
         * every input, exactly as the packet requires. */
        submit(&game->renderer, &ng_asset_splash_frames[0], 0, 0,
               PRIORITY_BACKDROP, 0x0fU, 0xffU, 0U);
        break;
    case BAJA_PHASE_TITLE:
        draw_race(game, 0);
        if (level < 5U) {
            put_text(game, 14, 9, SHADE_AMBER, "BAJA OUTRUN");
            put_text(game, 12, 11, 0, "ENSENADA PACIFIC RUN");
            if (((sim->phase_frame >> 5) & 1U) == 0U) {
                put_text(game, 14, 22, 0, "PRESS START");
            }
        }
        break;
    case BAJA_PHASE_SELECT: {
        uint8_t is_max = (uint8_t)(sim->driver == BAJA_DRIVER_MAX);
        draw_race(game, 0);
        put_text(game, 13, 6, 0, "CHOOSE DRIVER");
        put_text(game, 7, 12, is_max ? SHADE_AMBER : 0U, "MAX");
        put_text(game, 7, 13, 0, "AGE 8");
        put_text(game, 7, 14, 0, "NO 8");
        put_text(game, 27, 12, is_max ? 0U : SHADE_AMBER, "CRUZ");
        put_text(game, 27, 13, 0, "AGE 6");
        put_text(game, 27, 14, 0, "NO 6");
        put_text(game, is_max ? 5 : 25, 12, 0, ">");
        put_text(game, 8, 24, 0, "LEFT RIGHT CHOOSE");
        put_text(game, 11, 25, 0, "START CONFIRMS");
        break;
    }
    case BAJA_PHASE_COUNTDOWN: {
        uint32_t remaining = 3U - (sim->phase_frame / 60U);
        draw_race(game, 1);
        draw_hud(game);
        if (remaining > 3U) remaining = 3U;
        if (remaining > 0U) {
            put_uint(game, 19, 12, SHADE_AMBER, remaining, 1, 1);
        } else {
            put_text(game, 18, 12, SHADE_AMBER, "GO!");
        }
        break;
    }
    case BAJA_PHASE_RACING:
        draw_race(game, 1);
        draw_hud(game);
        break;
    case BAJA_PHASE_FINISHED:
        draw_race(game, 1);
        draw_hud(game);
        put_text(game, 16, 11, SHADE_AMBER, "FINISH");
        put_text(game, 14, 13, 0, "POSITION");
        put_uint(game, 23, 13, SHADE_AMBER, sim->position, 1, 1);
        if (((sim->phase_frame >> 5) & 1U) == 0U) {
            put_text(game, 12, 16, 0, "PRESS START TO RACE");
        }
        break;
    default:
        break;
    }

    game->last_render = ng_renderer_flush(&game->renderer);
    flush_fix(game);
}

void bajanew_game_init(BajanewGame *game)
{
    uint8_t row;
    uint8_t column;
    baja_sim_init_cooperative(&game->sim, ng_platform_kick_watchdog);
    ng_renderer_init(&game->renderer);
    game->frame = 0;
    game->sky_pan = 0;
    game->fix_dirty = 0xffffffffUL;
    game->fix_written = 0;
    game->best_frames = 0;
    game->initialized = 1;
    game->reserved[0] = 0;
    game->reserved[1] = 0;
    game->reserved[2] = 0;
    for (row = 0; row < BAJANEW_FIX_ROWS; ++row) {
        for (column = 0; column < BAJANEW_FIX_COLUMNS; ++column) {
            game->fix_shadow[row][column] = (uint8_t)' ';
            game->fix_next[row][column] = (uint8_t)' ';
        }
    }
    ng_assets_load_palettes();
    ng_platform_backdrop(0x1119U);
    ng_fix_clear();
    draw_frame(game);
}

void bajanew_game_set_autoplay(BajanewGame *game, uint8_t enabled)
{
    /* BAJANEW has no attract mode: normal play is never driven by a timer. */
    (void)game;
    (void)enabled;
}

void bajanew_game_tick(BajanewGame *game, NgPad pad)
{
    uint8_t was_racing;
    if (game->initialized == 0U) return;
    was_racing = (uint8_t)(game->sim.phase == BAJA_PHASE_RACING);
    baja_sim_step(&game->sim, map_input(pad));
    if (was_racing && game->sim.phase == BAJA_PHASE_FINISHED) {
        uint32_t frames = game->sim.race_frames;
        if (frames > 65535U) frames = 65535U;
        if (game->best_frames == 0U || (uint16_t)frames < game->best_frames) {
            game->best_frames = (uint16_t)frames;
        }
    }
    ++game->frame;
    draw_frame(game);
}
