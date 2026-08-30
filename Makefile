SHELL := /bin/bash
export PATH := /opt/homebrew/bin:$(PATH)

PYTHON := /opt/homebrew/bin/python3
CC := m68k-neogeo-elf-gcc
OBJCOPY := m68k-neogeo-elf-objcopy
Z80OBJCOPY := z80-neogeo-ihx-sdobjcopy
PALTOOL := paltool.py
TILETOOL := tiletool.py
ROMTOOL := romtool.py

BUILD := build
ASSET_BUILD := $(BUILD)/assets
ROMDIR := $(BUILD)/rom
GAME := bajanew
ASSET_NAMES := splash horizon player rival portraits roadtiles props
RAW_ART := $(wildcard art/raw/openai/01a05452-0194-7532-93b8-bca5bd770d7c/*.png)

NGCFLAGS := $(shell pkg-config --cflags ngdevkit)
NGLDFLAGS := $(shell pkg-config --libs ngdevkit)
NGSHARE := $(shell pkg-config --variable=sharedir ngdevkit)

CFLAGS := $(NGCFLAGS) -Igame -I$(ASSET_BUILD) -std=c99 -fomit-frame-pointer -O2 -g -Wall -Wextra -Werror
LDFLAGS := $(NGLDFLAGS)

ELF := $(BUILD)/bajanew.elf
PROM := $(ROMDIR)/$(GAME)-p1.p1
CROM1 := $(ROMDIR)/$(GAME)-c1.c1
CROM2 := $(ROMDIR)/$(GAME)-c2.c2
SROM := $(ROMDIR)/$(GAME)-s1.s1
MROM := $(ROMDIR)/$(GAME)-m1.m1
VROM := $(ROMDIR)/$(GAME)-v1.v1
CART := $(ROMDIR)/$(GAME).zip
MAME_HASH := $(ROMDIR)/neogeo.xml

.PHONY: all assets test rom verify production-check controls-check mame-smoke mame-play clean

all: test rom verify

assets: $(ASSET_BUILD)/.converted

$(ASSET_BUILD)/.prepared: tools/prepare_assets.py third_party/unscii/unscii8.png $(RAW_ART) 02_REFERENCE_LIBRARY/developer-splash/devsplashlogo.jpg
	$(PYTHON) tools/prepare_assets.py
	touch $@

$(ASSET_BUILD)/.converted: $(ASSET_BUILD)/.prepared
	for name in $(ASSET_NAMES); do \
		$(PALTOOL) $(ASSET_BUILD)/$$name.gif -o $(ASSET_BUILD)/$$name.pal; \
		$(TILETOOL) --sprite -c $(ASSET_BUILD)/$$name.gif -o $(ASSET_BUILD)/$$name.c1 $(ASSET_BUILD)/$$name.c2; \
	done
	$(TILETOOL) --fix -c $(ASSET_BUILD)/font.gif -o $(ASSET_BUILD)/font.fix
	touch $@

$(BUILD)/target/main.o: target/main.c game/sim.h $(ASSET_BUILD)/.converted | $(BUILD)/target
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/game/sim.o: game/sim.c game/sim.h | $(BUILD)/game
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD) $(BUILD)/target $(BUILD)/game $(ROMDIR):
	mkdir -p $@

$(ELF): $(BUILD)/target/main.o $(BUILD)/game/sim.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(PROM): $(ELF) | $(ROMDIR)
	$(OBJCOPY) -O binary -S -R .text2 --gap-fill 0xff --pad-to 1048576 $< $(BUILD)/program.bin
	dd if=$(BUILD)/program.bin of=$@ conv=swab status=none

$(CROM1): $(ASSET_BUILD)/.converted tools/pack_crom.py | $(ROMDIR)
	$(PYTHON) tools/pack_crom.py $@ 2097152 $(ASSET_NAMES:%=$(ASSET_BUILD)/%.c1)

$(CROM2): $(ASSET_BUILD)/.converted tools/pack_crom.py | $(ROMDIR)
	$(PYTHON) tools/pack_crom.py $@ 2097152 $(ASSET_NAMES:%=$(ASSET_BUILD)/%.c2)

$(SROM): $(ASSET_BUILD)/.converted tools/pad_file.py | $(ROMDIR)
	cp $(ASSET_BUILD)/font.fix $@
	$(PYTHON) tools/pad_file.py $@ 131072

$(MROM): $(NGSHARE)/nullsound_driver.ihx tools/pad_file.py | $(ROMDIR)
	$(Z80OBJCOPY) -I ihex -O binary $< $@
	$(PYTHON) tools/pad_file.py $@ 131072

$(VROM): tools/pad_file.py | $(ROMDIR)
	$(PYTHON) tools/pad_file.py $@ 524288

$(CART): $(PROM) $(CROM1) $(CROM2) $(SROM) $(MROM) $(VROM)
	$(ROMTOOL) -b cartridge -f zip -p $(PROM) -c $(CROM1) $(CROM2) -v $(VROM) -s $(SROM) -m $(MROM) -n $(GAME) -x "zip.comment=BAJANEW Ensenada vertical slice" -o $@

$(MAME_HASH): $(CART)
	$(ROMTOOL) -b hash -f mame -p $(PROM) -c $(CROM1) $(CROM2) -v $(VROM) -s $(SROM) -m $(MROM) -n $(GAME) -l "BAJA Outrun - Ensenada Vertical Slice" -o $@

rom: $(CART) $(MAME_HASH)

$(BUILD)/test_sim: host/test_sim.c game/sim.c game/sim.h | $(BUILD)
	$(CC_FOR_BUILD) -std=c99 -O2 -Wall -Wextra -Werror host/test_sim.c game/sim.c -o $@

CC_FOR_BUILD ?= cc
test: $(BUILD)/test_sim
	$(BUILD)/test_sim

production-check: assets
	$(PYTHON) tools/production_check.py

verify: production-check rom
	$(PYTHON) tools/verify_build.py

MAME_BIOS_DIR ?= /Users/gregmartin/Desktop/goneo
controls-check: verify
	$(PYTHON) tools/verify_mame_controls.py --mame mame --bios-dir "$(MAME_BIOS_DIR)"

mame-smoke: verify
	$(PYTHON) tools/run_mame_smoke.py --mame mame --bios-dir "$(MAME_BIOS_DIR)"

mame-play: controls-check
	mame -window -resolution 960x672 -skip_gameinfo -noautosave \
		-keyboardprovider sdl -ctrlrpath "$(CURDIR)/mame/ctrlr" -ctrlr bajanew_keyboard \
		-cfg_directory "$(CURDIR)/build/mame-cfg" \
		-hash "$(ROMDIR)" -rp "$(ROMDIR);$(NGSHARE);$(MAME_BIOS_DIR)" \
		aes -cart $(GAME)

clean:
	rm -rf $(BUILD) evidence
