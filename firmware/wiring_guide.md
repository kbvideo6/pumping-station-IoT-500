# Hardware Wiring Guide — CT Clamp to ESP32-S3

This document explains how to safely connect the **SCT-013-020 Non-Invasive CT Clamp Current Sensor** to the **Waveshare ESP32-S3 A7670E 4G Development Board**.

---

## ⚠️ Safety Warning
- The CT clamp is **non-invasive**. You **do not** need to cut or strip any high-voltage power cables.
- **Never open or close the CT clamp when current is flowing through the primary wire.** This can cause high-voltage arcs across the secondary terminals. Open/close only when the pump power circuit breaker is turned **OFF**.
- Clip the clamp around **ONLY ONE** of the current-carrying wires (Phase/Live or Neutral). If you clip it around a bundle containing both Phase and Neutral, their magnetic fields cancel each other out, and the reading will be `0.0 A`.

---

## 🛠️ Required Components
1. **SCT-013-020** CT Clamp (built-in burden resistor: output is `0 to 1V AC` for `0 to 20A AC` primary current).
2. **Resistors**: 2x `10 kΩ` resistors (for creating a 1.65V offset divider).
3. **Capacitor**: 1x `10 µF` electrolytic capacitor (for stabilizing the reference offset voltage).
4. **Waveshare ESP32-S3 A7670E** Development Board.
5. Breadboard or prototype stripboard for building the bias circuit.

---

## 📐 Circuit Schematic

Because the CT clamp output is an Alternating Current (AC) wave, it swings positive and negative (relative to GND). The ESP32 analog pins can only read positive voltages between `0V and 3.3V`.

We must build a **DC Bias Circuit** that shifts the AC signal up by `1.65V` (half of 3.3V) so that the negative swings remain above GND, safely within the `0V to 3.3V` range.

```
                  3.3V (VCC)
                   │
                  [ ] 10 kΩ Resistor
                   │
                   ├───┐ (1.65V Virtual Reference Point)
                   │   │
                   │  [+] 10 µF Capacitor
                  [ ]  [─] (Smooths voltage swings)
                  10   │
                  kΩ   ▼ GND
                   │
                   ├──────────────────────────── Secondary CT Clamp Terminal 1 (S1)
                   │
                  [ ] (Internal burden resistor inside SCT-013-020)
                   │
                   ├─────────── GPIO 1 ───────── Secondary CT Clamp Terminal 2 (S2)
                   │            (Analog Input)
                   ▼ GND
```

---

## 🔌 Connection Map

Follow these connections step-by-step:

### 1. The Voltage Divider (Offset Reference)
- Connect one **10 kΩ Resistor** between **3.3V** and a free terminal strip on your board.
- Connect the second **10 kΩ Resistor** between that same terminal strip and **GND**.
- *Result:* This node now has a stable DC voltage of `1.65V`.

### 2. Stabilization
- Connect the positive terminal `[+]` of the **10 µF Capacitor** to the `1.65V` node.
- Connect the negative terminal `[─]` of the **10 µF Capacitor** to **GND**.

### 3. CT Clamp Interface
- The SCT-013-020 CT clamp terminates in a standard **3.5mm stereo jack**.
- If cutting the jack off:
  - **Tip wire** (usually Blue/S1): Connect to the `1.65V` bias node.
  - **Sleeve wire** (usually Red/S2): Connect directly to **GPIO 1** of the ESP32.
  - **Ring wire** (usually white/shield): Connect to **GND** or leave unconnected (shielding).
- *Verify with a multimeter:* The resistance across the two signal wires of the CT clamp should be around `50Ω to 100Ω` (due to the built-in burden resistor).

---

## 🔍 Validation Checklist
1. **Bias Point Check**: Turn on the ESP32 (disconnect CT clamp). Measure the voltage at the `1.65V` node relative to GND. It should read approximately `1.65V DC`.
2. **ADC Pin Check**: Measure the voltage on **GPIO 1** relative to GND. It should also read `1.65V DC`.
3. **Primary Line Check**: Clamp around **only the Phase (L)** wire powering the pump. Ensure the clamp click-lock is fully engaged.
