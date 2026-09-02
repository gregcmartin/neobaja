#include "game/bajanew.h"

#include "ng/platform.h"

/* ----------------------------------------------------------- presentation --
 * The player rides a fixed screen berth: its sprite frame spans the same world
 * width as a rival's, at the depth that makes the two match when they touch.
 */
#define PLAYER_GROUND_Y 213
#define PLAYER_SCALE_Q8 BAJA_PLAYER_SCALE_Q8

/* Hardware sprite columns each backdrop layer's window needs: the screen's
 * twenty plus a partial column at each edge. */
#define BACKDROP_WINDOW 21

/* Draw order among the renderer's objects: sorted by the band they stand on,
 * far to near, then dust, then the player.  The backdrop and the road sit
 * below all of them in fixed hardware sprites, so an object is always over the
 * road; an object the road should hide - one whose feet fall behind a crest -
 * is simply not drawn. */

/* Hardware sprite columns the scenery may use in one frame.  The road claims
 * most of the table, so the field is budgeted nearest first rather than left
 * to drop columns at the worst possible moment. */
#define SCENERY_COLUMN_BUDGET 28
/* Baseline the selection portraits stand on. */
#define DRIVER_BASE_Y 190

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

/* Development timing marker.  Each stage of the frame writes its number here;
 * a MAME script taps the address and turns the gaps into 68000 cycles, which
 * is the only way to see costs smaller than a whole 60 Hz field. */
volatile uint8_t bajanew_stage = 0;
#define STAGE(n) do { bajanew_stage = (uint8_t)(n); } while (0)

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

/* FIX rows 0 and 1 sit above the visible picture and rows 30 and 31 below it,
 * so the HUD lives in rows 2..29. */
#define FIX_FIRST_ROW 2
#define FIX_LAST_ROW 29
#define FIX_BLANK 0x0020U
/* Tile codes of the 16x16 numerals, four tiles each, ivory then amber. */
#define BIG_BASE 0x100U
#define BIG_AMBER 0x40U

/* Only the rows the HUD actually writes are cleared and compared.  Walking
 * all 1120 cells twice a frame cost the 68000 a measurable slice of its
 * budget for rows that are blank all game. */
static void clear_next(BajanewGame *game)
{
    uint32_t dirty = game->fix_dirty;
    uint8_t row;
    for (row = 0; row < BAJANEW_FIX_ROWS; ++row) {
        if ((dirty & (1UL << row)) != 0UL) {
            uint32_t *cells = (uint32_t *)(void *)&game->fix_next[row][0];
            uint8_t word;
            for (word = 0; word < BAJANEW_FIX_COLUMNS / 2U; ++word) {
                cells[word] = 0x00200020UL;
            }
        }
    }
    game->fix_written = 0;
}

/* Text helpers resolve the row once and then walk it.  Indexing a two
 * dimensional FIX buffer per character costs the 68000 a 16-bit multiply
 * every time, and the HUD writes a few hundred characters a frame. */
static uint16_t *fix_row(BajanewGame *game, int16_t row)
{
    if (row < FIX_FIRST_ROW || row > FIX_LAST_ROW) return 0;
    game->fix_written |= (uint32_t)1UL << row;
    return &game->fix_next[row][0];
}

static void put_text(BajanewGame *game, int16_t column, int16_t row,
                     uint8_t shade, const char *text)
{
    uint16_t *cells = fix_row(game, row);
    if (cells == 0) return;
    while (*text != '\0' && column < BAJANEW_FIX_COLUMNS) {
        if (column >= 0) cells[column] = (uint16_t)((uint8_t)*text + shade);
        ++column;
        ++text;
    }
}

/* Big numerals: digits, the time marks, a point and a slash, two cells wide
 * and two tall, anchored at their top-left cell. */
static uint16_t big_code(char ch)
{
    static const char big_chars[] = "0123456789'\".:/";
    uint16_t index = 0;
    while (big_chars[index] != '\0' && big_chars[index] != ch) ++index;
    if (big_chars[index] == '\0') return 0;
    return (uint16_t)(BIG_BASE + index * 4U);
}

static void put_big(BajanewGame *game, int16_t column, int16_t row,
                    uint8_t amber, const char *text)
{
    uint16_t *top = fix_row(game, row);
    uint16_t *bottom = fix_row(game, (int16_t)(row + 1));
    if (top == 0 || bottom == 0) return;
    while (*text != '\0' && column + 1 < BAJANEW_FIX_COLUMNS) {
        uint16_t code = big_code(*text);
        if (code != 0U && column >= 0) {
            if (amber) code = (uint16_t)(code + BIG_AMBER);
            top[column] = code;
            top[column + 1] = (uint16_t)(code + 1U);
            bottom[column] = (uint16_t)(code + 2U);
            bottom[column + 1] = (uint16_t)(code + 3U);
        }
        column = (int16_t)(column + 2);
        ++text;
    }
}

/* Exact for every 16-bit value, and one 16x16 multiply instead of the 68000's
 * software division.  The HUD extracts about thirty digits a frame, which was
 * sixty divide calls. */
static uint16_t divide_by_ten(uint16_t value)
{
    return (uint16_t)(((uint32_t)value * 52429UL) >> 19);
}

static uint16_t divide_by_three(uint16_t value)
{
    return (uint16_t)(((uint32_t)value * 43691UL) >> 17);
}

/* Decimal digits of a value into a buffer, most significant first, blank
 * padded unless zero padded; the buffer gets a terminator. */
static void format_uint(char *out, uint16_t value, uint8_t digits, uint8_t pad_zero)
{
    uint8_t index;
    out[digits] = '\0';
    for (index = 0; index < digits; ++index) {
        uint16_t next = divide_by_ten(value);
        char glyph = (char)('0' + (char)(value - (uint16_t)(next * 10U)));
        if (!pad_zero && index > 0U && value == 0U) glyph = ' ';
        out[digits - index - 1U] = glyph;
        value = next;
    }
}

static void put_uint(BajanewGame *game, int16_t column, int16_t row, uint8_t shade,
                     uint16_t value, uint8_t digits, uint8_t pad_zero)
{
    char text[8];
    if (digits > 7U) digits = 7U;
    format_uint(text, value, digits, pad_zero);
    put_text(game, column, row, shade, text);
}

static void put_bar(BajanewGame *game, int16_t column, int16_t row,
                    uint8_t cells_wide, uint8_t filled_eighths)
{
    uint16_t *cells = fix_row(game, row);
    uint8_t i;
    if (cells == 0) return;
    for (i = 0; i < cells_wide; ++i) {
        uint8_t remaining = filled_eighths > (uint8_t)(i * 8U) ?
                            (uint8_t)(filled_eighths - (uint8_t)(i * 8U)) : 0U;
        uint16_t glyph = 0x80U;
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

    for (row = 0; row < BAJANEW_FIX_ROWS; ++row) {
        uint32_t *shadow_words;
        const uint32_t *next_words;
        uint8_t word;
        if ((dirty & (1UL << row)) == 0UL) continue;
        shadow_words = (uint32_t *)(void *)&game->fix_shadow[row][0];
        next_words = (const uint32_t *)(const void *)&game->fix_next[row][0];
        /* Two cells at a time: only a pair that actually changed is worth
         * looking at cell by cell. */
        for (word = 0; word < BAJANEW_FIX_COLUMNS / 2U; ++word) {
            if (shadow_words[word] != next_words[word]) {
                uint16_t *shadow = &game->fix_shadow[row][word * 2U];
                const uint16_t *next = &game->fix_next[row][word * 2U];
                uint8_t offset;
                for (offset = 0; offset < 2U; ++offset) {
                    if (shadow[offset] != next[offset]) {
                        shadow[offset] = next[offset];
                        ng_fix_put_tile((uint8_t)(word * 2U + offset), row, 0, next[offset]);
                    }
                }
            }
        }
    }
    game->fix_dirty = game->fix_written;
}

/* ---------------------------------------------------------------- sprites -- */

/* Place a sprite straight into the pool: no sorting, so the caller draws
 * nearest first. */
static void place(BajanewGame *game, const NgSpriteFrame *frame,
                  int16_t x, int16_t y, uint8_t zoom_x, uint8_t zoom_y, uint8_t flags)
{
    uint16_t columns = ng_pool_draw(&game->pool, frame, x, y, zoom_x, zoom_y, flags,
                                    frame->palette);
    if (columns != 0U) {
        int16_t shown_rows = (int16_t)((frame->height_tiles * (zoom_y + 1) + 255) >> 8);
        if (shown_rows < 1) shown_rows = 1;
        ng_renderer_mark_span(&game->renderer,
                              (int16_t)(y - (int16_t)((frame->origin_y * (zoom_y + 1)) >> 8)),
                              (int16_t)(shown_rows * 16), columns);
    }
}

/* Queue a world object, keeping the queue ordered nearest first.  Scenery
 * arrives already ordered, so a rival or a prop out of turn only shuffles the
 * few entries behind it. */
static void queue_item(BajanewGame *game, const NgSpriteFrame *frame, int16_t x, int16_t y,
                       uint16_t depth, uint8_t zoom_x, uint8_t zoom_y, uint8_t flags)
{
    BajanewDrawItem *items = game->items;
    uint16_t at = game->item_count;
    if (at >= BAJANEW_DRAW_ITEMS) return;
    while (at > 0U && items[at - 1U].depth > depth) {
        items[at] = items[at - 1U];
        --at;
    }
    items[at].frame = frame;
    items[at].x = x;
    items[at].y = y;
    items[at].depth = depth;
    items[at].zoom_x = zoom_x;
    items[at].zoom_y = zoom_y;
    items[at].flags = flags;
    items[at].palette = frame->palette;
    ++game->item_count;
}

static void draw_items(BajanewGame *game)
{
    const BajanewDrawItem *item = game->items;
    uint16_t remaining;
    for (remaining = game->item_count; remaining != 0U; --remaining, ++item) {
        place(game, item->frame, item->x, item->y, item->zoom_x, item->zoom_y, item->flags);
    }
    game->item_count = 0;
}

/* 65536 / n for the tile counts a sprite frame can have, so sizing an object
 * costs multiplies instead of the 68000's software division. */
static const uint16_t reciprocal_tiles[17] = {
    0, 65535, 32768, 21845, 16384, 13107, 10923, 9362, 8192,
    7282, 6554, 5958, 5461, 5041, 4681, 4369, 4096
};

/* Size a world object from the shared projection scale and queue it standing
 * on the road surface.  Choosing a nearer LOD keeps the shrink factor mild,
 * which is what stops a distant vehicle turning into sparkling noise. */
static void queue_world_sprite(BajanewGame *game, const BajanewSpriteDef *def,
                               const BajaObjectProjection *projection, uint8_t flags)
{
    uint32_t pixels;
    uint16_t width_tiles;
    int32_t zoom_x;
    int32_t zoom_y;

    if (def->frame == 0) return;
    width_tiles = def->frame->width_tiles;
    if (width_tiles == 0U || width_tiles > 16U) return;
    /* Both products are 16 by 16: the hardware multiply, not libgcc's. */
    pixels = ((uint32_t)(uint16_t)def->world_width_q8 * (uint16_t)projection->scale_q8) >> 16;
    if (pixels == 0U) return;
    if (pixels > 2048U) pixels = 2048U;

    zoom_x = (int32_t)(((uint32_t)(uint16_t)pixels * reciprocal_tiles[width_tiles]) >> 16) - 1;
    if (zoom_x < 0) zoom_x = 0;
    if (zoom_x > 15) zoom_x = 15;
    /* Uniform scale: shrinking each tile column to zoom_x+1 pixels means the
     * rows must shrink by the same fraction, and that fraction is the whole of
     * zoom_y regardless of how many tile rows the frame has. */
    zoom_y = ((zoom_x + 1) << 4) - 1;
    queue_item(game, def->frame, projection->screen_x, projection->ground_y,
               projection->depth, (uint8_t)zoom_x, (uint8_t)zoom_y, flags);
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

/* Register a placed layer's hardware footprint with the renderer's scanline
 * accounting: every tile row it maps counts against the per-line limit, even
 * the rows a shrink leaves transparent. */
static void mark_layer(BajanewGame *game, const NgStripLayer *layer, int16_t y)
{
    if (layer->head_y_control == 0U) return;
    ng_renderer_mark_span(&game->renderer, y, (int16_t)(layer->height_rows * 16),
                          layer->shown_columns);
}

static int16_t approach_pan(int16_t pan, int16_t target, int16_t limit)
{
    if (target > limit) target = limit;
    if (target < -limit) target = -limit;
    return (int16_t)(pan + (int16_t)((target - pan) / 8));
}

static void draw_backdrop(BajanewGame *game, const BajaView *view, const BajaRoadBand *bands)
{
    /* The horizon leans against the bend the road is taking, the near ground
     * twice as far as the sky, which is the only parallax cue above the
     * funnel. */
    int16_t offset = (int16_t)(BAJA_SCREEN_CENTER - bands[0].center_x);
    int16_t ground_y = (int16_t)(bajanew_ground_y + view->shake);
    int16_t needed = 0;
    uint8_t b;
    game->sky_pan = approach_pan(game->sky_pan, (int16_t)(offset / 4), bajanew_sky_pan);
    game->ground_pan = approach_pan(game->ground_pan, (int16_t)(offset / 2), bajanew_ground_pan);
    game->strip_words = (uint16_t)(game->strip_words + ng_strip_place(
        &game->sky, (int16_t)(game->sky_pan - bajanew_backdrop_origin_x),
        view->shake, 0xffU));
    mark_layer(game, &game->sky, view->shake);
    /* The ground only needs to reach the first band wide enough to cover the
     * screen on its own; every row below that would count against the
     * scanline limit for nothing. */
    for (b = 0; b < BAJA_ROAD_BANDS; ++b) {
        if (bajanew_road_strip[b].window_columns >= BACKDROP_WINDOW && bands[b].visible) {
            needed = (int16_t)(bands[b].top_y - ground_y);
            break;
        }
    }
    if (b == BAJA_ROAD_BANDS) needed = (int16_t)(BAJA_SCREEN_HEIGHT - ground_y);
    ng_strip_set_height(&game->ground, (uint8_t)((needed + 15) >> 4));
    game->strip_words = (uint16_t)(game->strip_words + ng_strip_place(
        &game->ground, (int16_t)(game->ground_pan - bajanew_backdrop_origin_x),
        ground_y, 0xffU));
    mark_layer(game, &game->ground, ground_y);
}

static void hide_layers(BajanewGame *game)
{
    uint8_t b;
    (void)ng_strip_hide(&game->sky);
    (void)ng_strip_hide(&game->ground);
    for (b = 0; b < BAJA_ROAD_BANDS; ++b) (void)ng_strip_hide(&game->road[b]);
}

static void draw_road(BajanewGame *game, const BajaRoadBand *bands)
{
    /* Levels 16 and up draw only the first N bands, so the sprite budget can
     * be measured against real hardware instead of guessed at. */
    uint8_t limit = bajanew_render_level >= 16U ?
                    (uint8_t)(bajanew_render_level - 16U) : BAJA_ROAD_BANDS;
    uint8_t b;
    if (limit > BAJA_ROAD_BANDS) limit = BAJA_ROAD_BANDS;
    for (b = 0; b < BAJA_ROAD_BANDS; ++b) {
        const BajaRoadBand *band = &bands[b];
        const BajanewStripDef *def = &bajanew_road_strip[b];
        NgStripLayer *layer = &game->road[b];
        int16_t zoom_y;
        if (b >= limit || !band->visible || band->height == 0U) {
            game->strip_words = (uint16_t)(game->strip_words + ng_strip_hide(layer));
            continue;
        }
        /* Shrink from the authored height.  A sprite shrunk below a sixteenth
         * of its tile rows would read past its tile map and draw a ghost of
         * itself further down, so the shrink is floored there; the nearer band
         * covers the few extra rows. */
        if (def->rows == 1U) zoom_y = (int16_t)(band->height << 4);
        else if (def->rows == 2U) zoom_y = (int16_t)(band->height << 3);
        else zoom_y = (int16_t)divide_by_three((uint16_t)(band->height << 4));
        zoom_y -= 1;
        if (zoom_y < (int16_t)(def->rows * 8 - 1)) zoom_y = (int16_t)(def->rows * 8 - 1);
        if (zoom_y > 255) zoom_y = 255;
        if (game->road_phase[b] != band->phase) {
            ng_platform_palette_load(def->palette, bajanew_road_palette[b][band->phase]);
            game->road_phase[b] = band->phase;
        }
        ng_strip_set_height(layer, (uint8_t)((band->height + 15U) >> 4));
        game->strip_words = (uint16_t)(game->strip_words + ng_strip_place(
            layer, (int16_t)(band->center_x - (int16_t)(def->strip_columns * 8U)),
            band->top_y, (uint8_t)zoom_y));
        mark_layer(game, layer, band->top_y);
    }
}

/* An object stands on the road; if the row its feet project to lies below the
 * visible bottom of its band, a nearer crest is in front of it. */
static uint8_t behind_crest(const BajaRoadBand *bands, const BajaObjectProjection *projection)
{
    const BajaRoadBand *band = &bands[projection->band];
    if (!band->visible) return 1;
    /* The object's own projection and the band edges round differently, and
     * the gap grows with the band, so the tolerance scales with its height:
     * tight for the thin far bands where crests actually hide things. */
    return (uint8_t)(projection->ground_y >
                     band->top_y + (int16_t)band->height + 3 + (int16_t)(band->height >> 1));
}

/* How far each kind of prop is drawn, in quarter metres.  Small scrub is a
 * pixel or two beyond a hundred metres and not worth a sprite; tall props and
 * signs read from the far end of the funnel. */
static const uint8_t scenery_reach_q[BAJA_SCENERY_KINDS] = {
    22, 22, 24, 20, 44, 36, 40, 28, 36, 44, 44, 44, 60, 60
};

static void draw_scenery(BajanewGame *game, const BajaView *view, const BajaRoadBand *bands)
{
    const BajaFp near_edge = game->sim.player_s;
    const BajaFp far_edge = game->sim.player_s + baja_fp_from_int(260);
    int16_t columns_left = SCENERY_COLUMN_BUDGET;
    uint16_t i;
    /* The course list is sorted by distance, so a cursor that only ever moves
     * forward finds the items in view without a walk from the start; it
     * rewinds when a restart puts the player behind it. */
    if (game->scenery_cursor > 0U &&
        game->sim.scenery[game->scenery_cursor - 1U].s >= near_edge) {
        game->scenery_cursor = 0;
    }
    while (game->scenery_cursor < BAJA_SCENERY_COUNT &&
           game->sim.scenery[game->scenery_cursor].s < near_edge) {
        ++game->scenery_cursor;
    }
    for (i = game->scenery_cursor; i < BAJA_SCENERY_COUNT; ++i) {
        const BajaScenery *item = &game->sim.scenery[i];
        const BajanewSpriteDef *def = &bajanew_scenery[item->kind];
        BajaObjectProjection projection;
        uint32_t pixels;
        int16_t columns;
        if (item->s > far_edge) break;
        /* Beyond the middle distance a prop is a few pixels: its far frame is
         * one column wide, so a busy horizon costs one sprite a prop. */
        if (item->s - near_edge > baja_fp_from_int(56)) def = &bajanew_scenery_far[item->kind];
        if ((item->s - near_edge) >> 18 > (BajaFp)scenery_reach_q[item->kind]) continue;
        if (columns_left <= 0) break;
        baja_project_object_in(&game->sim, view, item->s, item->e, &projection);
        if (!projection.visible || behind_crest(bands, &projection)) continue;
        pixels = ((uint32_t)(uint16_t)def->world_width_q8 * (uint16_t)projection.scale_q8) >> 16;
        columns = (int16_t)((pixels + 15U) >> 4);
        if (columns > columns_left) continue;
        queue_world_sprite(game, def, &projection, (item->e < 0) ? NG_RENDER_FLIP_X : 0U);
        columns_left = (int16_t)(columns_left - columns);
    }
}

static void draw_rivals(BajanewGame *game, const BajaView *view, const BajaRoadBand *bands)
{
    uint8_t i;
    for (i = 0; i < BAJA_RIVAL_COUNT; ++i) {
        const BajaRival *rival = &game->sim.rivals[i];
        BajaObjectProjection projection;
        const BajanewSpriteDef *def;
        uint32_t pixels;
        if (!rival->active) continue;
        baja_project_object_in(&game->sim, view, rival->s, rival->e, &projection);
        if (!projection.visible || behind_crest(bands, &projection)) continue;
        pixels = ((uint32_t)(uint16_t)bajanew_rival[0][0].world_width_q8 *
                  (uint16_t)projection.scale_q8) >> 16;
        if (pixels > 255U) pixels = 255U;
        def = rival_lod((uint8_t)(rival->profile & 1U), (uint16_t)pixels);
        queue_world_sprite(game, def, &projection, 0U);
    }
}

static void draw_player(BajanewGame *game, const BajaView *view)
{
    const BajaSim *sim = &game->sim;
    int32_t lean = (sim->steer * 12) / BAJA_FP_ONE;
    int32_t lift = (int32_t)((sim->bounce * 24) / BAJA_FP_ONE);
    int16_t x = (int16_t)(view->player_x + lean);
    int16_t y = (int16_t)(PLAYER_GROUND_Y - lift);
    uint8_t pose = POSE_NEUTRAL;

    if (sim->bounce < -(BAJA_FP_ONE / 5)) pose = POSE_AIR;
    else if (sim->bounce > BAJA_FP_ONE / 5) pose = POSE_SETTLED;
    else if (sim->steer < -(BAJA_FP_ONE / 3)) pose = POSE_LEFT;
    else if (sim->steer > (BAJA_FP_ONE / 3)) pose = POSE_RIGHT;
    else if (sim->speed < BAJA_FP_ONE / 8) pose = POSE_SETTLED;

    /* The player is the nearest thing in the scene and is placed first, so it
     * takes the top of the pool; its dust puffs sit just behind it. */
    place(game, &ng_asset_player_frames[pose], x, y, 0x0fU, 0xffU, 0U);
    if (sim->dust_event != 0U || sim->hazard_event != 0U || sim->bounce > BAJA_FP_ONE / 5) {
        uint8_t frame = (uint8_t)((game->frame >> 2) % 3U);
        place(game, &ng_asset_dust_frames[frame], (int16_t)(x - 36), (int16_t)(y + 2),
              0x0fU, 0xffU, 0U);
        place(game, &ng_asset_dust_frames[(frame + 1U) % 3U], (int16_t)(x + 36),
              (int16_t)(y + 2), 0x0fU, 0xffU, NG_RENDER_FLIP_X);
    }
}

/* The chosen racer stands full height; the other one steps back.  A frame's
 * origin is its centre-bottom and the pool anchors it there at any shrink,
 * so both portraits stand on the same line, centred on the same spot. */
static void draw_driver(BajanewGame *game, const NgSpriteFrame *frame,
                        int16_t x, uint8_t chosen)
{
    uint8_t zoom_x = chosen ? 0x0fU : 0x0bU;
    uint8_t zoom_y = (uint8_t)(((zoom_x + 1U) << 4) - 1U);
    place(game, frame, (int16_t)(x + frame->origin_x), DRIVER_BASE_Y, zoom_x, zoom_y, 0U);
}

/* ------------------------------------------------------------------- HUD -- */

#define MAP_X 250
#define MAP_Y 170

/* The route from start to finish with the car's dot on it, drawn as sprites
 * over the scene: the pool draws nearest first, so the HUD art goes in before
 * the player. */
static void draw_minimap(BajanewGame *game, uint16_t progress)
{
    uint16_t slice = (uint16_t)(((uint32_t)progress * 64U + 50U) / 100U);
    if (slice > BAJANEW_MAP_POINTS) slice = BAJANEW_MAP_POINTS;
    place(game, &ng_asset_map_dot_frames[0],
          (int16_t)(MAP_X + bajanew_map_points[slice][0]),
          (int16_t)(MAP_Y + bajanew_map_points[slice][1]), 0x0fU, 0xffU, 0U);
    place(game, &ng_asset_course_map_frames[0], MAP_X, MAP_Y, 0x0fU, 0xffU, 0U);
}

static void format_time(char *out, uint32_t frames)
{
    /* Race time without a single division: the frame counter converts
     * through two exact reciprocals. */
    uint16_t hundredths;
    uint16_t seconds;
    uint16_t minutes;
    if (frames > 13000U) frames = 13000U;
    hundredths = divide_by_three((uint16_t)(frames * 5U));
    seconds = divide_by_ten(divide_by_ten(hundredths));
    minutes = divide_by_three((uint16_t)(divide_by_ten(seconds) >> 1));
    out[0] = (char)('0' + minutes);
    out[1] = '\'';
    format_uint(&out[2], (uint16_t)(seconds - (uint16_t)(minutes * 60U)), 2, 1);
    out[4] = '"';
    format_uint(&out[5], (uint16_t)(hundredths - (uint16_t)(seconds * 100U)), 2, 1);
    out[7] = '\0';
}

/* Top row: position, race time and best leg; bottom corners: speed with its
 * rev bar and gear, and the stage with its progress.  Everything sits clear
 * of the columns the player vehicle occupies. */
static void draw_hud(BajanewGame *game)
{
    const BajaSim *sim = &game->sim;
    char text[8];
    uint16_t speed = (uint16_t)((sim->speed * 216) / BAJA_FP_ONE);
    uint16_t progress = (uint16_t)baja_fp_to_int(
        baja_fp_mul(sim->player_s, game->progress_scale));
    uint16_t revs;
    uint8_t cell;

    put_text(game, 1, 2, 0, "POS");
    format_uint(text, sim->position, 1, 1);
    put_big(game, 1, 3, 1, text);
    put_text(game, 3, 4, 0, "/4");

    put_text(game, 18, 2, 0, "TIME");
    format_time(text, sim->race_frames);
    put_big(game, 13, 3, 0, text);

    put_text(game, 31, 2, 0, "BEST LEG");
    if (game->best_frames != 0U) {
        format_time(text, game->best_frames);
        put_text(game, 32, 3, SHADE_AMBER, text);
    } else {
        put_text(game, 32, 3, 0, "-'--\"--");
    }

    format_uint(text, speed, 3, 0);
    put_big(game, 1, 25, 0, text);
    put_text(game, 7, 26, 0, "KMH");
    revs = (uint16_t)((sim->speed * 8) / BAJA_FP_ONE);
    if (revs > 6U) revs = 6U;
    put_bar(game, 1, 27, 6, (uint8_t)(revs * 8U + 4U));
    put_text(game, 8, 27, 0, "GEAR");
    put_uint(game, 13, 27, SHADE_AMBER, sim->gear, 1, 1);

    put_text(game, 31, 25, 0, "ENSENADA");
    put_text(game, 28, 26, SHADE_AMBER, "PACIFIC RUN");
    /* Course progress: a line with the car's marker on it, and the route map
     * above the stage name with the car's dot on it. */
    put_bar(game, 28, 27, 10, 80);
    cell = (uint8_t)divide_by_ten(progress);
    if (cell > 9U) cell = 9U;
    put_text(game, (int16_t)(28 + cell), 27, 0, "\x88");
    draw_minimap(game, progress);

    if (sim->surface == BAJA_SURFACE_DIRT) put_text(game, 16, 18, SHADE_AMBER, "OFF ROAD");
    else if (sim->surface == BAJA_SURFACE_SHOULDER) put_text(game, 18, 18, 0, "EDGE");

    /* Call-outs latch for half a second: a one frame event is invisible. */
    if (sim->hazard_event == BAJA_HAZARD_SOLID) { game->message_kind = 1; game->message_timer = 30; }
    else if (sim->collision_event != 0U) { game->message_kind = 2; game->message_timer = 30; }
    else if (sim->hazard_event != 0U) { game->message_kind = 3; game->message_timer = 20; }
    if (game->message_timer > 0U) {
        --game->message_timer;
        if (game->message_kind == 1U) put_text(game, 17, 16, SHADE_AMBER, "CRASH!");
        else if (game->message_kind == 2U) put_text(game, 16, 16, SHADE_AMBER, "CONTACT!");
        else put_text(game, 17, 16, 0, "SCRUB!");
    }
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
    (void)baja_project_bands_in(&game->sim, &view, bands);
    STAGE(4);
    if (level >= 4U) return;
    draw_backdrop(game, &view, bands);
    if (level >= 3U) return;
    draw_road(game, bands);
    STAGE(5);
    if (level >= 2U) return;
    if (with_actors) draw_player(game, &view);
    STAGE(6);
    draw_scenery(game, &view, bands);
    STAGE(7);
    if (level >= 1U) return;
    if (with_actors) draw_rivals(game, &view, bands);
    STAGE(8);
    draw_items(game);
}

static void draw_frame(BajanewGame *game)
{
    const BajaSim *sim = &game->sim;
    uint8_t level = bajanew_render_level;
    if (level >= 16U) level = 0U;
    if (level >= 7U) return;
    clear_next(game);
    ng_renderer_begin(&game->renderer);
    ng_pool_begin(&game->pool);
    game->item_count = 0;
    game->strip_words = 0;
    STAGE(3);
    if (level >= 6U) {
        ng_pool_end(&game->pool);
        game->last_render = ng_renderer_flush(&game->renderer);
        flush_fix(game);
        return;
    }

    switch ((BajaPhase)sim->phase) {
    case BAJA_PHASE_SPLASH:
        /* The developer mark stands alone for its five seconds and ignores
         * every input, exactly as the packet requires. */
        hide_layers(game);
        place(game, &ng_asset_splash_frames[0], 0, 0, 0x0fU, 0xffU, 0U);
        break;
    case BAJA_PHASE_TITLE:
        /* The logo is the nearest thing on the title, so it goes in first. */
        place(game, &ng_asset_logo_frames[0], BAJA_SCREEN_CENTER, 28, 0x0fU, 0xffU, 0U);
        draw_race(game, 0);
        if (level < 5U) {
            put_text(game, 11, 17, SHADE_AMBER, "ENSENADA  PACIFIC RUN");
            if (((sim->phase_frame >> 5) & 1U) == 0U) {
                put_text(game, 14, 22, 0, "PRESS START");
            }
            put_text(game, 10, 27, 0, "MAX CRUZ RACING TEAM");
        }
        break;
    case BAJA_PHASE_SELECT: {
        uint8_t is_max = (uint8_t)(sim->driver == BAJA_DRIVER_MAX);
        draw_race(game, 0);
        put_text(game, 13, 4, 0, "CHOOSE DRIVER");
        draw_driver(game, &ng_asset_driver_max_frames[0], 84, is_max);
        draw_driver(game, &ng_asset_driver_cruz_frames[0], 236, (uint8_t)!is_max);
        put_text(game, 8, 25, is_max ? SHADE_AMBER : 0U, "MAX");
        put_text(game, 6, 26, 0, "AGE 8  NO 2");
        put_text(game, 27, 25, is_max ? 0U : SHADE_AMBER, "CRUZ");
        put_text(game, 25, 26, 0, "AGE 6  NO 17");
        put_text(game, is_max ? 6 : 25, 25, 0, ">");
        put_text(game, 8, 28, 0, "LEFT RIGHT   START");
        break;
    }
    case BAJA_PHASE_COUNTDOWN: {
        uint32_t remaining = 3U - (sim->phase_frame / 60U);
        draw_race(game, 1);
        draw_hud(game);
        if (remaining > 3U) remaining = 3U;
        if (remaining > 0U) {
            char text[2];
            text[0] = (char)('0' + remaining);
            text[1] = '\0';
            put_big(game, 19, 14, 1, text);
        } else {
            put_text(game, 18, 15, SHADE_AMBER, "GO!");
        }
        break;
    }
    case BAJA_PHASE_RACING:
        draw_race(game, 1);
        draw_hud(game);
        break;
    case BAJA_PHASE_FINISHED: {
        char text[8];
        draw_race(game, 1);
        draw_hud(game);
        /* Results: where the leg was finished, how long it took, and what it
         * cost, held until the player asks for another run. */
        put_text(game, 14, 8, SHADE_AMBER, "LEG COMPLETE");
        put_text(game, 12, 10, 0, "POSITION");
        format_uint(text, sim->position, 1, 1);
        put_big(game, 22, 10, 1, text);
        put_text(game, 24, 11, 0, "/4");
        put_text(game, 12, 13, 0, "TIME");
        format_time(text, sim->race_frames);
        put_big(game, 16, 12, 0, text);
        put_text(game, 12, 15, 0, "CONTACTS");
        put_uint(game, 22, 15, SHADE_AMBER, (uint16_t)sim->collisions, 2, 0);
        put_text(game, 12, 16, 0, "CRASHES");
        put_uint(game, 22, 16, SHADE_AMBER, (uint16_t)sim->hazards, 2, 0);
        if (((sim->phase_frame >> 5) & 1U) == 0U && sim->phase_frame > 60U) {
            put_text(game, 11, 20, 0, "PRESS START TO RACE");
        }
        break;
    }
    default:
        break;
    }

    STAGE(9);
    ng_pool_end(&game->pool);
    game->last_render = ng_renderer_flush(&game->renderer);
    /* Every sprite is placed directly, so the report is the pool's and the
     * strips', not the command list's. */
    game->last_render.active_columns = (uint16_t)(BAJANEW_STRIP_SLOTS + game->pool.used);
    game->last_render.dropped_columns = game->pool.dropped_columns;
    game->last_render.vram_writes = (uint16_t)(game->pool.vram_writes + game->strip_words);
    STAGE(10);
    flush_fix(game);
    STAGE(11);
}

void bajanew_game_init(BajanewGame *game)
{
    uint8_t row;
    uint8_t column;
    uint16_t slot = 0;
    uint16_t *words = game->strip_table;
    uint8_t b;
    baja_sim_init_cooperative(&game->sim, ng_platform_kick_watchdog);
    ng_renderer_init(&game->renderer);
    ng_assets_load_palettes();
    /* Sky, ground, then every road band far to near, each in its own run of
     * hardware sprites; the renderer's objects start above them all. */
    ng_strip_init(&game->sky, slot, BACKDROP_WINDOW, ng_asset_sky_frames[0].tiles,
                  ng_asset_sky_frames[0].width_tiles,
                  (uint8_t)ng_asset_sky_frames[0].height_tiles,
                  ng_asset_sky_frames[0].palette);
    ng_strip_build_words(&game->sky, words);
    words += ng_strip_word_count(&game->sky);
    slot = (uint16_t)(slot + BACKDROP_WINDOW);
    ng_strip_init(&game->ground, slot, BACKDROP_WINDOW, ng_asset_ground_frames[0].tiles,
                  ng_asset_ground_frames[0].width_tiles,
                  (uint8_t)ng_asset_ground_frames[0].height_tiles,
                  ng_asset_ground_frames[0].palette);
    ng_strip_build_words(&game->ground, words);
    words += ng_strip_word_count(&game->ground);
    slot = (uint16_t)(slot + BACKDROP_WINDOW);
    for (b = 0; b < BAJA_ROAD_BANDS; ++b) {
        const BajanewStripDef *def = &bajanew_road_strip[b];
        ng_strip_init(&game->road[b], slot, def->window_columns, def->frame->tiles,
                      def->strip_columns, def->rows, def->palette);
        ng_strip_build_words(&game->road[b], words);
        words += ng_strip_word_count(&game->road[b]);
        ng_platform_kick_watchdog();
        ng_platform_palette_load(def->palette, bajanew_road_palette[b][0]);
        game->road_phase[b] = 0;
        slot = (uint16_t)(slot + def->window_columns);
    }
    game->road_slots = slot;
    /* The renderer keeps only the scanline accounting; the pool owns every
     * slot above the strips. */
    ng_renderer_set_first_slot(&game->renderer, NG_HW_SPRITE_CAPACITY);
    ng_pool_init(&game->pool, game->pool_cache, slot, (uint16_t)(NG_HW_SPRITE_CAPACITY - slot));
    game->item_count = 0;
    game->strip_words = 0;
    game->scenery_cursor = 0;
    game->message_kind = 0;
    game->message_timer = 0;
    game->frame = 0;
    game->sky_pan = 0;
    game->ground_pan = 0;
    /* Hundred percent of the course as a 16.16 scale, so the HUD never divides
     * by the track length. */
    game->progress_scale = baja_fp_div(baja_fp_from_int(100), game->sim.track.total_length);
    game->fix_dirty = 0xffffffffUL;
    game->fix_written = 0;
    game->best_frames = 0;
    game->initialized = 1;
    game->reserved[0] = 0;
    game->reserved[1] = 0;
    game->reserved[2] = 0;
    for (row = 0; row < BAJANEW_FIX_ROWS; ++row) {
        for (column = 0; column < BAJANEW_FIX_COLUMNS; ++column) {
            game->fix_shadow[row][column] = FIX_BLANK;
            game->fix_next[row][column] = FIX_BLANK;
        }
    }
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
    STAGE(1);
    was_racing = (uint8_t)(game->sim.phase == BAJA_PHASE_RACING);
    {
        uint8_t input = map_input(pad);
        uint8_t step;
        for (step = 0; step < BAJANEW_SIM_STEPS_PER_FRAME; ++step) {
            baja_sim_step(&game->sim, input);
        }
    }
    STAGE(2);
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
