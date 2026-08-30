#ifndef BAJA_SIM_H
#define BAJA_SIM_H

#include <stdint.h>

#define BAJA_FP_SHIFT 16
#define BAJA_FP_ONE ((int32_t)1 << BAJA_FP_SHIFT)
#define BAJA_TRACK_SEGMENTS 384
#define BAJA_RIVAL_COUNT 2
#define BAJA_ROAD_SAMPLE_MAX 32

typedef int32_t BajaFp;

typedef enum BajaPhase {
    BAJA_PHASE_SPLASH = 0,
    BAJA_PHASE_TITLE,
    BAJA_PHASE_SELECT,
    BAJA_PHASE_COUNTDOWN,
    BAJA_PHASE_RACING,
    BAJA_PHASE_FINISHED
} BajaPhase;

typedef enum BajaDriver {
    BAJA_DRIVER_MAX = 0,
    BAJA_DRIVER_CRUZ = 1
} BajaDriver;

typedef enum BajaSurface {
    BAJA_SURFACE_ROAD = 0,
    BAJA_SURFACE_SHOULDER,
    BAJA_SURFACE_DIRT
} BajaSurface;

enum BajaInput {
    BAJA_INPUT_LEFT = 1u << 0,
    BAJA_INPUT_RIGHT = 1u << 1,
    BAJA_INPUT_THROTTLE = 1u << 2,
    BAJA_INPUT_BRAKE = 1u << 3,
    BAJA_INPUT_START = 1u << 4
};

typedef struct BajaTrack {
    BajaFp center_x[BAJA_TRACK_SEGMENTS + 1];
    BajaFp height[BAJA_TRACK_SEGMENTS + 1];
    BajaFp curvature[BAJA_TRACK_SEGMENTS];
    BajaFp grade[BAJA_TRACK_SEGMENTS];
    BajaFp segment_length;
    BajaFp total_length;
} BajaTrack;

typedef struct BajaRival {
    BajaFp s;
    BajaFp e;
    BajaFp speed;
    BajaFp preferred_speed;
    BajaFp target_e;
    uint16_t decision_timer;
    uint16_t collision_cooldown;
    uint8_t profile;
    uint8_t was_ahead;
    uint8_t active;
    uint8_t reserved;
} BajaRival;

typedef struct BajaSim {
    BajaTrack track;
    BajaFp player_s;
    BajaFp player_e;
    BajaFp speed;
    BajaFp steer;
    BajaRival rivals[BAJA_RIVAL_COUNT];
    uint32_t frame;
    uint32_t phase_frame;
    uint32_t collisions;
    uint32_t overtakes;
    uint16_t collision_cooldown;
    uint8_t phase;
    uint8_t driver;
    uint8_t surface;
    uint8_t position;
    uint8_t previous_input;
    uint8_t collision_event;
    uint8_t dust_event;
    uint8_t reserved;
} BajaSim;

typedef struct BajaRoadSample {
    int16_t screen_x;
    int16_t screen_y;
    int16_t half_width;
    uint16_t depth;
    uint16_t segment;
    uint8_t visible;
    uint8_t shade;
} BajaRoadSample;

BajaFp baja_fp_from_int(int32_t value);
int32_t baja_fp_to_int(BajaFp value);
BajaFp baja_fp_mul(BajaFp a, BajaFp b);
BajaFp baja_fp_div(BajaFp a, BajaFp b);

void baja_track_init(BajaTrack *track);
void baja_track_sample(const BajaTrack *track, BajaFp s, BajaFp *x,
                       BajaFp *y, BajaFp *curve);

void baja_sim_init(BajaSim *sim);
void baja_sim_step(BajaSim *sim, uint8_t input);
void baja_sim_begin_race(BajaSim *sim);

uint8_t baja_project_road(const BajaSim *sim, BajaRoadSample *samples,
                          uint8_t capacity);

#endif
