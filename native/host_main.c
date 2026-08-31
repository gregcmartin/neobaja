#include <stdio.h>

#include "game/bajanew.h"
#include "ng/host_platform.h"

static uint16_t scripted_input(uint32_t frame)
{
    if (frame == 310U || frame == 330U) return NG_INPUT_START;
    if (frame == 320U) return NG_INPUT_RIGHT;
    if (frame >= 520U && frame < 1040U) {
        uint16_t input = NG_INPUT_A;
        if (frame >= 680U && frame < 740U) input |= NG_INPUT_RIGHT;
        if (frame >= 820U && frame < 880U) input |= NG_INPUT_LEFT;
        return input;
    }
    if (frame >= 1040U && frame < 1080U) return NG_INPUT_B;
    return 0;
}

int main(void)
{
    BajanewGame game;
    uint32_t frame;
    ng_platform_early_init();
    bajanew_game_init(&game);
    for (frame = 0; frame < 1200U; ++frame) {
        ng_host_set_pad(1, scripted_input(frame));
        ng_platform_wait_vblank();
        bajanew_game_tick(&game, ng_platform_read_pad(1));
    }
    printf(
        "{\"frames\":%lu,\"phase\":%u,\"driver\":%u,\"speed\":%ld,"
        "\"distance\":%ld,\"position\":%u,\"collisions\":%lu,"
        "\"overtakes\":%lu,\"columns\":%u,\"dropped_columns\":%u,"
        "\"overloaded_scanlines\":%u}\n",
        (unsigned long)game.frame,
        (unsigned)game.sim.phase,
        (unsigned)game.sim.driver,
        (long)game.sim.speed,
        (long)game.sim.player_s,
        (unsigned)game.sim.position,
        (unsigned long)game.sim.collisions,
        (unsigned long)game.sim.overtakes,
        game.last_render.active_columns,
        game.last_render.dropped_columns,
        game.last_render.overloaded_scanlines);
    if (game.sim.phase != BAJA_PHASE_RACING ||
        game.sim.driver != BAJA_DRIVER_CRUZ ||
        game.sim.player_s <= 0 || game.last_render.active_columns == 0U ||
        game.last_render.dropped_columns != 0U ||
        game.last_render.overloaded_scanlines != 0U) {
        return 1;
    }
    return 0;
}
