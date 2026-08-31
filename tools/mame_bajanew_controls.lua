-- BAJANEW native control gate. Drives real MAME input fields and reads the
-- cartridge's two telemetry ABIs; no gameplay state is injected directly.

local cpu = manager.machine.devices[":maincpu"]
local space = cpu.spaces["program"]
local screen = manager.machine.screens[":screen"]
local screenshot = os.getenv("BAJANEW_SCREENSHOT") or "bajanew-controls.png"
local fields = {left = {}, right = {}, a = {}, b = {}, start = {}}
local emulator_frames = 0
local booted = false
local state = "wait_boot"
local state_frames = 0
local pulse_frames = 0
local pulse_mask = 0
local select_step = 0
local race_step = 0
local race_step_frames = 0
local last_race_game_frame = nil
local values = {}

local function normalized(value)
    return tostring(value or ""):lower():gsub("[^a-z0-9]", "")
end

local function map_inputs()
    for _, port in pairs(manager.machine.ioport.ports) do
        for _, field in pairs(port.fields) do
            local display = field.default_name or field.name
            local name = normalized(display)
            local player = tonumber(field.player) or 0
            local enabled = field.enabled
            local p1 = enabled and
                (name:find("p1", 1, true) or name:find("player1", 1, true) or player == 0) and
                not name:find("p2", 1, true) and not name:find("player2", 1, true)
            if p1 then
                if name:find("joystickleft", 1, true) or name == "p1left" then table.insert(fields.left, field) end
                if name:find("joystickright", 1, true) or name == "p1right" then table.insert(fields.right, field) end
                if name:find("button1", 1, true) or name:find("buttona", 1, true) or name == "p1a" or name == "pa" then table.insert(fields.a, field) end
                if name:find("button2", 1, true) or name:find("buttonb", 1, true) or name == "p1b" or name == "pb" then table.insert(fields.b, field) end
            end
            if enabled and (name:find("1playerstart", 1, true) or name == "start1") then table.insert(fields.start, field) end
        end
    end
end

local function clear_inputs()
    for _, list in pairs(fields) do
        -- Force every mapped digital field inactive. clear_value() merely
        -- releases the scripted override and can preserve the preceding
        -- sampled state until MAME polls the natural input sequence.
        for _, field in ipairs(list) do field:set_value(0) end
    end
end

local function release_inputs()
    for _, list in pairs(fields) do
        for _, field in ipairs(list) do field:clear_value() end
    end
end

local function set_active(name)
    for _, field in ipairs(fields[name]) do field:set_value(1) end
end

local function apply_mask(mask)
    clear_inputs()
    if (mask & 0x004) ~= 0 then set_active("left") end
    if (mask & 0x008) ~= 0 then set_active("right") end
    if (mask & 0x010) ~= 0 then set_active("a") end
    if (mask & 0x020) ~= 0 then set_active("b") end
    if (mask & 0x100) ~= 0 then set_active("start") end
end

local function signed32(value)
    if value >= 0x80000000 then return value - 0x100000000 end
    return value
end

local function game_state()
    if space:read_u32(0x100040) ~= 0x42414a41 then return nil end
    return {
        frame = space:read_u32(0x100044),
        player_s = signed32(space:read_u32(0x100048)),
        player_e = signed32(space:read_u32(0x10004c)),
        speed = signed32(space:read_u32(0x100050)),
        collisions = space:read_u32(0x10006c),
        overtakes = space:read_u32(0x100070),
        input = space:read_u16(0x100074),
        abi_version = space:read_u8(0x100076),
        abi_size = space:read_u8(0x100077),
        phase = space:read_u8(0x100078),
        driver = space:read_u8(0x100079),
        surface = space:read_u8(0x10007a),
        position = space:read_u8(0x10007b),
        steer = signed32(space:read_u32(0x10007c))
    }
end

local function fail(message)
    release_inputs()
    emu.print_error("BAJANEW_CONTROL_FAIL " .. message)
    manager.machine:exit()
end

local function pulse(mask, frames)
    pulse_mask = mask
    pulse_frames = frames
end

local function check_fields()
    for _, name in ipairs({"left", "right", "a", "b", "start"}) do
        if #fields[name] == 0 then fail("missing MAME input field " .. name) end
    end
end

local function finish(gs)
    local coast_drop = values.coast_start - values.coast_end
    local brake_drop = values.brake_start - values.brake_end
    if values.idle_speed ~= 0 or values.idle_distance ~= 0 then fail("idle vehicle moved") end
    if values.throttle_speed <= 0 or values.throttle_distance <= 0 then fail("A did not accelerate") end
    if values.a_input ~= 0x010 then fail("A field did not reach cartridge") end
    if values.b_input ~= 0x020 then fail("B field did not reach cartridge") end
    if coast_drop <= 0 then fail("coast did not reduce speed") end
    if brake_drop <= coast_drop then fail("B brake was not stronger than coast") end
    if values.left_end >= values.left_start then
        fail(string.format("left steering did not move left start=%d end=%d", values.left_start, values.left_end))
    end
    if values.right_end <= values.right_mid or values.right_steer <= 0 then
        fail(string.format(
            "right steering did not recover rightward left=%d mid=%d right=%d input=%d steer=%d",
            values.left_end, values.right_mid, values.right_end,
            values.right_input, values.right_steer) ..
            " field=" .. tostring(fields.right[1].default_name or fields.right[1].name) ..
            " mask=" .. tostring(fields.right[1].mask))
    end
    if gs.driver ~= 1 then fail("driver selection did not choose Cruz") end
    if gs.abi_version ~= 1 or gs.abi_size ~= 64 then fail("game telemetry ABI mismatch") end
    if space:read_u16(0x100010) ~= 0 or space:read_u16(0x100012) ~= 0 or
       space:read_u16(0x10001a) ~= 0 then fail("renderer telemetry reports a drop or overload") end
    local err = screen:snapshot(screenshot)
    if err ~= nil then fail("screenshot failed: " .. tostring(err)) end
    release_inputs()
    emu.print_info(string.format(
        'BAJANEW_CONTROL_PASS {"idle_speed":%d,"throttle_speed":%d,' ..
        '"throttle_distance":%d,"coast_drop":%d,"brake_drop":%d,' ..
        '"left_start":%d,"left_end":%d,"right_end":%d,' ..
        '"a_input":%d,"b_input":%d,"driver":%d,"phase":%d,' ..
        '"active_columns":%d,"peak_scanline_columns":%d}',
        values.idle_speed, values.throttle_speed, values.throttle_distance,
        coast_drop, brake_drop, values.left_start, values.left_end,
        values.right_end, values.a_input, values.b_input, gs.driver, gs.phase,
        space:read_u16(0x10000c), space:read_u16(0x100016)))
    manager.machine:exit()
end

local function update_race(gs, advanced)
    local masks = {0x000, 0x010, 0x000, 0x010, 0x020, 0x014, 0x018}
    apply_mask(masks[race_step + 1])
    if not advanced then return end
    race_step_frames = race_step_frames + 1
    if race_step == 0 then
        if race_step_frames >= 30 then
            values.idle_speed = gs.speed
            values.idle_distance = gs.player_s
            race_step = 1
            race_step_frames = 0
        end
    elseif race_step == 1 then
        if race_step_frames == 8 then values.a_input = gs.input end
        if race_step_frames >= 120 then
            values.throttle_speed = gs.speed
            values.throttle_distance = gs.player_s
            values.coast_start = gs.speed
            race_step = 2
            race_step_frames = 0
        end
    elseif race_step == 2 then
        if race_step_frames >= 30 then
            values.coast_end = gs.speed
            race_step = 3
            race_step_frames = 0
        end
    elseif race_step == 3 then
        if race_step_frames >= 80 then
            values.brake_start = gs.speed
            race_step = 4
            race_step_frames = 0
        end
    elseif race_step == 4 then
        if race_step_frames == 8 then values.b_input = gs.input end
        if race_step_frames >= 30 then
            values.brake_end = gs.speed
            values.left_start = gs.player_e
            race_step = 5
            race_step_frames = 0
        end
    elseif race_step == 5 then
        if race_step_frames >= 24 then
            values.left_end = gs.player_e
            race_step = 6
            race_step_frames = 0
        end
    elseif race_step == 6 then
        if race_step_frames == 8 then values.right_input = gs.input end
        if race_step_frames == 45 then values.right_mid = gs.player_e end
        if race_step_frames >= 90 then
            values.right_end = gs.player_e
            values.right_steer = gs.steer
            finish(gs)
        end
    end
end

map_inputs()
check_fields()
clear_inputs()

bajanew_control_subscription = emu.add_machine_frame_notifier(function()
    emulator_frames = emulator_frames + 1
    local core_magic = space:read_u32(0x100000)
    local gs = game_state()
    if not booted then
        if core_magic == 0x4e474141 and gs ~= nil then
            booted = true
            state = "splash"
            state_frames = 0
        elseif emulator_frames > 1800 then
            fail("telemetry boot timeout")
        end
        return
    end

    if pulse_frames > 0 then
        apply_mask(pulse_mask)
        pulse_frames = pulse_frames - 1
        return
    end
    clear_inputs()
    state_frames = state_frames + 1

    if gs.phase == 0 then
        state = "splash"
    elseif gs.phase == 1 then
        if state ~= "title_started" then
            state = "title_started"
            pulse(0x100, 2)
        end
    elseif gs.phase == 2 then
        if select_step == 0 then
            pulse(0x008, 2)
            select_step = 1
        elseif select_step == 1 and state_frames > 4 then
            pulse(0x100, 2)
            select_step = 2
        end
    elseif gs.phase == 3 then
        state = "countdown"
        state_frames = 0
    elseif gs.phase == 4 then
        local advanced = last_race_game_frame == nil or gs.frame ~= last_race_game_frame
        if advanced then last_race_game_frame = gs.frame end
        update_race(gs, advanced)
    elseif gs.phase == 5 then
        fail("race finished before control gate completed")
    end
end)
