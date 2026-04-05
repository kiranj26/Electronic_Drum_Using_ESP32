# ESP32 Digital Drum Kit

A progressive hardware + software drum machine built on the ESP32-M1 DevKit.  
Each phase adds capability — from a zero-hardware browser demo to a fully wireless instrument you play with physical buttons, no cables, no laptop.

---

## Project Phases

### Phase 0 — UART + Web Browser MVP ✅ Complete
**Branch:** `phase-0-mvp` | **Hardware:** ESP32 + USB cable only

ESP32 sends drum commands over USB Serial. Chrome web app listens via Web Serial API and plays sounds via Web Audio API. Proof of concept — zero extra hardware.

```
Serial Monitor → USB → Chrome Web Serial API → Web Audio API → laptop speakers
```

---

### Phase 1 — Physical Buttons → UART → Browser ✅ Complete
**Branch:** `phase-1-buttons` | **Hardware:** + 7 buttons + breadboard + jumper wires

7 tactile buttons wired to GPIO pins. Each press fires a hardware interrupt, debounces in 10ms, sends a drum command over USB to Chrome. First time it feels like a real instrument.

```
Button press → ESP32 ISR → Serial UART → Chrome → Web Audio API → laptop speakers
```

**GPIO map:** GPIO 4=KICK, 5=SNARE, 12=HIHAT_CLOSED, 13=HIHAT_OPEN, 14=TOM_LOW, 15=TOM_MID, 18=CRASH

#### Phase 1 Demo

https://github.com/user-attachments/assets/dd6a18f4-1137-4b9d-854d-ca3681d37620

---

### Phase 2 — WiFi AP + WebSocket → iPhone Audio 🔄 In Progress
**Branch:** `phase-2-wifi-ap` | **Hardware:** No new hardware needed

**The big leap — completely wireless. No USB cable. No laptop. No router.**

ESP32 creates its own WiFi hotspot. Your iPhone connects to it, opens Safari, loads the drum web app served directly from the ESP32. Press a button → WebSocket message → phone speaker plays the sound.

```
Button press
      ↓
ESP32 GPIO ISR (10ms debounce)
      ↓
WebSocket broadcast over WiFi AP
      ↓
iPhone Safari receives message
      ↓
Web Audio API plays drum sound on phone speaker
```

**How to connect:**
1. Press power on ESP32
2. On iPhone → Settings → WiFi → connect to **"DrumKit-ESP32"** (password: `drumkit123`)
3. Open Safari → go to `http://192.168.4.1`
4. Tap "Start" → press buttons → hear drums

**What's new in firmware:**
- WiFi AP mode (no router needed)
- HTTP server serving bundled web app
- WebSocket server (port 81) for real-time push to phone
- Same button ISR + debounce from Phase 1

**What's new in web app:**
- WAV samples bundled as base64 inside the HTML (no external files)
- WebSocket replaces Web Serial API (works on iPhone Safari)
- "Tap to Start" screen (iOS Safari audio requirement)

#### Phase 2 Demo
> 📹 Video placeholder — will be added after testing

---

### Phase 3 — Polyphony + FreeRTOS Optimization
**Branch:** `phase-3-polyphony` | **Hardware:** Same as Phase 2

FreeRTOS task split — WiFi/WebSocket on Core 0, button input on Core 1. Eliminates any scheduling jitter between input detection and wireless broadcast.

---

### Phase 4 — On-Device I2S Audio
**Branch:** `phase-4-i2s-audio` | **Hardware:** + MAX98357A amp + SD card + 3W speaker

Move audio playback onto the ESP32 itself. WAV samples on SD card, polyphonically mixed, output through I2S amplifier to a physical speaker. Phone becomes optional.

```
Button press → ESP32 → I2S → MAX98357A → speaker  (no phone, no laptop)
```

---

### Phase 5 — OLED Display + Kit Switching
**Branch:** `phase-5-display` | **Hardware:** + 0.96" OLED (I2C)

Display current kit name and hit indicators. Toggle between Rock / Electronic / Jazz sample sets.

---

### Phase 6 — Enclosure + Final Build
**Branch:** `phase-6-enclosure` | **Hardware:** Full BOM

Physical enclosure, color-coded buttons, speaker grille, polished firmware. The finished instrument.

---

## Demo

### Phase 1 — Physical Buttons

https://github.com/user-attachments/assets/dd6a18f4-1137-4b9d-854d-ca3681d37620

---

## Hardware BOM by Phase

| Component | Phase needed | Have it? | Est. Cost |
|-----------|-------------|----------|----------|
| ESP32-M1 DevKit | Phase 0 | ✅ | — |
| USB cable (data) | Phase 0 | ✅ | — |
| 7x tactile push buttons | Phase 1 | ✅ | ~$3 |
| Jumper wires | Phase 1 | ✅ | ~$2 |
| Breadboard | Phase 1 | ✅ | — |
| iPhone (any) | Phase 2 | ✅ | — |
| MAX98357A I2S amp module | Phase 4 | ❌ | ~$3 |
| SD card module (SPI) | Phase 4 | ❌ | ~$2 |
| MicroSD card 4–8GB | Phase 4 | ❌ | ~$5 |
| Speaker 3W 8Ω | Phase 4 | ❌ | ~$4 |
| OLED 0.96" I2C | Phase 5 | ❌ | ~$4 |
| Project enclosure | Phase 6 | ❌ | ~$5–15 |
| **Phase 2 total** | | | **$0** |
| **Full build total** | | | **~$28–40** |

---

## Repository Structure

```
Electronic_Drum_Using_ESP32/
├── CLAUDE.md                    ← Claude Code pair programming context
├── README.md                    ← This file
├── GETTING_STARTED.md           ← Step-by-step setup guide
├── docs/
│   ├── system_requirements.md   ← Full hardware + firmware spec
│   └── project_context.md       ← Phase decisions and context
├── firmware/
│   ├── phase0/                  ← UART command echo sketch
│   ├── phase1/                  ← Button ISR + UART
│   └── phase2/                  ← WiFi AP + WebSocket (current)
└── web_app/
    ├── phase0/                  ← Chrome Web Serial + Web Audio app
    └── phase2/                  ← Self-contained iPhone web app
```

---

## Development Log

| Phase | Status | Branch |
|-------|--------|--------|
| Phase 0 — UART + Browser MVP | ✅ Complete | `phase-0-mvp` |
| Phase 1 — Physical Buttons | ✅ Complete | `phase-1-buttons` |
| Phase 2 — WiFi AP + iPhone Audio | 🔄 In Progress | `phase-2-wifi-ap` |
| Phase 3 — Polyphony + RTOS | Not started | `phase-3-polyphony` |
| Phase 4 — On-Device I2S Audio | Not started | `phase-4-i2s-audio` |
| Phase 5 — OLED + Kit Switch | Not started | `phase-5-display` |
| Phase 6 — Enclosure | Not started | `phase-6-enclosure` |
