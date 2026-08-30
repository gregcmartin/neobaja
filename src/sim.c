#include "baja/sim.h"

#include <stddef.h>
#include <string.h>

#define FP_RATIO(n, d) ((BajaFp)(((int32_t)(n) * BAJA_FP_ONE) / (d)))

#define SPLASH_FRAMES 300u
#define COUNTDOWN_FRAMES 180u
#define SEGMENT_LENGTH FP_RATIO(20, 1)
#define ROAD_EDGE FP_RATIO(1, 1)
#define SHOULDER_EDGE FP_RATIO(6, 5)
#define WORLD_ROAD_HALF FP_RATIO(4, 1)
#define MAX_LATERAL FP_RATIO(3, 2)

#define ROAD_SPEED_CAP FP_RATIO(5, 2)
#define DIRT_SPEED_CAP FP_RATIO(29, 20)
#define DRIVE_BASE FP_RATIO(3, 500)
#define COAST_BASE FP_RATIO(3, 1000)
#define BRAKE_BASE FP_RATIO(3, 125)
#define DIRT_DRAG FP_RATIO(3, 1000)

#define STEER_INPUT_RATE FP_RATIO(1, 10)
#define STEER_RETURN_RATE FP_RATIO(7, 50)
#define STEER_BASE_RESPONSE FP_RATIO(3, 250)
#define STEER_SPEED_RESPONSE FP_RATIO(1, 100)

#define VEHICLE_HALF_LENGTH FP_RATIO(6, 5)
#define VEHICLE_HALF_WIDTH FP_RATIO(11, 100)

typedef struct TrackPiece {
    int16_t count;
    int16_t curve_milli;
    int16_t grade_milli;
} TrackPiece;

static BajaFp fp_abs(BajaFp value)
{
    return value < 0 ? -value : value;
}

static BajaFp fp_clamp(BajaFp value, BajaFp low, BajaFp high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static BajaFp fp_approach(BajaFp value, BajaFp target, BajaFp rate)
{
    if (value < target) {
        value += rate;
        return value > target ? target : value;
    }
    if (value > target) {
        value -= rate;
        return value < target ? target : value;
    }
    return value;
}

BajaFp baja_fp_from_int(int32_t value)
{
    return value * BAJA_FP_ONE;
}

int32_t baja_fp_to_int(BajaFp value)
{
    return value / BAJA_FP_ONE;
}

BajaFp baja_fp_mul(BajaFp a, BajaFp b)
{
    return (BajaFp)(((int64_t)a * (int64_t)b) >> BAJA_FP_SHIFT);
}

BajaFp baja_fp_div(BajaFp a, BajaFp b)
{
    if (b == 0) return 0;
    return (BajaFp)(((int64_t)a << BAJA_FP_SHIFT) / b);
}

static BajaFp smoothstep(BajaFp t)
{
    BajaFp t2 = baja_fp_mul(t, t);
    return baja_fp_mul(t2, (3 * BAJA_FP_ONE) - (2 * t));
}

void baja_track_init(BajaTrack *track)
{
    static const TrackPiece pieces[] = {
        {48, 0, 0},
        {48, 3, 1},
        {48, -2, 0},
        {48, -4, -1},
        {48, 0, 2},
        {48, 5, 0},
        {48, -3, -2},
        {48, 0, 0}
    };
    BajaFp heading = 0;
    BajaFp current_curve = 0;
    BajaFp current_grade = 0;
    uint16_t segment = 0;
    uint16_t piece;

    memset(track, 0, sizeof(*track));
    track->segment_length = SEGMENT_LENGTH;
    track->total_length = SEGMENT_LENGTH * BAJA_TRACK_SEGMENTS;

    for (piece = 0; piece < (uint16_t)(sizeof(pieces) / sizeof(pieces[0])); ++piece) {
        BajaFp target_curve = FP_RATIO(pieces[piece].curve_milli, 1000);
        BajaFp target_grade = FP_RATIO(pieces[piece].grade_milli, 1000);
        BajaFp start_curve = current_curve;
        BajaFp start_grade = current_grade;
        int16_t i;

        for (i = 0; i < pieces[piece].count && segment < BAJA_TRACK_SEGMENTS; ++i) {
            BajaFp t = (BajaFp)(((int32_t)(i + 1) * BAJA_FP_ONE) / pieces[piece].count);
            BajaFp blend = smoothstep(t);
            current_curve = start_curve + baja_fp_mul(target_curve - start_curve, blend);
            current_grade = start_grade + baja_fp_mul(target_grade - start_grade, blend);
            track->curvature[segment] = current_curve;
            track->grade[segment] = current_grade;
            heading += current_curve;
            track->center_x[segment + 1] = track->center_x[segment] + heading;
            track->height[segment + 1] = track->height[segment] + current_grade;
            ++segment;
        }
    }
}

void baja_track_sample(const BajaTrack *track, BajaFp s, BajaFp *x,
                       BajaFp *y, BajaFp *curve)
{
    int32_t segment;
    BajaFp local;
    BajaFp t;

    if (s < 0) s = 0;
    if (s >= track->total_length) s = track->total_length - 1;
    segment = s / track->segment_length;
    if (segment >= BAJA_TRACK_SEGMENTS) segment = BAJA_TRACK_SEGMENTS - 1;
    local = s - ((BajaFp)segment * track->segment_length);
    t = baja_fp_div(local, track->segment_length);
    if (x != NULL) {
        *x = track->center_x[segment] +
             baja_fp_mul(track->center_x[segment + 1] - track->center_x[segment], t);
    }
    if (y != NULL) {
        *y = track->height[segment] +
             baja_fp_mul(track->height[segment + 1] - track->height[segment], t);
    }
    if (curve != NULL) *curve = track->curvature[segment];
}

static void reset_rivals(BajaSim *sim)
{
    BajaRival *evasive = &sim->rivals[0];
    BajaRival *blocker = &sim->rivals[1];

    memset(sim->rivals, 0, sizeof(sim->rivals));

    evasive->s = baja_fp_from_int(58);
    evasive->e = FP_RATIO(-7, 20);
    evasive->target_e = evasive->e;
    evasive->speed = FP_RATIO(39, 20);
    evasive->preferred_speed = FP_RATIO(43, 20);
    evasive->profile = 0;
    evasive->was_ahead = 1;
    evasive->active = 1;

    blocker->s = baja_fp_from_int(112);
    blocker->e = FP_RATIO(2, 5);
    blocker->target_e = blocker->e;
    blocker->speed = FP_RATIO(21, 10);
    blocker->preferred_speed = FP_RATIO(23, 10);
    blocker->profile = 1;
    blocker->was_ahead = 1;
    blocker->active = 1;
}

void baja_sim_init(BajaSim *sim)
{
    memset(sim, 0, sizeof(*sim));
    baja_track_init(&sim->track);
    sim->phase = BAJA_PHASE_SPLASH;
    sim->driver = BAJA_DRIVER_MAX;
    sim->surface = BAJA_SURFACE_ROAD;
    sim->position = BAJA_RIVAL_COUNT + 1;
    reset_rivals(sim);
}

void baja_sim_begin_race(BajaSim *sim)
{
    sim->player_s = 0;
    sim->player_e = 0;
    sim->speed = 0;
    sim->steer = 0;
    sim->surface = BAJA_SURFACE_ROAD;
    sim->position = BAJA_RIVAL_COUNT + 1;
    sim->collision_cooldown = 0;
    sim->collision_event = 0;
    sim->dust_event = 0;
    sim->collisions = 0;
    sim->overtakes = 0;
    sim->phase = BAJA_PHASE_RACING;
    sim->phase_frame = 0;
    reset_rivals(sim);
}

static void update_surface(BajaSim *sim)
{
    BajaFp lateral = fp_abs(sim->player_e);
    if (lateral <= ROAD_EDGE) sim->surface = BAJA_SURFACE_ROAD;
    else if (lateral <= SHOULDER_EDGE) sim->surface = BAJA_SURFACE_SHOULDER;
    else sim->surface = BAJA_SURFACE_DIRT;
    sim->dust_event = (uint8_t)(sim->surface != BAJA_SURFACE_ROAD && sim->speed > FP_RATIO(1, 5));
}

static void update_player(BajaSim *sim, uint8_t input)
{
    BajaFp speed_cap;
    BajaFp drag;
    BajaFp target_steer = 0;
    BajaFp speed_ratio;
    BajaFp lateral_response;
    BajaFp curve = 0;
    BajaFp curve_force;

    update_surface(sim);
    speed_cap = sim->surface == BAJA_SURFACE_DIRT ? DIRT_SPEED_CAP : ROAD_SPEED_CAP;

    if ((input & BAJA_INPUT_THROTTLE) != 0 && (input & BAJA_INPUT_BRAKE) == 0) {
        BajaFp headroom = speed_cap - sim->speed;
        if (headroom > 0) sim->speed += DRIVE_BASE + (headroom >> 8);
    } else {
        drag = COAST_BASE + (sim->speed >> 10);
        sim->speed -= drag;
    }

    if ((input & BAJA_INPUT_BRAKE) != 0) {
        sim->speed -= BRAKE_BASE + (sim->speed >> 8);
    }
    if (sim->surface == BAJA_SURFACE_SHOULDER) {
        sim->speed -= DIRT_DRAG >> 1;
    } else if (sim->surface == BAJA_SURFACE_DIRT) {
        sim->speed -= DIRT_DRAG + (sim->speed >> 9);
    }
    sim->speed = fp_clamp(sim->speed, 0, speed_cap);

    if ((input & BAJA_INPUT_LEFT) != 0 && (input & BAJA_INPUT_RIGHT) == 0) {
        target_steer = -BAJA_FP_ONE;
    } else if ((input & BAJA_INPUT_RIGHT) != 0 && (input & BAJA_INPUT_LEFT) == 0) {
        target_steer = BAJA_FP_ONE;
    }
    sim->steer = fp_approach(sim->steer, target_steer,
                             target_steer == 0 ? STEER_RETURN_RATE : STEER_INPUT_RATE);

    speed_ratio = baja_fp_div(sim->speed, ROAD_SPEED_CAP);
    lateral_response = STEER_BASE_RESPONSE + baja_fp_mul(STEER_SPEED_RESPONSE, speed_ratio);
    if (sim->surface == BAJA_SURFACE_DIRT) lateral_response = baja_fp_mul(lateral_response, FP_RATIO(3, 4));
    sim->player_e += baja_fp_mul(sim->steer, lateral_response);

    baja_track_sample(&sim->track, sim->player_s, NULL, NULL, &curve);
    curve_force = baja_fp_mul(speed_ratio, speed_ratio);
    curve_force = baja_fp_mul(curve_force, curve);
    curve_force = baja_fp_mul(curve_force, FP_RATIO(1, 2));
    sim->player_e -= curve_force;
    sim->player_e = fp_clamp(sim->player_e, -MAX_LATERAL, MAX_LATERAL);

    sim->player_s += sim->speed;
    if (sim->player_s >= sim->track.total_length) {
        sim->player_s = sim->track.total_length;
        sim->speed = 0;
        sim->phase = BAJA_PHASE_FINISHED;
        sim->phase_frame = 0;
    }
    update_surface(sim);
}

static void choose_rival_target(BajaSim *sim, BajaRival *rival)
{
    BajaFp gap = rival->s - sim->player_s;
    BajaFp lateral_gap = rival->e - sim->player_e;

    if (rival->profile == 0) {
        if (gap > 0 && gap < baja_fp_from_int(52) && fp_abs(lateral_gap) < FP_RATIO(9, 20)) {
            rival->target_e = sim->player_e <= 0 ? FP_RATIO(13, 20) : FP_RATIO(-13, 20);
        } else {
            rival->target_e = ((rival->s / baja_fp_from_int(240)) & 1) != 0 ?
                              FP_RATIO(9, 20) : FP_RATIO(-9, 20);
        }
        rival->decision_timer = 74;
    } else {
        if (gap > baja_fp_from_int(8) && gap < baja_fp_from_int(70)) {
            rival->target_e = fp_clamp(sim->player_e, FP_RATIO(-3, 4), FP_RATIO(3, 4));
        } else {
            rival->target_e = ((rival->s / baja_fp_from_int(180)) & 1) != 0 ?
                              FP_RATIO(-1, 4) : FP_RATIO(1, 2);
        }
        rival->decision_timer = 49;
    }
}

static void update_rival(BajaSim *sim, BajaRival *rival)
{
    BajaFp gap;
    BajaFp target_speed;
    BajaFp lane_rate;

    if (!rival->active) return;
    if (rival->collision_cooldown > 0) --rival->collision_cooldown;
    if (rival->decision_timer > 0) --rival->decision_timer;
    else choose_rival_target(sim, rival);

    gap = rival->s - sim->player_s;
    target_speed = rival->preferred_speed;
    if (rival->profile == 0 && gap > 0 && gap < baja_fp_from_int(45)) {
        target_speed += FP_RATIO(1, 10);
    }
    if (rival->profile == 1 && gap > 0 && gap < baja_fp_from_int(36)) {
        target_speed -= FP_RATIO(3, 20);
    }
    rival->speed = fp_approach(rival->speed, target_speed,
                               rival->speed < target_speed ? FP_RATIO(1, 800) : FP_RATIO(1, 500));
    lane_rate = rival->profile == 0 ? FP_RATIO(1, 175) : FP_RATIO(1, 230);
    rival->e = fp_approach(rival->e, rival->target_e, lane_rate);
    rival->e = fp_clamp(rival->e, FP_RATIO(-4, 5), FP_RATIO(4, 5));
    rival->s += rival->speed;

    if (rival->was_ahead && rival->s < sim->player_s) {
        rival->was_ahead = 0;
        ++sim->overtakes;
    } else if (!rival->was_ahead && rival->s > sim->player_s + baja_fp_from_int(5)) {
        rival->was_ahead = 1;
    }
}

static void check_collisions(BajaSim *sim)
{
    uint8_t i;
    sim->collision_event = 0;
    if (sim->collision_cooldown > 0) {
        --sim->collision_cooldown;
        return;
    }

    for (i = 0; i < BAJA_RIVAL_COUNT; ++i) {
        BajaRival *rival = &sim->rivals[i];
        BajaFp ds;
        BajaFp de;
        BajaFp contact_speed;
        if (!rival->active || rival->collision_cooldown != 0) continue;
        ds = fp_abs(rival->s - sim->player_s);
        de = fp_abs(rival->e - sim->player_e);
        if (ds >= (VEHICLE_HALF_LENGTH * 2) || de >= (VEHICLE_HALF_WIDTH * 2)) continue;

        contact_speed = baja_fp_mul(rival->speed, FP_RATIO(17, 20));
        sim->speed = baja_fp_mul(sim->speed, FP_RATIO(11, 20));
        if (sim->speed > contact_speed) sim->speed = contact_speed;
        if (rival->e >= sim->player_e) sim->player_e -= FP_RATIO(9, 100);
        else sim->player_e += FP_RATIO(9, 100);
        sim->player_e = fp_clamp(sim->player_e, -MAX_LATERAL, MAX_LATERAL);
        sim->collision_cooldown = 36;
        rival->collision_cooldown = 36;
        sim->collision_event = 1;
        ++sim->collisions;
        break;
    }
}

static void update_position(BajaSim *sim)
{
    uint8_t position = 1;
    uint8_t i;
    for (i = 0; i < BAJA_RIVAL_COUNT; ++i) {
        if (sim->rivals[i].active && sim->rivals[i].s > sim->player_s) ++position;
    }
    sim->position = position;
}

void baja_sim_step(BajaSim *sim, uint8_t input)
{
    uint8_t pressed = (uint8_t)(input & (uint8_t)~sim->previous_input);
    ++sim->frame;
    ++sim->phase_frame;
    sim->collision_event = 0;
    sim->dust_event = 0;

    switch ((BajaPhase)sim->phase) {
    case BAJA_PHASE_SPLASH:
        if (sim->phase_frame >= SPLASH_FRAMES) {
            sim->phase = BAJA_PHASE_TITLE;
            sim->phase_frame = 0;
        }
        break;
    case BAJA_PHASE_TITLE:
        if ((pressed & BAJA_INPUT_START) != 0) {
            sim->phase = BAJA_PHASE_SELECT;
            sim->phase_frame = 0;
        }
        break;
    case BAJA_PHASE_SELECT:
        if ((pressed & (BAJA_INPUT_LEFT | BAJA_INPUT_RIGHT)) != 0) {
            sim->driver = (uint8_t)(sim->driver == BAJA_DRIVER_MAX ? BAJA_DRIVER_CRUZ : BAJA_DRIVER_MAX);
        }
        if ((pressed & BAJA_INPUT_START) != 0) {
            sim->phase = BAJA_PHASE_COUNTDOWN;
            sim->phase_frame = 0;
            sim->speed = 0;
            sim->player_s = 0;
            sim->player_e = 0;
            sim->steer = 0;
            reset_rivals(sim);
        }
        break;
    case BAJA_PHASE_COUNTDOWN:
        if (sim->phase_frame >= COUNTDOWN_FRAMES) {
            sim->phase = BAJA_PHASE_RACING;
            sim->phase_frame = 0;
        }
        break;
    case BAJA_PHASE_RACING: {
        uint8_t i;
        update_player(sim, input);
        for (i = 0; i < BAJA_RIVAL_COUNT; ++i) update_rival(sim, &sim->rivals[i]);
        check_collisions(sim);
        update_position(sim);
        break;
    }
    case BAJA_PHASE_FINISHED:
        if ((pressed & BAJA_INPUT_START) != 0) {
            sim->phase = BAJA_PHASE_COUNTDOWN;
            sim->phase_frame = 0;
            sim->speed = 0;
            sim->player_s = 0;
            sim->player_e = 0;
            sim->steer = 0;
            sim->collisions = 0;
            sim->overtakes = 0;
            reset_rivals(sim);
        }
        break;
    default:
        baja_sim_init(sim);
        break;
    }
    sim->previous_input = input;
}

uint8_t baja_project_road(const BajaSim *sim, BajaRoadSample *samples,
                          uint8_t capacity)
{
    const int16_t screen_center = 160;
    const int16_t horizon = 72;
    const BajaFp focal = baja_fp_from_int(640);
    const BajaFp camera_height = baja_fp_from_int(3);
    BajaFp camera_x;
    BajaFp camera_y;
    uint8_t count = capacity < BAJA_ROAD_SAMPLE_MAX ? capacity : BAJA_ROAD_SAMPLE_MAX;
    uint8_t i;
    int16_t nearest_visible_y = 224;

    if (samples == NULL || count == 0) return 0;
    baja_track_sample(&sim->track, sim->player_s, &camera_x, &camera_y, NULL);
    camera_x += baja_fp_mul(sim->player_e, WORLD_ROAD_HALF);

    for (i = 0; i < count; ++i) {
        int32_t depth_units = 8 + ((int32_t)i * (int32_t)i);
        BajaFp depth = baja_fp_from_int(depth_units);
        BajaFp world_s = sim->player_s + depth;
        BajaFp road_x;
        BajaFp road_y;
        BajaFp scale;
        BajaFp center_offset;
        BajaFp projected_y;
        BajaFp projected_width;

        if (world_s >= sim->track.total_length) world_s = sim->track.total_length - 1;
        baja_track_sample(&sim->track, world_s, &road_x, &road_y, NULL);
        scale = baja_fp_div(focal, depth);
        center_offset = baja_fp_mul(road_x - camera_x, scale);
        projected_y = baja_fp_mul((camera_y + camera_height) - road_y, scale);
        projected_width = baja_fp_mul(WORLD_ROAD_HALF, scale);

        samples[i].screen_x = (int16_t)(screen_center + baja_fp_to_int(center_offset));
        samples[i].screen_y = (int16_t)(horizon + baja_fp_to_int(projected_y));
        if (samples[i].screen_y > 223) samples[i].screen_y = 223;
        samples[i].half_width = (int16_t)baja_fp_to_int(projected_width);
        if (samples[i].half_width > 176) samples[i].half_width = 176;
        if (samples[i].half_width < 1) samples[i].half_width = 1;
        samples[i].depth = (uint16_t)depth_units;
        samples[i].segment = (uint16_t)(world_s / sim->track.segment_length);
        samples[i].shade = (uint8_t)((samples[i].segment >> 1) & 1);
        samples[i].visible = 0;
    }

    for (i = 0; i < count; ++i) {
        BajaRoadSample *sample = &samples[i];
        if (sample->screen_y < horizon || sample->screen_y >= nearest_visible_y) continue;
        sample->visible = 1;
        nearest_visible_y = sample->screen_y;
    }
    return count;
}
