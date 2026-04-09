# ESP32 Digital Drum Kit — Project Context

## What This Project Is
A progressive drum kit project that starts with zero extra hardware and adds capability phase by phase. The immediate goal is a **fully wireless drum kit** — press physical buttons, hear sounds on your iPhone, no cables, no laptop, no router. The long-term goal is a fully standalone physical instrument with on-device audio.

---

## Phase 0 — UART + Web Browser MVP ✓ COMPLETE

### Why This Approach
- Zero extra hardware — ESP32 + USB cable only
- Web Serial API (Chrome-only) opens serial port — no drivers, no installs
- Web Audio API handles playback — low latency, works offline
- Proved end-to-end pipeline before adding any hardware

### Architecture
```
ESP32 → Serial.println("KICK") → USB → Chrome Web Serial API → Web Audio API
```

### Limitations Accepted
- Audio on laptop only — laptop must be open
- Chrome-only (Web Serial not in Safari/Firefox)
- No physical buttons — Serial Monitor or on-screen click only

---

## Phase 1 — Physical Buttons ✓ COMPLETE

### What Changed
- 7 tactile buttons wired to GPIO 4,5,12,13,14,15,18 via breadboard
- ISR per button (IRAM_ATTR, FALLING edge, 10ms debounce)
- Same command protocol as Phase 0 — browser app unchanged
- First time it felt like a real instrument

### What Stayed the Same
- Chrome web app (zero changes)
- Command strings (KICK, SNARE, etc.)
- Audio plays on laptop

---

## Phase 2 — WiFi AP + WebSocket → iPhone Audio ✓ COMPLETE

### The Problem Phase 2 Solves
Phase 1 still requires a USB cable to a laptop. The user has an iPhone and wants zero cables, zero laptop, zero router. Just press a button and hear sound on the phone.

### Why WiFi Access Point (not WiFi client, not Bluetooth)
| Option | Latency | Needs router | Extra hardware | Verdict |
|--------|---------|-------------|---------------|---------|
| Bluetooth A2DP | 100–300ms | No | No | Too slow for drums |
| WiFi client mode | 10–20ms | Yes | No | Needs home network |
| **WiFi AP mode** | **3–8ms** | **No** | **No** | **Chosen** |
| ESP-NOW | 2–5ms | No | Second ESP32 | Overkill for now |

ESP32 in AP mode = its own hotspot. iPhone connects directly. No infrastructure at all.

### Architecture
```
ESP32 boots →
  ├── WiFi AP: "DrumKit-ESP32" / "drumkit123"
  ├── HTTP server (port 80) → serves index.html
  └── WebSocket server (port 81) → pushes drum commands

iPhone →
  ├── Connects to "DrumKit-ESP32" WiFi
  ├── Safari → http://192.168.4.1
  ├── Loads web app (HTML + JS + CSS + base64 WAVs — all one file)
  ├── WebSocket connects to ws://192.168.4.1:81
  └── Button press → WebSocket message → Web Audio API → phone speaker
```

### Key Decisions for Phase 2
| Decision | Choice | Reason |
|----------|--------|--------|
| Wireless protocol | WiFi AP mode | No router, lowest latency, no extra hardware |
| Command transport | WebSocket | Real-time push, persistent, ~1ms delivery |
| WAV delivery | Base64 inside HTML | Avoids SPIFFS file serving, one file download |
| Web app hosting | ESP32 HTTP server | Phone loads directly from ESP32, no laptop |
| Phone browser | iPhone Safari | Web Audio + WebSocket fully supported |
| Audio unlock | "Tap to Start" screen | iOS Safari requires user gesture before audio |

### What Changes from Phase 1
| | Phase 1 | Phase 2 |
|--|---------|---------|
| Connection | USB cable | WiFi (no cable) |
| Web app location | Laptop local server | Served by ESP32 |
| Transport | UART Serial | WebSocket |
| Audio output | Laptop speakers | iPhone speaker |
| Infrastructure | Laptop + Chrome | Just iPhone |

### What Stays the Same
- Button wiring (identical GPIO pins)
- ISR + debounce logic (copy from Phase 1)
- Command strings (KICK, SNARE, etc.)
- Drum pad UI look and feel

### Libraries Added
- `links2004/WebSockets` — WebSocket server on ESP32

### Latency Budget
| Stage | Target |
|-------|--------|
| Button → ISR | < 1ms |
| ISR → WebSocket broadcast | < 2ms |
| WiFi AP → iPhone | < 8ms |
| Web Audio playback | < 5ms |
| **Total** | **< 16ms** |

---

## Phase 3 — FreeRTOS Dual-Core Task Split ✓ COMPLETE

### The Problem Phase 3 Solves
Phase 2 ran everything in `loop()` on Core 1. WebSocket polling and HTTP handling competed with button flag checks. Under WiFi load, button presses could lag.

### Fix
| Core | Task | Responsibility |
|------|------|---------------|
| Core 0 | WiFiTask (priority 19) | `ws_server.loop()` + `http_server.handleClient()` |
| Core 1 | InputTask (priority 20) | Read ISR flags → `ws_server.broadcastTXT()` |
| Any | ISRs (IRAM_ATTR) | Set `volatile trigger_flags[]` only |

`loop()` sleeps with `vTaskDelay(portMAX_DELAY)`.

### What Stays the Same
- Same GPIO wiring and ISR debounce logic
- Same WebSocket protocol and command strings
- Same web app (copied from Phase 2)

---

## Phase 4a — SD Card WAV Loading (NEXT UP)

### Goal
Prove that 8 WAV drum samples can be loaded from a microSD card over SPI at boot. No audio output yet — purely validates the data pipeline.

### Hardware
- Adafruit MicroSD Card Breakout Board ✅ (owned)
- MicroSD card FAT32-formatted ✅ (owned)

### GPIO conflict resolution
GPIO 18 (SPI SCK) and GPIO 5 (SD CS) conflict with existing button assignments. Solution: remap CRASH → GPIO 32, SNARE → GPIO 33.

| Button | Phase 1–3 GPIO | Phase 4a+ GPIO |
|--------|---------------|----------------|
| SNARE | 5 | 33 |
| CRASH | 18 | 32 |

### SPI pin assignments
| Signal | GPIO |
|--------|------|
| SCK | 18 |
| MOSI | 23 |
| MISO | 19 |
| CS | 5 |

### SD file layout
```
/kick.wav  /snare.wav  /hihat_closed.wav  /hihat_open.wav
/tom_low.wav  /tom_mid.wav  /crash.wav  /ride.wav
```
All files: 22050Hz, 16-bit, mono PCM, max ~200KB each.

---

## Phase 4b — I2S Amp + Speaker Audio (FUTURE)

### Goal
Route Phase 4a's WAV buffers through a MAX98357A I2S amplifier to a physical speaker. Fully standalone — no phone, no laptop.

### Hardware
- MAX98357A I2S amp breakout ❌ (to purchase, ~$3)
- 4Ω or 8Ω speaker, 2–3W ❌ (to purchase, ~$4)

### I2S pin assignments
| Signal | GPIO |
|--------|------|
| BCLK | 26 |
| LRC (WS) | 25 |
| DOUT | 22 |

### Target
- Button press → audible sound < 10ms
- 4-voice polyphony (simultaneous hits)
- No pops, no crackling, stable after 30min play

---

## Key Decisions Log (All Phases)

| Decision | Choice | Reason |
|----------|--------|--------|
| Phase 0 audio | Browser Web Audio API | No hardware needed |
| Phase 0 transport | USB Serial / UART | Built into every DevKit |
| Phase 1 input | ISR FALLING edge | Lowest latency |
| Phase 1 debounce | Software 10ms | No extra components, tuned on hardware |
| Phase 2 wireless | WiFi AP mode | No router, no Bluetooth lag, no extra hardware |
| Phase 2 transport | WebSocket | Real-time push, works on iPhone Safari |
| Phase 2 WAV delivery | Base64 in HTML | Single file, no SPIFFS complexity |
| iOS AudioContext fix | resume() before every play() | iOS suspends AudioContext when page not touched — WebSocket triggers were silent without this |
| Sample format | 22050Hz 16-bit mono WAV | Compact, low decode cost |

---

## Drum Pad Mapping (consistent across all phases)

| Pad | Command | GPIO | Color |
|-----|---------|------|-------|
| Kick | `KICK` | GPIO 4 | Red |
| Snare | `SNARE` | GPIO 5 | White |
| Hi-Hat Closed | `HIHAT_CLOSED` | GPIO 12 | Yellow |
| Hi-Hat Open | `HIHAT_OPEN` | GPIO 13 | Yellow |
| Low Tom | `TOM_LOW` | GPIO 14 | Blue |
| Mid Tom | `TOM_MID` | GPIO 15 | Blue |
| Crash | `CRASH` | GPIO 18 | Orange |

---

## Hardware Procurement Plan

| Item | Needed by | Have it | Est. Cost |
|------|-----------|---------|----------|
| 7x tactile push buttons | Phase 1 | ✅ | ~$3 |
| Jumper wires | Phase 1 | ✅ | ~$2 |
| Breadboard | Phase 1 | ✅ | — |
| iPhone | Phase 2 | ✅ | — |
| Adafruit microSD card breakout | Phase 4a | ✅ | ~$8 |
| MicroSD card 4–8GB | Phase 4a | ✅ | ~$5 |
| MAX98357A I2S amp | Phase 4b | ❌ | ~$3 |
| Speaker 3W 8Ω | Phase 4b | ❌ | ~$4 |
| OLED 0.96" I2C | Phase 5 | ❌ | ~$4 |
| Project enclosure | Phase 6 | ❌ | ~$5–15 |

---

## Branching Strategy

```
main
 ├── phase-0-mvp       ✅ merged
 ├── phase-1-buttons   ✅ merged
 ├── phase-2-wifi-ap   ✅ merged
 ├── phase-3-polyphony ✅ merged
 ├── phase-4a-sd-card
 ├── phase-4b-i2s-audio
 ├── phase-5-display
 └── phase-6-enclosure
```

Each phase branches from main after previous phase merges. Never develop on main directly.
