#include "baja/sim.h"

#include <stdio.h>

static uint8_t input_for_frame(uint32_t frame)
{
    if (frame < 180) return BAJA_INPUT_THROTTLE;
    if (frame < 270) return BAJA_INPUT_THROTTLE | BAJA_INPUT_RIGHT;
    if (frame < 360) return BAJA_INPUT_THROTTLE | BAJA_INPUT_LEFT;
    if (frame < 390) return BAJA_INPUT_BRAKE;
    return BAJA_INPUT_THROTTLE;
}

static void emit_row(uint32_t frame, const BajaSim *sim)
{
    printf("%lu,%u,%ld,%ld,%ld,%u,%u,%ld,%ld,%ld,%ld,%lu,%lu\n",
           (unsigned long)frame,
           (unsigned)sim->phase,
           (long)sim->speed,
           (long)sim->player_s,
           (long)sim->player_e,
           (unsigned)sim->surface,
           (unsigned)sim->position,
           (long)(sim->rivals[0].s - sim->player_s),
           (long)sim->rivals[0].e,
           (long)(sim->rivals[1].s - sim->player_s),
           (long)sim->rivals[1].e,
           (unsigned long)sim->collisions,
           (unsigned long)sim->overtakes);
}

int main(void)
{
    BajaSim sim;
    uint32_t frame;
    baja_sim_init(&sim);
    baja_sim_begin_race(&sim);
    puts("frame,phase,speed_q16,distance_q16,lateral_q16,surface,position,r0_gap_q16,r0_lane_q16,r1_gap_q16,r1_lane_q16,collisions,overtakes");
    for (frame = 0; frame < 3600 && sim.phase == BAJA_PHASE_RACING; ++frame) {
        baja_sim_step(&sim, input_for_frame(frame));
        if (frame % 60u == 0u || sim.collision_event != 0) {
            emit_row(frame, &sim);
        }
    }
    emit_row(frame, &sim);
    return 0;
}
