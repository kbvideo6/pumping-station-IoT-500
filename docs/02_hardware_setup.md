# 2 — Hardware Setup & Wiring Guide

> **Audience:** Field Technician / Electrical Engineer
> **Replaces:** Root-level `installation_guide.md` (now deleted)

---

## Parts Required Per Station

| Item | Specification | Notes |
|------|--------------|-------|
| Waveshare ESP32-S3 A7670E | 4G LTE + GPS development board | Main controller |
| PZEM-004T v3.0 | AC Power Meter module | Sensor (100A split-core CT included) |
| 1NCE IoT SIM Card | Micro-SIM, flat-rate €10/10 years | Or any LTE SIM with APN credentials |
| DIN-rail PSU | 5V DC / 2A minimum (e.g. Mean Well MDR-10-5) | Powers the ESP32 board |
| IP65 plastic enclosure | Min. 150×100×70 mm with cable glands | Houses the board inside the cabinet |
| 4G Antenna | External SMA whip antenna | Supplied with Waveshare board |
| GNSS Antenna | Passive or active GPS patch antenna | Supplied with Waveshare board |

---

## Section 1 — SIM Card Activation (1NCE)

1. Go to the [1NCE Customer Portal](https://configuration.1nce.com) and create an account.
2. Click **Activate SIM Card** → enter the **ICCID** and **Activation Code** printed on the SIM plastic holder.
3. Wait **10–30 minutes** for network allocation.
4. Insert the SIM into the **Micro-SIM slot** on the Waveshare board. Golden contacts face **downward** toward the PCB.

> **Note:** The firmware is pre-configured for `iot.1nce.net` APN — no username or password required.

---

## Section 2 — PZEM-004T Wiring

The PZEM-004T communicates with the ESP32 board using **Modbus RTU over UART** (Serial2).

### 2.1 Power Connections

The PZEM-004T module needs a **5V power supply** and must be connected to the **AC line** being measured:

```
  230V AC Mains
  ─────┬─────────────────────────────────────
       │ L (Live)           ┌──────────────┐
       ├───────────────────►│              │
       │                    │  PZEM-004T   │
  ─────┼────────────────────│  AC Module   │
       │ N (Neutral)        │              │
       └───────────────────►│              │
                            └──────┬───────┘
                                   │ 5V / GND (power to module)
```

Connect the **PZEM-004T power input terminals** (`L` and `N`) to the mains supply (or to the load/pump side — either works). The module draws ~1W from the mains for self-power.

### 2.2 Current Transformer (CT Clamp)

- Clip the **included split-core CT clamp** around a **single conductor** of the pump's supply cable (L1, L2, or L3 — pick one active phase).
- Do **not** clamp around both L and N together — the magnetic fields cancel and you will read zero.
- Plug the CT into the round jack socket on the PZEM module.

### 2.3 UART Serial Data Wiring (PZEM ↔ ESP32-S3)

Connect the PZEM-004T TTL serial header to the ESP32-S3 board:

| PZEM-004T Pin | ESP32-S3 GPIO | Wire Color Suggestion |
|--------------|---------------|----------------------|
| 5V | 5V (board header) | Red |
| GND | GND (board header) | Black |
| TX | **GPIO 16** (Serial2 RX) | Yellow |
| RX | **GPIO 15** (Serial2 TX) | Green |

> ⚠️ **Important:** The PZEM TX connects to the ESP32 RX (GPIO 16), and PZEM RX connects to ESP32 TX (GPIO 15). Cross-connect TX→RX.

> ⚠️ **Voltage:** PZEM TTL serial operates at **3.3V logic levels** — safe for direct connection to the ESP32-S3 (also 3.3V). Do not connect to 5V-only microcontrollers without a level shifter.

### 2.4 Wiring Diagram (ASCII)

```
  ┌─────────────────────────────────┐
  │       PZEM-004T v3.0            │
  │  ┌──────────────────────────┐   │
  │  │ L ──── Mains L           │   │
  │  │ N ──── Mains N           │   │  ┌──────────────────────────┐
  │  │ CT ─── CT clamp on L1    │   │  │  Waveshare ESP32-S3       │
  │  │                          │   │  │  A7670E 4G Board          │
  │  │ 5V ──────────────────────┼───┼─►│ 5V                        │
  │  │ GND ─────────────────────┼───┼─►│ GND                       │
  │  │ TX ──────────────────────┼───┼─►│ GPIO16 (Serial2 RX)       │
  │  │ RX ──────────────────────┼───┼──│ GPIO15 (Serial2 TX)       │
  │  └──────────────────────────┘   │  │                           │
  └─────────────────────────────────┘  │ GPIO18 ──► Modem TX       │
                                        │ GPIO17 ──► Modem RX       │
                                        │ GPIO4  ──► Modem PWRKEY   │
                                        │ GPIO5  ──► Modem RESET    │
                                        │ GPIO2  ──► Status LED     │
                                        └──────────────────────────┘
```

---

## Section 3 — Antenna Placement

### 4G Cellular Antenna
- Screw the external **SMA whip antenna** onto the gold SMA connector.
- If the main electrical cabinet is **metal**, route the antenna **outside** through a rubber-gasketed hole or place it near a non-metallic vent. A metal cabinet will block LTE signals.
- In weak signal areas, use a high-gain external whip antenna (5–7 dBi).

### GPS / GNSS Antenna
- Connect the **GPS patch antenna** to the U.FL/IPEX connector labeled `GNSS` on the board.
- Mount the antenna **horizontally** with a **clear view of the sky** for fastest satellite fix.
- Stick it to the top surface of the plastic enclosure, or route it outside the metal cabinet.

---

## Section 4 — Enclosure Mounting

1. Mount the Waveshare board inside an **IP65 plastic enclosure** using M3 standoffs or adhesive PCB standoffs.
2. Pre-drill cable glands for:
   - Mains power cable (to DIN-rail PSU → 5V to board)
   - PZEM power + serial cable (PZEM is usually mounted separately near the mains entry)
   - CT clamp cable
   - Antenna cable (if routing outside)
3. Mount the enclosure inside the electrical cabinet using double-sided mounting tape or DIN-rail clips.
4. **Do not use a metal enclosure for the ESP32 board** — it will block the 4G signal.

> 💡 The PZEM-004T module can be mounted separately inside the cabinet near the mains terminals, with only the 4-wire UART cable running to the ESP32 board enclosure.

---

## Section 5 — Power Supply

| Supply | Specification |
|--------|--------------|
| Mains Input | 230V AC, 50Hz |
| DIN-Rail PSU Output | 5V DC, ≥2A |
| Board Supply | Connect PSU 5V/GND to board's 5V header or USB-C |
| PZEM Power | Taken directly from mains terminals (self-powered) |

---

## Section 6 — LED Status Indicator

The onboard LED on GPIO 2 shows the current device state:

| LED Pattern | Meaning |
|-------------|---------|
| Solid ON | Booting / Initializing |
| Slow Blink (1 Hz) | Connecting to 4G cellular network |
| Fast Blink (4 Hz) | Authenticating with Firebase cloud |
| Heartbeat (double-pulse, every 2s) | Normal operation — data uploading |
| Rapid Flash | Error state — check serial monitor |

---

*Next: [Firmware & Provisioning →](./03_firmware_provisioning.md)*
