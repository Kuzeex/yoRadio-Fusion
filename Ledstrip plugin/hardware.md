# LED Strip – Recommended Wiring & Power Supply

This document describes the recommended hardware setup for connecting a WS2812B LED strip.

---

## Signal Line: Level Shifter

The ESP32-S3 operates at **3.3V logic**, while WS2812B LEDs require a **5V data signal** for reliable operation. A level shifter is mandatory between the ESP32 and the LED strip.

### Recommended IC: 74LVC1G125 (single buffer, 3-state)

The 74LVC1G125 is a single-channel buffer/level shifter that accepts 3.3V input and outputs a clean 5V signal when powered from 5V. It is small, cheap, and purpose-built for this task.

```
ESP32-S3 GPIO48 (3.3V) ──[ 74LVC1G125 ]──► LED strip DATA IN (5V)
                              │
                             5V / GND
```

**Wiring:**

| 74LVC1G125 pin | Connection |
|---|---|
| Pin 1 (OE, active low) | GND (always enabled) |
| Pin 2 (A, input) | ESP32 GPIO (e.g. GPIO48) |
| Pin 3 (GND) | GND |
| Pin 4 (Y, output) | LED strip DATA IN |
| Pin 5 (Vcc) | 5V |

Add a **100nF decoupling capacitor** between Vcc and GND, as close to the IC as possible.

---

## Power Supply

### Calculation (WS2812B, 144 LEDs)

| Scenario | Current |
|---|---|
| Theoretical maximum (all white, full brightness) | 144 × 60mA = **8.64A** |
| Practical maximum (brightness 80%, mixed colors) | ~**6–7A** |
| Typical music playback (VU / Fire / Rainbow effects) | ~**1.5–3A** average |

### Recommendation

| Use case | Power supply |
|---|---|
| Safe minimum | 5V / 6A |
| Recommended | **5V / 8A** |
| Heavy use (Meter VU, full white, high brightness) | 5V / 10A |

Use a quality switched-mode PSU with short-circuit and overcurrent protection.

---

## Wiring Diagram

```
                    ┌─────────────────────────────────────┐
                    │       Custom ESP32-S3 Board          │
                    │                                      │
                    │  GPIO48 ──[ 74LVC1G125 ]── RGB-KI ──┼──► DATA
                    └─────────────────────────────────────┘
                                                           │
5V PSU ──────────────────────────────────────────────────►│ 5V  ┐
                                                           │     ├── LED strip (start)
GND ─────────────────────────────────────────────────────►│ GND ┘
         │
      [1000µF/6.3V] ← connect in PARALLEL: + to 5V, - to GND
         │              place BEFORE the first LED, as close as possible
```

For strips longer than 1 meter, also inject power at the far end:

```
5V PSU ──────────────────────────────────────────────────────► 5V  ┐
                                                                     ├── LED strip (end)
GND ─────────────────────────────────────────────────────────► GND ┘
         │
      [1000µF/6.3V] ← connect in PARALLEL: + to 5V, - to GND
         │              place AFTER the last LED, as close as possible
```

---

## Decoupling Capacitors

| Location | Value | Purpose |
|---|---|---|
| 74LVC1G125 Vcc–GND | 100nF ceramic | IC supply decoupling (already on board) |
| LED strip, first LED, 5V–GND | **1000µF / 6.3V** electrolytic | Suppress power-on current spike |
| LED strip, last LED, 5V–GND | **1000µF / 6.3V** electrolytic | Reduce voltage drop at far end |

> The 1000µF capacitors go on the **power rail of the LED strip**, not on the signal line. They are placed at the strip itself, not on the ESP32 board.

---

## Wiring Cable Recommendations

Voltage drop on thin wire causes color shift (strip end appears yellowish or dimmer):

| LED count | Recommended wire gauge (5V / GND) |
|---|---|
| up to 60 | 0.5 mm² (AWG 20) |
| 60–144 | **0.75–1.0 mm²** (AWG 18) |
| 144 + power injection | 0.75 mm² per end |

Keep the 5V power cable as short as possible. The data signal cable length is less critical thanks to the 74LVC1G125 buffer.

---

## Summary Checklist

- [ ] 74LVC1G125 level shifter between ESP32 GPIO and LED data line
- [ ] 100nF ceramic cap on 74LVC1G125 Vcc–GND
- [ ] 5V / 8A (or higher) power supply
- [ ] ESP32 board powered separately, not from the LED strip supply
- [ ] 1000µF / 6.3V cap at the start of the LED strip
- [ ] 1000µF / 6.3V cap at the end of the LED strip (power injection)
- [ ] 0.75–1.0 mm² wire for 5V and GND to the strip