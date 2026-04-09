const { app, BrowserWindow, ipcMain, dialog, clipboard } = require('electron');
const path = require('path');
const fs = require('fs');
const { exec } = require('child_process');
const http = require('http');

let win;
let executionBuffer = "";
let lastHeartbeat = 0;

// Simple HTTP server for Roblox communication
const server = http.createServer((req, res) => {
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

  if (req.method === 'OPTIONS') {
    res.writeHead(204);
    res.end();
    return;
  }

  if (req.url === '/get-script') {
    if (win) win.webContents.send('server-log', 'Script requested by loader');
    res.writeHead(200, { 'Content-Type': 'text/plain' });
    res.end(executionBuffer); 
    executionBuffer = ""; // Clear after serving
  } else if (req.url === '/heartbeat') {
    lastHeartbeat = Date.now();
    if (win) win.webContents.send('server-log', 'Heartbeat received');
    res.writeHead(200, { 'Content-Type': 'text/plain' });
    res.end('OK');
  } else if (req.url === '/loader') {
    if (win) win.webContents.send('server-log', 'Loader file requested');
    try {
      const loader = fs.readFileSync(path.join(__dirname, 'loader.lua'), 'utf8');
      res.writeHead(200, { 'Content-Type': 'text/plain' });
      res.end(loader);
    } catch (err) {
      res.writeHead(500);
      res.end('Error reading loader');
    }
  } else {
    res.writeHead(404);
    res.end();
  }
});

// --- NEW: HEARTBEAT STATUS ---
ipcMain.handle('check-loader-status', () => {
  const isConnected = (Date.now() - lastHeartbeat) < 5000; // Connected if heartbeat in last 5s
  return isConnected;
});

server.on('error', (e) => {
  if (e.code === 'EADDRINUSE') {
    console.error('Script server port 5500 is already in use. Another instance might be running.');
    // Don't crash, just log. The other instance might handle it.
  } else {
    console.error('Script server error:', e);
  }
});

server.listen(5500, '127.0.0.1', () => {
  console.log('Free Script server running at http://127.0.0.1:5500/');
});

process.on('uncaughtException', (err) => {
  console.error('Uncaught Exception:', err);
  // Optional: show a dialog
});

process.on('unhandledRejection', (reason, promise) => {
  console.error('Unhandled Rejection at:', promise, 'reason:', reason);
});

function createWindow() {
  win = new BrowserWindow({
    width: 500,
    height: 350,
    frame: false,
    transparent: true,
    resizable: false,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false
    }
  });

  win.loadFile(path.join(__dirname, 'app-free.html'));
}

app.whenReady().then(() => {
  createWindow();

  // Ensure directories exist
  const workspacePath = path.join(app.getPath('userData'), 'workspace');
  if (!fs.existsSync(workspacePath)) fs.mkdirSync(workspacePath, { recursive: true });

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit();
});

// IPC Handlers
ipcMain.on('close-app', () => app.quit());
ipcMain.on('minimize-app', () => win.minimize());

ipcMain.handle('check-roblox', async () => {
  return new Promise((resolve) => {
    const cmd = process.platform === 'win32' ? 'tasklist' : 'ps aux';
    exec(cmd, (err, stdout) => {
      if (err) return resolve(false);
      resolve(stdout.toLowerCase().includes('roblox'));
    });
  });
});

ipcMain.handle('execute-script', (event, content) => {
  executionBuffer = content;
  return true;
});

ipcMain.handle('inject-internal', async () => {
  return new Promise((resolve) => {
    const loaderPath = path.join(__dirname, 'bin', 'rcl_loader');
    console.log('Running internal loader from:', loaderPath);
    
    // Check if loader exists
    if (!fs.existsSync(loaderPath)) {
      return resolve({ success: false, error: 'Internal loader binary (rcl_loader) not found. Please run build.sh.' });
    }

    // Run the loader
    exec(`"${loaderPath}"`, (err, stdout, stderr) => {
      if (err) {
        console.error('Internal Loader Error:', err);
        return resolve({ success: false, error: err.message });
      }
      console.log('Internal Loader Output:', stdout);
      resolve({ success: true, output: stdout });
    });
  });
});

ipcMain.handle('inject-standalone', async () => {
  const oneLiner = 'local s,r=pcall(game.HttpGet,game,"http://127.0.0.1:5500/loader") if s then loadstring(r)() else warn("RCL ERR:"..tostring(r)) end';
  clipboard.writeText(oneLiner);

  const appleScript = `
    set robloxNames to {"Roblox", "RobloxPlayer", "RobloxPlayerBeta"}
    set foundProcess to ""
    
    tell application "System Events"
      repeat with rName in robloxNames
        if exists (process rName) then
          set foundProcess to rName
          exit repeat
        end if
      end repeat
      
      if foundProcess is not "" then
        -- Focus the app directly
        tell application foundProcess to activate
        delay 1.5
        
        -- Open Chat
        keystroke "/"
        delay 0.8
        
        -- Clear current line and Paste
        keystroke "a" using {command down}
        keystroke (ASCII character 8) -- backspace
        delay 0.2
        keystroke "v" using {command down}
        delay 0.8
        
        -- Execute
        keystroke return
        return "SUCCESS:" & foundProcess
      else
        return "ERROR:Roblox process not found"
      end if
    end tell
  `;

  return new Promise((resolve) => {
    exec(`osascript -e '${appleScript}'`, { timeout: 20000 }, (err, stdout) => {
      if (err) return resolve({ success: false, error: err.message });
      const result = stdout.trim();
      if (result.startsWith('SUCCESS:')) {
        resolve({ success: true, process: result.split(':')[1] });
      } else {
        resolve({ success: false, error: result.split(':')[1] || 'Unknown error' });
      }
    });
  });
});

ipcMain.handle('get-loader', () => {
  return 'local s,r=pcall(game.HttpGet,game,"http://127.0.0.1:5500/loader") if s then loadstring(r)() else warn("RCL ERR:"..tostring(r)) end';
});

ipcMain.handle('save-file', async (event, { name, content }) => {
  const filePath = path.join(app.getPath('userData'), 'workspace', `${name}.lua`);
  try {
    fs.writeFileSync(filePath, content);
    return { success: true, path: filePath };
  } catch (err) {
    return { success: false, error: err.message };
  }
});
