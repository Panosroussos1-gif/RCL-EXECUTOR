const { app, BrowserWindow, ipcMain, dialog } = require('electron');
const path = require('path');
const fs = require('fs');
const { exec } = require('child_process');
const http = require('http');

let win;
let executionBuffer = "";

// Simple HTTP server for Roblox communication
const server = http.createServer((req, res) => {
  if (req.url === '/get-script') {
    res.writeHead(200, { 'Content-Type': 'text/plain' });
    res.end(executionBuffer); 
    executionBuffer = ""; 
  } else {
    res.writeHead(404);
    res.end();
  }
});

server.listen(5500, '127.0.0.1', () => {
  console.log('Free Script server running at http://127.0.0.1:5500/');
});

function createWindow() {
  win = new BrowserWindow({
    width: 650,
    height: 450,
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

ipcMain.handle('save-file', async (event, { name, content }) => {
  const filePath = path.join(app.getPath('userData'), 'workspace', `${name}.lua`);
  try {
    fs.writeFileSync(filePath, content);
    return { success: true, path: filePath };
  } catch (err) {
    return { success: false, error: err.message };
  }
});
