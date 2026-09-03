-- Capture native 320x224 frames from real MAME while driving the cartridge
-- through its own input fields.  Nothing is written into gameplay state.

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
-- Machine frames.  The cartridge renders every other vblank, so the splash's
-- 300 game frames take about 600 of these on top of the BIOS handover.
local script = {
    {700, 708, 16}, {800, 808, 16},
    {880, 7400, 4},
    {2300, 2360, 8},
}
-- Between the scripted moments, steer toward the centre of the road so the
-- captures show the game being driven rather than ploughing through scrub.
local function centring_steer()
    local e = space:read_i32(0x10004c)
    if e > 4096 then return 1 end
    if e < -4096 then return 2 end
    return 0
end
local captures = {300, 650, 750, 880, 1000, 1100, 1350, 1600, 2000, 2500, 3000, 3500, 4200, 5200, 7000}

emu.print_info(string.format("BAJANEW_CAPTURE_LOADED left=%d right=%d a=%d b=%d start=%d",
    #fields.left, #fields.right, #fields.a, #fields.b, #fields.start))
-- BAJANEW_CAPTURE_TICKS=1: key the script and the shots on the cartridge's
-- own frame counter (two machine frames a tick) instead of machine frames,
-- so two builds that run at different speeds get the same inputs at the
-- same ticks and must produce the same pictures.
local tick_mode = os.getenv("BAJANEW_CAPTURE_TICKS") ~= nil
-- In tick mode the shots are taken from a write tap on the cartridge's stage
-- marker (BAJANEW_STAGE_ADDR), at the moment a frame's VRAM has settled, so
-- two builds can be compared sprite for sprite.
local next_shot = 1
local function shoot(index)
    local path = string.format("%s/frame%02d.png", out_dir, index)
    local err = screen:snapshot(path)
    shots = shots + 1
    emu.print_info(string.format(
        "CAPTURE %s mf=%d gframe=%d vbl=%d phase=%d speed=%d pos=%d cols=%d peak=%d drop=%d",
        path, frames, space:read_u32(0x100044), space:read_u32(0x100008),
        space:read_u8(0x100078), space:read_u32(0x100050),
        space:read_u8(0x10007b), space:read_u16(0x10000c),
        space:read_u16(0x100016), space:read_u16(0x100010)))
    if err ~= nil then emu.print_error("snapshot failed: " .. tostring(err)) end
    -- BAJANEW_CAPTURE_SCB=1: the sprite control blocks beside each
    -- shot (one line per sprite: zoom, Y control, X control), so a
    -- stray sprite in a capture can be traced to its slot.
    if os.getenv("BAJANEW_CAPTURE_SCB") then
        local scb = io.open(string.format("%s/frame%02d.scb", out_dir, index), "w")
        if scb ~= nil then
            for sprite = 1, 380 do
                space:write_u16(0x3c0000, 0x8000 + sprite)
                local z = space:read_u16(0x3c0002)
                space:write_u16(0x3c0000, 0x8200 + sprite)
                local y = space:read_u16(0x3c0002)
                space:write_u16(0x3c0000, 0x8400 + sprite)
                local x = space:read_u16(0x3c0002)
                local words = {}
                for row = 0, 15 do
                    space:write_u16(0x3c0000, sprite * 64 + row)
                    words[#words + 1] = string.format("%04x", space:read_u16(0x3c0002))
                end
                scb:write(string.format("%d z%04x y%04x x%04x %s\n", sprite, z, y, x,
                                        table.concat(words, " ")))
            end
            -- FIX map: one line per row, forty tile words.
            for row = 0, 31 do
                local cells = {}
                for column = 0, 39 do
                    space:write_u16(0x3c0000, 0x7000 + column * 32 + row)
                    cells[#cells + 1] = string.format("%04x", space:read_u16(0x3c0002))
                end
                scb:write(string.format("F%02d %s\n", row, table.concat(cells, " ")))
            end
            scb:close()
        end
    end
end

if tick_mode then
    local stage_addr = tonumber(os.getenv("BAJANEW_STAGE_ADDR") or "0x100794")
    stage_tap = space:install_write_tap(stage_addr, stage_addr + 1, "stage", function(offset, data, mask)
        if (mask & 0xff00) == 0 or ((data >> 8) & 0xff) ~= 12 then return end
        local clock = space:read_u32(0x100044) * 2
        while next_shot <= #captures and clock >= captures[next_shot] do
            shoot(next_shot)
            next_shot = next_shot + 1
        end
    end)
end

capture_sub = emu.add_machine_frame_notifier(function()
    frames = frames + 1
    local clock = frames
    if tick_mode then
        -- The counter is only meaningful once the cartridge has stamped its
        -- telemetry magic; before that the RAM is whatever the BIOS left.
        if space:read_u32(0x100040) == 0x42414a41 then clock = space:read_u32(0x100044) * 2 else clock = 0 end
    end
    local mask = 0
    for _, entry in ipairs(script) do
        if clock >= entry[1] and clock <= entry[2] then mask = mask | entry[3] end
    end
    if space:read_u8(0x100078) == 4 then mask = mask | centring_steer() end
    apply(mask)

    -- BAJANEW_CAPTURE_LEVEL=n: hold the cartridge's render level (the byte
    -- after bajanew_stage, address from BAJANEW_STAGE_ADDR) so a capture can
    -- peel layers off the scene.
    local level_env = os.getenv("BAJANEW_CAPTURE_LEVEL")
    if level_env ~= nil and clock > 1100 then
        local stage_addr = tonumber(os.getenv("BAJANEW_STAGE_ADDR") or "0x100794")
        space:write_u8(stage_addr + 1, tonumber(level_env))
    end
    if not tick_mode then
        for index, at in ipairs(captures) do
            if frames == at then shoot(index) end
        end
    end
    if clock >= 7300 then
        emu.print_info(string.format("BAJANEW_CAPTURE_DONE shots=%d", shots))
        manager.machine:exit()
    end
end)
