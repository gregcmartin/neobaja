#include <assert.h>
#include <stdio.h>
#include "../game/sim.h"

static void frames(BajaSim *s, uint32_t n, uint8_t input) {
    while (n--) baja_sim_step(s, input);
}

static void enter_race(BajaSim *s) {
    frames(s, 300u, 0u);
    assert(s->scene == BAJA_TITLE);
    baja_sim_step(s, BAJA_IN_START);
    baja_sim_step(s, 0u);
    assert(s->scene == BAJA_SELECT);
    baja_sim_step(s, BAJA_IN_A);
    baja_sim_step(s, 0u);
    assert(s->scene == BAJA_COUNTDOWN);
    frames(s, 240u, 0u);
    assert(s->scene == BAJA_RACE);
}

static void test_input_gates(void) {
    BajaSim s;
    baja_sim_init(&s);
    frames(&s, 299u, BAJA_IN_START | BAJA_IN_A | BAJA_IN_RIGHT);
    assert(s.scene == BAJA_SPLASH);
    assert(s.speed == 0u && s.selected_racer == 0u);
    baja_sim_step(&s, 0u);
    assert(s.scene == BAJA_TITLE);
    frames(&s, 600u, 0u);
    assert(s.scene == BAJA_TITLE);
    baja_sim_step(&s, BAJA_IN_START);
    baja_sim_step(&s, 0u);
    assert(s.scene == BAJA_SELECT);
    frames(&s, 600u, 0u);
    assert(s.scene == BAJA_SELECT);
    baja_sim_step(&s, BAJA_IN_RIGHT);
    assert(s.selected_racer == 1u);
}

static void test_throttle_coast_brake(void) {
    BajaSim coast, brake;
    baja_sim_init(&coast);
    enter_race(&coast);
    frames(&coast, 120u, BAJA_IN_A);
    assert(coast.speed > 0u);
    brake = coast;
    frames(&coast, 30u, 0u);
    frames(&brake, 30u, BAJA_IN_B);
    assert(brake.speed < coast.speed);
    frames(&brake, 600u, BAJA_IN_B);
    assert(brake.speed == 0u);
}

static void test_steering_and_recenter(void) {
    BajaSim left, right;
    baja_sim_init(&left);
    enter_race(&left);
    frames(&left, 100u, BAJA_IN_A);
    right = left;
    frames(&left, 20u, BAJA_IN_A | BAJA_IN_LEFT);
    frames(&right, 20u, BAJA_IN_A | BAJA_IN_RIGHT);
    assert(left.steer < 0 && right.steer > 0);
    assert(left.player_x < right.player_x);
    frames(&right, 40u, BAJA_IN_A);
    assert(right.steer == 0);
}

static void test_offroad_and_determinism(void) {
    BajaSim a, b;
    baja_sim_init(&a);
    enter_race(&a);
    b = a;
    for (unsigned i = 0; i < 600u; ++i) {
        uint8_t in = BAJA_IN_A;
        if (i > 80u && i < 260u) in |= BAJA_IN_RIGHT;
        if (i > 420u && i < 450u) in = BAJA_IN_B | BAJA_IN_LEFT;
        baja_sim_step(&a, in);
        baja_sim_step(&b, in);
    }
    assert(a.course_pos == b.course_pos);
    assert(a.player_x == b.player_x);
    assert(a.rival_pos == b.rival_pos);
    assert(a.offroad == b.offroad);
    assert(a.rough_frames == b.rough_frames);
}

static void test_idle_rival_offroad_and_collision(void) {
    BajaSim s;
    baja_sim_init(&s);
    enter_race(&s);
    const uint32_t rival_start = s.rival_pos;
    frames(&s, 120u, 0u);
    assert(s.speed == 0u && s.course_pos == 0u);
    assert(s.rival_pos > rival_start);

    s.player_x = 1300;
    s.speed = 900u;
    frames(&s, 80u, BAJA_IN_A);
    assert(s.offroad != 0u);
    assert(s.speed <= 610u);
    s.player_x = 0;
    frames(&s, 2u, 0u);
    assert(s.offroad == 0u);

    s.course_pos = 50000u;
    s.rival_pos = s.course_pos + 600u;
    s.player_x = 0;
    s.rival_x = 0;
    s.speed = 700u;
    s.collision_frames = 0u;
    baja_sim_step(&s, BAJA_IN_A);
    assert(s.collision_frames > 0u);
    assert(s.speed < 700u);
}

static void test_finish_and_restart(void) {
    BajaSim s;
    baja_sim_init(&s);
    enter_race(&s);
    s.course_pos = s.finish_pos - 1u;
    s.speed = 200u;
    baja_sim_step(&s, BAJA_IN_A);
    assert(s.scene == BAJA_FINISH);
    baja_sim_step(&s, 0u);
    baja_sim_step(&s, BAJA_IN_A);
    assert(s.scene == BAJA_COUNTDOWN);
    assert(s.speed == 0u && s.course_pos == 0u);
}

static void test_overtake_order_change(void) {
    BajaSim s;
    baja_sim_init(&s);
    enter_race(&s);
    for (unsigned i = 0; i < 5000u && !s.passed_rival; ++i) {
        s.player_x = -1000;
        s.rival_x = 760;
        baja_sim_step(&s, BAJA_IN_A);
    }
    assert(s.passed_rival != 0u);
    assert(s.position == 1u);
}

int main(void) {
    test_input_gates();
    test_throttle_coast_brake();
    test_steering_and_recenter();
    test_offroad_and_determinism();
    test_idle_rival_offroad_and_collision();
    test_finish_and_restart();
    test_overtake_order_change();
    puts("BAJANEW simulation tests: PASS");
    return 0;
}
