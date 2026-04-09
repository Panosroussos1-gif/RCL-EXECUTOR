-- .RCL Executor Loader
-- Run this in Roblox to connect to the Electron App.
-- This script will "pull" any code you press "EXECUTE" on in the app.

local SERVER_URL = "http://127.0.0.1:5500/get-script"
local POLLING_RATE = 0.2 -- How fast to check for new scripts (in seconds)

print(".RCL Loader: Connecting to http://127.0.0.1:5500...")

while true do
    local success, scriptContent = pcall(function()
        return game:HttpGet(SERVER_URL)
    end)

    if success and scriptContent and scriptContent ~= "" then
        print(".RCL Loader: Executing script...")
        
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
    elseif not success then
        -- This usually means the Electron app isn't open or the port is blocked
        -- warn(".RCL Connection Error: Make sure the Electron app is open.")
    end

    task.wait(POLLING_RATE)
end
