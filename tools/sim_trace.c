/* Deterministic gameplay + projection trace.  Every column is derived from the
 * immutable simulation snapshot so a diff shows exactly what the renderer saw. */
#include "baja/sim.h"

#include <stdio.h>

int main(void)
{
    BajaSim sim;
    BajaRoadBand bands[BAJA_ROAD_BANDS];
    BajaObjectProjection rival;
    uint32_t frame;

    baja_sim_init(&sim);
    baja_sim_begin_race(&sim);
    printf("frame,speed,player_s,player_e,surface,gear,position,collisions,"
           "overtakes,far_center_x,near_center_x,far_top_y,near_top_y,visible_bands,"
           "rival0_x,rival0_y,rival0_scale\n");

    for (frame = 0; frame < 1800U; ++frame) {
        uint8_t input = BAJA_INPUT_THROTTLE;
        uint8_t visible = 0;
        uint8_t i;
        if (sim.player_e > BAJA_FP_ONE / 16) input |= BAJA_INPUT_LEFT;
        else if (sim.player_e < -(BAJA_FP_ONE / 16)) input |= BAJA_INPUT_RIGHT;
        baja_sim_step(&sim, input);
        (void)baja_project_bands(&sim, bands);
        baja_project_object(&sim, sim.rivals[0].s, sim.rivals[0].e, &rival);
        for (i = 0; i < BAJA_ROAD_BANDS; ++i) visible = (uint8_t)(visible + bands[i].visible);
        printf("%lu,%ld,%ld,%ld,%u,%u,%u,%lu,%lu,%d,%d,%d,%d,%u,%d,%d,%u\n",
               (unsigned long)frame,
               (long)sim.speed, (long)sim.player_s, (long)sim.player_e,
               (unsigned)sim.surface, (unsigned)sim.gear, (unsigned)sim.position,
               (unsigned long)sim.collisions, (unsigned long)sim.overtakes,
               bands[0].center_x, bands[BAJA_ROAD_BANDS - 1].center_x,
               bands[0].top_y, bands[BAJA_ROAD_BANDS - 1].top_y, visible,
               rival.screen_x, rival.ground_y, rival.scale_q8);
    }
    return 0;
}
