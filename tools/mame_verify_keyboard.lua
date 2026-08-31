-- Verifies the tracked BAJANEW controller profile after MAME has loaded it.

local expected = {
    left = "left",
    right = "right",
    a = "a",
    b = "b",
    start = "1"
}
local found = {}

local function normalized(value)
    return tostring(value or ""):lower():gsub("[^a-z0-9]", "")
end

for _, port in pairs(manager.machine.ioport.ports) do
    for _, field in pairs(port.fields) do
        local display = field.default_name or field.name
        local name = normalized(display)
        local player = tonumber(field.player) or 0
        local enabled = field.enabled
        local p1 = enabled and
            (name:find("p1", 1, true) or name:find("player1", 1, true) or player == 0) and
            not name:find("p2", 1, true) and not name:find("player2", 1, true)
        local key = nil
        if p1 and (name:find("joystickleft", 1, true) or name == "p1left") then key = "left" end
        if p1 and (name:find("joystickright", 1, true) or name == "p1right") then key = "right" end
        if p1 and (name:find("button1", 1, true) or name == "p1a" or name == "pa") then key = "a" end
        if p1 and (name:find("button2", 1, true) or name == "p1b" or name == "pb") then key = "b" end
        if enabled and (name:find("1playerstart", 1, true) or name == "start1") then key = "start" end
        if key ~= nil and found[key] == nil then
            local seq = field:default_input_seq("standard")
            found[key] = manager.machine.input:seq_name(seq):lower()
        end
    end
end

for key, token in pairs(expected) do
    local sequence = normalized(found[key])
    if found[key] == nil or not sequence:find(token, 1, true) then
        emu.print_error(string.format(
            "BAJANEW_KEYBOARD_FAIL %s expected '%s' in '%s'",
            key, token, tostring(found[key])))
        manager.machine:exit()
        return
    end
end

emu.print_info(string.format(
    'BAJANEW_KEYBOARD_PASS {"left":"%s","right":"%s","a":"%s","b":"%s","start":"%s"}',
    found.left, found.right, found.a, found.b, found.start))
manager.machine:exit()
