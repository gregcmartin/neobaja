-- Digest the whole of VRAM after every drawn frame while driving the same
-- scripted race as tools/mame_capture.lua.  Two builds that must place the
-- same sprites - a C placement path and its assembly twin - produce identical
-- digest streams; any divergence names the frame.
local cpu = manager.machine.devices[":maincpu"]
local space = cpu.spaces["program"]
local frames = 0
local last_game_frame = -1
-- The game's stage marker: 12 means the frame, its telemetry and its sound
-- command are all settled.
local STAGE = tonumber(os.getenv("BAJANEW_STAGE_ADDR") or "0")
local out = io.open(os.getenv("BAJANEW_DIGEST_FILE") or "vram_digest.txt", "w")
local fields = {left = {}, right = {}, a = {}, b = {}, start = {}}
local function normalized(value) return tostring(value or ""):lower():gsub("[^a-z0-9]", "") end
for _, port in pairs(manager.machine.ioport.ports) do
    for key, field in pairs(port.fields) do
        local name = normalized(key)
        if field.enabled then
            if name == "p1left" then table.insert(fields.left, field) end
            if name == "p1right" then table.insert(fields.right, field) end
            if name == "p1a" then table.insert(fields.a, field) end
            if name == "p1b" then table.insert(fields.b, field) end
            if name == "1playerstart" or name == "start1" or name == "p1start" then table.insert(fields.start, field) end
        end
    end
end
local function apply(mask)
    for _, list in pairs(fields) do for _, field in ipairs(list) do field:set_value(0) end end
    if (mask & 1) ~= 0 then for _, f in ipairs(fields.left) do f:set_value(1) end end
    if (mask & 2) ~= 0 then for _, f in ipairs(fields.right) do f:set_value(1) end end
    if (mask & 4) ~= 0 then for _, f in ipairs(fields.a) do f:set_value(1) end end
    if (mask & 16) ~= 0 then for _, f in ipairs(fields.start) do f:set_value(1) end end
end
local script = { {700, 708, 16}, {800, 808, 16}, {880, 7400, 4}, {2300, 2360, 8} }
local function vram_read(addr)
    space:write_u16(0x3c0000, addr)
    return space:read_u16(0x3c0002)
end
local DUMP_FRAME = tonumber(os.getenv("BAJANEW_DUMP_FRAME") or "-1")
local GAME = tonumber(os.getenv("BAJANEW_GAME_ADDR") or "0")
local GAME_SIZE = tonumber(os.getenv("BAJANEW_GAME_SIZE") or "0")
local function ramdump()
    if GAME == 0 then return end
    local f = io.open((os.getenv("BAJANEW_DIGEST_FILE") or "vram_digest.txt") .. ".ram", "wb")
    local parts = {}
    for i = 0, GAME_SIZE - 1 do parts[#parts+1] = string.char(space:read_u8(GAME + i)) end
    f:write(table.concat(parts)) -- RAMDUMP
    f:close()
end
local function dump(game_frame)
    ramdump()
    local f = io.open((os.getenv("BAJANEW_DIGEST_FILE") or "vram_digest.txt") .. ".dump", "w")
    for sprite = 1, 380 do
        local parts = {}
        for r = 0, 15 do parts[#parts+1] = string.format("%04x", vram_read(sprite * 64 + r)) end
        f:write(string.format("%d z%04x y%04x x%04x %s\n", sprite, vram_read(0x8000 + sprite),
            vram_read(0x8200 + sprite), vram_read(0x8400 + sprite), table.concat(parts, " ")))
    end
    f:close()
end
digest_sub = emu.add_machine_frame_notifier(function()
    frames = frames + 1
    local mask = 0
    for _, entry in ipairs(script) do
        if frames >= entry[1] and frames <= entry[2] then mask = mask | entry[3] end
    end
    apply(mask)
    local game_frame = space:read_u32(0x100044)
    local done = STAGE == 0 or space:read_u8(STAGE) == 12
    if game_frame ~= last_game_frame and done and frames % 4 == 0 then
        last_game_frame = game_frame
        local h = 2166136261
        -- sprite control blocks and the tile maps of the first 380 sprites
        for addr = 0x8000, 0x857c do h = ((h ~ vram_read(addr)) * 16777619) & 0xffffffff end
        for addr = 0, 0x5f00, 7 do h = ((h ~ vram_read(addr)) * 16777619) & 0xffffffff end
        out:write(string.format("%d %08x %d\n", game_frame, h, space:read_u32(0x100048)))
        if game_frame == DUMP_FRAME then dump(game_frame) end
    end
    if frames >= 3000 then out:close(); manager.machine:exit() end
end)
