# 4 — Dashboard User Guide

> **Audience:** Operators, Supervisors, Client Staff — no technical knowledge required.

---

## Getting Started

### Login
1. Open your web browser (Chrome, Firefox, Safari, or Edge).
2. Navigate to the dashboard URL provided by your administrator.
3. Enter your **email** and **password**.
4. Click **Anmelden** (Login).

> If you have forgotten your password, contact your system administrator.

---

## Dashboard Overview

After logging in, you will see the main **Dashboard** with:
- A **map** showing all pump station locations with color-coded status markers
- A **station list** below the map with key metrics at a glance
- A **language toggle** (DE / EN) in the top-right corner
- A **logout** button

---

## Station Status Colors

| Color | Meaning |
|-------|---------|
| 🟢 Green | Station online and operating normally |
| 🟡 Yellow / Orange | Warning — a threshold has been exceeded |
| 🔴 Red | Alert active — immediate attention may be required |
| ⚫ Grey | Station offline or not reporting |

---

## Viewing a Station's Live Data

1. Click any station on the **map** or in the **station list**.
2. The **Station Detail** page opens, showing:

### Live Metrics Panel (updates every 30 seconds)

| Metric Card | What It Means |
|-------------|--------------|
| **Voltage (V)** | AC mains voltage at the pump. Normal range: 200–250V |
| **Current (A)** | Electrical current the pump is drawing. High = overload; Low = stopped |
| **Power (W)** | Actual power being consumed |
| **Energy (kWh)** | Cumulative energy since last meter reset |
| **Frequency (Hz)** | Mains frequency. Normal: 50Hz |
| **Power Factor** | Electrical efficiency. Good: 0.90–1.00; Poor: below 0.80 |
| **Pump Status** | Running / Stopped / Alert |
| **Last Seen** | Timestamp of the last received reading |

### Live Chart

The chart shows the last **60 readings** (approximately 30 minutes) of:
- **Current (A)** — left axis (cyan line)
- **Voltage (V)** — right axis (amber line)

You can zoom in on any part of the chart by scrolling or pinching on mobile.

---

## Viewing Alert History

From the Station Detail page, click the **Alerts** tab to see:
- A chronological list of all alerts for this station
- Alert type (High Current, Low Voltage, Offline, etc.)
- Start time and resolution time
- Whether the alert was automatically resolved or manually acknowledged

### Alert Types

| Alert | Description |
|-------|-------------|
| HIGH CURRENT | Pump drawing more current than its threshold — possible overload |
| LOW CURRENT | Pump drawing less current than threshold — possible stop or dry run |
| HIGH VOLTAGE | Mains voltage above safe limit |
| LOW VOLTAGE | Mains voltage below safe limit |
| OFFLINE | Device not reporting — communication or power failure |

---

## Viewing Historical Data

1. From the Station Detail page, click the **History** tab.
2. Select a **start date** and **end date**.
3. Click **Laden** (Load).
4. A table and dual-axis chart appear with all readings for that period.

### Export to CSV / Excel

1. After loading the historical data, click **CSV Export**.
2. A `.csv` file is downloaded to your computer.
3. Open it in **Microsoft Excel** or **Google Sheets** for reporting.

The export includes columns:
`Timestamp, Voltage (V), Current (A), Power (W), Energy (kWh), Frequency (Hz), Power Factor, Pump Status`

---

## Managing Stations (Admin Only)

Navigate to **Stationen verwalten** from the top navigation bar.

### Add a New Station
1. Click **Station hinzufügen** (Add Station).
2. Enter Station ID, name, GPS coordinates, and pump power.
3. Click **Provisionieren** to generate a provisioning token for the technician.

### Edit a Station
- Click the **pencil icon** next to any station in the list.
- You can update: name, alert thresholds (high/low current and voltage), location.

### Delete a Station
- Click the **trash icon** next to a station.
- Confirm deletion. This removes the station from the dashboard (historical data is preserved in the archive).

---

## Managing Users (Admin Only)

Navigate to **Benutzerverwaltung** (User Management).

| Action | How |
|--------|-----|
| Add User | Click **Benutzer hinzufügen**, enter email + role |
| Change Role | Click edit icon → select new role (Admin / Operator / Viewer) |
| Disable User | Toggle the active/inactive switch |
| Delete User | Click trash icon |

> **IMPORTANT:** Security upgrades have enabled Firebase Custom Claims. If a user is promoted to Admin, or a new Admin account is created, they must **Log Out and Log In** for their browser to fetch the new security token containing the Admin claim.

---

## Language Toggle

- Use the **DE / EN** button in the top-right corner to switch the interface language.
- Your preference is saved in the browser.

---

## Tips

- **Bookmark** the dashboard URL for quick access.
- The dashboard **auto-refreshes** every 30 seconds — no need to manually reload.
- The **map clusters** stations when zoomed out. Click a cluster to zoom in and see individual stations.
- All times are shown in your **local time zone** (set by your operating system).

---

*Next: [Backend & Cloud Setup →](./05_backend_cloud_setup.md)*
