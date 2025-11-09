
# Cero-Client
A **custom Minecraft launcher** for everyone — built with [Electron.js](https://www.electronjs.org/). (5%)

---

## Features & Tasks

- [ ] **Instances**
  - [ ] Install a loader (e.g. Forge, Fabric, etc.)
  - [ ] Save and manage instances
- [ ] **Play Minecraft**
- [ ] **Connect Microsoft Account**

---

## Installation

Go to the [**Releases page**](https://github.com/CeroWorks/Cero-Client/releases) and download the appropriate package for your OS.  
> **Note:** macOS is not yet supported (coming soon).

---

## Build It Yourself

You’ll need:
- **Node.js** ≥ `v24.11.0`
- **npm** ≥ `v11.6.1`

Then run:

```bash
npm install
````

To build for your platform (example for Linux):

```bash
npx electron-builder --linux
```

> Change `--linux` to `--win` or `--mac` depending on your target OS.