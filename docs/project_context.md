# ESP32 Digital Drum Kit — Project Context

## What This Project Is
A progressive drum kit project that starts with zero extra hardware and adds capability phase by phase. The end goal is a standalone physical instrument, but the first deliverable is a fully working browser-based drum machine driven by an ESP32 over USB — no extra components needed.

## The Problem with Starting at the End
The full system requires: MAX98357A amplifier, SD card module, 8 buttons, speaker, OLED, enclosure.  
Right now we only have an **ESP32-M1 DevKit and a USB cable**.

Rather than block on hardware procurement, we use the ESP32's USB/Serial connection to a Chrome web app that handles all audio. This is Phase 0.

---

## Phase 0 — UART + Web Browser MVP ✓ COMPLETE

### Why This Approach
- **Zero extra hardware** — ESP32 + USB cable is all that's needed
- **Web Serial API** (Chrome-only) lets the browser open a serial port — no drivers, no Node.js, no Electron
- **Web Audio API** handles sample playback — low latency, no installs, works offline
- **Identical command protocol** to what Phase 1 will use — so firmware changes are additive, not rewrites
- **Standalone test mode** — browser app works without ESP32 connected (click pads with mouse)

### Architecture
```
ESP32 Firmware
  └── Serial.println("KICK")   ← 115200 baud UART over USB
          |
     USB cable
          |
Chrome Web App (index.html)
  ├── Web Serial API            ← reads serial port, parses command strings
  └── Web Audio API             ← decodes WAV buffers, plays on trigger
```

### Command Protocol
Newline-terminated uppercase strings. Simple, human-readable, easy to test in Serial Monitor.

```
KICK
SNARE
HIHAT_CLOSED
HIHAT_OPEN
TOM_LOW
TOM_MID
CRASH
RIDE
```

ESP32 firmware in Phase 0 acts as a "command echo" — you type in Serial Monitor, it echoes the canonical command. In Phase 1, buttons replace typing.

### Files
```
firmware/phase0/phase0.ino   ← reads Serial, echoes drum commands
web_app/phase0/index.html    ← Chrome app, self-contained
web_app/phase0/app.js        ← Web Serial + Web Audio logic
web_app/phase0/styles.css    ← drum pad UI
web_app/phase0/samples/      ← WAV files (22050Hz 16-bit mono)
```

### Limitations Accepted in Phase 0
- Audio plays in browser, not from ESP32 — requires laptop to be open during play
- Latency is browser-dependent (~20–100ms, not the 10ms target) — this is fine for Phase 0
- No physical buttons — input is Serial Monitor or on-screen click
- Chrome-only (Web Serial API not in Firefox/Safari)

---

## Phase 1 — Physical Buttons (next)

### What Changes
- Wire 8 tactile buttons to GPIO 4, 5, 12, 13, 14, 15, 18, 19
- Firmware adds ISR on FALLING edge per button
- Software debounce: 30ms minimum gap per button
- Firmware sends the same command strings as Phase 0 — browser app unchanged
- Latency improves: button → ISR → UART → browser → audio (~30–60ms)

### What Stays the Same
- Browser web app (zero changes)
- Command protocol (zero changes)
- Serial baud rate (zero changes)

---

## Phase 2 — On-Device I2S Audio

### What Changes
- Add MAX98357A I2S amplifier and SD card module
- WAV samples stored on SD, streamed and polyphonically mixed on ESP32
- I2S output replaces browser audio — PC no longer required during play
- FreeRTOS task split: AudioMixTask on Core 0, InputTask on Core 1
- Latency target: < 10ms (hard requirement for real performance feel)
- Browser app becomes a companion tool (visualization, MIDI, kit editor) rather than the primary audio output

---

## Key Decisions Log

| Decision | Choice | Reason |
|----------|--------|--------|
| Phase 0 audio | Browser (Web Audio API) | No hardware needed |
| Phase 0 transport | USB Serial / UART | Built into every ESP32 DevKit |
| Phase 0 interface | Chrome Web Serial API | No drivers, no install, works from a local HTML file |
| Phase 1+ audio | On-device I2S → MAX98357A | Clean audio, DMA, standalone |
| Sample format | 22050Hz 16-bit mono WAV | Fits SPIFFS, low decode cost |
| Input method | ISR FALLING edge | Lowest latency |
| Debounce | Software 30ms | No extra components |
| Polyphony (Phase 3) | 32-bit accumulator mix | Prevents clipping on simultaneous hits |

---

## Hardware Procurement Plan

| Item | Needed by | Est. Cost | Where to buy |
|------|-----------|----------|-------------|
| 8x tactile push buttons | Phase 1 | ~$3 | Amazon, AliExpress |
| Jumper wires (M-M) | Phase 1 | ~$2 | Amazon |
| MAX98357A I2S amp module | Phase 2 | ~$3 | Amazon, Adafruit |
| SD card module (SPI) | Phase 2 | ~$2 | Amazon |
| MicroSD card 4–8GB | Phase 2 | ~$5 | Amazon |
| Speaker 3W 8Ω | Phase 2 | ~$4 | Amazon, Adafruit |
| OLED 0.96" I2C SSD1306 | Phase 4 | ~$4 | Amazon |
| Project enclosure | Phase 5 | ~$5–15 | Amazon, Hammond |

**Total Phase 1 upgrade cost: ~$5**  
**Total full build cost: ~$28–40**

---

## Drum Pad Mapping (consistent across all phases)

| Pad | Command | GPIO (Phase 1+) | Color suggestion |
|-----|---------|-----------------|-----------------|
| Kick | `KICK` | GPIO 4 | Red |
| Snare | `SNARE` | GPIO 5 | White |
| Hi-Hat Closed | `HIHAT_CLOSED` | GPIO 12 | Yellow |
| Hi-Hat Open | `HIHAT_OPEN` | GPIO 13 | Yellow |
| Low Tom | `TOM_LOW` | GPIO 14 | Blue |
| Mid Tom | `TOM_MID` | GPIO 15 | Blue |
| Crash | `CRASH` | GPIO 18 | Orange |
| Ride | `RIDE` | GPIO 19 | Orange |

---

## Branching Strategy

```
main
 └── phase-0-mvp          ← current branch
      └── phase-1-buttons  ← branches when phase-0-mvp merges to main
           └── phase-2-i2s-audio
                └── phase-3-polyphony
                     └── phase-4-display
                          └── phase-5-enclosure
```

Each phase branch is a PR back to `main` when fully tested.  
Never develop directly on `main`.

---

## Audio Sample Sources (Phase 0)
Get free WAV samples before starting:
- freesound.org (search "kick drum wav", filter by license)
- sampleswap.org (free drum kit packs)
- Convert to 22050Hz 16-bit mono WAV using Audacity

Place files in `web_app/phase0/samples/` with exact filenames:  
`kick.wav`, `snare.wav`, `hihat_closed.wav`, `hihat_open.wav`, `tom_low.wav`, `tom_mid.wav`, `crash.wav`, `ride.wav`
