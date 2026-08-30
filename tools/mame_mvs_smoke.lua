local frame = 0
local out = os.getenv("BAJANEW_EVIDENCE") or "."
local screen = nil
local fields = {}

local function collect_fields()
    fields = {}
    for _, port in pairs(manager.machine.ioport.ports) do
        for name, field in pairs(port.fields) do
            local key = string.lower(name)
            if not fields[key] then fields[key] = {} end
            table.insert(fields[key], field)
        end
    end
end

local function set_named(name, value)
    local matches = fields[string.lower(name)]
    if matches then
        for _, field in ipairs(matches) do field:set_value(value) end
    else print("BAJANEW MVS missing input field: " .. name) end
end

local function snap(name)
    local err = screen:snapshot(out .. "/" .. name .. ".png")
    if err then print("BAJANEW MVS snapshot error " .. tostring(err)) end
    print("BAJANEW MVS SNAP " .. name .. " frame=" .. frame)
end

screen = manager.machine.screens[":screen"]
collect_fields()

reset_subscription = emu.add_machine_reset_notifier(function()
    screen = manager.machine.screens[":screen"]
    collect_fields()
end)

emu.register_frame_done(function()
    frame = frame + 1
    if frame == 1120 then snap("mvs-title-before-coin") end
    if frame == 1140 then set_named("Coin 1", 1) end
    if frame == 1145 then set_named("Coin 1", 0) end
    if frame == 1200 then snap("mvs-title-after-coin") end
    if frame == 1230 then set_named("1 Player Start", 1) end
    if frame == 1235 then set_named("1 Player Start", 0) end
    if frame == 1300 then snap("mvs-select") end
    if frame == 1330 then set_named("P1 A", 1) end
    if frame == 1335 then set_named("P1 A", 0) end
    if frame == 1600 then set_named("P1 A", 1) end
    if frame == 1760 then snap("mvs-race") end
    if frame == 1800 then set_named("P1 A", 0) end
    if frame == 1820 then manager.machine:exit() end
end, "frame")
