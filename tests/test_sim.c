#include "baja/sim.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        exit(1); \
    } \
} while (0)

static void step_many(BajaSim *sim, uint8_t input, int frames)
{
    int i;
    for (i = 0; i < frames; ++i) baja_sim_step(sim, input);
}

static void test_menus_wait_for_input(void)
{
    BajaSim sim;
    baja_sim_init(&sim);
    step_many(&sim, BAJA_INPUT_START | BAJA_INPUT_THROTTLE, 299);
    REQUIRE(sim.phase == BAJA_PHASE_SPLASH);
    baja_sim_step(&sim, 0);
    REQUIRE(sim.phase == BAJA_PHASE_TITLE);
    step_many(&sim, 0, 600);
    REQUIRE(sim.phase == BAJA_PHASE_TITLE);

    baja_sim_step(&sim, BAJA_INPUT_START);
    REQUIRE(sim.phase == BAJA_PHASE_SELECT);
    baja_sim_step(&sim, 0);
    baja_sim_step(&sim, BAJA_INPUT_RIGHT);
    REQUIRE(sim.driver == BAJA_DRIVER_CRUZ);
    step_many(&sim, 0, 500);
    REQUIRE(sim.phase == BAJA_PHASE_SELECT);
    baja_sim_step(&sim, BAJA_INPUT_START);
    REQUIRE(sim.phase == BAJA_PHASE_COUNTDOWN);
    step_many(&sim, BAJA_INPUT_THROTTLE, 179);
    REQUIRE(sim.phase == BAJA_PHASE_COUNTDOWN);
    REQUIRE(sim.speed == 0);
    baja_sim_step(&sim, 0);
    REQUIRE(sim.phase == BAJA_PHASE_RACING);
}

static void test_idle_throttle_coast_and_brake(void)
{
    BajaSim sim;
    BajaSim coast;
    BajaSim brake;
    BajaFp powered;
    baja_sim_init(&sim);
    baja_sim_begin_race(&sim);
    step_many(&sim, 0, 240);
    REQUIRE(sim.speed == 0);
    REQUIRE(sim.player_s == 0);

    step_many(&sim, BAJA_INPUT_THROTTLE, 150);
    powered = sim.speed;
    REQUIRE(powered > 0);
    REQUIRE(sim.player_s > 0);
    coast = sim;
    brake = sim;
    step_many(&coast, 0, 24);
    step_many(&brake, BAJA_INPUT_BRAKE, 24);
    REQUIRE(coast.speed < powered);
    REQUIRE(brake.speed < coast.speed);
    REQUIRE(brake.speed >= 0);
}

static void test_steering_and_return(void)
{
    BajaSim base;
    BajaSim left;
    BajaSim right;
    BajaFp held;
    baja_sim_init(&base);
    baja_sim_begin_race(&base);
    step_many(&base, BAJA_INPUT_THROTTLE, 120);
    left = base;
    right = base;
    step_many(&left, BAJA_INPUT_LEFT | BAJA_INPUT_THROTTLE, 10);
    step_many(&right, BAJA_INPUT_RIGHT | BAJA_INPUT_THROTTLE, 10);
    REQUIRE(left.player_e < base.player_e);
    REQUIRE(right.player_e > base.player_e);
    REQUIRE(left.steer < 0);
    REQUIRE(right.steer > 0);
    held = left.steer;
    step_many(&left, BAJA_INPUT_THROTTLE, 8);
    REQUIRE(abs(left.steer) < abs(held));
}

static void test_offroad_penalty(void)
{
    BajaSim road;
    BajaSim dirt;
    BajaFp dirt_start_e;
    baja_sim_init(&road);
    baja_sim_begin_race(&road);
    step_many(&road, BAJA_INPUT_THROTTLE, 130);
    dirt = road;
    dirt.player_e = BAJA_FP_ONE * 13 / 10;
    step_many(&road, BAJA_INPUT_THROTTLE, 90);
    step_many(&dirt, BAJA_INPUT_THROTTLE, 90);
    REQUIRE(dirt.surface == BAJA_SURFACE_DIRT);
    REQUIRE(dirt.speed < road.speed);
    REQUIRE(dirt.dust_event != 0);
    dirt_start_e = dirt.player_e;
    step_many(&dirt, BAJA_INPUT_LEFT | BAJA_INPUT_THROTTLE, 45);
    REQUIRE(dirt.speed > 0);
    REQUIRE(abs(dirt.player_e) < abs(dirt_start_e));
}

static void test_rivals_are_independent(void)
{
    BajaSim sim;
    BajaFp lane0;
    BajaFp lane1;
    baja_sim_init(&sim);
    baja_sim_begin_race(&sim);
    lane0 = sim.rivals[0].e;
    lane1 = sim.rivals[1].e;
    step_many(&sim, BAJA_INPUT_THROTTLE, 180);
    REQUIRE(sim.rivals[0].s != sim.rivals[1].s);
    REQUIRE(sim.rivals[0].speed != sim.rivals[1].speed);
    REQUIRE(sim.rivals[0].profile != sim.rivals[1].profile);
    REQUIRE(sim.rivals[0].e != sim.rivals[1].e);
    REQUIRE(sim.rivals[0].e != lane0 || sim.rivals[1].e != lane1);
}

static void test_collision_and_noncollision(void)
{
    BajaSim hit;
    BajaSim miss;
    BajaFp before;
    baja_sim_init(&hit);
    baja_sim_begin_race(&hit);
    step_many(&hit, BAJA_INPUT_THROTTLE, 120);
    hit.rivals[0].s = hit.player_s;
    hit.rivals[0].e = hit.player_e;
    hit.rivals[0].speed = hit.speed / 2;
    before = hit.speed;
    baja_sim_step(&hit, BAJA_INPUT_THROTTLE);
    REQUIRE(hit.collisions == 1);
    REQUIRE(hit.collision_event == 1);
    REQUIRE(hit.speed < before);

    miss = hit;
    miss.collisions = 0;
    miss.collision_cooldown = 0;
    miss.rivals[0].collision_cooldown = 0;
    miss.rivals[0].s = miss.player_s;
    miss.rivals[0].e = miss.player_e + BAJA_FP_ONE;
    baja_sim_step(&miss, BAJA_INPUT_THROTTLE);
    REQUIRE(miss.collisions == 0);
}

static void test_overtake_and_position(void)
{
    BajaSim sim;
    baja_sim_init(&sim);
    baja_sim_begin_race(&sim);
    sim.speed = BAJA_FP_ONE * 5 / 2;
    sim.rivals[0].s = sim.player_s + BAJA_FP_ONE;
    sim.rivals[0].speed = BAJA_FP_ONE / 2;
    sim.rivals[0].e = BAJA_FP_ONE / 2;
    sim.rivals[0].target_e = sim.rivals[0].e;
    step_many(&sim, BAJA_INPUT_THROTTLE, 4);
    REQUIRE(sim.overtakes >= 1);
    REQUIRE(sim.position <= 2);
}

static void test_projection(void)
{
    BajaSim sim;
    BajaRoadSample samples[BAJA_ROAD_SAMPLE_MAX];
    uint8_t count;
    uint8_t visible = 0;
    uint8_t i;
    int16_t previous_y = 224;
    baja_sim_init(&sim);
    baja_sim_begin_race(&sim);
    sim.player_s = sim.track.segment_length * 250;
    count = baja_project_road(&sim, samples, BAJA_ROAD_SAMPLE_MAX);
    REQUIRE(count == BAJA_ROAD_SAMPLE_MAX);
    REQUIRE(samples[0].half_width > samples[count - 1].half_width);
    REQUIRE(samples[0].depth < samples[count - 1].depth);
    REQUIRE(samples[0].screen_x != samples[count - 1].screen_x);
    for (i = 0; i < count; ++i) {
        if (samples[i].visible != 0) {
            REQUIRE(samples[i].screen_y < previous_y);
            previous_y = samples[i].screen_y;
            ++visible;
        }
    }
    REQUIRE(visible >= 8);
}

static void test_finish_and_restart(void)
{
    BajaSim sim;
    uint8_t i;
    baja_sim_init(&sim);
    baja_sim_begin_race(&sim);
    for (i = 0; i < BAJA_RIVAL_COUNT; ++i) sim.rivals[i].active = 0;
    sim.player_s = sim.track.total_length - BAJA_FP_ONE;
    sim.speed = BAJA_FP_ONE;
    baja_sim_step(&sim, BAJA_INPUT_THROTTLE);
    REQUIRE(sim.phase == BAJA_PHASE_FINISHED);
    REQUIRE(sim.speed == 0);
    baja_sim_step(&sim, 0);
    baja_sim_step(&sim, BAJA_INPUT_START);
    REQUIRE(sim.phase == BAJA_PHASE_COUNTDOWN);
    REQUIRE(sim.player_s == 0);
}

static uint8_t replay_input(uint32_t frame)
{
    uint8_t input = BAJA_INPUT_THROTTLE;
    if ((frame / 90u) % 3u == 1u) input |= BAJA_INPUT_LEFT;
    if ((frame / 90u) % 3u == 2u) input |= BAJA_INPUT_RIGHT;
    if (frame % 173u < 9u) input = BAJA_INPUT_BRAKE;
    return input;
}

static void test_determinism(void)
{
    BajaSim a;
    BajaSim b;
    uint32_t frame;
    baja_sim_init(&a);
    baja_sim_init(&b);
    baja_sim_begin_race(&a);
    baja_sim_begin_race(&b);
    for (frame = 0; frame < 2600; ++frame) {
        uint8_t input = replay_input(frame);
        baja_sim_step(&a, input);
        baja_sim_step(&b, input);
    }
    REQUIRE(memcmp(&a, &b, sizeof(a)) == 0);
}

int main(void)
{
    test_menus_wait_for_input();
    test_idle_throttle_coast_and_brake();
    test_steering_and_return();
    test_offroad_penalty();
    test_rivals_are_independent();
    test_collision_and_noncollision();
    test_overtake_and_position();
    test_projection();
    test_finish_and_restart();
    test_determinism();
    puts("PASS: 10 deterministic gameplay and projection tests");
    return 0;
}
