-- .RCL Executor Loader
-- Run this in Roblox to connect to the Electron App.

local GET_URL = "http://127.0.0.1:5500/get-script"
local HEARTBEAT_URL = "http://127.0.0.1:5500/heartbeat"
local POLLING_RATE = 0.5 -- Script check frequency
local HEARTBEAT_RATE = 2 -- Connection check frequency

print(".RCL Loader: Connected to app.")

-- Heartbeat loop
task.spawn(function()
    while true do
        pcall(function() game:HttpGet(HEARTBEAT_URL) end)
        task.wait(HEARTBEAT_RATE)
    end
end)

-- Main execution loop
while true do
    local success, scriptContent = pcall(function()
        return game:HttpGet(GET_URL)
    end)

    if success and scriptContent and scriptContent ~= "" then
        print(".RCL Loader: Executing new script...")
        task.spawn(function()
            local func, err = loadstring(scriptContent)
            if func then
                local execSuccess, execErr = pcall(func)
                if not execSuccess then
                    warn(".RCL Execution Error: " .. tostring(execErr))
                end
            else
                warn(".RCL Compilation Error: " .. tostring(err))
            end
        end)
    end

    task.wait(POLLING_RATE)
end
