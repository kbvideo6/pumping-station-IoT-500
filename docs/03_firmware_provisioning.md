# 3 — Firmware Flashing & Station Provisioning

> **Audience:** Developer / Technical Installer
> **Prerequisites:** VS Code + PlatformIO extension installed

---

## Overview

Each physical ESP32-S3 board must be:
1. **Configured** with its unique Station ID and a provisioning token
2. **Compiled** to produce the firmware binary
3. **Flashed** onto the board over USB
4. **Verified** via the web dashboard

---

## Prerequisites

- [VS Code](https://code.visualstudio.com/) with the [PlatformIO](https://platformio.org/install/ide?install=vscode) extension installed
- Clone the repository: `git clone https://github.com/kbvideo6/pumping-station-IoT-500.git`
- Open the **`firmware/`** folder in VS Code / PlatformIO

### Dependencies (auto-installed by PlatformIO on first build)

| Library | Purpose |
|---------|---------|
| `mandulaj/PZEM-004T-v30 @ ^1.1.2` | PZEM-004T Modbus driver |
| `vshymanskyy/TinyGSM` | 4G modem AT command abstraction |
| `vshymanskyy/StreamDebugger` | Debug serial forwarding |
| `mobizt/Firebase-ESP-Client` | Firebase RTDB / Firestore SDK |
| `bblanchon/ArduinoJson` | JSON serialization |

---

## Step 1 — Create the Station on the Dashboard

Before flashing a board, register the station in the cloud:

1. Log into the web dashboard as **Admin**.
2. Navigate to **Stationen verwalten** (Station Management).
3. Click **Station hinzufügen** (Add Station).
4. Fill in:
   - **Station ID** — e.g. `STATION_002` (must be unique, no spaces)
   - **Name** — e.g. `Pumpstation Nord-West`
   - **Pump Power (kW)** — rated motor power
   - **GPS Coordinates** — leave as `0.0 / 0.0`; the onboard GPS will auto-update the location on first boot
5. Click **Provisionieren**.
6. A long string appears — this is the **Provisioning Token**. **Copy it now** (it is only shown once).

---

## Step 2 — Configure the Firmware

Open `firmware/src/config.h` in VS Code and update these two lines:

```cpp
#define DEFAULT_STATION_ID    "STATION_002"              // ← Station ID you created
#define DEFAULT_CUSTOM_TOKEN  "PASTE_TOKEN_HERE"         // ← Provisioning token (step 1)
```

> **All other settings** (Firebase URL, APN, pins, thresholds) are pre-configured as defaults and do not need to change for standard deployments.

### Optional — Change Alert Thresholds at Compile Time

Default thresholds (can also be changed later per-station on the dashboard):

```cpp
#define DEFAULT_HIGH_CURRENT_THRESHOLD  18.0   // Amperes — overload alert
#define DEFAULT_LOW_CURRENT_THRESHOLD    2.0   // Amperes — no-load / stopped alert
#define DEFAULT_HIGH_VOLTAGE_THRESHOLD  250.0  // Volts — overvoltage alert
#define DEFAULT_LOW_VOLTAGE_THRESHOLD   200.0  // Volts — undervoltage alert
```

---

## Step 3 — Flash the Board

1. Connect the Waveshare ESP32-S3 board to your computer using a **USB-C cable**.
2. In PlatformIO (VS Code bottom toolbar), click the **→ Upload** (right-arrow) button.
3. PlatformIO will:
   - Download/update all library dependencies
   - Compile the firmware
   - Flash the binary to the board
4. Look for `SUCCESS` in the terminal output.

### Command-Line Alternative

```bash
cd firmware
pio run --target upload
```

---

## Step 4 — First Boot Sequence

After flashing and powering the board (via USB or DIN-rail PSU):

| Time | What Happens | LED |
|------|-------------|-----|
| 0–5s | Board initializes UART, watchdog, PZEM | Solid ON |
| 5–30s | Modem powers up, connects to 4G network | Slow blink |
| 30–60s | Exchanges provisioning token with Firebase for a permanent device token | Fast blink |
| 60s+ | Begins uploading live PZEM readings every 30 seconds | Heartbeat |

> **Token exchange is permanent** — after the first successful boot, `DEFAULT_CUSTOM_TOKEN` is no longer used. The board stores its permanent credentials in NVS (non-volatile flash storage).

---

## Step 5 — Verify on Dashboard

1. Open the web dashboard.
2. Go to the **Dashboard** or **Map** view.
3. The new station should appear within 60 seconds as **🟢 Online**.
4. Click the station name to see its live metrics (voltage, current, power, etc.).
5. Verify that GPS coordinates are updating (may take up to 5 minutes for first satellite fix outdoors).

---

## OTA (Over-the-Air) Updates

The firmware supports OTA updates. To push a firmware update to deployed boards:

1. Build the new firmware: `pio run --target buildfs` (or use the PlatformIO build button)
2. Upload the `.bin` file to the Firebase Storage bucket at the path configured in `ota.h`
3. Boards check for updates every **24 hours**. The next OTA check will detect the new version and automatically update.

> OTA does not disrupt operation during the download phase — the board continues sending readings until the final reboot.

---

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| Board stuck at fast blink | Invalid provisioning token | Regenerate token on dashboard and re-flash |
| No PZEM readings (all zeros or `--`) | Wiring error | Check GPIO 15/16 cross-connection, confirm PZEM powered |
| No 4G connection | Inactive SIM or wrong APN | Verify SIM activated, check `CELLULAR_APN` in `config.h` |
| Board not appearing on map | GPS no fix | Move GPS antenna to open sky; indoor fix can take 10+ min |
| Upload fails in PlatformIO | Wrong USB driver | Install CP2104 or CH340 driver for your OS |

---

*Next: [Dashboard User Guide →](./04_dashboard_user_guide.md)*
