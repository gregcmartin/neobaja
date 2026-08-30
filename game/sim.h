#ifndef BAJANEW_SIM_H
#define BAJANEW_SIM_H

#include <stdint.h>

enum BajaScene {
    BAJA_SPLASH = 0,
    BAJA_TITLE,
    BAJA_SELECT,
    BAJA_COUNTDOWN,
    BAJA_RACE,
    BAJA_FINISH
};

enum BajaInput {
    BAJA_IN_LEFT  = 1u << 0,
    BAJA_IN_RIGHT = 1u << 1,
    BAJA_IN_A     = 1u << 2,
    BAJA_IN_B     = 1u << 3,
    BAJA_IN_START = 1u << 4
};

typedef struct BajaSim {
    enum BajaScene scene;
    uint32_t scene_frames;
    uint32_t race_frames;
    uint32_t course_pos;
    uint32_t rival_pos;
    uint32_t finish_pos;
    uint16_t speed;
    uint16_t rival_speed;
    int16_t player_x;
    int16_t steer;
    int16_t rival_x;
    int16_t road_curve;
    uint16_t collision_frames;
    uint16_t rough_frames;
    uint8_t selected_racer;
    uint8_t position;
    uint8_t offroad;
    uint8_t passed_rival;
    uint8_t previous_input;
} BajaSim;

void baja_sim_init(BajaSim *sim);
void baja_sim_step(BajaSim *sim, uint8_t input);
int16_t baja_curve_at(uint32_t course_pos);

#endif
