
# Cero-Client

A **custom Minecraft launcher** for everyone **(French/English)** — built with [Electron.js](https://www.electronjs.org/). (5% complete)

---

## Features & Tasks

- [ ] **Instances**

	- [ ] Install a loader (e.g., Forge, Fabric, etc.)
	- [ ] Save and manage instances
- [ ] **Play Minecraft**
	- [x] Download Manifest
	- [x] Read Metadata
	- [ ] Download Libs
	- [ ] Download Client
	- [ ] Download Assets
	- [ ] Start Client
	- [ ] Install Fabric
	- [ ] Install Forge
	- [ ] Start Fabric
	- [ ] Start Forge
- [ ] **Connect Microsoft Account**

- [ ] Create an Installer
- [x] Create an Updater

---

## Installation

Go to the [**Releases page**](https://github.com/CeroWorks/Cero-Client/releases) and download the package for your OS.

> **Note:** macOS is not yet supported (coming soon).

---

## Build It Yourself

You can build the **Electron.js App** or the **GoLang Updater**.

---

### Electron App

**Requirements:**

* **Node.js** ≥ `v24.11.0`
* **npm** ≥ `v11.6.1`
* **Python** ≥ `3.13`

**Install dependencies:**

```bash
npm install
```

**Build for your platform (example for Linux):**

```bash
npm run build
```

> Replace `--linux` with `--win` for Windows

---

### Updater

**Requirements:**

* **GoLang** ≥ `1.25.3`
* **Node.js** ≥ `v24.11.0`
* **Python** ≥ `3.13`
* **npm** ≥ `v11.6.1`

**Build for your platform (example for Linux):**

```bash
npm run buildUpdater
```

Use Updater interface:

```text
/------- CeroClient Updater -------\
| Choose OS :                      |
| (0) Linux                        |
| (1) Windows                      |
\----------------------------------/
OS : Linux
```

> Replace `Linux` with `Windows` depending on your target OS.
