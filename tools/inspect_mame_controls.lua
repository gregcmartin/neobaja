local frame = 0
local wanted = {
    ["P1 A"] = true,
    ["P1 B"] = true,
    ["P1 Left"] = true,
    ["P1 Right"] = true,
    ["P1 Start"] = true,
}

emu.register_frame_done(function()
    frame = frame + 1
    if frame == 10 then
        for tag, port in pairs(manager.machine.ioport.ports) do
            for name, field in pairs(port.fields) do
                if wanted[name] then
                    local token = manager.machine.ioport:input_type_to_token(field.type, field.player)
                    local seq = field:default_input_seq("standard")
                    local keys = manager.machine.input:seq_to_tokens(seq)
                    print(string.format("BAJANEW CONTROL %s %s type=%s seq=%s", tag, name, token, keys))
                end
            end
        end
        manager.machine:exit()
    end
end, "frame")
