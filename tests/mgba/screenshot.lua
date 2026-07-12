-- mGBA smoke-test script: screenshot after 120 frames to .scratchwork/mgba_shot.png
local outdir = "D:/Coding/GBScribe/.scratchwork"
local marker = io.open(outdir .. "/lua_loaded.txt", "w")
if marker then marker:write("loaded\n") marker:close() end

local frames = 0
callbacks:add("frame", function()
    frames = frames + 1
    if frames == 120 then
        emu:screenshot(outdir .. "/mgba_shot.png")
        local f = io.open(outdir .. "/lua_done.txt", "w")
        if f then f:write("done\n") f:close() end
    end
end)
