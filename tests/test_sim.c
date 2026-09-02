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

static int checks = 0;

static void step_many(BajaSim *sim, uint8_t input, int frames)
{
    int i;
    for (i = 0; i < frames; ++i) baja_sim_step(sim, input);
}

/* A plain centring driver: throttle, and steer toward the road centre.  It is
 * the least skilled input that still counts as driving, not a tuned script. */
static void drive_frames(BajaSim *sim, int frames)
{
    int i;
    for (i = 0; i < frames; ++i) {
        uint8_t input = BAJA_INPUT_THROTTLE;
        if (sim->player_e > BAJA_FP_ONE / 16) input |= BAJA_INPUT_LEFT;
        else if (sim->player_e < -(BAJA_FP_ONE / 16)) input |= BAJA_INPUT_RIGHT;
        baja_sim_step(sim, input);
    }
}

static void race_at_speed(BajaSim *sim, int frames)
{
    baja_sim_init(sim);
    baja_sim_begin_race(sim);
    step_many(sim, BAJA_INPUT_THROTTLE, frames);
}

static void test_menus_wait_for_input(void)
{
    BajaSim sim;
    baja_sim_init(&sim);
    step_many(&sim, BAJA_INPUT_START | BAJA_INPUT_THROTTLE, 299);
    REQUIRE(sim.phase == BAJA_PHASE_SPLASH);
    baja_sim_step(&sim, 0);
    REQUIRE(sim.phase == BAJA_PHASE_TITLE);
    step_many(&sim, 0, 900);
    REQUIRE(sim.phase == BAJA_PHASE_TITLE);

    baja_sim_step(&sim, BAJA_INPUT_START);
    REQUIRE(sim.phase == BAJA_PHASE_SELECT);
    baja_sim_step(&sim, 0);
    baja_sim_step(&sim, BAJA_INPUT_RIGHT);
    REQUIRE(sim.driver == BAJA_DRIVER_CRUZ);
    step_many(&sim, 0, 900);
    REQUIRE(sim.phase == BAJA_PHASE_SELECT);
    REQUIRE(sim.driver == BAJA_DRIVER_CRUZ);
    baja_sim_step(&sim, BAJA_INPUT_START);
    REQUIRE(sim.phase == BAJA_PHASE_COUNTDOWN);
    step_many(&sim, BAJA_INPUT_THROTTLE, 209);
    REQUIRE(sim.phase == BAJA_PHASE_COUNTDOWN);
    REQUIRE(sim.speed == 0);
    REQUIRE(sim.player_s == 0);
    baja_sim_step(&sim, 0);
    REQUIRE(sim.phase == BAJA_PHASE_RACING);
    ++checks;
}

static void test_idle_throttle_coast_and_brake(void)
{
    BajaSim sim;
    BajaSim coast;
    BajaSim brake;
    BajaFp powered;
    baja_sim_init(&sim);
    baja_sim_begin_race(&sim);
    step_many(&sim, 0, 300);
    REQUIRE(sim.speed == 0);
    REQUIRE(sim.player_s == 0);

    step_many(&sim, BAJA_INPUT_THROTTLE, 240);
    powered = sim.speed;
    REQUIRE(powered > 0);
    REQUIRE(sim.player_s > 0);
    REQUIRE(sim.gear > 1);
    coast = sim;
    brake = sim;
    step_many(&coast, 0, 30);
    step_many(&brake, BAJA_INPUT_BRAKE, 30);
    REQUIRE(coast.speed < powered);
    REQUIRE(brake.speed < coast.speed);
    REQUIRE(brake.speed >= 0);
    step_many(&brake, BAJA_INPUT_BRAKE, 600);
    REQUIRE(brake.speed == 0);
    ++checks;
}

static void test_steering_and_return(void)
{
    BajaSim base;
    BajaSim left;
    BajaSim right;
    BajaSim tap;
    BajaFp held;
    race_at_speed(&base, 200);
    left = base;
    right = base;
    tap = base;
    step_many(&left, BAJA_INPUT_LEFT, 30);
    step_many(&right, BAJA_INPUT_RIGHT, 30);
    REQUIRE(left.player_e < base.player_e);
    REQUIRE(right.player_e > base.player_e);

    /* A brief tap is a correction, not a lane change. */
    step_many(&tap, BAJA_INPUT_RIGHT, 6);
    REQUIRE(tap.player_e > base.player_e);
    REQUIRE(tap.player_e < right.player_e / 2);

    held = right.steer;
    REQUIRE(held > 0);
    step_many(&right, 0, 20);
    REQUIRE(right.steer == 0);
    ++checks;
}

static void test_offroad_penalty_is_recoverable(void)
{
    BajaSim road;
    BajaSim dirt;
    race_at_speed(&road, 400);
    dirt = road;
    step_many(&dirt, BAJA_INPUT_THROTTLE | BAJA_INPUT_RIGHT, 120);
    REQUIRE(dirt.surface == BAJA_SURFACE_DIRT);
    REQUIRE(dirt.speed < road.speed);
    REQUIRE(dirt.dust_event != 0);
    REQUIRE(dirt.rough_timer > 0);
    /* Steering back returns the player to the road and to road pace. */
    step_many(&dirt, BAJA_INPUT_THROTTLE | BAJA_INPUT_LEFT, 120);
    REQUIRE(dirt.surface == BAJA_SURFACE_ROAD);
    drive_frames(&dirt, 180);
    REQUIRE(dirt.speed > road.speed / 2);
    REQUIRE(dirt.speed > BAJA_FP_ONE / 2);
    ++checks;
}

static void test_curve_pushes_outward_and_is_holdable(void)
{
    BajaSim sim;
    BajaFp curve = 0;
    BajaFp drift;
    BajaSim steered;
    int i;
    race_at_speed(&sim, 400);
    /* Find a segment with real curvature and measure the uncorrected drift. */
    for (i = 0; i < 4000; ++i) {
        baja_track_sample(&sim.track, sim.player_s, NULL, NULL, &curve);
        if (curve > BAJA_FP_ONE / 8) break;
        baja_sim_step(&sim, BAJA_INPUT_THROTTLE);
    }
    REQUIRE(curve > BAJA_FP_ONE / 8);
    steered = sim;
    drift = sim.player_e;
    step_many(&sim, BAJA_INPUT_THROTTLE, 45);
    REQUIRE(sim.player_e < drift);
    /* Holding steering into the bend beats the push. */
    step_many(&steered, BAJA_INPUT_THROTTLE | BAJA_INPUT_RIGHT, 45);
    REQUIRE(steered.player_e > sim.player_e);
    ++checks;
}

static void test_road_bands_form_a_funnel(void)
{
    BajaSim sim;
    BajaRoadBand bands[BAJA_ROAD_BANDS];
    uint8_t count;
    uint8_t i;
    int visible = 0;
    int16_t previous_top = -1;

    race_at_speed(&sim, 300);
    count = baja_project_bands(&sim, bands);
    REQUIRE(count == BAJA_ROAD_BANDS);
    for (i = 0; i < count; ++i) {
        if (!bands[i].visible) continue;
        ++visible;
        REQUIRE(bands[i].height > 0);
        REQUIRE(bands[i].top_y >= 0);
        REQUIRE(bands[i].top_y < BAJA_SCREEN_HEIGHT);
        /* Nearer bands are lower on screen and never overlap a farther one. */
        REQUIRE(bands[i].top_y > previous_top);
        previous_top = bands[i].top_y;
    }
    REQUIRE(visible >= BAJA_ROAD_BANDS - 4);
    /* The funnel widens monotonically toward the camera. */
    for (i = 1; i < BAJA_ROAD_BANDS; ++i) {
        REQUIRE(baja_band_half_width[i] > baja_band_half_width[i - 1]);
    }
    REQUIRE(baja_band_dy[0] > 0);
    REQUIRE(baja_band_dy[BAJA_ROAD_BANDS] == BAJA_SCREEN_HEIGHT - BAJA_HORIZON_Y);
    ++checks;
}

static void test_bands_track_curve_and_surface_phase(void)
{
    BajaSim sim;
    BajaRoadBand straight[BAJA_ROAD_BANDS];
    BajaRoadBand curved[BAJA_ROAD_BANDS];
    BajaFp curve = 0;
    int i;
    int phase_changes = 0;
    uint8_t previous;

    race_at_speed(&sim, 60);
    /* Compare from the road centre so the measurement isolates curvature from
     * the player's own lateral offset. */
    sim.player_e = 0;
    (void)baja_project_bands(&sim, straight);
    REQUIRE(straight[0].center_x == BAJA_SCREEN_CENTER);
    REQUIRE(straight[BAJA_ROAD_BANDS - 1].center_x == BAJA_SCREEN_CENTER);
    for (i = 0; i < 4000; ++i) {
        baja_track_sample(&sim.track, sim.player_s, NULL, NULL, &curve);
        if (curve > BAJA_FP_ONE / 4) break;
        drive_frames(&sim, 1);
    }
    REQUIRE(curve > BAJA_FP_ONE / 4);
    sim.player_e = 0;
    (void)baja_project_bands(&sim, curved);
    /* A bend swings the distant road far off centre while the road under the
     * camera stays put. */
    REQUIRE(curved[0].center_x > BAJA_SCREEN_CENTER + 24);
    REQUIRE(curved[BAJA_ROAD_BANDS - 1].center_x == BAJA_SCREEN_CENTER);
    for (i = 1; i < BAJA_ROAD_BANDS; ++i) {
        REQUIRE(curved[i].center_x <= curved[i - 1].center_x);
    }

    /* Every band streams its surface phase, with the wavelength growing with
     * depth so the far road does not flicker. */
    previous = curved[3].phase;
    for (i = 0; i < 120; ++i) {
        BajaRoadBand bands[BAJA_ROAD_BANDS];
        uint8_t b;
        drive_frames(&sim, 1);
        (void)baja_project_bands(&sim, bands);
        if (bands[3].phase != previous) ++phase_changes;
        previous = bands[3].phase;
        for (b = 0; b < BAJA_ROAD_BANDS; ++b) {
            if (baja_band_stripe_shift[b] == 0U) REQUIRE(bands[b].phase == 0U);
        }
    }
    REQUIRE(phase_changes >= 2);
    REQUIRE(baja_band_stripe_shift[0] > baja_band_stripe_shift[BAJA_ROAD_BANDS - 1]);
    for (i = 0; i < BAJA_ROAD_BANDS; ++i) REQUIRE(baja_band_stripe_shift[i] > 0U);
    ++checks;
}

static void test_hills_occlude_distant_road(void)
{
    BajaSim sim;
    BajaRoadBand bands[BAJA_ROAD_BANDS];
    int i;
    int hidden_seen = 0;

    race_at_speed(&sim, 200);
    for (i = 0; i < 4000; ++i) {
        uint8_t b;
        uint8_t hidden = 0;
        baja_sim_step(&sim, BAJA_INPUT_THROTTLE);
        (void)baja_project_bands(&sim, bands);
        for (b = 0; b < BAJA_ROAD_BANDS; ++b) {
            if (!bands[b].visible) ++hidden;
        }
        if (hidden > 0) ++hidden_seen;
    }
    /* A crest must hide road behind it at some point on this course. */
    REQUIRE(hidden_seen > 0);
    ++checks;
}

static void test_object_projection_matches_the_road(void)
{
    BajaSim sim;
    BajaObjectProjection near_object;
    BajaObjectProjection far_object;
    BajaObjectProjection behind;
    BajaObjectProjection left_object;
    BajaObjectProjection right_object;

    race_at_speed(&sim, 300);
    baja_project_object(&sim, sim.player_s + baja_fp_from_int(10), 0, &near_object);
    baja_project_object(&sim, sim.player_s + baja_fp_from_int(90), 0, &far_object);
    REQUIRE(near_object.visible);
    REQUIRE(far_object.visible);
    /* Distance shrinks the object and lifts it toward the horizon. */
    REQUIRE(near_object.scale_q8 > far_object.scale_q8);
    REQUIRE(near_object.ground_y > far_object.ground_y);
    REQUIRE(far_object.ground_y > BAJA_HORIZON_Y);
    REQUIRE(near_object.band > far_object.band);

    baja_project_object(&sim, sim.player_s - baja_fp_from_int(20), 0, &behind);
    REQUIRE(!behind.visible);

    baja_project_object(&sim, sim.player_s + baja_fp_from_int(20),
                        -BAJA_FP_ONE, &left_object);
    baja_project_object(&sim, sim.player_s + baja_fp_from_int(20),
                        BAJA_FP_ONE, &right_object);
    REQUIRE(left_object.screen_x < right_object.screen_x);
    ++checks;
}

static void test_rivals_are_independent(void)
{
    BajaSim sim;
    int i;
    int lane_moves[BAJA_RIVAL_COUNT];
    BajaFp last_e[BAJA_RIVAL_COUNT];
    uint8_t r;

    race_at_speed(&sim, 30);
    for (r = 0; r < BAJA_RIVAL_COUNT; ++r) {
        lane_moves[r] = 0;
        last_e[r] = sim.rivals[r].e;
    }
    for (i = 0; i < 4200; ++i) {
        drive_frames(&sim, 1);
        for (r = 0; r < BAJA_RIVAL_COUNT; ++r) {
            if (sim.rivals[r].e != last_e[r]) ++lane_moves[r];
            last_e[r] = sim.rivals[r].e;
        }
    }
    for (r = 0; r < BAJA_RIVAL_COUNT; ++r) {
        REQUIRE(lane_moves[r] > 60);
        REQUIRE(sim.rivals[r].speed > 0);
    }
    /* No two rivals share a path. */
    REQUIRE(sim.rivals[0].s != sim.rivals[1].s);
    REQUIRE(sim.rivals[1].s != sim.rivals[2].s);
    REQUIRE(sim.rivals[0].e != sim.rivals[1].e);
    REQUIRE(sim.overtakes > 0);
    REQUIRE(sim.position >= 1 && sim.position <= BAJA_RIVAL_COUNT + 1);
    ++checks;
}

static void test_collision_and_non_collision(void)
{
    BajaSim hit;
    BajaSim clear;
    BajaFp before;

    race_at_speed(&hit, 400);
    clear = hit;

    hit.rivals[0].s = hit.player_s + BAJA_FP_ONE;
    hit.rivals[0].e = hit.player_e;
    before = hit.speed;
    baja_sim_step(&hit, BAJA_INPUT_THROTTLE);
    REQUIRE(hit.collision_event != 0);
    REQUIRE(hit.collisions == 1);
    REQUIRE(hit.speed < before);
    REQUIRE(hit.speed > 0);
    /* Contact must not take control away. */
    step_many(&hit, BAJA_INPUT_THROTTLE, 120);
    REQUIRE(hit.speed > before / 2);

    clear.rivals[0].s = clear.player_s + baja_fp_from_int(40);
    clear.rivals[1].s = clear.player_s + baja_fp_from_int(60);
    clear.rivals[2].s = clear.player_s + baja_fp_from_int(80);
    baja_sim_step(&clear, BAJA_INPUT_THROTTLE);
    REQUIRE(clear.collision_event == 0);
    REQUIRE(clear.collisions == 0);
    ++checks;
}

static void test_race_lifecycle_and_restart(void)
{
    BajaSim sim;
    int i;
    race_at_speed(&sim, 10);
    for (i = 0; i < 40000 && sim.phase == BAJA_PHASE_RACING; ++i) {
        drive_frames(&sim, 1);
    }
    REQUIRE(sim.phase == BAJA_PHASE_FINISHED);
    REQUIRE(sim.player_s == sim.track.total_length);
    REQUIRE(sim.race_frames > 60U * 45U);
    step_many(&sim, 0, 90);
    baja_sim_step(&sim, BAJA_INPUT_START);
    REQUIRE(sim.phase == BAJA_PHASE_COUNTDOWN);
    REQUIRE(sim.player_s == 0);
    REQUIRE(sim.speed == 0);
    REQUIRE(sim.collisions == 0);
    ++checks;
}

static void test_deterministic_replay(void)
{
    BajaSim a;
    BajaSim b;
    int i;
    baja_sim_init(&a);
    baja_sim_init(&b);
    for (i = 0; i < 3000; ++i) {
        uint8_t input = (uint8_t)(((i / 7) & 1) ? BAJA_INPUT_THROTTLE : 0);
        if ((i % 53) == 0) input |= BAJA_INPUT_START;
        if ((i % 31) == 0) input |= BAJA_INPUT_LEFT;
        if ((i % 37) == 0) input |= BAJA_INPUT_RIGHT;
        if ((i % 97) == 0) input |= BAJA_INPUT_BRAKE;
        baja_sim_step(&a, input);
        baja_sim_step(&b, input);
    }
    REQUIRE(memcmp(&a, &b, sizeof(a)) == 0);
    ++checks;
}

static void test_scenery_sits_off_the_racing_line(void)
{
    BajaSim sim;
    uint16_t i;
    int kinds[BAJA_SCENERY_KINDS];
    int distinct = 0;
    baja_sim_init(&sim);
    for (i = 0; i < BAJA_SCENERY_KINDS; ++i) kinds[i] = 0;
    for (i = 0; i < BAJA_SCENERY_COUNT; ++i) {
        BajaFp e = sim.scenery[i].e;
        uint8_t kind = sim.scenery[i].kind;
        REQUIRE(kind < BAJA_SCENERY_KINDS);
        /* Gantries stand over the road; everything else stays off it. */
        if (kind == BAJA_SCENERY_GANTRY_START || kind == BAJA_SCENERY_GANTRY_FINISH) {
            REQUIRE(e == 0);
        } else {
            REQUIRE(e > BAJA_FP_ONE || e < -BAJA_FP_ONE);
        }
        REQUIRE(sim.scenery[i].s >= 0);
        if (sim.scenery[i].s < sim.track.total_length) kinds[kind] = 1;
        if (i > 0) REQUIRE(sim.scenery[i].s > sim.scenery[i - 1].s);
    }
    for (i = 0; i < BAJA_SCENERY_KINDS; ++i) distinct += kinds[i];
    REQUIRE(distinct >= 12);
    /* Dense enough to dress the course: a prop every dozen metres or so. */
    REQUIRE(sim.scenery[300].s < sim.track.total_length);
    ++checks;
}

/* A crest at speed throws the vehicle into the air and it lands on
 * compressed suspension; the same crest at a crawl does not. */
static void test_crests_launch_the_vehicle(void)
{
    BajaSim sim;
    int i;
    int jumped = 0;
    int airborne_frames = 0;
    BajaFp lowest = 0;

    race_at_speed(&sim, 60);
    for (i = 0; i < 7000 && sim.phase == BAJA_PHASE_RACING; ++i) {
        drive_frames(&sim, 1);
        if (sim.jump_event) ++jumped;
        if (sim.air_timer > 0U) {
            ++airborne_frames;
            if (sim.bounce < lowest) lowest = sim.bounce;
        }
    }
    REQUIRE(jumped >= 2);
    REQUIRE(airborne_frames > jumped * 10);
    REQUIRE(lowest < -(BAJA_FP_ONE / 5));

    /* Crawling over the course never leaves the ground. */
    baja_sim_init(&sim);
    baja_sim_begin_race(&sim);
    jumped = 0;
    for (i = 0; i < 4000; ++i) {
        uint8_t input = sim.speed > BAJA_FP_ONE / 5 ? 0U : BAJA_INPUT_THROTTLE;
        baja_sim_step(&sim, input);
        if (sim.jump_event) ++jumped;
    }
    REQUIRE(jumped == 0);
    ++checks;
}

/* Driving off the road into a rock stops the vehicle and shoves it back;
 * staying on the road never touches a prop. */
static void test_roadside_props_are_hazards(void)
{
    BajaSim sim;
    int i;
    uint16_t target = 0;
    BajaFp before;

    race_at_speed(&sim, 300);
    /* Hold the racing line: no prop is ever struck. */
    for (i = 0; i < 1500; ++i) drive_frames(&sim, 1);
    REQUIRE(sim.hazards == 0);

    /* Find the next solid prop and steer onto it. */
    for (i = 0; i < BAJA_SCENERY_COUNT; ++i) {
        if (sim.scenery[i].s > sim.player_s + baja_fp_from_int(40) &&
            baja_scenery_hazard(sim.scenery[i].kind) == BAJA_HAZARD_SOLID) {
            target = (uint16_t)i;
            break;
        }
    }
    REQUIRE(target != 0U);
    for (i = 0; i < 3000 && sim.hazards == 0; ++i) {
        uint8_t input = BAJA_INPUT_THROTTLE;
        BajaFp want = sim.scenery[target].e;
        if (sim.player_e < want - BAJA_FP_ONE / 32) input |= BAJA_INPUT_RIGHT;
        else if (sim.player_e > want + BAJA_FP_ONE / 32) input |= BAJA_INPUT_LEFT;
        before = sim.speed;
        baja_sim_step(&sim, input);
        if (sim.hazard_event) {
            REQUIRE(sim.hazard_event == BAJA_HAZARD_SOLID);
            REQUIRE(sim.speed < before / 2);
            REQUIRE(sim.player_s > sim.scenery[target].s - baja_fp_from_int(6));
        }
    }
    REQUIRE(sim.hazards == 1);
    /* The player keeps control: throttle still builds speed afterwards. */
    for (i = 0; i < 120; ++i) drive_frames(&sim, 1);
    REQUIRE(sim.speed > BAJA_FP_ONE / 4);
    REQUIRE(baja_scenery_hazard(BAJA_SCENERY_GANTRY_START) == BAJA_HAZARD_NONE);
    ++checks;
}

int main(void)
{
    test_menus_wait_for_input();
    test_idle_throttle_coast_and_brake();
    test_steering_and_return();
    test_offroad_penalty_is_recoverable();
    test_curve_pushes_outward_and_is_holdable();
    test_road_bands_form_a_funnel();
    test_bands_track_curve_and_surface_phase();
    test_hills_occlude_distant_road();
    test_object_projection_matches_the_road();
    test_rivals_are_independent();
    test_collision_and_non_collision();
    test_race_lifecycle_and_restart();
    test_deterministic_replay();
    test_scenery_sits_off_the_racing_line();
    test_crests_launch_the_vehicle();
    test_roadside_props_are_hazards();
    printf("PASS: %d deterministic gameplay and projection tests\n", checks);
    return 0;
}
