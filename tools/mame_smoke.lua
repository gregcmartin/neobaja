local frame = 0
local out = os.getenv("BAJANEW_EVIDENCE") or "."
local screen = nil
local fields = {}

local function collect_fields()
    for tag, port in pairs(manager.machine.ioport.ports) do
        for name, field in pairs(port.fields) do
            local key = string.lower(name)
            if not fields[key] then fields[key] = {} end
            table.insert(fields[key], field)
            print(string.format("BAJANEW INPUT %s :: %s", tag, name))
        end
    end
end

local function find_field(parts)
    for name, matches in pairs(fields) do
        local match = true
        for _, part in ipairs(parts) do
            if not string.find(name, part, 1, true) then match = false end
        end
        if match then return matches[1] end
    end
    return nil
end

local function set_field(parts, value)
    local field = find_field(parts)
    if field then
        field:set_value(value)
    else
        print("BAJANEW missing input field: " .. table.concat(parts, "+"))
    end
end

local function set_named(name, value)
    local matches = fields[string.lower(name)]
    if matches then
        for _, field in ipairs(matches) do field:set_value(value) end
    else print("BAJANEW missing exact input field: " .. name) end
end

local function snap(name)
    local err = screen:snapshot(out .. "/" .. name .. ".png")
    if err then print("BAJANEW snapshot error " .. tostring(err)) end
    print("BAJANEW SNAP " .. name .. " frame=" .. frame)
end

screen = manager.machine.screens[":screen"]
collect_fields()

reset_subscription = emu.add_machine_reset_notifier(function()
    screen = manager.machine.screens[":screen"]
    collect_fields()
end)

emu.register_frame_done(function()
    frame = frame + 1
    if frame == 240 then snap("splash") end
    if frame == 1120 then snap("title") end

    if frame == 1180 then set_named("P1 Start", 1) end
    if frame == 1185 then set_named("P1 Start", 0) end
    if frame == 1240 then snap("select-max") end

    if frame == 1260 then set_named("P1 Right", 1) end
    if frame == 1265 then set_named("P1 Right", 0) end
    if frame == 1300 then snap("select-cruz") end
    if frame == 1320 then set_named("P1 A", 1) end
    if frame == 1325 then set_named("P1 A", 0) end
    if frame == 1410 then snap("countdown") end

    if frame == 1580 then set_named("P1 A", 1) end
    if frame == 1710 then snap("race-a") end
    if frame == 1740 then set_named("P1 Right", 1) end
    if frame == 1780 then set_named("P1 Right", 0) end
    if frame == 1830 then snap("race-b") end
    if frame == 1880 then set_named("P1 A", 0) end
    if frame == 1920 then snap("coast") end
    if frame == 1980 then set_named("P1 A", 1) end
    if frame == 2650 then snap("rival-approach") end
    if frame == 3000 then snap("rival-contact") end
    if frame == 3050 then set_named("P1 Right", 1) end
    if frame == 3210 then set_named("P1 Right", 0); set_named("P1 Left", 1) end
    if frame == 3270 then snap("offroad") end
    if frame == 3370 then set_named("P1 Left", 0) end
    if frame == 4500 then snap("late-race") end
    if frame == 4540 then set_named("P1 A", 0); manager.machine:exit() end
end, "frame")
