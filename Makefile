CC ?= cc
CFLAGS ?= -std=c99 -O2 -Wall -Wextra -Werror -pedantic
CPPFLAGS ?= -Iinclude

HOST_BUILD := build/host
SIM_OBJECT := $(HOST_BUILD)/sim.o
TEST_BIN := $(HOST_BUILD)/test_sim
TRACE_BIN := $(HOST_BUILD)/sim_trace
TRACE_CSV := $(HOST_BUILD)/sim_trace.csv

.PHONY: all check test sanitize m68k-check trace clean

all: check

check: test sanitize m68k-check trace

test: $(TEST_BIN)
	$(TEST_BIN)

sanitize: $(HOST_BUILD)/test_sim_sanitize
	ASAN_OPTIONS=detect_leaks=0 $(HOST_BUILD)/test_sim_sanitize

m68k-check: build/m68k/sim.o
	m68k-neogeo-elf-size $<

trace: $(TRACE_CSV)

$(TRACE_CSV): $(TRACE_BIN)
	$(TRACE_BIN) > $@
	@test "$$(wc -l < $@)" -gt 20

$(TEST_BIN): tests/test_sim.c $(SIM_OBJECT) | $(HOST_BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

$(TRACE_BIN): tools/sim_trace.c $(SIM_OBJECT) | $(HOST_BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

$(HOST_BUILD)/test_sim_sanitize: tests/test_sim.c src/sim.c include/baja/sim.h | $(HOST_BUILD)
	clang $(CPPFLAGS) -std=c99 -O1 -g -Wall -Wextra -Werror -pedantic \
		-fsanitize=address,undefined src/sim.c tests/test_sim.c -o $@

build/m68k/sim.o: src/sim.c include/baja/sim.h
	mkdir -p $(dir $@)
	m68k-neogeo-elf-gcc $(CPPFLAGS) -std=c99 -Os -Wall -Wextra -Werror \
		-pedantic -c $< -o $@

$(SIM_OBJECT): src/sim.c include/baja/sim.h | $(HOST_BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(HOST_BUILD):
	mkdir -p $@

clean:
	rm -rf build
