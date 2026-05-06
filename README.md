# TRYX Panorama Linux GUI

Qt6 GUI application for managing TRYX Panorama AIO cooler displays on Linux.

## Supported Models

| Model | Status |
|-------|--------|
| Panorama SE 360 | Tested |
| Panorama 360 | Tested |
| Panorama SE 240 | Should work (same display protocol) |
| Panorama | Should work |
| Panorama WB | Should work |

## Features

- Upload images, videos, GIFs (auto-converts non-MP4 formats)
- 7 built-in device presets (Cooling delivery, Migration, etc.)
- Real-time system metrics on display (temperature, usage, frequency)
- Hardware name badges (auto-detected from system)
- Brightness control (0-100)
- Display settings: position, alignment, color, filter
- Screen splitting configuration
- Keepalive daemon for persistent display
- Auto-detects device (scans /dev/ttyACM*)
- System tray integration (requires tray support — see Troubleshooting below)
- Minimize to tray on close, start minimized, autostart on login (systemd user service)
- Consistent dark theme across all dialogs and widgets
- Settings persistence between sessions
- Async device communication (non-blocking GUI)

## Requirements

**Build:**
- Qt6 (Core, Gui, Widgets)
- C++17 compiler
- qmake6

**Runtime:**
- `ffmpeg` — media conversion
- `adb` (android-tools) — required for uploading and managing media on the device. Serial-based features (metrics, brightness, presets) keep working without it.
- `glxinfo` (mesa-utils) — GPU name detection (optional)

**Permissions:**
- User must be in `dialout` group (or `uucp` on Arch) for serial access

## Build

```bash
git clone https://github.com/ggbeauty989/tryx-panorama-linux.git
cd tryx-panorama-linux
qmake6
make -j$(nproc)
./build/tryx-panorama-manager
```

## Install

To run the binary from anywhere and enable autostart on login:

```bash
mkdir -p ~/.local/bin ~/.config/systemd/user
cp build/tryx-panorama-manager ~/.local/bin/
cp systemd/tryx-panorama.service ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable --now tryx-panorama.service
```

Alternatively, leave the service disabled and tick **Autostart on login** in the Settings page.

## Project Structure

```
src/
  core/              # Device protocol library
  main.cpp           # Entry point
  mainwindow.*       # Main window with navigation
  panoramapage.*     # Display + metrics configuration
  displaypage.*      # Full-screen media browser and upload
  splitconfig.*      # Screen splitting configuration
  homepage.*         # System monitoring dashboard
  settingspage.*     # App settings (port, tray, autostart)
  devicemanager.*    # Async device communication
  systemmonitor.*    # System metrics reader
  traymanager.*      # System tray
include/panorama/    # Protocol headers
media/               # Built-in videos
systemd/             # User service unit for autostart
```

## Tested on

| Distro | Kernel | CPU | GPU1 | GPU2 |
|--------|--------|-----|------|------|
| Fedora 43 | 6.19.10 | AMD Ryzen 9 9950X3D | AMD Radeon RX 7900 XTX | NVIDIA GeForce RTX 5060 Ti |
| Ubuntu 26.04 | 7.0.0-15 | AMD Ryzen 7 9800X3D | AMD Radeon RX 9070 XT Nitro+ | — |

## Troubleshooting

### L'app si chiude completamente invece di restare in background (system tray)

L'applicazione usa `QSystemTrayIcon::isSystemTrayAvailable()` per determinare se può ridursi a icona nella system tray quando la finestra viene chiusa.
Se la system tray **non è disponibile**, il `closeEvent` di `MainWindow` forza la chiusura completa del processo chiamando `qApp->quit()`.

Questo accade tipicamente su:

- **GNOME** (da Ubuntu 17.10+ le tray icon X11 legacy non sono supportate nativamente)
- **Wayland puro** senza compatibilità XWayland per le tray icon
- Alcune configurazioni di **KDE Plasma** su Wayland

#### Soluzioni

**GNOME / Ubuntu:**
```bash
# Installa l'estensione AppIndicator (riavvia la sessione dopo)
sudo apt install gnome-shell-extension-appindicator
```
Oppure installa l'estensione **"Tray Icons Reloaded"** da [GNOME Extensions](https://extensions.gnome.org/).

**KDE Plasma:**
Verifica che la system tray sia attiva nel pannello (tasto destro sul pannello → "Add Widgets" → "System Tray").

**Avvio minimizzato:**
Se la tray funziona, puoi avviare l'app direttamente in background con:
```bash
tryx-panorama-manager --hidden
```

**Systemd (autostart):**
Il servizio systemd incluso (`systemd/tryx-panorama.service`) avvia l'app all'accesso. Assicurati che la tray sia funzionante prima di abilitarlo:
```bash
systemctl --user enable --now tryx-panorama.service
```

#### Comportamento tecnico (per sviluppatori)

Il flusso di chiusura è gestito in `src/mainwindow.cpp` (`closeEvent`):
- Se `TrayManager::isAvailable()` restituisce `true`: la finestra viene nascosta (`hide()`), l'evento di chiusura viene ignorato (`event->ignore()`), e l'app rimane in esecuzione con l'icona nella tray.
- Se `false`: viene chiamato `event->accept()` + `qApp->quit()`, terminando completamente il processo.

`TrayManager::isAvailable()` delega a `QSystemTrayIcon::isSystemTrayAvailable()` (definita in `src/traymanager.cpp`).

In `main.cpp` è impostato `QApplication::setQuitOnLastWindowClosed(false)` per evitare che Qt termini l'app quando la finestra viene nascosta — ma questo viene sovrascritto dal `closeEvent` quando la tray non è presente.

## Background

This project was reverse-engineered from KANALI (the official Windows app):

- Unpacked KANALI resources to extract the built-in media library (15 videos)
- Full protocol analysis to discover device commands for system metrics display
- Implemented real-time CPU/GPU/Disk temperature monitoring on the cooler screen
- Built the Qt6 GUI from scratch (Home, Panorama, Display, Screen Splitting, Rota, Settings)
- Auto-detection of CPU/GPU hardware names for badge display
- Auto-conversion of non-MP4 media formats (WebM, MKV, AVI, GIF) before upload to device
- Fixed serial communication issues (timeouts, wrong command formats, broken ADB quoting)
- Restructured into a single qmake project

## License

[MIT](LICENSE)

## Credits

Based on [reed-tpse](https://github.com/fadli0029/reed-tpse) protocol library.
