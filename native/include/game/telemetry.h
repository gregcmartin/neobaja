#ifndef GAME_BAJANEW_TELEMETRY_H
#define GAME_BAJANEW_TELEMETRY_H

#include <stdint.h>

#define BAJANEW_TELEMETRY_MAGIC 0x42414a41UL
#define BAJANEW_TELEMETRY_ABI_VERSION 1U
#define BAJANEW_TELEMETRY_ABI_SIZE 64U

typedef struct BajanewTelemetry {
    uint32_t magic;
    uint32_t frame;
    int32_t player_s;
    int32_t player_e;
    int32_t speed;
    int32_t rival0_s;
    int32_t rival0_e;
    int32_t rival0_speed;
    int32_t rival1_s;
    int32_t rival1_e;
    int32_t rival1_speed;
    uint32_t collisions;
    uint32_t overtakes;
    uint16_t input;
    uint8_t abi_version;
    uint8_t abi_size;
    uint8_t phase;
    uint8_t driver;
    uint8_t surface;
    uint8_t position;
    int32_t steer;
} BajanewTelemetry;

_Static_assert(sizeof(BajanewTelemetry) == BAJANEW_TELEMETRY_ABI_SIZE,
               "BAJANEW telemetry ABI must remain 64 bytes");

extern volatile BajanewTelemetry bajanew_telemetry;

#endif
