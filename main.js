const { app, BrowserWindow } = require('electron')
const minecraft = require('./src/js/game');

const path = require('node:path')

const createWindow = () => {
  const win = new BrowserWindow({
    width: 800,
    height: 600,
    frame: false,
    webPreferences: {
      preload: path.join(__dirname, 'src/js/preload.js')
    }
  })

  win.loadFile('src/frontend/index.html')
}

app.whenReady().then(() => {
  createWindow()
})