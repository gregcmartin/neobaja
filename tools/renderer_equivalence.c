/* Differential harness for the Forge68 flush optimisation.
 *
 * Runs the real game loop and digests the whole of the host's VRAM after every
 * frame.  Built once normally and once with NG_RENDERER_NO_RUN_REUSE, the two
 * digest streams must match exactly: the optimised path is only allowed to
 * skip work the hardware would not have noticed. */
#include <stdio.h>

#include "game/bajanew.h"
#include "ng/host_platform.h"

#define FRAMES 1500U
#define VRAM_WORDS 0x8800

static uint16_t scripted_input(const BajanewGame *game, uint32_t frame)
{
    uint16_t input = 0;
    if (frame == 160U || frame == 180U) return NG_INPUT_START;
    if (frame == 170U) return NG_INPUT_RIGHT;
    if (game->sim.phase != BAJA_PHASE_RACING) return 0;
    input = NG_INPUT_A;
    if (frame >= 600U && frame < 645U) input |= NG_INPUT_RIGHT;
    else if (frame >= 900U && frame < 945U) input |= NG_INPUT_LEFT;
    else if (frame >= 1200U && frame < 1220U) input = NG_INPUT_B;
    else if (game->sim.player_e > BAJA_FP_ONE / 16) input |= NG_INPUT_LEFT;
    else if (game->sim.player_e < -(BAJA_FP_ONE / 16)) input |= NG_INPUT_RIGHT;
    return input;
}

int main(void)
{
    static BajanewGame game;
    uint32_t frame;

    ng_platform_early_init();
    bajanew_game_init(&game);
    for (frame = 0; frame < FRAMES; ++frame) {
        uint32_t digest = 2166136261UL;
        uint16_t address;
        uint8_t field;
        ng_host_set_pad(1, scripted_input(&game, frame));
        for (field = 0; field < BAJANEW_FIELDS_PER_FRAME; ++field) {
            ng_platform_wait_vblank();
        }
        bajanew_game_tick(&game, ng_platform_read_pad(1));
        for (address = 0; address < VRAM_WORDS; ++address) {
            digest ^= ng_host_vram_read(address);
            digest *= 16777619UL;
        }
        printf("%lu %08lx\n", (unsigned long)frame, (unsigned long)digest);
    }
    return 0;
}
