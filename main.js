const { app, BrowserWindow, ipcMain, dialog } = require('electron');
const path = require('path');
const fs = require('fs');
const { exec } = require('child_process');
const http = require('http');

let win;
let logWatcher = null;
let lastLogSize = 0;
let executionBuffer = "";

// Create a simple HTTP server to communicate with Roblox
const server = http.createServer((req, res) => {
  if (req.url === '/get-script') {
    res.writeHead(200, { 'Content-Type': 'text/plain' });
    res.end(executionBuffer); 
    executionBuffer = ""; // Clear after serving
  } else {
    res.writeHead(404);
    res.end();
  }
});

server.on('error', (e) => {
  console.error('Script server error:', e);
});

server.listen(5500, '127.0.0.1', () => {
  console.log('Script server running at http://127.0.0.1:5500/');
});

function createWindow() {
  const splash = new BrowserWindow({
    width: 400,
    height: 300,
    transparent: true,
    frame: false,
    alwaysOnTop: true,
    resizable: false,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false
    }
  });

  splash.loadFile(path.join(__dirname, 'splash.html'));

  win = new BrowserWindow({
    width: 800,
    height: 600,
    show: false,
    frame: false,
    transparent: true,
    resizable: false,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false,
      devTools: true
    }
  });

  win.loadFile(path.join(__dirname, 'app.html'));

  // After 4.5 seconds, hide splash and show main window
  setTimeout(() => {
    splash.close();
    win.show();
    win.focus();
  }, 4500);
}

// Find the most recent Roblox log file
function getLatestRobloxLog() {
  let logDir;
  if (process.platform === 'win32') {
    // Windows path: %LocalAppData%\Roblox\logs
    logDir = path.join(process.env.LOCALAPPDATA, 'Roblox/logs');
  } else {
    // macOS path: ~/Library/Logs/Roblox
    logDir = path.join(app.getPath('home'), 'Library/Logs/Roblox');
  }

  if (!fs.existsSync(logDir)) return null;

  try {
    const files = fs.readdirSync(logDir)
      .filter(f => f.endsWith('.log'))
      .map(f => ({ name: f, time: fs.statSync(path.join(logDir, f)).mtime.getTime() }))
      .sort((a, b) => b.time - a.time);

    return files.length > 0 ? path.join(logDir, files[0].name) : null;
  } catch (e) {
    console.error('Error reading log directory:', e);
    return null;
  }
}

function startWatchingLogs() {
  if (logWatcher) clearInterval(logWatcher);

  logWatcher = setInterval(() => {
    const logPath = getLatestRobloxLog();
    if (!logPath) return;

    try {
      const stats = fs.statSync(logPath);
      if (stats.size > lastLogSize) {
        const stream = fs.createReadStream(logPath, {
          start: lastLogSize,
          end: stats.size
        });

        stream.on('data', (chunk) => {
          const lines = chunk.toString().split('\n');
          lines.forEach(line => {
            if (line.trim()) {
              win.webContents.send('roblox-log', line.trim());
            }
          });
        });

        lastLogSize = stats.size;
      } else if (stats.size < lastLogSize) {
        // Log file rotated or cleared
        lastLogSize = 0;
      }
    } catch (e) {
      console.error('Error reading log:', e);
    }
  }, 1000);
}

app.whenReady().then(() => {
  createWindow();
  startWatchingLogs();

  // Create workspace directories
  const workspacePath = path.join(app.getPath('userData'), 'workspace');
  const autoexecPath = path.join(app.getPath('userData'), 'autoexec');
  if (!fs.existsSync(workspacePath)) fs.mkdirSync(workspacePath, { recursive: true });
  if (!fs.existsSync(autoexecPath)) fs.mkdirSync(autoexecPath, { recursive: true });

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

// --- REAL FUNCTIONALITY IPC HANDLERS ---

// Window Controls
ipcMain.on('close-app', () => app.quit());
ipcMain.on('minimize-app', () => win.minimize());

// Check if Roblox is running (Real detection)
ipcMain.handle('check-roblox', async () => {
  return new Promise((resolve) => {
    const cmd = process.platform === 'win32' ? 'tasklist' : 'ps aux';
    exec(cmd, (err, stdout) => {
      if (err) return resolve(false);
      resolve(stdout.toLowerCase().includes('roblox'));
    });
  });
});

// File Management
ipcMain.handle('save-file', async (event, { name, content, isAutoExec }) => {
  const dir = isAutoExec ? path.join(app.getPath('userData'), 'autoexec') : path.join(app.getPath('userData'), 'workspace');
  const filePath = path.join(dir, name.endsWith('.lua') ? name : `${name}.lua`);
  
  try {
    fs.writeFileSync(filePath, content);
    return { success: true, path: filePath };
  } catch (err) {
    return { success: false, error: err.message };
  }
});

ipcMain.handle('read-workspace', async () => {
  const workspacePath = path.join(app.getPath('userData'), 'workspace');
  try {
    const files = fs.readdirSync(workspacePath);
    return files.filter(f => f.endsWith('.lua'));
  } catch (err) {
    return [];
  }
});

ipcMain.handle('read-file', async (event, fileName) => {
  const filePath = path.join(app.getPath('userData'), 'workspace', fileName);
  try {
    return fs.readFileSync(filePath, 'utf-8');
  } catch (err) {
    return null;
  }
});

// Get workspace path for the user to see
ipcMain.handle('get-workspace-path', () => {
  return path.join(app.getPath('userData'), 'workspace');
});

// --- NEW FUNCTIONALITY: GAME DETECTION ---
ipcMain.handle('detect-game', async () => {
  const logPath = getLatestRobloxLog();
  if (!logPath) return { success: false, error: 'No Roblox logs found.' };

  try {
    const content = fs.readFileSync(logPath, 'utf8');
    
    // Look for PlaceId and UniverseId in logs
    // Typical log line: [FLog::Network] Game join completed. UniverseId: 1234567, PlaceId: 8901234
    const placeIdMatch = content.match(/PlaceId:\s*(\d+)/i);
    const universeIdMatch = content.match(/UniverseId:\s*(\d+)/i);
    
    if (placeIdMatch) {
      const placeId = placeIdMatch[1];
      const universeId = universeIdMatch ? universeIdMatch[1] : 'Unknown';
      
      return { 
        success: true, 
        placeId, 
        universeId,
        logPath: path.basename(logPath)
      };
    }
    
    return { success: false, error: 'Could not find game ID in the latest log.' };
  } catch (err) {
    return { success: false, error: err.message };
  }
});

// --- NEW FUNCTIONALITY: UPDATE SYSTEM ---
const CURRENT_VERSION = require('./package.json').version;
const LATEST_VERSION_MOCK = "1.1.0"; // Simulated newer version

ipcMain.handle('check-update', async () => {
  // Simulate a network delay
  await new Promise(r => setTimeout(r, 1000));
  
  if (LATEST_VERSION_MOCK !== CURRENT_VERSION) {
    return { 
      updateAvailable: true, 
      latestVersion: LATEST_VERSION_MOCK,
      currentVersion: CURRENT_VERSION
    };
  }
  return { updateAvailable: false };
});

ipcMain.on('perform-update', (event) => {
  // In a real app, this would download an installer and run it
  // For this project, we'll just simulate it and restart the app
  console.log("Updating to version " + LATEST_VERSION_MOCK);
  
  // Simulate download time
  setTimeout(() => {
    dialog.showMessageBoxSync({
      type: 'info',
      title: 'Update Complete',
      message: `The executor has been updated to version ${LATEST_VERSION_MOCK}. The app will now restart.`,
      buttons: ['OK']
    });
    
    // Restart logic (simulated)
    app.relaunch();
    app.exit();
  }, 3000);
});
