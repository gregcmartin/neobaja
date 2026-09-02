-- Price each stage of the cartridge frame in 68000 cycles, from real MAME.
-- BAJANEW_STAGE_ADDR names bajanew_stage; BAJANEW_LEVEL peels layers off the scene.

local cpu = manager.machine.devices[":maincpu"]
local space = cpu.spaces["program"]
local screen = manager.machine.screens[":screen"]
local out_dir = os.getenv("BAJANEW_CAPTURE_DIR") or "capture"
local fields = {left = {}, right = {}, a = {}, b = {}, start = {}}
local frames = 0
local shots = 0

local function normalized(value)
    return tostring(value or ""):lower():gsub("[^a-z0-9]", "")
end

-- MAME names Neo Geo joystick buttons "P1 A"/"P1 B" in the field key while the
-- default name is the unexpanded "%p A", so match on the key.
for _, port in pairs(manager.machine.ioport.ports) do
    for key, field in pairs(port.fields) do
        local name = normalized(key)
        if field.enabled then
            if name == "p1left" then table.insert(fields.left, field) end
            if name == "p1right" then table.insert(fields.right, field) end
            if name == "p1a" then table.insert(fields.a, field) end
            if name == "p1b" then table.insert(fields.b, field) end
            if name == "1playerstart" or name == "start1" or name == "p1start" then
                table.insert(fields.start, field)
            end
        end
    end
end

local function apply(mask)
    for _, list in pairs(fields) do
        for _, field in ipairs(list) do field:set_value(0) end
    end
    if (mask & 1) ~= 0 then for _, f in ipairs(fields.left) do f:set_value(1) end end
    if (mask & 2) ~= 0 then for _, f in ipairs(fields.right) do f:set_value(1) end end
    if (mask & 4) ~= 0 then for _, f in ipairs(fields.a) do f:set_value(1) end end
    if (mask & 8) ~= 0 then for _, f in ipairs(fields.b) do f:set_value(1) end end
    if (mask & 16) ~= 0 then for _, f in ipairs(fields.start) do f:set_value(1) end end
end

-- {first frame, last frame, input mask}: splash runs out on its own, then two
-- deliberate Start presses, then throttle with a few corrections.

-- Turn the cartridge's stage marker writes into 68000 cycles.
local STAGE = tonumber(os.getenv("BAJANEW_STAGE_ADDR") or "0x1002a2")
local CLOCK = 12000000.0
local names = {
    [1] = "loop overhead -> tick entry",
    [2] = "simulation step",
    [3] = "FIX clear + renderer begin",
    [4] = "view + road band projection",
    [5] = "backdrop + road submit",
    [6] = "player place",
    [7] = "scenery project + queue",
    [8] = "rivals project + queue",
    [9] = "pool draw items",
    [10] = "pool end + renderer flush",
    [11] = "FIX flush",
}
local totals, counts = {}, {}
local last_time, last_stage = nil, nil
local samples = 0
function reset_stats() totals, counts = {}, {} end
function report(label)
    local flush = 0
    if counts[7] and counts[7] > 0 then flush = totals[7] / counts[7] * CLOCK end
    local total = 0
    for stage = 2, 11 do
        if counts[stage] and counts[stage] > 0 then total = total + totals[stage] / counts[stage] * CLOCK end
    end
    emu.print_info(string.format("FLUSH %-28s cols=%3d flush=%7.0f total=%7.0f",
        label, space:read_u16(0x10000c), flush, total))
end

local script = { {900, 906, 16}, {980, 986, 16}, {1100, 60000, 4} }

-- The 68000 bus is 16 bits wide, so the byte marker arrives in the high half
-- of the word it shares with the render level.
stage_tap = manager.machine.devices[":maincpu"].spaces["program"]:install_write_tap(
    STAGE, STAGE + 1, "stage", function(offset, data, mask)
        local now = manager.machine.time:as_double()
        if (mask & 0xff00) == 0 then return end
        data = (data >> 8) & 0xff
        if last_time ~= nil and last_stage ~= nil and data == last_stage + 1 then
            local key = data
            totals[key] = (totals[key] or 0) + (now - last_time)
            counts[key] = (counts[key] or 0) + 1
        end
        last_time, last_stage = now, data
        if data == 8 then samples = samples + 1 end
    end)

emu.print_info("STAGES_LOADED")
stages_sub = emu.add_machine_frame_notifier(function()
    frames = frames + 1
    local mask = 0
    for _, entry in ipairs(script) do
        if frames >= entry[1] and frames <= entry[2] then mask = mask | entry[3] end
    end
    apply(mask)
    -- Sweep render levels and report the flush cost of each scene.
    local level = tonumber(os.getenv("BAJANEW_LEVEL") or "0")
    if frames > 2100 then space:write_u8(STAGE + 1, level) end
    if frames == 2200 then reset_stats() end
    if frames == 3600 then
        local total = 0
        for stage = 2, 11 do
            if counts[stage] and counts[stage] > 0 then
                local cycles = totals[stage] / counts[stage] * CLOCK
                total = total + cycles
                emu.print_info(string.format("STAGE %d %-34s %7.0f cycles", stage, names[stage], cycles))
            end
        end
        emu.print_info(string.format("STAGE - %-36s %7.0f cycles (field = 200000)", "TOTAL measured", total))
        manager.machine:exit()
    end
end)
