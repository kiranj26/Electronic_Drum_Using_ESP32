# ESP32 Digital Drum Kit

A progressive hardware + software drum machine project built on the ESP32-M1 DevKit.  
Each phase adds capability — from a zero-hardware browser demo all the way to a standalone physical instrument.

---

## Project Phases

### Phase 0 — UART + Web Browser MVP ✅ Complete
**Branch:** `phase-0-mvp`  
**Hardware needed:** ESP32-M1 DevKit + USB cable (nothing else)

The ESP32 reads commands over Serial (UART). A Chrome web app listens via the **Web Serial API** and plays drum sounds in the browser using the **Web Audio API**.  
No amplifier. No buttons. No SD card. Just a USB cable and a browser.

**How it works:**
```
[Serial Monitor / ESP32 firmware]
            |
       USB (UART)
            |
[Chrome Web App — Web Serial API]
            |
[Web Audio API — plays drum sounds]
```

**Goal:** Prove end-to-end sound triggering. Type `KICK` in Serial Monitor → hear a kick drum in your browser.

**Deliverables:**
- `firmware/phase0/` — ESP32 sketch that sends drum command strings over Serial
- `web_app/phase0/` — Single-page Chrome app (HTML + JS) using Web Serial + Web Audio

---

### Phase 1 — Physical Buttons → UART → Browser ✅ Complete
**Branch:** `phase-1-buttons`  
**Hardware needed:** Phase 0 + 7 tactile push buttons + jumper wires + breadboard

7 buttons wired to ESP32 GPIO pins via breadboard. Each button press triggers a hardware interrupt on the ESP32, which sends a drum command string over UART to the Chrome web app. 10ms software debounce.

**How it works:**
```
Physical button press
        ↓
ESP32 GPIO interrupt (FALLING edge, IRAM_ATTR ISR)
        ↓
10ms debounce check
        ↓
Serial.println("KICK") over USB
        ↓
Chrome Web Serial API reads it
        ↓
Web Audio API plays drum sound
```

**GPIO map:**
| GPIO | Drum |
|------|------|
| 4 | KICK |
| 5 | SNARE |
| 12 | HIHAT_CLOSED |
| 13 | HIHAT_OPEN |
| 14 | TOM_LOW |
| 15 | TOM_MID |
| 18 | CRASH |

**Deliverables:**
- `firmware/phase1/` — Button ISR + debounce + UART output

## Demo

### Phase 1 — Physical Buttons Demo
![Phase 1 Demo](docs/assets/phase1_demo.gif)
https://github.com/user-attachments/assets/dd6a18f4-1137-4b9d-854d-ca3681d37620

---

### Phase 2 — On-Device Audio (I2S + MAX98357A)
**Branch:** `phase-2-i2s-audio`  
**Hardware needed:** Phase 1 + MAX98357A I2S amp + 3W speaker + SD card module

Move audio playback onto the ESP32 itself. WAV samples stored on SD card, mixed polyphonically, output through I2S amplifier. Browser companion becomes optional.

**Goal:** Standalone drum kit — no PC required during performance.

**Deliverables:**
- `firmware/phase2/` — I2S driver + polyphonic WAV mixer + SD sample manager
- WAV sample set (22050Hz 16-bit mono)

---

### Phase 3 — Polyphony + FreeRTOS Optimization
**Branch:** `phase-3-polyphony`  
**Hardware needed:** Phase 2 (same)

Proper FreeRTOS task layout — audio on Core 0, input on Core 1. 4–8 voice polyphonic mixer. Latency tuned to < 10ms.

**Goal:** Real-time performance quality. Hit multiple pads simultaneously.

---

### Phase 4 — OLED Display + Kit Switching
**Branch:** `phase-4-display`  
**Hardware needed:** Phase 3 + 0.96" OLED (I2C) + optional rotary encoder

Display current kit name, BPM, hit indicators. Toggle between Rock / Electronic / Jazz sample sets.

---

### Phase 5 — Enclosure + Final Build
**Branch:** `phase-5-enclosure`  
**Hardware needed:** Full BOM (~$28–40 total)

Physical enclosure, color-coded buttons, speaker grille, polished firmware. The finished instrument.

---

## Hardware BOM by Phase

| Component | Phase needed | Est. Cost |
|-----------|-------------|----------|
| ESP32-M1 DevKit | Phase 0 | (have it) |
| USB cable | Phase 0 | (have it) |
| 8x tactile push buttons | Phase 1 | ~$3 |
| Jumper wires | Phase 1 | ~$2 |
| MAX98357A I2S amp module | Phase 2 | ~$3 |
| SD card module (SPI) | Phase 2 | ~$2 |
| MicroSD card 4–8GB | Phase 2 | ~$5 |
| Speaker 3W 8Ω | Phase 2 | ~$4 |
| OLED 0.96" I2C | Phase 4 | ~$4 |
| Project enclosure | Phase 5 | ~$5–15 |
| **Phase 0 total** | | **$0** |
| **Full build total** | | **~$28–40** |

---

## Repository Structure

```
Electronic_Drum_Using_ESP32/
├── CLAUDE.md                    ← Claude Code pair programming context
├── README.md                    ← This file
├── docs/
│   ├── system_requirements.md   ← Full hardware + firmware spec (end goal)
│   └── project_context.md       ← Phase-by-phase decisions and context
├── firmware/
│   ├── phase0/                  ← UART command sender sketch
│   ├── phase1/                  ← Button input + UART
│   └── phase2/                  ← I2S audio (future)
└── web_app/
    ├── phase0/                  ← Web Serial + Web Audio MVP
    └── phase1/                  ← Improved UI (future)
```

---

## Development Log

| Phase | Status | Branch |
|-------|--------|--------|
| Phase 0 — UART + Browser MVP | **Complete** | `phase-0-mvp` |
| Phase 1 — Physical Buttons | **Complete** | `phase-1-buttons` |
| Phase 2 — On-Device I2S Audio | Not started | `phase-2-i2s-audio` |
| Phase 3 — Polyphony + RTOS | Not started | `phase-3-polyphony` |
| Phase 4 — OLED + Kit Switch | Not started | `phase-4-display` |
| Phase 5 — Enclosure | Not started | `phase-5-enclosure` |
