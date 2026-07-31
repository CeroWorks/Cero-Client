# CeroClient

<p align="center">
  CeroClient is a free Minecraft client designed to be highly optimized and lightweight.</p>

## Features & Tasks

- [ ] **Instances**
	- [ ] Install a loader (e.g., Forge, Fabric, etc.)
	- [ ] Save and manage instances
- [x] **Play Minecraft**
	- [x] Download Manifest
	- [x] Read Metadata
	- [x] Download Libs
	- [x] Download Client
	- [x] Download Assets
	- [x] Start Client
	- [ ] Install Fabric
	- [ ] Install Forge
	- [ ] Start Fabric
	- [ ] Start Forge
- [x] **Connect Microsoft Account**
- [x] Create an Installer
- [x] Create an Updater
- [ ] Rewrite the Launcher in C/C++
- [ ] Friend & Chat system
    - [x] Send Message
    - [ ] Invite in his world
    - [x] Add Friend
    - [x] Remove Friend
- [ ] Optimize the game
    - [ ] Rewrite all Minecraft version in C++
        - [ ] Modding support in Lua
    - [ ] Switch to Vulkan
        - [ ] Shader Support
- [ ] Add Android Support
- [ ] Multi Language Support

---

## Installation

All downloads and instructions for CeroClient are available on our [Website](https://cerostudio.fr/ceroclient).

## Building from Source

If you want to compile CeroClient yourself, you can use the provided build scripts in the repository.

**Requirements (Linux):**
* **GCC / G++** ≥ `13.3.0`
* **Python** ≥ `3.9`
* **Rust / Cargo** (Latest stable)
* **pkg-config**
* **Dependencies:** `gtk+-3.0`, `webkit2gtk-4.1`, `libcurl` (and `ayatana-appindicator3-0.1` or `appindicator3-0.1` for system tray support)

**Requirements (Windows):**
* **MinGW-w64** (GCC / G++)
* **Rust / Cargo** (Latest stable)
* Pre-compiled dependencies in `%USERPROFILE%\mingw-deps\x64-windows`

### Instructions

1. Clone the repository.
2. Install all python dependencies: `pip install -r requirements.txt`
3. Run the build script: `python3 build.py`

*Note: The build scripts will automatically download the required UI assets from our CDN if the internal packaging tool is not present.*

---

## License

This project is distributed under the **PolyForm Strict License 1.0.0**. 

The code source is available for reading, auditing, and personal use. However, modification, redistribution, and commercial use of the code are strictly prohibited. See the [LICENSE](./LICENSE) file for full details.

---

© 2025-2026 Cero Studio. All rights reserved.
