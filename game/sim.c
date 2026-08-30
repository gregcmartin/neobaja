#include "sim.h"

#define SPLASH_FRAMES 300u
#define COUNTDOWN_FRAMES 240u
#define MAX_SPEED 960u
#define OFFROAD_MAX_SPEED 610u
#define FINISH_DISTANCE 330000u
#define PLAYER_EDGE 1160

static int16_t clamp_s16(int16_t value, int16_t low, int16_t high) {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

int16_t baja_curve_at(uint32_t course_pos) {
    const uint32_t phase = (course_pos / 1200u) % 192u;
    if (phase < 40u) return 0;
    if (phase < 72u) return (int16_t)((phase - 40u) * 7u);
    if (phase < 104u) return (int16_t)((104u - phase) * 7u);
    if (phase < 128u) return 0;
    if (phase < 160u) return -(int16_t)((phase - 128u) * 6u);
    return -(int16_t)((192u - phase) * 6u);
}

void baja_sim_init(BajaSim *sim) {
    *sim = (BajaSim){0};
    sim->scene = BAJA_SPLASH;
    sim->finish_pos = FINISH_DISTANCE;
    sim->rival_pos = 19000u;
    sim->rival_speed = 690u;
    sim->rival_x = 330;
    sim->position = 2u;
}

static void begin_scene(BajaSim *sim, enum BajaScene scene) {
    sim->scene = scene;
    sim->scene_frames = 0u;
}

static void reset_race(BajaSim *sim) {
    sim->race_frames = 0u;
    sim->course_pos = 0u;
    sim->rival_pos = 19000u;
    sim->speed = 0u;
    sim->rival_speed = 690u;
    sim->player_x = 0;
    sim->steer = 0;
    sim->rival_x = 330;
    sim->road_curve = 0;
    sim->collision_frames = 0u;
    sim->rough_frames = 0u;
    sim->position = 2u;
    sim->offroad = 0u;
    sim->passed_rival = 0u;
}

static void step_race(BajaSim *sim, uint8_t input) {
    const uint8_t left = (input & BAJA_IN_LEFT) != 0u;
    const uint8_t right = (input & BAJA_IN_RIGHT) != 0u;
    const uint8_t throttle = (input & BAJA_IN_A) != 0u;
    const uint8_t brake = (input & BAJA_IN_B) != 0u;
    const uint16_t speed_cap = sim->offroad ? OFFROAD_MAX_SPEED : MAX_SPEED;

    if (throttle && !brake) {
        if (sim->speed < speed_cap) {
            uint16_t gain = sim->offroad ? 2u : 5u;
            sim->speed = (uint16_t)(sim->speed + gain);
            if (sim->speed > speed_cap) sim->speed = speed_cap;
        }
    } else if (brake) {
        sim->speed = sim->speed > 13u ? (uint16_t)(sim->speed - 13u) : 0u;
    } else {
        uint16_t loss = sim->offroad ? 6u : 2u;
        sim->speed = sim->speed > loss ? (uint16_t)(sim->speed - loss) : 0u;
    }

    if (left != right) {
        sim->steer += left ? -9 : 9;
    } else if (sim->steer > 0) {
        sim->steer -= sim->steer > 7 ? 7 : sim->steer;
    } else if (sim->steer < 0) {
        int16_t recovery = sim->steer < -7 ? 7 : (int16_t)-sim->steer;
        sim->steer += recovery;
    }
    sim->steer = clamp_s16(sim->steer, -128, 128);

    if (sim->speed > 0u) {
        int16_t lateral = (int16_t)((sim->steer * (int32_t)(sim->speed + 180u)) / 6400);
        int16_t curve_pull = (int16_t)(sim->road_curve / 42);
        sim->player_x = clamp_s16((int16_t)(sim->player_x + lateral - curve_pull), -1720, 1720);
    }

    sim->offroad = (sim->player_x < -PLAYER_EDGE || sim->player_x > PLAYER_EDGE);
    if (sim->offroad) {
        sim->rough_frames++;
        if (sim->speed > OFFROAD_MAX_SPEED) sim->speed -= 7u;
    } else {
        sim->rough_frames = 0u;
    }

    sim->road_curve = baja_curve_at(sim->course_pos);
    sim->course_pos += sim->speed >> 4;

    if ((sim->race_frames % 180u) == 0u) {
        sim->rival_speed = sim->rival_speed == 690u ? 825u : 690u;
    }
    sim->rival_pos += sim->rival_speed >> 4;
    if (sim->rival_pos > sim->course_pos && sim->rival_pos - sim->course_pos < 9000u) {
        if (sim->rival_x > sim->player_x) sim->rival_x -= 3;
        else sim->rival_x += 2;
    } else {
        int16_t lane_target = ((sim->race_frames / 240u) & 1u) ? -430 : 430;
        if (sim->rival_x < lane_target) sim->rival_x += 2;
        if (sim->rival_x > lane_target) sim->rival_x -= 2;
    }
    sim->rival_x = clamp_s16(sim->rival_x, -760, 760);

    if (sim->rival_pos > sim->course_pos) {
        uint32_t dz = sim->rival_pos - sim->course_pos;
        int16_t dx = (int16_t)(sim->rival_x - sim->player_x);
        if (dz < 700u && dx > -260 && dx < 260 && sim->collision_frames == 0u) {
            sim->collision_frames = 24u;
            sim->speed = (uint16_t)((sim->speed * 3u) / 5u);
            sim->player_x += dx <= 0 ? 150 : -150;
        }
    }
    if (sim->collision_frames > 0u) sim->collision_frames--;

    if (sim->course_pos > sim->rival_pos + 500u) {
        sim->passed_rival = 1u;
        sim->position = 1u;
    } else {
        sim->position = 2u;
    }

    sim->race_frames++;
    if (sim->course_pos >= sim->finish_pos) begin_scene(sim, BAJA_FINISH);
}

void baja_sim_step(BajaSim *sim, uint8_t input) {
    const uint8_t pressed = (uint8_t)(input & (uint8_t)~sim->previous_input);

    switch (sim->scene) {
        case BAJA_SPLASH:
            if (sim->scene_frames >= SPLASH_FRAMES - 1u) begin_scene(sim, BAJA_TITLE);
            break;
        case BAJA_TITLE:
            if (pressed & BAJA_IN_START) begin_scene(sim, BAJA_SELECT);
            break;
        case BAJA_SELECT:
            if (pressed & BAJA_IN_LEFT) sim->selected_racer = 0u;
            if (pressed & BAJA_IN_RIGHT) sim->selected_racer = 1u;
            if (pressed & (BAJA_IN_A | BAJA_IN_START)) {
                reset_race(sim);
                begin_scene(sim, BAJA_COUNTDOWN);
            }
            break;
        case BAJA_COUNTDOWN:
            if (sim->scene_frames >= COUNTDOWN_FRAMES - 1u) begin_scene(sim, BAJA_RACE);
            break;
        case BAJA_RACE:
            step_race(sim, input);
            break;
        case BAJA_FINISH:
            if (pressed & (BAJA_IN_A | BAJA_IN_START)) {
                reset_race(sim);
                begin_scene(sim, BAJA_COUNTDOWN);
            }
            break;
    }

    sim->scene_frames++;
    sim->previous_input = input;
}
