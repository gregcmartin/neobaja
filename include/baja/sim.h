#ifndef BAJA_SIM_H
#define BAJA_SIM_H

#include <stdint.h>

#define BAJA_FP_SHIFT 16
#define BAJA_FP_ONE ((int32_t)1 << BAJA_FP_SHIFT)
#define BAJA_TRACK_SEGMENTS 384
#define BAJA_RIVAL_COUNT 3
#define BAJA_SCENERY_COUNT 48

/* Screen contract for the projection.  The renderer may quantise these to
 * sprite tiles and zoom steps but never feeds them back into gameplay. */
#define BAJA_SCREEN_WIDTH 320
#define BAJA_SCREEN_HEIGHT 224
#define BAJA_SCREEN_CENTER 160
#define BAJA_HORIZON_Y 84
#define BAJA_ROAD_BANDS 19

typedef int32_t BajaFp;
typedef void (*BajaServiceHook)(void);

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

/* Scenery kinds are gameplay-visible because the course owns their world
 * placement; the renderer only chooses which sheet frame draws them. */
typedef enum BajaSceneryKind {
    BAJA_SCENERY_ROCK_PALE = 0,
    BAJA_SCENERY_ROCK_GREY,
    BAJA_SCENERY_AGAVE,
    BAJA_SCENERY_BUSH,
    BAJA_SCENERY_PALM,
    BAJA_SCENERY_CACTUS,
    BAJA_SCENERY_CHEVRON,
    BAJA_SCENERY_FLAG,
    BAJA_SCENERY_CROWD,
    BAJA_SCENERY_KINDS
} BajaSceneryKind;

typedef struct BajaTrack {
    BajaFp center_x[BAJA_TRACK_SEGMENTS + 1];
    BajaFp height[BAJA_TRACK_SEGMENTS + 1];
    BajaFp curvature[BAJA_TRACK_SEGMENTS];
    BajaFp grade[BAJA_TRACK_SEGMENTS];
    BajaFp segment_length;
    BajaFp total_length;
} BajaTrack;

typedef struct BajaScenery {
    BajaFp s;
    BajaFp e;
    uint8_t kind;
    uint8_t reserved[3];
} BajaScenery;

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
    BajaScenery scenery[BAJA_SCENERY_COUNT];
    BajaFp player_s;
    BajaFp player_e;
    BajaFp speed;
    BajaFp steer;
    BajaFp bounce;           /* suspension travel, world units */
    BajaFp bounce_rate;
    BajaRival rivals[BAJA_RIVAL_COUNT];
    uint32_t frame;
    uint32_t phase_frame;
    uint32_t race_frames;
    uint32_t collisions;
    uint32_t overtakes;
    uint16_t collision_cooldown;
    uint16_t rough_timer;
    uint8_t phase;
    uint8_t driver;
    uint8_t surface;
    uint8_t position;
    uint8_t previous_input;
    uint8_t collision_event;
    uint8_t dust_event;
    uint8_t gear;
} BajaSim;

/* One horizontal slice of the projected road.  A band owns a fixed depth
 * interval, so its width never changes; only its row, centre, and surface
 * phase move. */
typedef struct BajaRoadBand {
    int16_t center_x;
    int16_t top_y;
    uint8_t height;
    uint8_t phase;
    uint8_t visible;
    uint8_t reserved;
} BajaRoadBand;

typedef struct BajaObjectProjection {
    int16_t screen_x;     /* projected centre column */
    int16_t ground_y;     /* projected contact point with the road surface */
    uint16_t depth;       /* whole world units ahead of the camera */
    uint16_t scale_q8;    /* screen pixels per world unit, 8.8 fixed */
    uint8_t band;         /* band index owning this depth, for draw order */
    uint8_t visible;
    uint8_t reserved[2];
} BajaObjectProjection;

BajaFp baja_fp_from_int(int32_t value);
int32_t baja_fp_to_int(BajaFp value);
BajaFp baja_fp_mul(BajaFp a, BajaFp b);
BajaFp baja_fp_div(BajaFp a, BajaFp b);

void baja_track_init(BajaTrack *track);
void baja_track_init_cooperative(BajaTrack *track, BajaServiceHook service_hook);
void baja_track_sample(const BajaTrack *track, BajaFp s, BajaFp *x,
                       BajaFp *y, BajaFp *curve);
BajaFp baja_track_heading(const BajaTrack *track, BajaFp s);

/* Road-tangent view frame taken at the camera, so projection measures only
 * real curvature and grade. */
typedef struct BajaTrackFrame {
    BajaFp base_s;
    BajaFp base_x;
    BajaFp base_y;
    BajaFp heading;
    BajaFp grade;
} BajaTrackFrame;

void baja_track_frame_init(const BajaTrack *track, BajaFp base_s,
                           BajaTrackFrame *frame);
void baja_track_frame_sample(const BajaTrack *track, const BajaTrackFrame *frame,
                             BajaFp s, BajaFp *lateral, BajaFp *rise);

void baja_sim_init(BajaSim *sim);
void baja_sim_init_cooperative(BajaSim *sim, BajaServiceHook service_hook);
void baja_sim_step(BajaSim *sim, uint8_t input);
void baja_sim_begin_race(BajaSim *sim);

/* Fixed funnel geometry, shared by the projection and the strip generator. */
extern const int16_t baja_band_dy[BAJA_ROAD_BANDS + 1];
extern const int16_t baja_band_half_width[BAJA_ROAD_BANDS];

uint8_t baja_project_bands(const BajaSim *sim, BajaRoadBand *bands);
void baja_project_object(const BajaSim *sim, BajaFp object_s, BajaFp object_e,
                         BajaObjectProjection *projection);

#endif
