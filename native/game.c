#include "game/bajanew.h"

#include "assets.h"
#include "ng/platform.h"

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

static void clear_next(BajanewGame *game)
{
    uint8_t row;
    uint8_t column;
    for (row = 0; row < BAJANEW_FIX_ROWS; ++row) {
        for (column = 0; column < BAJANEW_FIX_COLUMNS; ++column) {
            game->fix_next[row][column] = (uint8_t)' ';
        }
    }
}

static void put_char(BajanewGame *game, uint8_t column, uint8_t row, char value)
{
    if (column >= BAJANEW_FIX_COLUMNS || row >= BAJANEW_FIX_ROWS) return;
    game->fix_next[row][column] = (uint8_t)value;
}

static void put_text(BajanewGame *game, uint8_t column, uint8_t row, const char *text)
{
    while (*text != '\0' && column < BAJANEW_FIX_COLUMNS) {
        put_char(game, column, row, *text);
        ++column;
        ++text;
    }
}

static void put_uint(BajanewGame *game, uint8_t column, uint8_t row,
                     uint32_t value, uint8_t digits)
{
    uint8_t index;
    for (index = 0; index < digits; ++index) {
        uint8_t position = (uint8_t)(digits - index - 1U);
        put_char(game, (uint8_t)(column + position), row,
                 (char)('0' + (value % 10U)));
        value /= 10U;
    }
}

static void flush_fix(BajanewGame *game)
{
    uint8_t row;
    uint8_t column;
    for (row = 0; row < BAJANEW_FIX_ROWS; ++row) {
        for (column = 0; column < BAJANEW_FIX_COLUMNS; ++column) {
            uint8_t next = game->fix_next[row][column];
            if (game->fix_shadow[row][column] != next) {
                game->fix_shadow[row][column] = next;
                ng_fix_putc(column, row, 0, (char)next);
            }
        }
    }
}

static const BajaRoadSample *sample_for_y(const BajaRoadSample *samples,
                                           uint8_t count, int16_t target_y)
{
    const BajaRoadSample *best = 0;
    int16_t best_distance = 32767;
    uint8_t i;
    for (i = 0; i < count; ++i) {
        int16_t distance;
        if (samples[i].visible == 0U) continue;
        distance = (int16_t)(samples[i].screen_y - target_y);
        if (distance < 0) distance = (int16_t)-distance;
        if (distance < best_distance) {
            best_distance = distance;
            best = &samples[i];
        }
    }
    return best;
}

static void draw_wire_road(BajanewGame *game)
{
    BajaRoadSample samples[BAJA_ROAD_SAMPLE_MAX];
    uint8_t count = baja_project_road(&game->sim, samples, BAJA_ROAD_SAMPLE_MAX);
    uint8_t row;

    put_text(game, 0, 7, "~~~~~~~~~~~~ ENSENADA ~~~~~~~~~~~~~");
    for (row = 8; row < 28; ++row) {
        const BajaRoadSample *sample = sample_for_y(samples, count, (int16_t)(row * 8 + 4));
        int16_t center;
        int16_t half;
        int16_t left;
        int16_t right;
        uint8_t column;
        char terrain;
        char road;
        if (sample == 0) continue;
        center = (int16_t)(sample->screen_x / 8);
        half = (int16_t)(sample->half_width / 8);
        if (half < 1) half = 1;
        left = (int16_t)(center - half);
        right = (int16_t)(center + half);
        terrain = sample->shade != 0U ? '.' : '\'';
        road = sample->shade != 0U ? '=' : '-';
        for (column = 0; column < BAJANEW_FIX_COLUMNS; ++column) {
            int16_t x = (int16_t)column;
            char pixel = terrain;
            if (x >= left && x <= right) pixel = road;
            if (x == left || x == right) pixel = '#';
            if (x == center && (sample->segment & 2U) == 0U) pixel = ':';
            put_char(game, column, row, pixel);
        }
    }
}

static void draw_hud(BajanewGame *game)
{
    uint32_t speed = (uint32_t)((game->sim.speed * 100) / BAJA_FP_ONE);
    uint32_t progress = (uint32_t)(((int64_t)game->sim.player_s * 100) /
                                   game->sim.track.total_length);
    put_text(game, 1, 1, "SPD");
    put_uint(game, 5, 1, speed, 3);
    put_text(game, 10, 1, "POS");
    put_uint(game, 14, 1, game->sim.position, 1);
    put_text(game, 16, 1, "/3");
    put_text(game, 21, 1, "STAGE");
    put_uint(game, 27, 1, progress, 3);
    put_char(game, 30, 1, '%');
    put_text(game, 1, 3, game->sim.driver == BAJA_DRIVER_MAX ? "MAX #2" : "CRUZ #17");
    put_text(game, 26, 3, "A GO  B BRAKE");
    if (game->sim.surface == BAJA_SURFACE_DIRT) put_text(game, 16, 3, "DIRT");
    else if (game->sim.surface == BAJA_SURFACE_SHOULDER) put_text(game, 16, 3, "EDGE");
    if (game->sim.collision_event != 0U) put_text(game, 15, 5, "CONTACT!");
}

static void draw_ui(BajanewGame *game)
{
    clear_next(game);
    switch ((BajaPhase)game->sim.phase) {
    case BAJA_PHASE_SPLASH:
        put_text(game, 10, 12, "GREG MARTIN PRESENTS");
        put_text(game, 13, 15, "BAJA 2.5D");
        put_text(game, 8, 19, "CLEAN-ROOM PROGRAMMING BUILD");
        break;
    case BAJA_PHASE_TITLE:
        put_text(game, 12, 9, "BAJA 2.5D");
        put_text(game, 9, 13, "ENSENADA VERTICAL SLICE");
        put_text(game, 13, 20, "PRESS START");
        break;
    case BAJA_PHASE_SELECT:
        put_text(game, 11, 7, "CHOOSE DRIVER");
        put_text(game, 5, 13, game->sim.driver == BAJA_DRIVER_MAX ? "> MAX  #2  RED" : "  MAX  #2  RED");
        put_text(game, 5, 17, game->sim.driver == BAJA_DRIVER_CRUZ ? "> CRUZ #17 BLUE" : "  CRUZ #17 BLUE");
        put_text(game, 6, 23, "LEFT/RIGHT   START CONFIRMS");
        break;
    case BAJA_PHASE_COUNTDOWN: {
        uint32_t remaining = 3U - (game->sim.phase_frame / 60U);
        draw_wire_road(game);
        draw_hud(game);
        if (remaining > 3U) remaining = 3U;
        put_text(game, 17, 13, "READY");
        put_uint(game, 19, 16, remaining, 1);
        break;
    }
    case BAJA_PHASE_RACING:
        draw_wire_road(game);
        draw_hud(game);
        break;
    case BAJA_PHASE_FINISHED:
        draw_wire_road(game);
        draw_hud(game);
        put_text(game, 14, 12, "FINISH!");
        put_text(game, 11, 16, "PRESS START AGAIN");
        break;
    default:
        break;
    }
    flush_fix(game);
}

static void submit_sprite(NgRenderer *renderer, const NgSpriteFrame *frame,
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

static void draw_sprites(BajanewGame *game)
{
    ng_renderer_begin(&game->renderer);
    if (game->sim.phase == BAJA_PHASE_RACING ||
        game->sim.phase == BAJA_PHASE_COUNTDOWN ||
        game->sim.phase == BAJA_PHASE_FINISHED) {
        uint8_t i;
        for (i = 0; i < BAJA_RIVAL_COUNT; ++i) {
            BajaObjectProjection projected;
            baja_project_object(&game->sim, game->sim.rivals[i].s,
                                game->sim.rivals[i].e, &projected);
            if (projected.visible != 0U) {
                submit_sprite(&game->renderer,
                              &ng_asset_drone_frames[(game->frame >> 4) & 1U],
                              projected.screen_x, projected.screen_y,
                              (uint8_t)(10U + i), projected.zoom_x,
                              projected.zoom_y, i == 0U ? 0U : NG_RENDER_FLIP_X);
            }
        }
        submit_sprite(&game->renderer,
                      &ng_asset_ranger_frames[(game->frame >> 3) & 3U],
                      160, 216, 60, 0x0fU, 0xffU,
                      game->sim.steer < 0 ? NG_RENDER_FLIP_X : 0U);
    } else {
        submit_sprite(&game->renderer, &ng_asset_spark_frames[(game->frame >> 3) & 3U],
                      160, 126, 30, 0x0fU, 0xffU, 0U);
        submit_sprite(&game->renderer, &ng_asset_ranger_frames[(game->frame >> 4) & 3U],
                      160, 214, 40, 0x0fU, 0xffU, 0U);
        submit_sprite(&game->renderer, &ng_asset_drone_frames[(game->frame >> 5) & 1U],
                      72, 182, 35, 0x0fU, 0xffU, 0U);
        submit_sprite(&game->renderer, &ng_asset_drone_frames[(game->frame >> 5) & 1U],
                      248, 182, 35, 0x0fU, 0xffU, NG_RENDER_FLIP_X);
    }
    game->last_render = ng_renderer_flush(&game->renderer);
}

void bajanew_game_init(BajanewGame *game)
{
    uint8_t row;
    uint8_t column;
    baja_sim_init_cooperative(&game->sim, ng_platform_kick_watchdog);
    ng_renderer_init(&game->renderer);
    game->last_scene.submitted = 0;
    game->last_scene.culled = 0;
    game->last_scene.chunk_index = 0;
    game->last_scene.missing_chunk = 0;
    game->frame = 0;
    game->initialized = 1;
    for (row = 0; row < BAJANEW_FIX_ROWS; ++row) {
        for (column = 0; column < BAJANEW_FIX_COLUMNS; ++column) {
            game->fix_shadow[row][column] = (uint8_t)' ';
            game->fix_next[row][column] = (uint8_t)' ';
        }
    }
    ng_assets_load_palettes();
    ng_platform_backdrop(0x8000U);
    ng_fix_clear();
    draw_ui(game);
    draw_sprites(game);
}

void bajanew_game_set_autoplay(BajanewGame *game, uint8_t enabled)
{
    (void)game;
    (void)enabled;
}

void bajanew_game_tick(BajanewGame *game, NgPad pad)
{
    if (game->initialized == 0U) return;
    baja_sim_step(&game->sim, map_input(pad));
    ++game->frame;
    draw_ui(game);
    draw_sprites(game);
}
