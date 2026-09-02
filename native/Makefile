ROOT := ..
FORGE68 := $(ROOT)/sdk/forge68
BUILD := $(ROOT)/build/native
GEN := $(ROOT)/build/assets/generated
AUDIO_GEN := $(FORGE68)/build/generated
ROMDIR := $(BUILD)/roms

PYTHON ?= python3
HOST_CC ?= cc
M68K_CC ?= m68k-elf-gcc
M68K_OBJCOPY ?= m68k-elf-objcopy
M68K_SIZE ?= m68k-elf-size
MAME ?= mame
MAME_BIOS_DIR ?=

WARNINGS := -Wall -Wextra -Werror -Wshadow
INCLUDES := -I$(ROOT)/include -I$(ROOT)/native/include -I$(FORGE68)/include -I$(GEN)
# -MMD/-MP: without generated header dependencies a changed struct layout
# silently links stale objects together, which is exactly how a game boots
# and then reads its own state at the wrong offsets.
DEPFLAGS := -MMD -MP
HOST_CFLAGS := -std=c11 -O2 $(WARNINGS) $(INCLUDES) $(DEPFLAGS) -DNG_HOST
M68K_CFLAGS := -m68000 -std=c11 -O2 -g3 -gdwarf-2 -ffreestanding -fno-builtin \
	-ffunction-sections -fdata-sections -fomit-frame-pointer $(WARNINGS) $(INCLUDES) \
	$(DEPFLAGS)
M68K_LDFLAGS := -m68000 -nostdlib -Wl,-T,$(ROOT)/native/link.ld \
	-Wl,-Map,$(BUILD)/m68k/bajanew.map -Wl,--gc-sections

ENGINE_REL := animation audio bank camera collision crash entity fix gameplay strip \
	persistence renderer scene system timing
HOST_ENGINE_OBJECTS := $(addprefix $(BUILD)/host/engine/,$(addsuffix .o,$(ENGINE_REL)))
HOST_OBJECTS := $(HOST_ENGINE_OBJECTS) $(BUILD)/host/engine/platform_host.o \
	$(BUILD)/host/game/sim.o $(BUILD)/host/game/game.o \
	$(BUILD)/host/game/host_main.o $(BUILD)/host/generated/assets.o \
	$(BUILD)/host/generated/bajanew_assets.o
HOST_BINARY := $(BUILD)/bajanew-host

TARGET_ENGINE_OBJECTS := $(addprefix $(BUILD)/m68k/engine/,$(addsuffix .o,$(ENGINE_REL)))
TARGET_OBJECTS := $(TARGET_ENGINE_OBJECTS) $(BUILD)/m68k/game/sim.o \
	$(BUILD)/m68k/game/game.o $(BUILD)/m68k/target/platform.o \
	$(BUILD)/m68k/target/memory.o $(BUILD)/m68k/game/target_main.o \
	$(BUILD)/m68k/generated/assets.o $(BUILD)/m68k/generated/bajanew_assets.o
TARGET_CRT := $(BUILD)/m68k/crt0.o
TARGET_ELF := $(BUILD)/m68k/bajanew.elf
TARGET_RAW := $(BUILD)/m68k/bajanew.p
ROM_ZIP := $(ROMDIR)/puzzledp.zip

.PHONY: all assets host rom verify renderer-check mame-smoke mame-keyboard mame-controls play clean

all: host rom verify

# BAJANEW compiles its own approved artwork; only the audio driver and V-ROM
# still come from the SDK build.
assets:
	@mkdir -p $(GEN)
	$(PYTHON) $(ROOT)/tools/build_assets.py
	@$(PYTHON) $(FORGE68)/tools/ngasset.py $(ROOT)/assets/manifest.json --output $(GEN) > $(GEN)/asset_compile.json
	$(MAKE) -C $(FORGE68) audio PYTHON=$(PYTHON)

host: $(HOST_BINARY)
	@mkdir -p $(BUILD)/evidence
	$(HOST_BINARY) > $(BUILD)/evidence/bajanew-host.json

rom: $(ROM_ZIP)
	@ls -lh $(ROM_ZIP) $(TARGET_ELF)

# The flush optimisation is only allowed to skip work the hardware would not
# have noticed, so prove it against a build with the fast paths compiled out.
renderer-check: | assets
	$(PYTHON) $(ROOT)/tools/verify_renderer.py

verify: $(ROM_ZIP) host renderer-check
	$(PYTHON) $(ROOT)/tools/verify_rom.py --rom-dir $(ROMDIR) \
		--elf $(TARGET_ELF) --asset-report $(GEN)/asset_report.json \
		--audio-report $(AUDIO_GEN)/audio/audio_report.json \
		--render-report $(BUILD)/evidence/bajanew-host.json \
		--size-tool $(M68K_SIZE)

mame-smoke: verify
	@test -n "$(MAME_BIOS_DIR)" || (echo "Set MAME_BIOS_DIR to the directory containing neogeo.zip" && exit 2)
	$(PYTHON) $(FORGE68)/tools/run_mame_smoke.py --mame "$(MAME)" \
		--rom-dir $(ROMDIR) --bios-dir "$(MAME_BIOS_DIR)" \
		--script $(FORGE68)/tools/mame_smoke.lua \
		--screenshot $(BUILD)/evidence/bajanew-mame.png \
		--audio $(BUILD)/evidence/bajanew-mame.wav
	$(PYTHON) $(ROOT)/tools/verify_screen_alignment.py \
		--capture $(BUILD)/evidence/bajanew-mame.png \
		--asset $(ROOT)/assets/source/splash.png

mame-keyboard: verify
	@test -n "$(MAME_BIOS_DIR)" || (echo "Set MAME_BIOS_DIR to the directory containing neogeo.zip" && exit 2)
	$(PYTHON) $(ROOT)/tools/verify_mame_keyboard.py --mame "$(MAME)" \
		--rom-dir $(ROMDIR) --bios-dir "$(MAME_BIOS_DIR)" \
		--ctrlr-dir $(ROOT)/mame/ctrlr --script $(ROOT)/tools/mame_verify_keyboard.lua \
		--output $(BUILD)/evidence/bajanew-keyboard.json

mame-controls: verify
	@test -n "$(MAME_BIOS_DIR)" || (echo "Set MAME_BIOS_DIR to the directory containing neogeo.zip" && exit 2)
	$(PYTHON) $(ROOT)/tools/run_mame_controls.py --mame "$(MAME)" \
		--rom-dir $(ROMDIR) --bios-dir "$(MAME_BIOS_DIR)" \
		--script $(ROOT)/tools/mame_bajanew_controls.lua \
		--screenshot $(BUILD)/evidence/bajanew-controls.png \
		--output $(BUILD)/evidence/bajanew-controls.json

play: verify
	@test -n "$(MAME_BIOS_DIR)" || (echo "Set MAME_BIOS_DIR to the directory containing neogeo.zip" && exit 2)
	$(MAME) puzzledp -rompath "$(abspath $(ROMDIR));$(abspath $(MAME_BIOS_DIR))" \
		-bios euro -skip_gameinfo -ctrlrpath "$(abspath $(ROOT)/mame/ctrlr)" \
		-ctrlr bajanew

$(BUILD)/host/engine/%.o: $(FORGE68)/engine/%.c | assets
	@mkdir -p $(dir $@)
	$(HOST_CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/host/game/sim.o: $(ROOT)/src/sim.c | assets
	@mkdir -p $(dir $@)
	$(HOST_CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/host/game/game.o: $(ROOT)/native/game.c | assets
	@mkdir -p $(dir $@)
	$(HOST_CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/host/game/host_main.o: $(ROOT)/native/host_main.c | assets
	@mkdir -p $(dir $@)
	$(HOST_CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/host/generated/%.o: $(GEN)/%.c | assets
	@mkdir -p $(dir $@)
	$(HOST_CC) $(HOST_CFLAGS) -c $< -o $@

$(HOST_BINARY): $(HOST_OBJECTS)
	@mkdir -p $(dir $@)
	$(HOST_CC) $(HOST_CFLAGS) $^ -o $@

$(BUILD)/m68k/engine/%.o: $(FORGE68)/engine/%.c | assets
	@mkdir -p $(dir $@)
	$(M68K_CC) $(M68K_CFLAGS) -c $< -o $@

$(BUILD)/m68k/game/sim.o: $(ROOT)/src/sim.c | assets
	@mkdir -p $(dir $@)
	$(M68K_CC) $(M68K_CFLAGS) -c $< -o $@

$(BUILD)/m68k/game/game.o: $(ROOT)/native/game.c | assets
	@mkdir -p $(dir $@)
	$(M68K_CC) $(M68K_CFLAGS) -c $< -o $@

$(BUILD)/m68k/game/target_main.o: $(ROOT)/native/target_main.c | assets
	@mkdir -p $(dir $@)
	$(M68K_CC) $(M68K_CFLAGS) -c $< -o $@

$(BUILD)/m68k/target/platform.o: $(FORGE68)/target/neogeo/platform.c | assets
	@mkdir -p $(dir $@)
	$(M68K_CC) $(M68K_CFLAGS) -c $< -o $@

$(BUILD)/m68k/target/memory.o: $(FORGE68)/target/neogeo/memory.c | assets
	@mkdir -p $(dir $@)
	$(M68K_CC) $(M68K_CFLAGS) -c $< -o $@

$(BUILD)/m68k/generated/%.o: $(GEN)/%.c | assets
	@mkdir -p $(dir $@)
	$(M68K_CC) $(M68K_CFLAGS) -c $< -o $@

$(TARGET_CRT): $(FORGE68)/target/neogeo/crt0.S
	@mkdir -p $(dir $@)
	$(M68K_CC) -m68000 -g3 -gdwarf-2 -c $< -o $@

$(TARGET_ELF): $(TARGET_CRT) $(TARGET_OBJECTS) $(ROOT)/native/link.ld
	$(M68K_CC) $(M68K_LDFLAGS) $(TARGET_CRT) $(TARGET_OBJECTS) -lgcc -o $@

$(TARGET_RAW): $(TARGET_ELF)
	$(M68K_OBJCOPY) -O binary -S $< $@

$(ROM_ZIP): $(TARGET_RAW) | assets
	@mkdir -p $(ROMDIR)
	$(PYTHON) $(FORGE68)/tools/rompack.py --p-raw $(TARGET_RAW) \
		--s1 $(GEN)/s1.bin --m1 $(FORGE68)/build/m68k/m1.bin \
		--v1 $(AUDIO_GEN)/audio/v1.bin --c1 $(GEN)/c1.bin --c2 $(GEN)/c2.bin \
		--output $(ROMDIR)

-include $(HOST_OBJECTS:.o=.d) $(TARGET_OBJECTS:.o=.d)

clean:
	rm -rf $(BUILD)
