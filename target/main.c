#include <ngdevkit/neogeo.h>
#include <ngdevkit/ng-fix.h>
#include <ngdevkit/ng-video.h>
#include <stdio.h>

#include "../game/sim.h"
#include "assets.generated.h"

#define PAL_SPLASH 1u
#define PAL_HORIZON 2u
#define PAL_PLAYER 3u
#define PAL_RIVAL 4u
#define PAL_PORTRAITS 5u
#define PAL_ROAD 6u
#define PAL_PROPS 7u

#define ROAD_COLS 36u
#define ROAD_ROWS 9u
#define ROAD_FRAME_TILES (ROAD_COLS * ROAD_ROWS)
#define SPR_HORIZON 1u
#define SPR_ROAD 21u
#define SPR_PLAYER (SPR_ROAD + ROAD_FRAME_TILES)
#define SPR_RIVAL (SPR_PLAYER + 8u)
#define SPR_PROPS (SPR_RIVAL + 5u)

static const u16 palettes[][16] = {
    {0x8000, 0x0fff, 0x6ddd, 0x055f, 0x0ff0, 0x0fa0, 0x0888, 0x0444,
     0x0000, 0x0f22, 0x02af, 0x0aa0, 0x0fff, 0x0000, 0x0000, 0x0000},
#include "splash.pal"
#include "horizon.pal"
#include "player.pal"
#include "rival.pal"
#include "portraits.pal"
#include "roadtiles.pal"
#include "props.pal"
};

static BajaSim sim;
static enum BajaScene rendered_scene = (enum BajaScene)255;
static volatile u8 bios_start_pulse;

void player_start(void) {
    bios_user_mode = 2;
    bios_player_mod1 = 1;
    bios_start_pulse = 1;
}

void coin_sound(void) {}

static void load_palettes(void) {
    const u16 *colors = (const u16 *)palettes;
    for (u16 i = 0; i < (u16)(sizeof(palettes) / sizeof(u16)); ++i) {
        MMAP_PALBANK1[i] = colors[i];
    }
    *((volatile u16 *)0x401ffe) = 0x4237;
}

static u16 scb3_position(u16 screen_y, u16 height) {
    return (u16)((((0x200u - screen_y) & 0x1ffu) << 7) | (height & 0x3fu));
}

static void hide_all_sprites(void) {
    *REG_VRAMMOD = 1;
    *REG_VRAMADDR = ADDR_SCB3;
    for (u16 i = 0; i < 381u; ++i) *REG_VRAMRW = 0u;
}

static void place_bitmap(u16 sprite, u16 tile_base, u16 palette, u16 width,
                         u16 height, s16 x, u16 y, u16 zoom) {
    const u16 attr = (u16)(palette << 8);
    for (u16 column = 0; column < width; ++column) {
        *REG_VRAMMOD = 1;
        *REG_VRAMADDR = (u16)(ADDR_SCB1 + (sprite + column) * 64u);
        u16 tile = (u16)(tile_base + column);
        for (u16 row = 0; row < height; ++row, tile = (u16)(tile + width)) {
            *REG_VRAMRW = tile;
            *REG_VRAMRW = attr;
        }
        *REG_VRAMMOD = 0x200;
        *REG_VRAMADDR = (u16)(ADDR_SCB2 + sprite + column);
        *REG_VRAMRW = zoom;
        if (column == 0u) {
            *REG_VRAMRW = scb3_position(y, height);
            *REG_VRAMRW = (u16)((x & 0x1ff) << 7);
        } else {
            *REG_VRAMRW = 1u << 6;
        }
    }
}

static void setup_horizon(void) {
    place_bitmap(SPR_HORIZON, TILE_HORIZON, PAL_HORIZON, 20u, 5u, 0, 0u, 0x0fffu);
}

static int16_t road_center_for_row(u16 row) {
    int16_t bend = (int16_t)((sim.road_curve * (int16_t)(8u - row)) / 72);
    return (int16_t)(160 + bend);
}

static u16 road_half_width(u16 row) {
    return (u16)(25u + row * 14u);
}

static void draw_road(void) {
    const u16 scroll = (u16)((sim.course_pos >> 5) & 3u);
    const u16 frame_base = (u16)(TILE_ROADTILES + scroll * ROAD_FRAME_TILES);
    for (u16 row = 0; row < ROAD_ROWS; ++row) {
        s16 bend = (s16)(road_center_for_row(row) - 160);
        for (u16 column = 0; column < ROAD_COLS; ++column) {
            u16 sprite = (u16)(SPR_ROAD + row * ROAD_COLS + column);
            u16 tile = (u16)(frame_base + row * ROAD_COLS + column);
            place_bitmap(sprite, tile, PAL_ROAD, 1u, 1u,
                         (s16)(column * 16u - 128 + bend), (u16)(80u + row * 16u), 0x0fffu);
        }
    }
}

static void hide_sprite_columns(u16 sprite, u16 width) {
    *REG_VRAMMOD = 1;
    *REG_VRAMADDR = (u16)(ADDR_SCB3 + sprite);
    for (u16 column = 0; column < width; ++column) *REG_VRAMRW = 0u;
}

static void draw_props(void) {
    for (u16 index = 0; index < 6u; ++index) {
        u32 phase = (sim.course_pos / 28u + index * 113u) % 720u;
        u16 depth = (u16)(phase / 80u);
        u16 y = (u16)(82u + depth * 15u);
        u16 half = road_half_width(depth > 8u ? 8u : depth);
        int16_t center = road_center_for_row(depth > 8u ? 8u : depth);
        int16_t x = (int16_t)(center + ((index & 1u) ? (int16_t)half + 7 : -(int16_t)half - 38));
        u16 scale = (u16)(4u + depth);
        if (scale > 15u) scale = 15u;
        u16 zoom = (u16)((scale << 8) | (scale * 17u));
        u16 prop = (u16)((index + (sim.course_pos / 9000u)) % 8u);
        place_bitmap((u16)(SPR_PROPS + index * 2u),
                     (u16)(TILE_PROPS + prop * 4u), PAL_PROPS,
                     2u, 2u, x, y, zoom);
    }
}

static void draw_player(void) {
    u16 frame = 0u;
    if (sim.collision_frames) frame = 2u;
    else if (sim.rough_frames && ((sim.race_frames >> 2) & 1u)) frame = 4u;
    else if (sim.steer < -16) frame = 1u;
    else if (sim.steer > 16) frame = 3u;
    s16 x = (s16)(96 + sim.player_x / 24);
    u16 y = (u16)(128u + ((sim.rough_frames && (sim.race_frames & 2u)) ? 2u : 0u));
    place_bitmap(SPR_PLAYER, (u16)(TILE_PLAYER + frame * 48u), PAL_PLAYER,
                 8u, 6u, x, y, 0x0fffu);
}

static void draw_rival(void) {
    if (sim.rival_pos + 400u < sim.course_pos) {
        hide_sprite_columns(SPR_RIVAL, 5u);
        return;
    }
    u32 dz = sim.rival_pos > sim.course_pos ? sim.rival_pos - sim.course_pos : 0u;
    if (dz > 21000u) {
        hide_sprite_columns(SPR_RIVAL, 5u);
        return;
    }
    u16 frame = dz > 14000u ? 0u : dz > 7000u ? 1u : dz > 2200u ? 2u : 3u;
    u16 y = (u16)(82u + (21000u - dz) / 310u);
    if (y > 132u) y = 132u;
    s16 x = (s16)(120 + sim.rival_x / 20 + sim.road_curve / 12);
    place_bitmap(SPR_RIVAL, (u16)(TILE_RIVAL + frame * 20u), PAL_RIVAL,
                 5u, 4u, x, y, 0x0fffu);
}

static void draw_race_world(void) {
    setup_horizon();
    draw_road();
    draw_props();
    draw_rival();
    draw_player();
}

static void fixed_text(u8 x, u8 y, const char *text) {
    ng_text(x, y, 0u, text);
}

static void draw_hud(void) {
    char text[40];
    fixed_text(1, 2, "POSITION");
    snprintf(text, sizeof(text), "%u/2", sim.position);
    fixed_text(2, 3, text);
    fixed_text(15, 2, "TIME");
    snprintf(text, sizeof(text), "%02lu.%u",
             (unsigned long)(sim.race_frames / 60u),
             (unsigned)((sim.race_frames / 6u) % 10u));
    fixed_text(15, 3, text);
    fixed_text(30, 2, "BEST --.--");
    snprintf(text, sizeof(text), "%3u KM/H G%u   ",
             (unsigned)(sim.speed / 6u),
             sim.speed < 180u ? 1u : sim.speed < 420u ? 2u : sim.speed < 700u ? 3u : 4u);
    fixed_text(1, 27, text);
    snprintf(text, sizeof(text), "ENSENADA %3lu%%",
             (unsigned long)((sim.course_pos * 100u) / sim.finish_pos));
    fixed_text(25, 27, text);
    if (sim.offroad) fixed_text(14, 25, "ROUGH SHOULDER");
    else if (sim.collision_frames) fixed_text(16, 25, "CONTACT!");
    else fixed_text(14, 25, "              ");
}

static void enter_scene(enum BajaScene scene) {
    hide_all_sprites();
    ng_cls();
    switch (scene) {
        case BAJA_SPLASH:
            place_bitmap(1u, TILE_SPLASH, PAL_SPLASH, 20u, 14u, 0, 0u, 0x0fffu);
            break;
        case BAJA_TITLE:
            setup_horizon();
            draw_road();
            place_bitmap(SPR_PLAYER, TILE_PLAYER, PAL_PLAYER, 8u, 6u, 96, 128u, 0x0fffu);
            ng_center_text(5, 0u, "BAJA OUTRUN");
            ng_center_text(7, 0u, "MAX CRUZ RACING");
            ng_center_text(12, 0u, "PRESS PLAYER 1 START");
            break;
        case BAJA_SELECT:
            setup_horizon();
            place_bitmap(41u, TILE_PORTRAITS, PAL_PORTRAITS, 8u, 8u, 16, 62u, 0x0fffu);
            place_bitmap(49u, (u16)(TILE_PORTRAITS + 64u), PAL_PORTRAITS, 8u, 8u, 176, 62u, 0x0fffu);
            ng_center_text(4, 0u, "CHOOSE YOUR RACER");
            fixed_text(8, 25, "MAX  #8");
            fixed_text(28, 25, "CRUZ #6");
            ng_center_text(28, 0u, "LEFT/RIGHT   A CONFIRM");
            break;
        case BAJA_COUNTDOWN:
        case BAJA_RACE:
        case BAJA_FINISH:
            draw_race_world();
            break;
    }
}

static void update_scene_text(void) {
    if (sim.scene == BAJA_SELECT) {
        fixed_text(5, 25, sim.selected_racer == 0u ? ">>" : "  ");
        fixed_text(25, 25, sim.selected_racer == 1u ? ">>" : "  ");
    } else if (sim.scene == BAJA_COUNTDOWN) {
        draw_hud();
        if (sim.scene_frames < 60u) ng_center_text(13, 0u, "READY");
        else if (sim.scene_frames < 120u) ng_center_text(13, 0u, "3");
        else if (sim.scene_frames < 180u) ng_center_text(13, 0u, "2");
        else ng_center_text(13, 0u, "1");
    } else if (sim.scene == BAJA_RACE) {
        draw_race_world();
        draw_hud();
    } else if (sim.scene == BAJA_FINISH) {
        draw_race_world();
        draw_hud();
        ng_center_text(12, 0u, "ENSENADA FINISH");
        ng_center_text(14, 0u, "A OR START TO RESTART");
    }
}

static u8 read_input(void) {
    u8 input = 0u;
    if (bios_p1current & CNT_LEFT) input |= BAJA_IN_LEFT;
    if (bios_p1current & CNT_RIGHT) input |= BAJA_IN_RIGHT;
    if (bios_p1current & CNT_A) input |= BAJA_IN_A;
    if (bios_p1current & CNT_B) input |= BAJA_IN_B;
    if ((bios_statcurnt & CNT_START1) || bios_start_pulse) input |= BAJA_IN_START;
    return input;
}

static int run_game(u8 show_splash) {
    bios_fix_clear();
    load_palettes();
    baja_sim_init(&sim);
    if (!show_splash) {
        sim.scene = BAJA_TITLE;
        sim.scene_frames = 0u;
    }
    rendered_scene = (enum BajaScene)255;
    bios_start_pulse = 0u;

    for (;;) {
        if (rendered_scene != sim.scene) {
            rendered_scene = sim.scene;
            enter_scene(sim.scene);
        }
        update_scene_text();
        ng_wait_vblank();
        u8 input = read_input();
        baja_sim_step(&sim, input);
        bios_start_pulse = 0u;
    }
    return 0;
}

int main(void) {
    return run_game(1u);
}

int main_mvs_title(void) {
    return run_game(0u);
}
