/* Deterministic host run of the real game loop.
 *
 * This is the same gameplay core and the same renderer submission path the
 * cartridge uses, so the sprite budget it reports is the budget the hardware
 * sees.  It proves nothing about how the game feels; that remains Greg's live
 * MAME session. */
#include <stdio.h>

#include "game/bajanew.h"
#include "ng/host_platform.h"

#define RUN_FRAMES 3600U

typedef struct Worst {
    uint16_t active_columns;
    uint16_t peak_scanline_columns;
    uint16_t peak_scanline;
    uint16_t dropped_columns;
    uint16_t dropped_commands;
    uint16_t overloaded_scanlines;
} Worst;

/* A plain centring driver with deliberate excursions: enough to exercise the
 * off-road, braking and contact paths without pretending to be a play session. */
static uint16_t scripted_input(const BajanewGame *game, uint32_t frame)
{
    uint16_t input = 0;
    if (frame == 320U || frame == 360U) return NG_INPUT_START;
    if (frame == 340U) return NG_INPUT_RIGHT;
    if (game->sim.phase != BAJA_PHASE_RACING) return 0;

    input = NG_INPUT_A;
    if (frame >= 1200U && frame < 1290U) input |= NG_INPUT_RIGHT;
    else if (frame >= 1800U && frame < 1890U) input |= NG_INPUT_LEFT;
    else if (frame >= 2400U && frame < 2440U) input = NG_INPUT_B;
    else if (game->sim.player_e > BAJA_FP_ONE / 16) input |= NG_INPUT_LEFT;
    else if (game->sim.player_e < -(BAJA_FP_ONE / 16)) input |= NG_INPUT_RIGHT;
    return input;
}

int main(void)
{
    static BajanewGame game;
    Worst worst = {0, 0, 0, 0, 0, 0};
    uint32_t frame;
    uint32_t moving_frames = 0;
    uint32_t offroad_frames = 0;
    uint32_t dust_frames = 0;
    int32_t road_x_low = 32767;
    int32_t road_x_high = -32768;
    int32_t horizon_low = 32767;
    int32_t horizon_high = -32768;
    uint8_t phases_seen = 0;

    ng_platform_early_init();
    bajanew_game_init(&game);
    for (frame = 0; frame < RUN_FRAMES; ++frame) {
        BajaRoadBand bands[BAJA_ROAD_BANDS];
        const NgRenderStats *stats;
        ng_host_set_pad(1, scripted_input(&game, frame));
        ng_platform_wait_vblank();
        bajanew_game_tick(&game, ng_platform_read_pad(1));

        stats = &game.last_render;
        if (stats->active_columns > worst.active_columns) {
            worst.active_columns = stats->active_columns;
        }
        if (stats->peak_scanline_columns > worst.peak_scanline_columns) {
            worst.peak_scanline_columns = stats->peak_scanline_columns;
            worst.peak_scanline = stats->peak_scanline;
        }
        worst.dropped_columns = (uint16_t)(worst.dropped_columns + stats->dropped_columns);
        worst.dropped_commands = (uint16_t)(worst.dropped_commands + stats->dropped_commands);
        worst.overloaded_scanlines = (uint16_t)(worst.overloaded_scanlines +
                                                stats->overloaded_scanlines);

        phases_seen = (uint8_t)(phases_seen | (1U << game.sim.phase));
        if (game.sim.phase == BAJA_PHASE_RACING) {
            (void)baja_project_bands(&game.sim, bands);
            if (bands[0].center_x < road_x_low) road_x_low = bands[0].center_x;
            if (bands[0].center_x > road_x_high) road_x_high = bands[0].center_x;
            if (bands[0].top_y < horizon_low) horizon_low = bands[0].top_y;
            if (bands[0].top_y > horizon_high) horizon_high = bands[0].top_y;
            if (game.sim.speed > 0) ++moving_frames;
            if (game.sim.surface != BAJA_SURFACE_ROAD) ++offroad_frames;
            if (game.sim.dust_event) ++dust_frames;
        }
    }

    printf(
        "{\"frames\":%lu,\"phase\":%u,\"phases_seen\":%u,\"driver\":%u,"
        "\"speed\":%ld,\"distance\":%ld,\"position\":%u,\"collisions\":%lu,"
        "\"overtakes\":%lu,\"moving_frames\":%lu,\"offroad_frames\":%lu,"
        "\"dust_frames\":%lu,\"far_road_x\":[%ld,%ld],\"far_road_y\":[%ld,%ld],"
        "\"active_columns\":%u,\"column_capacity\":%u,"
        "\"peak_scanline_columns\":%u,\"peak_scanline\":%u,"
        "\"scanline_capacity\":%u,\"dropped_columns\":%u,"
        "\"dropped_commands\":%u,\"overloaded_scanlines\":%u}\n",
        (unsigned long)game.frame, (unsigned)game.sim.phase, (unsigned)phases_seen,
        (unsigned)game.sim.driver, (long)game.sim.speed, (long)game.sim.player_s,
        (unsigned)game.sim.position, (unsigned long)game.sim.collisions,
        (unsigned long)game.sim.overtakes, (unsigned long)moving_frames,
        (unsigned long)offroad_frames, (unsigned long)dust_frames,
        (long)road_x_low, (long)road_x_high, (long)horizon_low, (long)horizon_high,
        worst.active_columns, NG_HW_SPRITE_CAPACITY,
        worst.peak_scanline_columns, worst.peak_scanline,
        NG_SCANLINE_COLUMN_CAPACITY, worst.dropped_columns,
        worst.dropped_commands, worst.overloaded_scanlines);

    if (game.sim.driver != BAJA_DRIVER_CRUZ) return 1;
    if (worst.dropped_columns != 0U || worst.dropped_commands != 0U) return 1;
    if (worst.overloaded_scanlines != 0U) return 1;
    if (worst.active_columns == 0U || worst.active_columns > NG_HW_SPRITE_CAPACITY) return 1;
    if (moving_frames == 0U || offroad_frames == 0U || dust_frames == 0U) return 1;
    if (road_x_high - road_x_low < 40) return 1;
    return 0;
}
