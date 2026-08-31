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
    {620, 628, 16}, {760, 768, 16},
    {880, 6000, 4},
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
local captures = {300, 650, 665, 680, 900, 1100, 1350, 1600, 2000, 2500, 3000, 3500, 4200}

emu.print_info(string.format("BAJANEW_CAPTURE_LOADED left=%d right=%d a=%d b=%d start=%d",
    #fields.left, #fields.right, #fields.a, #fields.b, #fields.start))
capture_sub = emu.add_machine_frame_notifier(function()
    frames = frames + 1
    local mask = 0
    for _, entry in ipairs(script) do
        if frames >= entry[1] and frames <= entry[2] then mask = mask | entry[3] end
    end
    if space:read_u8(0x100078) == 4 then mask = mask | centring_steer() end
    apply(mask)

    for index, at in ipairs(captures) do
        if frames == at then
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
        end
    end
    if frames >= 6000 then
        emu.print_info(string.format("BAJANEW_CAPTURE_DONE shots=%d", shots))
        manager.machine:exit()
    end
end)
