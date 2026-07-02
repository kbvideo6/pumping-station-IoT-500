# Cabinet Installation & Scaling Guide

This guide describes how to deploy the Pumping Station IoT monitoring hardware inside electrical cabinets, activate cellular SIM cards, and scale the system to 400+ stations.

---

## 📶 Section 1: SIM Card Activation (1NCE M2M)

The pilot uses a **1NCE IoT 4G SIM Card** (recommended flat rate for €10/10 years).

1. **Register**: Go to the [1NCE Customer Portal](https://configuration.1nce.com) and create an account.
2. **Activate SIM**: Click **Activate SIM Card** and enter the **ICCID** and **Activation Code** printed on the plastic SIM holder.
3. **Wait for network allocation**: Activation typically takes 10 to 30 minutes. Once activated, insert the SIM card into the **Micro-SIM slot** of the Waveshare ESP32-S3 board (ensure the golden contacts face downwards toward the PCB).

*Note: The firmware configuration APN is already set to `iot.1nce.net` (no username/password needed).*

---

## 🛠️ Section 2: Cabinet Hardware Installation

Follow these guidelines for physical deployment inside the pump station electrical panel:

### 1. Enclosure Mounting
- Mount the Waveshare board inside a **waterproof IP65 plastic enclosure** with pre-drilled cable glands.
- Secure the enclosure inside the electrical cabinet using double-sided mounting tape or DIN-rail clips.
- *Caution:* Do not use a metal enclosure for the board, as it will block 4G cellular signals.

### 2. Antenna Placement
- Screw the external **4G cellular antenna** onto the board's gold SMA connector.
- Route the cellular antenna cable out of the enclosure. If the main electrical cabinet is metal, route the antenna *outside* the metal cabinet (using a rubber-gasket hole) or place the antenna close to non-metallic windows or vents. In weak signal areas, mount a high-gain whip antenna externally.
- Connect a **passive or active GPS/GNSS antenna** to the U.FL/IPEX connector labeled `GNSS` on the board. For the best satellite fix, position the GPS antenna horizontally with a clear, unobstructed view of the sky (e.g., sticking it to the top surface of the plastic box or route it outside the metal cabinet).

### 3. Power Supply Connection
- Provide a stable **5V DC** supply to the ESP32 board.
- The safest method is placing a **5V / 2A DIN-rail power supply** (e.g., Mean Well MDR-10-5) inside the cabinet, wiring it to the 230V AC lines, and connecting its 5V output to the board's 5V/GND headers or USB-C port.

### 4. CT Clamp Installation
- Clip the SCT-013-020 current clamp around the **pump's active Phase cable** (L1 or L2 or L3) inside the cabinet.
- Connect the clamp's jack into your bias circuit box, then route the signal to GPIO 1.

---

## 🚀 Section 3: Flashing and Scaling Steps (For 400+ Stations)

Once the template is established, you can clone and provision new stations yourself in less than 5 minutes per device.

### Workflow Diagram
```
[Admin Portal Dashboard] ──(Create new station)──▶ [Generates Provision Token]
                                                             │
[Physical ESP32 Board] ◀──(Flash firmware with token)────────┘
```

### Step-by-Step Cloning Procedure

1. **Provision on the Web Dashboard**:
   - Log in to the Cloud Dashboard as an Administrator.
   - Navigate to **Stationen verwalten** (Station Management) and click **Station hinzufügen** (Add Station).
   - Enter a unique ID (e.g., `STATION_002`), name, and pump power. You can leave the GPS coordinates as `0.0, 0.0` or empty, as the onboard GPS receiver will automatically update the coordinates and place the marker on the map once it obtains a satellite fix!
   - Click **Provisionieren**. The screen will display a long character sequence called the **Provisionierungstoken** (Provisioning Token). Copy this token.

2. **Configure the Firmware**:
   - Open the firmware code folder in PlatformIO (VS Code).
   - Open `src/config.h`.
   - Update the following lines with the values you just created:
     ```cpp
     #define DEFAULT_STATION_ID    "STATION_002" // The new ID you added
     #define DEFAULT_CUSTOM_TOKEN  "PASTE_THE_COPIED_TOKEN_HERE"
     ```
   - *Ensure the APN is still set to `iot.1nce.net` (for 1NCE SIMs).*

3. **Flash the Board**:
   - Connect the new Waveshare ESP32-S3 board to your computer using a USB-C cable.
   - In PlatformIO, click the **Upload** arrow at the bottom status bar.
   - Once compilation finishes and the terminal outputs `SUCCESS`, unplug the board.

4. **Deploy & Verify**:
   - Power up the board. The status LED will cycle:
     - **Solid ON**: Booting.
     - **Slow Blink**: Connecting to cellular 4G network.
     - **Fast Blink**: Contacting Firebase authentication and exchanging the provisioning token for a permanent key.
     - **Heartbeat (double pulse)**: Normal operation.
   - Go to the Dashboard map or list view. The new station will show as **🟢 Normal** (Online) and start streaming live current readings.
   - Edit the High/Low current thresholds for this station directly on the dashboard tab.
