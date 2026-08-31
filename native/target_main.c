#include "game/bajanew.h"
#include "game/telemetry.h"
#include "ng/audio.h"
#include "ng/bank.h"
#include "ng/crash.h"
#include "ng/platform.h"
#include "ng/system.h"
#include "ng/telemetry.h"
#include "ng/timing.h"

volatile NgTelemetry ng_telemetry __attribute__((section(".telemetry")));
volatile BajanewTelemetry bajanew_telemetry __attribute__((section(".game_telemetry")));
static BajanewGame ng_game;
static NgAudio ng_audio;
static NgProgramBanks ng_banks;
static NgRasterSchedule ng_raster;

extern volatile uint32_t ng_vblank_counter;

static void raster_event(const NgRasterEvent *event)
{
    if (event->channel == 0U) ng_platform_backdrop(event->value);
}

static void clear_telemetry(void)
{
    volatile uint8_t *bytes = (volatile uint8_t *)&ng_telemetry;
    uint16_t index;
    for (index = 0; index < (uint16_t)sizeof(ng_telemetry); ++index) bytes[index] = 0U;
    ng_telemetry.abi_version = NG_TELEMETRY_ABI_VERSION;
    ng_telemetry.abi_size = NG_TELEMETRY_ABI_SIZE;

    bytes = (volatile uint8_t *)&bajanew_telemetry;
    for (index = 0; index < (uint16_t)sizeof(bajanew_telemetry); ++index) bytes[index] = 0U;
    bajanew_telemetry.abi_version = BAJANEW_TELEMETRY_ABI_VERSION;
    bajanew_telemetry.abi_size = BAJANEW_TELEMETRY_ABI_SIZE;
}

static void update_game_telemetry(NgPad pad)
{
    bajanew_telemetry.frame = ng_game.frame;
    bajanew_telemetry.player_s = ng_game.sim.player_s;
    bajanew_telemetry.player_e = ng_game.sim.player_e;
    bajanew_telemetry.speed = ng_game.sim.speed;
    bajanew_telemetry.rival0_s = ng_game.sim.rivals[0].s;
    bajanew_telemetry.rival0_e = ng_game.sim.rivals[0].e;
    bajanew_telemetry.rival0_speed = ng_game.sim.rivals[0].speed;
    bajanew_telemetry.rival1_s = ng_game.sim.rivals[1].s;
    bajanew_telemetry.rival1_e = ng_game.sim.rivals[1].e;
    bajanew_telemetry.rival1_speed = ng_game.sim.rivals[1].speed;
    bajanew_telemetry.collisions = ng_game.sim.collisions;
    bajanew_telemetry.overtakes = ng_game.sim.overtakes;
    bajanew_telemetry.input = pad.held;
    bajanew_telemetry.phase = ng_game.sim.phase;
    bajanew_telemetry.driver = ng_game.sim.driver;
    bajanew_telemetry.surface = ng_game.sim.surface;
    bajanew_telemetry.position = ng_game.sim.position;
    bajanew_telemetry.steer = ng_game.sim.steer;
}

static void update_telemetry(void)
{
    const NgSystemState *system = ng_system_state();
    ng_telemetry.frames = ng_game.frame;
    ng_telemetry.vblanks = ng_vblank_counter;
    ng_telemetry.active_columns = ng_game.last_render.active_columns;
    ng_telemetry.vram_writes = ng_game.last_render.vram_writes;
    ng_telemetry.dropped_columns = ng_game.last_render.dropped_columns;
    ng_telemetry.dropped_commands = ng_game.last_render.dropped_commands;
    ng_telemetry.culled_commands = ng_game.last_render.culled_commands;
    ng_telemetry.peak_scanline_columns = ng_game.last_render.peak_scanline_columns;
    ng_telemetry.peak_scanline = ng_game.last_render.peak_scanline;
    ng_telemetry.overloaded_scanlines = ng_game.last_render.overloaded_scanlines;
    ng_telemetry.scene_missing_chunk = 0;
    ng_telemetry.scene_chunk = 0;
    ng_telemetry.system_hardware = system->hardware;
    ng_telemetry.system_user_request = system->user_request;
    ng_telemetry.system_user_mode = system->user_mode;
    ng_telemetry.system_start_flags = system->start_flags;
    ng_telemetry.audio_driver_ready = ng_audio.driver_ready;
    ng_telemetry.audio_last_command = ng_audio.last_sent;
    ng_telemetry.audio_last_result = ng_audio.last_result;
    ng_telemetry.audio_queued = ng_audio.count;
    ng_telemetry.audio_dropped = ng_audio.dropped;
    ng_telemetry.program_bank = ng_banks.active;
    ng_telemetry.raster_event_count = ng_raster.count;
    ng_telemetry.raster_conflicts = ng_raster.conflicts;
    ng_telemetry.raster_overflow = ng_raster.overflow;
    ng_telemetry.crash_valid = ng_crash_valid();
    ng_telemetry.player_starts = (uint16_t)system->player_start_count;
    ng_telemetry.coins = (uint16_t)system->coin_count;
    ng_telemetry.demo_ends = system->demo_end_count;
    ng_telemetry.bank_switches = ng_banks.switches;
    ng_telemetry.bank_invalid_switches = ng_banks.invalid_switches;
    ng_telemetry.raster_events_fired = ng_raster.fired;
}

void ng_target_main(void)
{
    uint32_t deadline;
    clear_telemetry();
    ng_system_init();
    ng_system_set_user_mode(1);
    ng_audio_init(&ng_audio);
    ng_program_banks_init(&ng_banks, 4);
    ng_raster_init(&ng_raster);
    (void)ng_raster_add(&ng_raster, 72, 0, 0x8000U, 0);
    (void)ng_audio_enqueue(&ng_audio, NG_AUDIO_CMD_PLAY_MUSIC);
    bajanew_game_init(&ng_game);
    update_game_telemetry((NgPad){0, 0, 0});
    bajanew_telemetry.magic = BAJANEW_TELEMETRY_MAGIC;
    ng_telemetry.magic = NG_TELEMETRY_MAGIC;
    ng_platform_enable_interrupts();

    deadline = ng_vblank_counter + BAJANEW_FIELDS_PER_FRAME;
    for (;;) {
        NgPad pad;
        /* Pace on the vblank counter rather than counting waits: a frame that
         * runs long must not push the next one a whole field further out. */
        while ((int32_t)(ng_vblank_counter - deadline) < 0) {
            ng_platform_wait_vblank();
        }
        deadline = ng_vblank_counter + BAJANEW_FIELDS_PER_FRAME;
        (void)ng_raster_activate(&ng_raster, ng_timing_profile(NG_VIDEO_NTSC), raster_event);
        pad = ng_platform_read_pad(1);
        bajanew_game_tick(&ng_game, pad);
        ng_audio_update(&ng_audio);
        update_telemetry();
        update_game_telemetry(pad);
        if (ng_system_state()->return_requested) {
            ng_audio_stop_all(&ng_audio);
            ng_audio_update(&ng_audio);
            ng_system_return_to_bios();
        }
    }
}
