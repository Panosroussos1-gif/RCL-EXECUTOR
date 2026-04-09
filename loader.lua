-- .RCL Executor Loader
-- Run this in Roblox to connect to the Electron App.

local PORT = 5500
local HOSTS = {"127.0.0.1", "localhost"}
local POLLING_RATE = 0.5 -- Script check frequency
local HEARTBEAT_RATE = 2 -- Connection check frequency

local function request(path)
    for _, host in ipairs(HOSTS) do
        local url = "http://" .. host .. ":" .. PORT .. path
        local success, result = pcall(function()
            return game:HttpGet(url)
        end)
        if success and result then
            return result
        end
    end
    return nil
end

print(".RCL Loader: Attempting connection...")

-- Heartbeat loop
task.spawn(function()
    while true do
        request("/heartbeat")
        task.wait(HEARTBEAT_RATE)
    end
end)

-- Main execution loop
while true do
    local scriptContent = request("/get-script")

    if scriptContent and scriptContent ~= "" then
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
