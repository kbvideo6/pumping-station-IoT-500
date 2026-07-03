# 1 — System Overview

> **Audience:** Everyone — no technical knowledge required.

---

## What Does This System Do?

The **Pumping Station IoT Monitoring System** allows you to remotely watch and manage hundreds of water pump stations in real time — from any web browser, anywhere in the world.

Without this system, a field technician must physically drive to each pump station to check if it is running correctly. With this system, all that information is available instantly on a web dashboard.

---

## Key Benefits at a Glance

| Benefit | Detail |
|---------|--------|
| 📡 **Real-time Monitoring** | Live electrical readings every 30 seconds from every station |
| 🔔 **Automatic Alerts** | Email notifications the moment a pump overloads, stops, or voltage drops |
| 📊 **Historical Data** | Full log of all readings — export to Excel/CSV for reporting |
| 🌍 **Interactive Map** | See all stations on a map, color-coded by status |
| 🔒 **Secure Access** | Role-based login (Admin, Operator, Viewer) |
| 📈 **Scalable** | Supports 1 to 400+ stations with no infrastructure changes |

---

## System Components

```
┌─────────────────────────────────────────────────────────┐
│                   PUMP STATION (on-site)                │
│                                                         │
│  ┌─────────────┐    Modbus RTU    ┌──────────────────┐  │
│  │ PZEM-004T   │◄────────────────►│  ESP32-S3 Board  │  │
│  │ Power Meter │                  │  (Waveshare 4G)  │  │
│  └─────────────┘                  └────────┬─────────┘  │
│                                            │ 4G LTE      │
└────────────────────────────────────────────┼────────────┘
                                             │
                                    ┌────────▼────────┐
                                    │  Google Firebase │
                                    │  (Cloud Database)│
                                    └────────┬─────────┘
                                             │
                                    ┌────────▼─────────┐
                                    │  Web Dashboard   │
                                    │  (Any Browser)   │
                                    └──────────────────┘
```

### Component Descriptions

#### 1. PZEM-004T AC Power Meter (On-site Sensor)
- Clamps onto the pump's electrical cable inside the cabinet
- Measures: **Voltage (V), Current (A), Power (W), Energy (kWh), Frequency (Hz), Power Factor**
- Communicates with the main board via a short cable (Modbus protocol)
- Requires no external power — draws power from the ESP32 board's 5V rail

#### 2. Waveshare ESP32-S3 A7670E 4G Board (The "Brain")
- A small computer installed inside the electrical cabinet
- Reads sensor data every 5 seconds, averages it, then uploads every 30 seconds
- Connects to the internet via a **4G SIM card** (no Wi-Fi needed)
- Has an onboard **GPS** — automatically determines and reports its own location
- Has a **status LED** to indicate operational state at a glance

#### 3. Google Firebase Cloud (Data Storage & Logic)
- Stores all live and historical readings securely in the cloud
- Triggers automatic alert emails when thresholds are exceeded
- Free tier is sufficient for the pilot; cost scales linearly for 400+ stations

#### 4. Web Dashboard (Your Control Panel)
- Accessible from any modern web browser (Chrome, Firefox, Safari, Edge)
- No app installation required
- Features: live metrics, map view, alert history, data export, station management

---

## What Data Is Collected?

Every 30 seconds, each station reports:

| Metric | Unit | Purpose |
|--------|------|---------|
| Voltage | V | Detect over/under voltage (grid faults) |
| Current | A | Detect pump overload or pump failure |
| Active Power | W | Monitor energy consumption |
| Energy | kWh | Billing and efficiency tracking |
| Frequency | Hz | Detect grid instability |
| Power Factor | — | Detect electrical efficiency issues |
| GPS Location | lat/lon | Automatic station mapping |
| Online Status | — | Is the device reachable? |

---

## Alert System

The system sends email alerts automatically for:

- 🔴 **HIGH CURRENT** — pump drawing too much power (overload / blockage)
- 🔵 **LOW CURRENT** — pump stopped or tripped (dry run / power cut)
- 🟡 **HIGH VOLTAGE** — grid overvoltage (risk to pump motor)
- 🟡 **LOW VOLTAGE** — grid undervoltage (pump running inefficiently)
- ⚫ **DEVICE OFFLINE** — communication lost for more than 5 minutes

All thresholds are configurable per station from the dashboard.

---

## User Roles

| Role | Can Do |
|------|--------|
| **Admin** | Everything — manage users, stations, settings |
| **Operator** | View all stations, acknowledge alerts, export data |
| **Viewer** | Read-only access to dashboard |

---

*Next: [Hardware Setup & Wiring →](./02_hardware_setup.md)*
