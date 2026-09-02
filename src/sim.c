#include "baja/sim.h"

#include <stddef.h>

#define FP_RATIO(n, d) ((BajaFp)(((int32_t)(n) * BAJA_FP_ONE) / (d)))

#define SPLASH_FRAMES 300u
#define COUNTDOWN_FRAMES 210u

/* World units are metres.  The road is eight metres across, the camera rides
 * three metres above the surface, and the projection uses a 160 unit focal
 * length so the near edge of the funnel lands on the bottom scanline. */
#define SEGMENT_LENGTH ((BajaFp)1 << BAJA_SEGMENT_SHIFT)
#define SEGMENT_T_SHIFT (BAJA_SEGMENT_SHIFT - BAJA_FP_SHIFT)
#define WORLD_ROAD_HALF FP_RATIO(4, 1)
#define CAMERA_HEIGHT FP_RATIO(3, 1)

#define ROAD_EDGE FP_RATIO(78, 100)
#define SHOULDER_EDGE FP_RATIO(105, 100)
#define MAX_LATERAL FP_RATIO(2, 1)

#define ROAD_SPEED_CAP FP_RATIO(75, 100)
#define ROAD_SPEED_RECIP FP_RATIO(400, 300)
#define DIRT_SPEED_CAP FP_RATIO(42, 100)
#define DRIVE_BASE FP_RATIO(4, 1000)
#define COAST_BASE FP_RATIO(8, 10000)
#define BRAKE_BASE FP_RATIO(8, 1000)
#define DIRT_DRAG FP_RATIO(12, 10000)

#define STEER_INPUT_RATE FP_RATIO(1, 10)
#define STEER_RETURN_RATE FP_RATIO(7, 50)
#define STEER_BASE_RESPONSE FP_RATIO(8, 1000)
#define STEER_SPEED_RESPONSE FP_RATIO(22, 1000)
#define CURVE_PULL FP_RATIO(1, 45)

#define VEHICLE_HALF_LENGTH FP_RATIO(5, 2)
#define VEHICLE_HALF_WIDTH FP_RATIO(3, 10)
/* Screen pixels of camera shake per world unit of suspension travel. */
#define SHAKE_PIXELS FP_RATIO(26, 1)
/* Pixels from centre beyond which the player's berth is compressed. */
#define PLAYER_SOFT_EDGE 100

typedef struct TrackPiece {
    int16_t count;
    int16_t curve_milli;
    int16_t grade_milli;
} TrackPiece;

const int16_t baja_band_dy[BAJA_ROAD_BANDS + 1] = {
    2, 3, 4, 5, 6, 7, 9, 11, 13, 17, 21, 25, 31, 39, 48, 60, 71, 82, 94, 106,
    117, 128, 140
};

/* Surface phase half wavelength per band, as a fixed-point shift.  It is
 * never shorter than the band's own depth span, so a band shows one phase at a
 * time, and it grows with depth so the pattern keeps a readable size on screen
 * instead of aliasing into flicker.  Eight metres at the near end streams past
 * at about eleven flips a second at top speed, which is the speed cue. */
const uint8_t baja_band_stripe_shift[BAJA_ROAD_BANDS] = {
    23, 22, 21, 20, 20, 20, 20, 19, 20, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19
};

const int16_t baja_band_half_width[BAJA_ROAD_BANDS] = {
    3, 5, 6, 7, 9, 11, 13, 16, 20, 25, 31, 37, 47, 58, 72, 87, 102, 117, 133,
    149, 163, 179
};

/* depth = camera_height * focal / dy_mid */
static const BajaFp band_depth_fp[BAJA_ROAD_BANDS] = {
    12582912, 8987794, 6990507, 5719505, 4839582, 3932160, 3145728, 2621440,
    2097152, 1655646, 1367708, 1123474, 898779, 723156, 582542, 480264,
    411206, 357469, 314573, 282128, 256794, 234756
};

/* screen pixels per world unit at the middle of each band */
static const BajaFp band_scale_fp[BAJA_ROAD_BANDS] = {
    54613, 76459, 98304, 120149, 141995, 174763, 218453, 262144, 327680,
    415061, 502443, 611669, 764587, 950272, 1179648, 1430869, 1671168,
    1922389, 2184533, 2435755, 2676053, 2927275
};

/* screen pixels per world unit, and the depth, at each band boundary */
static const BajaFp band_edge_scale_fp[BAJA_ROAD_BANDS + 1] = {
    39719, 63716, 86016, 108134, 130162, 156684, 194181, 238313, 291271,
    366231, 454591, 551702, 679633, 847376, 1052609, 1293171, 1541711,
    1787997, 2045095, 2303314, 2550256, 2796032, 3252527
};

static const BajaFp band_edge_depth_fp[BAJA_ROAD_BANDS + 1] = {
    17301504, 10785353, 7989150, 6355006, 5279543, 4385871, 3538944, 2883584,
    2359296, 1876399, 1511677, 1245591, 1011127, 810968, 652849, 531403,
    445735, 384338, 336021, 298350, 269461, 245775, 211280
};

/* Pixels per metre at each depth the object projection handles, in 8.8: one
 * eighth of a metre apart up to 32 metres, then a metre apart to the far end
 * of the funnel.  Filled once so no object ever costs a division. */
#define SCALE_FINE_ENTRIES 256
#define SCALE_FINE_SHIFT (BAJA_FP_SHIFT - 3)
#define SCALE_FAR_ENTRIES 240
static uint16_t scale_by_depth[SCALE_FINE_ENTRIES + SCALE_FAR_ENTRIES];
/* Which band owns each whole metre of depth, for draw order and crests. */
static uint8_t band_by_depth[SCALE_FAR_ENTRIES + 32];
static uint8_t depth_tables_ready = 0;

static void build_depth_tables(BajaServiceHook service_hook)
{
    uint32_t i;
    if (depth_tables_ready) return;
    for (i = 1; i < SCALE_FINE_ENTRIES; ++i) {
        uint32_t scale = (40960UL * 8UL) / i;
        scale_by_depth[i] = (uint16_t)(scale > 65535UL ? 65535UL : scale);
        if ((i & 63U) == 0U && service_hook != NULL) service_hook();
    }
    scale_by_depth[0] = 65535U;
    for (i = 0; i < SCALE_FAR_ENTRIES; ++i) {
        scale_by_depth[SCALE_FINE_ENTRIES + i] = (uint16_t)(40960UL / (32U + i));
        if ((i & 63U) == 0U && service_hook != NULL) service_hook();
    }
    for (i = 0; i < SCALE_FAR_ENTRIES + 32; ++i) {
        BajaFp depth = baja_fp_from_int((int32_t)i);
        uint8_t band = BAJA_ROAD_BANDS - 1;
        while (band > 0 && depth > band_depth_fp[band]) --band;
        band_by_depth[i] = band;
    }
    depth_tables_ready = 1;
}

static void zero_bytes(void *memory, uint32_t bytes)
{
    uint8_t *cursor = (uint8_t *)memory;
    uint32_t index;
    for (index = 0; index < bytes; ++index) cursor[index] = 0U;
}

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

/* Deterministic value noise over course distance.  It never advances on its
 * own, so a parked vehicle cannot be shaken into moving. */
static int32_t course_noise(BajaFp s, uint32_t salt)
{
    uint32_t n = (uint32_t)(s >> 14) + (salt * 0x9e3779b9UL);
    n ^= n >> 15;
    n *= 0x85ebca6bUL;
    n ^= n >> 13;
    n *= 0xc2b2ae35UL;
    n ^= n >> 16;
    return (int32_t)((n >> 20) & 0x1ffU) - 256;
}

void baja_track_init_cooperative(BajaTrack *track, BajaServiceHook service_hook)
{
    /* Ensenada: a fast coastal opening, a pair of cliff-side bends, a crest,
     * then a technical descent into the finish straight. */
    static const TrackPiece pieces[] = {
        {24, 0, 0},
        {26, 250, 240},
        {16, 0, 700},
        {12, -330, -560},
        {24, -240, -880},
        {18, 0, -160},
        {26, 400, 160},
        {16, -160, 950},
        {12, 0, -720},
        {28, -350, -240},
        {20, 190, 320},
        {18, -250, 800},
        {12, 320, -950},
        {26, 140, -320},
        {20, -200, 240},
        {24, 0, 0},
        {28, 240, -160},
        {22, -300, 160},
        {26, 160, 400},
        {30, 0, -240},
        {24, -260, 0},
        {30, 0, 0}
    };
    BajaFp heading = 0;
    BajaFp current_curve = 0;
    BajaFp current_grade = 0;
    uint16_t segment = 0;
    uint16_t piece;

    zero_bytes(track, (uint32_t)sizeof(*track));
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
            if (service_hook != NULL && (segment & 7U) == 0U) service_hook();
        }
    }
    /* Any unclaimed tail runs straight and level so the finish reads clearly. */
    while (segment < BAJA_TRACK_SEGMENTS) {
        track->curvature[segment] = 0;
        track->grade[segment] = 0;
        track->center_x[segment + 1] = track->center_x[segment] + heading;
        track->height[segment + 1] = track->height[segment];
        ++segment;
    }
}

void baja_track_init(BajaTrack *track)
{
    baja_track_init_cooperative(track, NULL);
}

void baja_track_sample(const BajaTrack *track, BajaFp s, BajaFp *x,
                       BajaFp *y, BajaFp *curve)
{
    int32_t segment;
    BajaFp local;
    BajaFp t;

    if (s < 0) s = 0;
    if (s >= track->total_length) s = track->total_length - 1;
    segment = (int32_t)(s >> BAJA_SEGMENT_SHIFT);
    if (segment >= BAJA_TRACK_SEGMENTS) segment = BAJA_TRACK_SEGMENTS - 1;
    local = s & (SEGMENT_LENGTH - 1);
    t = local >> SEGMENT_T_SHIFT;
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

BajaFp baja_track_heading(const BajaTrack *track, BajaFp s)
{
    int32_t segment;
    if (s < 0) s = 0;
    if (s >= track->total_length) s = track->total_length - 1;
    segment = (int32_t)(s >> BAJA_SEGMENT_SHIFT);
    if (segment >= BAJA_TRACK_SEGMENTS) segment = BAJA_TRACK_SEGMENTS - 1;
    return track->center_x[segment + 1] - track->center_x[segment];
}

/* The camera looks along the road, not along a fixed world axis.  Everything
 * the renderer sees is therefore measured against the tangent taken at the
 * player: only real curvature and real grade bend the view.  Without this the
 * accumulated heading would drag the whole course sideways under the car. */
void baja_track_frame_init(const BajaTrack *track, BajaFp base_s, BajaTrackFrame *frame)
{
    int32_t segment;
    if (frame == NULL) return;
    if (base_s < 0) base_s = 0;
    if (base_s >= track->total_length) base_s = track->total_length - 1;
    segment = (int32_t)(base_s >> BAJA_SEGMENT_SHIFT);
    if (segment >= BAJA_TRACK_SEGMENTS) segment = BAJA_TRACK_SEGMENTS - 1;
    frame->base_s = base_s;
    baja_track_sample(track, base_s, &frame->base_x, &frame->base_y, NULL);
    frame->heading = track->center_x[segment + 1] - track->center_x[segment];
    frame->grade = track->height[segment + 1] - track->height[segment];
}

void baja_track_frame_sample(const BajaTrack *track, const BajaTrackFrame *frame,
                             BajaFp s, BajaFp *lateral, BajaFp *rise)
{
    BajaFp x = 0;
    BajaFp y = 0;
    BajaFp segments;

    if (s >= track->total_length) s = track->total_length - 1;
    baja_track_sample(track, s, &x, &y, NULL);
    segments = (s - frame->base_s) >> SEGMENT_T_SHIFT;
    if (lateral != NULL) {
        *lateral = x - frame->base_x - baja_fp_mul(frame->heading, segments);
    }
    if (rise != NULL) {
        *rise = y - frame->base_y - baja_fp_mul(frame->grade, segments);
    }
}

/* Append a prop, keeping the list strictly ordered by course distance so the
 * renderer can window it with one cursor. */
static void place_scenery(BajaSim *sim, uint16_t *count, BajaFp s, BajaFp e, uint8_t kind)
{
    BajaScenery *item;
    if (*count >= BAJA_SCENERY_COUNT) return;
    if (*count > 0U && s <= sim->scenery[*count - 1U].s) {
        s = sim->scenery[*count - 1U].s + FP_RATIO(1, 2);
    }
    if (s >= sim->track.total_length) return;
    item = &sim->scenery[*count];
    item->s = s;
    item->e = e;
    item->kind = kind;
    item->reserved[0] = 0;
    item->reserved[1] = 0;
    item->reserved[2] = 0;
    ++*count;
}

/* Ensenada's roadside: the sea side to the left is palms, pale rock, agave
 * and flags; the hillside to the right is cactus, grey rock and scrub.
 * Chevrons line the outside of every real bend, crowds gather at the start
 * and the finish, a sign passes every few hundred metres, and gantries stand
 * over the road at each end.  Everything is read from the course, so the
 * same course always dresses the same way. */
static void reset_scenery(BajaSim *sim)
{
    static const uint8_t sea_kinds[8] = {
        BAJA_SCENERY_PALM, BAJA_SCENERY_ROCK_PALE, BAJA_SCENERY_AGAVE,
        BAJA_SCENERY_BUSH, BAJA_SCENERY_ROCK_PALE, BAJA_SCENERY_FLAG,
        BAJA_SCENERY_AGAVE, BAJA_SCENERY_ROCK_GREY
    };
    static const uint8_t hill_kinds[8] = {
        BAJA_SCENERY_CACTUS, BAJA_SCENERY_ROCK_GREY, BAJA_SCENERY_AGAVE,
        BAJA_SCENERY_BUSH, BAJA_SCENERY_CACTUS, BAJA_SCENERY_ROCK_PALE,
        BAJA_SCENERY_BUSH, BAJA_SCENERY_ROCK_GREY
    };
    static const uint8_t sign_kinds[3] = {
        BAJA_SCENERY_SIGN_ENSENADA, BAJA_SCENERY_SIGN_PACIFIC, BAJA_SCENERY_SIGN_BAJA
    };
    const BajaFp total = sim->track.total_length;
    const BajaFp step = FP_RATIO(11, 1);
    BajaFp s = FP_RATIO(6, 1);
    uint16_t count = 0;
    uint16_t slot = 0;
    uint8_t signs = 0;

    place_scenery(sim, &count, FP_RATIO(16, 1), 0, BAJA_SCENERY_GANTRY_START);
    while (s < total - FP_RATIO(24, 1)) {
        int32_t noise = course_noise(s, 7U + slot);
        int32_t pick = course_noise(s, 31U + slot);
        BajaFp curve;
        BajaFp offset = FP_RATIO(125, 100) + ((BajaFp)(noise + 256) * 90);
        uint8_t left = (uint8_t)((slot & 1U) == 0U);
        baja_track_sample(&sim->track, s, NULL, NULL, &curve);

        if (curve > FP_RATIO(15, 100) || curve < -FP_RATIO(15, 100)) {
            /* The outside of a bend is lined with chevrons the player can
             * read before arriving; the inside keeps its scrub. */
            BajaFp outside = curve > 0 ? -FP_RATIO(13, 10) : FP_RATIO(13, 10);
            BajaFp inside = -outside + (curve > 0 ? (BajaFp)(noise + 256) * 60
                                                  : -(BajaFp)(noise + 256) * 60);
            if ((slot & 1U) == 0U) {
                place_scenery(sim, &count, s, outside, BAJA_SCENERY_CHEVRON);
            } else {
                place_scenery(sim, &count, s, inside,
                              curve > 0 ? hill_kinds[pick & 7] : sea_kinds[pick & 7]);
            }
        } else if (s < FP_RATIO(110, 1) || s > total - FP_RATIO(170, 1)) {
            /* Spectators and flags crowd both ends of the course. */
            place_scenery(sim, &count, s, left ? -FP_RATIO(13, 10) : FP_RATIO(13, 10),
                          ((slot >> 1) & 1U) ? BAJA_SCENERY_FLAG : BAJA_SCENERY_CROWD);
        } else if ((slot % 44U) == 20U) {
            place_scenery(sim, &count, s, left ? -FP_RATIO(15, 10) : FP_RATIO(15, 10),
                          sign_kinds[signs % 3U]);
            ++signs;
        } else {
            place_scenery(sim, &count, s, left ? -offset : offset,
                          left ? sea_kinds[pick & 7] : hill_kinds[pick & 7]);
            /* Now and then the other side gets something too. */
            if ((pick & 0x70) == 0x70) {
                place_scenery(sim, &count, s + FP_RATIO(3, 1),
                              left ? offset + FP_RATIO(2, 10) : -offset - FP_RATIO(2, 10),
                              left ? hill_kinds[(pick >> 3) & 7] : sea_kinds[(pick >> 3) & 7]);
            }
        }
        s += step + (BajaFp)noise * 8;
        ++slot;
    }
    place_scenery(sim, &count, total - FP_RATIO(14, 1), 0, BAJA_SCENERY_GANTRY_FINISH);
    while (count < BAJA_SCENERY_COUNT) {
        /* Unused entries park past the finish where nothing draws them. */
        BajaScenery *item = &sim->scenery[count];
        item->s = total + (BajaFp)count;
        item->e = FP_RATIO(2, 1);
        item->kind = BAJA_SCENERY_BUSH;
        item->reserved[0] = 0;
        item->reserved[1] = 0;
        item->reserved[2] = 0;
        ++count;
    }
}

static void reset_rivals(BajaSim *sim)
{
    static const BajaFp start_s[BAJA_RIVAL_COUNT] = {
        FP_RATIO(70, 1), FP_RATIO(130, 1), FP_RATIO(205, 1)
    };
    static const BajaFp start_e[BAJA_RIVAL_COUNT] = {
        FP_RATIO(-45, 100), FP_RATIO(40, 100), FP_RATIO(-10, 100)
    };
    static const BajaFp pace[BAJA_RIVAL_COUNT] = {
        FP_RATIO(66, 100), FP_RATIO(62, 100), FP_RATIO(69, 100)
    };
    static const uint8_t profiles[BAJA_RIVAL_COUNT] = {0, 1, 0};
    uint8_t i;

    zero_bytes(sim->rivals, (uint32_t)sizeof(sim->rivals));
    for (i = 0; i < BAJA_RIVAL_COUNT; ++i) {
        BajaRival *rival = &sim->rivals[i];
        rival->s = start_s[i];
        rival->e = start_e[i];
        rival->target_e = start_e[i];
        rival->speed = baja_fp_mul(pace[i], FP_RATIO(9, 10));
        rival->preferred_speed = pace[i];
        rival->profile = profiles[i];
        rival->was_ahead = 1;
        rival->active = 1;
    }
}

void baja_sim_init_cooperative(BajaSim *sim, BajaServiceHook service_hook)
{
    zero_bytes(sim, (uint32_t)sizeof(*sim));
    build_depth_tables(service_hook);
    baja_track_init_cooperative(&sim->track, service_hook);
    reset_scenery(sim);
    sim->phase = BAJA_PHASE_SPLASH;
    sim->driver = BAJA_DRIVER_MAX;
    sim->surface = BAJA_SURFACE_ROAD;
    sim->position = BAJA_RIVAL_COUNT + 1;
    sim->gear = 1;
    reset_rivals(sim);
}

void baja_sim_init(BajaSim *sim)
{
    baja_sim_init_cooperative(sim, NULL);
}

static void reset_run(BajaSim *sim)
{
    sim->player_s = 0;
    sim->player_e = 0;
    sim->speed = 0;
    sim->steer = 0;
    sim->bounce = 0;
    sim->bounce_rate = 0;
    sim->surface = BAJA_SURFACE_ROAD;
    sim->position = BAJA_RIVAL_COUNT + 1;
    sim->collision_cooldown = 0;
    sim->rough_timer = 0;
    sim->collision_event = 0;
    sim->dust_event = 0;
    sim->collisions = 0;
    sim->overtakes = 0;
    sim->race_frames = 0;
    sim->gear = 1;
    reset_rivals(sim);
}

void baja_sim_begin_race(BajaSim *sim)
{
    reset_run(sim);
    sim->phase = BAJA_PHASE_RACING;
    sim->phase_frame = 0;
}

static void update_surface(BajaSim *sim)
{
    BajaFp lateral = fp_abs(sim->player_e);
    if (lateral <= ROAD_EDGE) sim->surface = BAJA_SURFACE_ROAD;
    else if (lateral <= SHOULDER_EDGE) sim->surface = BAJA_SURFACE_SHOULDER;
    else sim->surface = BAJA_SURFACE_DIRT;
}

static void update_player(BajaSim *sim, uint8_t input)
{
    BajaFp speed_cap;
    BajaFp target_steer = 0;
    BajaFp speed_ratio;
    BajaFp lateral_response;
    BajaFp curve = 0;
    BajaFp curve_force;
    BajaFp roughness;
    int32_t noise;

    update_surface(sim);
    speed_cap = sim->surface == BAJA_SURFACE_DIRT ? DIRT_SPEED_CAP : ROAD_SPEED_CAP;

    if ((input & BAJA_INPUT_THROTTLE) != 0 && (input & BAJA_INPUT_BRAKE) == 0) {
        BajaFp headroom = speed_cap - sim->speed;
        if (headroom > 0) sim->speed += DRIVE_BASE + (headroom >> 6);
    } else {
        sim->speed -= COAST_BASE + (sim->speed >> 9);
    }
    if ((input & BAJA_INPUT_BRAKE) != 0) {
        sim->speed -= BRAKE_BASE + (sim->speed >> 6);
    }
    if (sim->surface == BAJA_SURFACE_SHOULDER) {
        sim->speed -= DIRT_DRAG >> 1;
    } else if (sim->surface == BAJA_SURFACE_DIRT) {
        sim->speed -= DIRT_DRAG + (sim->speed >> 7);
    }
    sim->speed = fp_clamp(sim->speed, 0, speed_cap);

    if ((input & BAJA_INPUT_LEFT) != 0 && (input & BAJA_INPUT_RIGHT) == 0) {
        target_steer = -BAJA_FP_ONE;
    } else if ((input & BAJA_INPUT_RIGHT) != 0 && (input & BAJA_INPUT_LEFT) == 0) {
        target_steer = BAJA_FP_ONE;
    }
    sim->steer = fp_approach(sim->steer, target_steer,
                             target_steer == 0 ? STEER_RETURN_RATE : STEER_INPUT_RATE);

    /* 1 / ROAD_SPEED_CAP as a constant multiply. */
    speed_ratio = baja_fp_mul(sim->speed, ROAD_SPEED_RECIP);
    lateral_response = STEER_BASE_RESPONSE +
                       baja_fp_mul(STEER_SPEED_RESPONSE, speed_ratio);
    if (sim->surface == BAJA_SURFACE_DIRT) {
        lateral_response = baja_fp_mul(lateral_response, FP_RATIO(7, 10));
    }
    sim->player_e += baja_fp_mul(sim->steer, lateral_response);

    /* Centrifugal push uses the local curvature, so a constant bend produces a
     * constant force the player can hold against. */
    baja_track_sample(&sim->track, sim->player_s, NULL, NULL, &curve);
    curve_force = baja_fp_mul(speed_ratio, speed_ratio);
    curve_force = baja_fp_mul(curve_force, curve);
    curve_force = baja_fp_mul(curve_force, CURVE_PULL);
    sim->player_e -= curve_force;
    sim->player_e = fp_clamp(sim->player_e, -MAX_LATERAL, MAX_LATERAL);

    /* Suspension travel is read from the course, never from wall time. */
    roughness = sim->surface == BAJA_SURFACE_ROAD ? FP_RATIO(1, 40) : FP_RATIO(9, 100);
    if (sim->surface == BAJA_SURFACE_SHOULDER) roughness = FP_RATIO(5, 100);
    noise = course_noise(sim->player_s, 3U);
    sim->bounce_rate = baja_fp_mul(roughness, speed_ratio) * noise / 256;
    sim->bounce += sim->bounce_rate;
    sim->bounce = baja_fp_mul(sim->bounce, FP_RATIO(3, 5));
    sim->bounce = fp_clamp(sim->bounce, -FP_RATIO(1, 2), FP_RATIO(1, 2));

    sim->dust_event = (uint8_t)(sim->speed > FP_RATIO(1, 10) &&
                                (sim->surface != BAJA_SURFACE_ROAD ||
                                 sim->speed > baja_fp_mul(ROAD_SPEED_CAP, FP_RATIO(1, 2))));
    if (sim->surface != BAJA_SURFACE_ROAD && sim->speed > FP_RATIO(1, 5)) {
        if (sim->rough_timer < 60U) ++sim->rough_timer;
    } else if (sim->rough_timer > 0U) {
        --sim->rough_timer;
    }

    sim->player_s += sim->speed;
    if (sim->player_s >= sim->track.total_length) {
        sim->player_s = sim->track.total_length;
        sim->speed = 0;
        sim->phase = BAJA_PHASE_FINISHED;
        sim->phase_frame = 0;
    }
    update_surface(sim);

    {
        int32_t gear = baja_fp_to_int(baja_fp_mul(speed_ratio, baja_fp_from_int(5))) + 1;
        if (gear < 1) gear = 1;
        if (gear > 5) gear = 5;
        sim->gear = (uint8_t)gear;
    }
}

static void choose_rival_target(BajaSim *sim, BajaRival *rival)
{
    BajaFp gap = rival->s - sim->player_s;
    BajaFp lateral_gap = rival->e - sim->player_e;
    int32_t wander = course_noise(rival->s, 11U + rival->profile);

    if (rival->profile == 0) {
        /* Evasive: protects momentum and moves to the open side. */
        if (gap > 0 && gap < baja_fp_from_int(26) &&
            fp_abs(lateral_gap) < FP_RATIO(45, 100)) {
            rival->target_e = sim->player_e <= 0 ? FP_RATIO(60, 100) : FP_RATIO(-60, 100);
        } else {
            rival->target_e = ((BajaFp)wander * 3) / 2;
        }
        rival->decision_timer = 74;
    } else {
        /* Blocker: shadows the player inside a bounded window only. */
        if (gap > baja_fp_from_int(4) && gap < baja_fp_from_int(34)) {
            rival->target_e = fp_clamp(sim->player_e, FP_RATIO(-70, 100), FP_RATIO(70, 100));
        } else {
            rival->target_e = ((BajaFp)wander * 2);
        }
        rival->decision_timer = 46;
    }
    rival->target_e = fp_clamp(rival->target_e, FP_RATIO(-75, 100), FP_RATIO(75, 100));
}

static void update_rival(BajaSim *sim, BajaRival *rival)
{
    BajaFp gap;
    BajaFp target_speed;
    BajaFp lane_rate;
    BajaFp curve = 0;

    if (!rival->active) return;
    if (rival->collision_cooldown > 0) --rival->collision_cooldown;
    if (rival->decision_timer > 0) --rival->decision_timer;
    else choose_rival_target(sim, rival);

    gap = rival->s - sim->player_s;
    target_speed = rival->preferred_speed;
    if (rival->profile == 0 && gap > 0 && gap < baja_fp_from_int(22)) {
        target_speed += FP_RATIO(4, 100);
    }
    if (rival->profile == 1 && gap > 0 && gap < baja_fp_from_int(18)) {
        target_speed -= FP_RATIO(5, 100);
    }
    /* Rivals lose time in bends exactly like the player does. */
    baja_track_sample(&sim->track, rival->s, NULL, NULL, &curve);
    target_speed -= baja_fp_mul(fp_abs(curve), FP_RATIO(6, 100));
    if (target_speed < FP_RATIO(2, 10)) target_speed = FP_RATIO(2, 10);

    rival->speed = fp_approach(rival->speed, target_speed,
                               rival->speed < target_speed ?
                               FP_RATIO(1, 2000) : FP_RATIO(1, 1200));
    lane_rate = rival->profile == 0 ? FP_RATIO(1, 150) : FP_RATIO(1, 200);
    rival->e = fp_approach(rival->e, rival->target_e, lane_rate);
    rival->e -= baja_fp_mul(baja_fp_mul(curve, CURVE_PULL), FP_RATIO(7, 10));
    rival->e = fp_clamp(rival->e, FP_RATIO(-95, 100), FP_RATIO(95, 100));
    rival->s += rival->speed;
    if (rival->s > sim->track.total_length) rival->s = sim->track.total_length;

    if (rival->was_ahead && rival->s < sim->player_s) {
        rival->was_ahead = 0;
        ++sim->overtakes;
    } else if (!rival->was_ahead && rival->s > sim->player_s + baja_fp_from_int(4)) {
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

        /* Contact scrubs speed toward the rival's pace and pushes the player
         * off the contact side without taking control away. */
        contact_speed = baja_fp_mul(rival->speed, FP_RATIO(85, 100));
        sim->speed = baja_fp_mul(sim->speed, FP_RATIO(72, 100));
        if (sim->speed > contact_speed) sim->speed = contact_speed;
        if (rival->e >= sim->player_e) sim->player_e -= FP_RATIO(11, 100);
        else sim->player_e += FP_RATIO(11, 100);
        sim->player_e = fp_clamp(sim->player_e, -MAX_LATERAL, MAX_LATERAL);
        rival->speed = baja_fp_mul(rival->speed, FP_RATIO(92, 100));
        sim->collision_cooldown = 40;
        rival->collision_cooldown = 40;
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
            sim->driver = (uint8_t)(sim->driver == BAJA_DRIVER_MAX ?
                                    BAJA_DRIVER_CRUZ : BAJA_DRIVER_MAX);
        }
        if ((pressed & BAJA_INPUT_START) != 0) {
            reset_run(sim);
            sim->phase = BAJA_PHASE_COUNTDOWN;
            sim->phase_frame = 0;
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
        ++sim->race_frames;
        update_player(sim, input);
        for (i = 0; i < BAJA_RIVAL_COUNT; ++i) update_rival(sim, &sim->rivals[i]);
        check_collisions(sim);
        update_position(sim);
        break;
    }
    case BAJA_PHASE_FINISHED:
        if ((pressed & BAJA_INPUT_START) != 0 && sim->phase_frame > 60U) {
            reset_run(sim);
            sim->phase = BAJA_PHASE_COUNTDOWN;
            sim->phase_frame = 0;
        }
        break;
    default:
        baja_sim_init(sim);
        break;
    }
    sim->previous_input = input;
}

void baja_view_init(const BajaSim *sim, BajaView *view)
{
    const BajaTrack *track = &sim->track;
    int32_t player_x;
    int32_t segment;
    BajaFp lateral;
    BajaFp rise;
    uint8_t k;
    if (view == NULL) return;
    baja_track_frame_init(track, sim->player_s, &view->frame);
    /* The camera follows the player only part of the way across the road.  A
     * band is drawn by translating one strip, and a translation cannot shear:
     * the further the camera sits from the road's centre line, the more each
     * near band's edge is wrong across its own height.  Tracking at sixty
     * percent keeps that error under a few pixels while the car still crosses
     * the screen visibly. */
    view->camera_lateral = (baja_fp_mul(sim->player_e, WORLD_ROAD_HALF) *
                            BAJA_CAMERA_TRACK_Q8) >> 8;
    view->camera_rise = CAMERA_HEIGHT;
    view->shake = (int16_t)baja_fp_to_int(baja_fp_mul(sim->bounce, SHAKE_PIXELS));
    /* Where the player's berth lands on screen: the rest of the lateral offset
     * the camera did not absorb, at the berth's own scale.  Beyond the road's
     * edge the excess is halved so the car never leaves the frame. */
    player_x = (baja_fp_mul(sim->player_e, WORLD_ROAD_HALF) - view->camera_lateral);
    /* 8.8 metres times 8.8 pixels per metre stays inside 32 bits. */
    player_x = ((player_x >> 8) * BAJA_PLAYER_SCALE_Q8) >> 16;
    if (player_x > PLAYER_SOFT_EDGE) {
        player_x = PLAYER_SOFT_EDGE + ((player_x - PLAYER_SOFT_EDGE) >> 1);
    } else if (player_x < -PLAYER_SOFT_EDGE) {
        player_x = -PLAYER_SOFT_EDGE + ((player_x + PLAYER_SOFT_EDGE) >> 1);
    }
    view->player_x = (int16_t)(BAJA_SCREEN_CENTER + player_x);

    /* Walk the segments ahead once.  In the road-tangent frame the road's
     * lateral offset and rise are both piecewise linear between segment
     * boundaries, and each boundary's value follows from the last with two
     * additions, so every later sample is a single interpolation. */
    segment = (int32_t)(view->frame.base_s >> BAJA_SEGMENT_SHIFT);
    if (segment >= BAJA_TRACK_SEGMENTS) segment = BAJA_TRACK_SEGMENTS - 1;
    view->local = view->frame.base_s & (SEGMENT_LENGTH - 1);
    lateral = track->center_x[segment] - view->frame.base_x +
              baja_fp_mul(view->frame.heading, view->local >> SEGMENT_T_SHIFT);
    rise = track->height[segment] - view->frame.base_y +
           baja_fp_mul(view->frame.grade, view->local >> SEGMENT_T_SHIFT);
    for (k = 0; k <= BAJA_VIEW_SEGMENTS; ++k) {
        int32_t index = segment + k;
        view->seg_lateral[k] = lateral;
        view->seg_rise[k] = rise;
        /* Past the end the course runs on straight and level. */
        if (index < BAJA_TRACK_SEGMENTS) {
            lateral += track->center_x[index + 1] - track->center_x[index] - view->frame.heading;
            rise += track->height[index + 1] - track->height[index] - view->frame.grade;
        }
    }
}

void baja_view_sample(const BajaView *view, BajaFp depth, BajaFp *lateral, BajaFp *rise)
{
    BajaFp ahead = view->local + depth;
    uint32_t k = (uint32_t)ahead >> BAJA_SEGMENT_SHIFT;
    int32_t t8;
    if (ahead < 0) { k = 0; ahead = 0; }
    if (k >= BAJA_VIEW_SEGMENTS) {
        if (lateral != NULL) *lateral = view->seg_lateral[BAJA_VIEW_SEGMENTS];
        if (rise != NULL) *rise = view->seg_rise[BAJA_VIEW_SEGMENTS];
        return;
    }
    /* Eight bits of fraction: one 16 by 16 multiply per interpolation. */
    t8 = (int32_t)(((uint32_t)ahead >> (BAJA_SEGMENT_SHIFT - 8)) & 0xffU);
    if (lateral != NULL) {
        *lateral = view->seg_lateral[k] +
                   (int32_t)(int16_t)((view->seg_lateral[k + 1] - view->seg_lateral[k]) >> 8) * t8;
    }
    if (rise != NULL) {
        *rise = view->seg_rise[k] +
                (int32_t)(int16_t)((view->seg_rise[k + 1] - view->seg_rise[k]) >> 8) * t8;
    }
}

/* Fraction of a segment as eight bits, so an interpolation is one 16 by 16
 * multiply: the segment delta in 8.8 metres times the fraction in 256ths. */
#define SEGMENT_T8_SHIFT (BAJA_SEGMENT_SHIFT - 8)

uint8_t baja_project_bands_in(const BajaSim *sim, const BajaView *view,
                              BajaRoadBand *bands)
{
    int16_t edge_y[BAJA_ROAD_BANDS + 1];
    BajaFp edge_lateral[BAJA_ROAD_BANDS + 1];
    const BajaFp *seg_lateral;
    const BajaFp *seg_rise;
    BajaFp local;
    BajaFp camera_lateral;
    int32_t camera_rise;
    int16_t horizon;
    int16_t limit;
    int32_t i;
    int32_t b;

    if (bands == NULL || view == NULL) return 0;
    seg_lateral = view->seg_lateral;
    seg_rise = view->seg_rise;
    local = view->local;
    camera_lateral = view->camera_lateral;
    camera_rise = view->camera_rise;
    horizon = (int16_t)(BAJA_HORIZON_Y + view->shake);

    /* One interpolation per band boundary gives both the row it projects to
     * and the road's lateral position there.  A band's centre is the mean of
     * its two boundaries, which is the translation that halves the shear
     * error at each end of the band. */
    for (i = 0; i <= BAJA_ROAD_BANDS; ++i) {
        BajaFp ahead = local + band_edge_depth_fp[i];
        uint32_t k = (uint32_t)ahead >> BAJA_SEGMENT_SHIFT;
        int32_t t8 = (int32_t)(((uint32_t)ahead >> SEGMENT_T8_SHIFT) & 0xffU);
        int32_t lateral = seg_lateral[k] +
            (int32_t)(int16_t)((seg_lateral[k + 1] - seg_lateral[k]) >> 8) * t8;
        int32_t rise = seg_rise[k] +
            (int32_t)(int16_t)((seg_rise[k + 1] - seg_rise[k]) >> 8) * t8;
        int32_t drop = (int32_t)(int16_t)((camera_rise - rise) >> 8) *
                       (int32_t)(int16_t)(band_edge_scale_fp[i] >> 8);
        edge_lateral[i] = lateral;
        edge_y[i] = (int16_t)(horizon + (int16_t)(drop >> 16));
    }

    limit = BAJA_SCREEN_HEIGHT;
    for (b = BAJA_ROAD_BANDS - 1; b >= 0; --b) {
        BajaRoadBand *band = &bands[b];
        int32_t lateral;
        int16_t top = edge_y[b];
        int16_t bottom = edge_y[b + 1];

        band->reserved = 0;
        if (bottom > limit) bottom = limit;
        if (top < 0) top = 0;
        if (top >= bottom || bottom <= 0) {
            band->visible = 0;
            band->height = 0;
            band->top_y = (int16_t)BAJA_SCREEN_HEIGHT;
            band->center_x = BAJA_SCREEN_CENTER;
            band->phase = 0;
            continue;
        }
        lateral = ((edge_lateral[b] + edge_lateral[b + 1]) >> 1) - camera_lateral;
        lateral = (int32_t)(int16_t)(lateral >> 8) * (int32_t)(int16_t)(band_scale_fp[b] >> 8);
        band->center_x = (int16_t)(BAJA_SCREEN_CENTER + (int16_t)(lateral >> 16));
        band->top_y = top;
        band->height = (uint8_t)((bottom - top) > 255 ? 255 : (bottom - top));
        band->phase = (uint8_t)(((uint32_t)(sim->player_s + band_depth_fp[b]) >>
                                 baja_band_stripe_shift[b]) & 1U);
        band->visible = 1;
        limit = top;
    }
    return BAJA_ROAD_BANDS;
}

uint8_t baja_project_bands(const BajaSim *sim, BajaRoadBand *bands)
{
    BajaView view;
    baja_view_init(sim, &view);
    return baja_project_bands_in(sim, &view, bands);
}

void baja_project_object_in(const BajaSim *sim, const BajaView *view,
                            BajaFp object_s, BajaFp object_e,
                            BajaObjectProjection *projection)
{
    BajaFp depth;
    BajaFp lateral = 0;
    BajaFp rise = 0;
    int32_t scale;
    int32_t x;
    int32_t y;
    uint8_t band;

    if (projection == NULL) return;
    projection->visible = 0;
    projection->reserved[0] = 0;
    projection->reserved[1] = 0;
    projection->band = BAJA_ROAD_BANDS - 1;
    projection->screen_x = 0;
    projection->ground_y = 0;
    projection->depth = 0;
    projection->scale_q8 = 0;
    depth = object_s - sim->player_s;
    if (depth <= band_depth_fp[BAJA_ROAD_BANDS - 1] || depth >= baja_fp_from_int(260)) {
        return;
    }

    baja_view_sample(view, depth, &lateral, &rise);
    lateral += object_e * 4 - view->camera_lateral;
    /* Anything more than a hundred metres to the side is off screen at every
     * depth this projection handles, and would overflow the 8.8 products. */
    if (lateral >= baja_fp_from_int(120) || lateral <= -baja_fp_from_int(120)) return;

    /* Screen pixels per metre at this depth, from the table. */
    if (depth < baja_fp_from_int(32)) {
        scale = scale_by_depth[(uint32_t)depth >> SCALE_FINE_SHIFT];
    } else {
        scale = scale_by_depth[SCALE_FINE_ENTRIES + ((uint32_t)depth >> BAJA_FP_SHIFT) - 32U];
    }
    /* Half the scale keeps both factors signed 16-bit for one hardware
     * multiply; the shift takes the half back out. */
    x = (int32_t)(int16_t)(lateral >> 8) * (int32_t)(int16_t)(scale >> 1);
    y = (int32_t)(int16_t)((view->camera_rise - rise) >> 8) * (int32_t)(int16_t)(scale >> 1);
    projection->screen_x = (int16_t)(BAJA_SCREEN_CENTER + (x >> 15));
    projection->ground_y = (int16_t)(BAJA_HORIZON_Y + view->shake + (y >> 15));
    projection->depth = (uint16_t)(depth >> BAJA_FP_SHIFT);
    projection->scale_q8 = (uint16_t)scale;

    band = band_by_depth[(uint32_t)depth >> BAJA_FP_SHIFT];
    projection->band = band;
    projection->visible = (uint8_t)(projection->screen_x > -96 &&
                                    projection->screen_x < 416 &&
                                    projection->ground_y > BAJA_HORIZON_Y - 8 &&
                                    projection->ground_y < BAJA_SCREEN_HEIGHT + 64);
}

void baja_project_object(const BajaSim *sim, BajaFp object_s, BajaFp object_e,
                         BajaObjectProjection *projection)
{
    BajaView view;
    baja_view_init(sim, &view);
    baja_project_object_in(sim, &view, object_s, object_e, projection);
}
