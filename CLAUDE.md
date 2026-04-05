# ESP32 Digital Drum Kit — Claude Code Pair Programming Guide

## TL;DR — What We're Building
A progressive drum kit project. We start with zero extra hardware (just ESP32 + USB cable + browser) and add capability phase by phase until we have a fully wireless, standalone physical instrument.

**Current phase: Phase 2 — WiFi AP + WebSocket + Phone Audio**
**Current branch: `phase-2-wifi-ap`**

---

## Phase Overview

| Phase | Name | Branch | Hardware | Status |
|-------|------|--------|----------|--------|
| 0 | UART + Browser MVP | `phase-0-mvp` | ESP32 + USB only | **Complete** |
| 1 | Physical Buttons → UART → Browser | `phase-1-buttons` | + 7 buttons + breadboard | **Complete** |
| 2 | WiFi AP + WebSocket → Phone Audio | `phase-2-wifi-ap` | No new hardware | In Progress |
| 3 | Polyphony + FreeRTOS | `phase-3-polyphony` | Same as Phase 2 | Not started |
| 4 | On-Device I2S Audio | `phase-4-i2s-audio` | + MAX98357A + SD card + speaker | Not started |
| 5 | OLED + Kit Switching | `phase-5-display` | + OLED | Not started |
| 6 | Enclosure + Final Build | `phase-6-enclosure` | Full BOM | Not started |

---

## Phase 0 — UART + Web Browser MVP ✅ COMPLETE

### Architecture
```
[ESP32 firmware]
   Serial.println("KICK")   ← sends command string over UART
        |
   USB cable
        |
[Chrome Web App]
   Web Serial API            ← reads the serial port
        |
   Web Audio API             ← plays drum sound in browser
```

### Command Protocol (all phases use this)
| Command | Drum sound |
|---------|-----------|
| `KICK` | Kick drum |
| `SNARE` | Snare drum |
| `HIHAT_CLOSED` | Closed hi-hat |
| `HIHAT_OPEN` | Open hi-hat |
| `TOM_LOW` | Low tom |
| `TOM_MID` | Mid tom |
| `CRASH` | Crash cymbal |
| `RIDE` | Ride cymbal |

---

## Phase 1 — Physical Buttons → UART → Browser ✅ COMPLETE

### What was built
- 7 tactile buttons wired to GPIO 4,5,12,13,14,15,18 via breadboard
- ISR per button (`IRAM_ATTR`, `FALLING` edge)
- 10ms software debounce via `millis()` timestamp
- `Serial.println(command)` sends to Chrome over USB

### GPIO Pin Map (Phase 1+)
| GPIO | Drum |
|------|------|
| 4 | `KICK` |
| 5 | `SNARE` |
| 12 | `HIHAT_CLOSED` |
| 13 | `HIHAT_OPEN` |
| 14 | `TOM_LOW` |
| 15 | `TOM_MID` |
| 18 | `CRASH` |

> Avoid GPIO 6–11 (internal flash). GPIO 34–39 are input-only.

---

## Phase 2 — WiFi AP + WebSocket → Phone Audio (CURRENT)

### Goal
Completely wireless. No USB cable. No laptop. Press a button → iPhone plays drum sound.

### Architecture
```
ESP32 (WiFi Access Point mode)
  ├── SSID: "DrumKit-ESP32"  Password: "drumkit123"
  ├── IP:   192.168.4.1
  ├── HTTP server → serves single bundled HTML file
  └── WebSocket server (port 81) → pushes drum commands to phone
           |
    [iPhone connects to "DrumKit-ESP32" WiFi]
           |
    Safari opens http://192.168.4.1
           |
    Web app loads (HTML + JS + CSS + base64 WAV samples — all in one file)
           |
    WebSocket connects back to ESP32 on ws://192.168.4.1:81
           |
    Button press → ESP32 → WebSocket message → Safari → Web Audio API → phone speaker
```

### Why WiFi AP (not WiFi client)
- No router needed — ESP32 is its own hotspot
- Works anywhere — no home network dependency
- Direct connection = lower latency (~3–8ms vs ~10–20ms via router)
- iPhone connects like any WiFi network

### Key Technical Decisions
| Decision | Choice | Reason |
|----------|--------|--------|
| Transport | WebSocket | Real-time push, persistent connection, ~1ms delivery |
| Web app delivery | Single bundled HTML served by ESP32 | No laptop, no external server |
| WAV samples | Base64-encoded inside HTML | Avoids SPIFFS file serving complexity |
| AP credentials | SSID: DrumKit-ESP32, Pass: drumkit123 | Fixed for easy connection |
| WebSocket port | 81 | Avoids conflict with HTTP on port 80 |
| Phone browser | iPhone Safari | Web Audio API + WebSocket both supported |

### Libraries Required
| Library | Purpose | How to add |
|---------|---------|-----------|
| `WiFi.h` | AP mode | Built-in ESP32 Arduino core |
| `WebServer.h` | HTTP server, serves HTML | Built-in ESP32 Arduino core |
| `WebSocketsServer` | WebSocket push to phone | PlatformIO: `links2004/WebSockets` |

### platformio.ini additions for Phase 2
```ini
lib_deps =
  links2004/WebSockets @ ^2.4.1
```

### Phase 2 File Layout
```
firmware/phase2/
├── platformio.ini
└── src/
    └── main.cpp          ← WiFi AP + HTTP + WebSocket + button ISRs

web_app/phase2/
└── index.html            ← Single self-contained file
                             (HTML + CSS + JS + base64 WAV samples bundled)
```

### Phase 2 Firmware Responsibilities
1. Boot → start WiFi AP "DrumKit-ESP32"
2. Start HTTP server on port 80 → serve `index.html` on GET /
3. Start WebSocket server on port 81
4. Init 7 button GPIO pins (INPUT_PULLUP, ISR FALLING edge, 10ms debounce)
5. On button press → `webSocket.broadcastTXT("KICK")` (same command strings as before)
6. Main loop: `webSocket.loop()` + `server.handleClient()` + check trigger flags

### Phase 2 Web App Responsibilities
1. On load → connect WebSocket to `ws://192.168.4.1:81`
2. On WebSocket message → parse command → play corresponding WAV sample
3. WAV samples bundled as base64 strings in JS — decoded to AudioBuffer on load
4. "Tap to Start" screen on load (iOS Safari requires user gesture before audio)
5. Visual pad grid lights up on hit (same as Phase 0/1 web app)

### iOS Safari Constraints
- **Must have a "Tap to Start" screen** — iOS blocks audio until user taps
- Web Serial API does not exist on iOS — not needed (WebSocket replaces it)
- Web Audio API works fully in Safari iOS 14.5+
- WebSocket works fully in Safari

### Memory Constraints
- ESP32 has 520KB SRAM
- WiFi stack uses ~100KB
- WebSocket server uses ~20KB
- Leaves ~400KB for application — sufficient
- Base64 WAV samples live in phone memory, not ESP32 — no SPIFFS needed

### Latency Budget (Phase 2)
| Stage | Target |
|-------|--------|
| Button press → ISR | < 1ms |
| ISR → WebSocket broadcast | < 2ms |
| WiFi AP → iPhone | < 8ms |
| Web Audio playback start | < 5ms |
| **Total** | **< 16ms** |

---

## Phase 3 — Polyphony + FreeRTOS (FUTURE)

Proper FreeRTOS task split — WiFi/WebSocket on Core 0, button input on Core 1.
4–8 voice polyphony in web app (already supported by Web Audio API).

---

## Phase 4 — On-Device I2S Audio (FUTURE)

### What changes from Phase 2
- Add MAX98357A I2S amplifier (GPIO 25/26/22)
- Add SD card SPI module (GPIO 23/21/20/16)
- WAV files on SD card, streamed and mixed on ESP32
- Phone/web app becomes optional — ESP32 plays audio standalone
- I2S: I2S_NUM_0, Master TX, 22050Hz, 16-bit, DMA 8×512

### Voice struct (do not change shape without updating mixer)
```cpp
struct Voice {
  int16_t* buffer;
  uint32_t position;
  uint32_t length;
  float    volume;
  bool     active;
};
```

---

## Code Style Rules (All Phases)

- `snake_case` for variables and functions
- `UPPER_SNAKE_CASE` for constants and `#define`
- Comment GPIO numbers inline: `#define BTN_KICK 4  // GPIO4`
- ISRs must be `IRAM_ATTR` — set flags only, no Serial, no I/O, no malloc
- No `delay()` anywhere in the input or audio path
- No heap allocation in DMA callback (Phase 4+)
- No dynamic allocation in audio hot path — pre-allocate at boot

## What NOT to Do
- Do not use `delay()` in ISRs or WiFi/WebSocket callbacks
- Do not call `Serial.print()` inside ISRs
- Do not block `webSocket.loop()` or `server.handleClient()` with long operations
- Do not use GPIO 6–11
- Do not merge phase branches out of order

## Branching Strategy
- `main` — stable, tagged releases only
- Each phase has its own branch, branched from main after previous phase merges
- PRs go: `phase-N` → `main` when phase is fully working and tested
- Current branch: `phase-2-wifi-ap`

## Audio Sample Spec
- Format: WAV, PCM, uncompressed
- Sample rate: 22050 Hz
- Bit depth: 16-bit
- Channels: Mono
- Files: kick, snare, hihat_closed, hihat_open, tom_low, tom_mid, crash, ride
- Phase 2: base64-encoded inside web app HTML
- Phase 4+: raw WAV on SD card

## Testing Checklist by Phase

### Phase 0 — COMPLETE ✓
- [x] Web app connects to ESP32 serial port in Chrome
- [x] Typing `KICK` in Serial Monitor plays kick sound in browser
- [x] All 8 command strings trigger correct sounds
- [x] On-screen pads work without ESP32 (standalone test mode)

### Phase 1 — COMPLETE ✓
- [x] All 7 buttons register on press
- [x] No double-trigger at realistic press speed (10ms debounce)
- [x] Button → browser sound working end-to-end
- [x] Fast repeated hits all register cleanly

### Phase 2 (current)
- [ ] iPhone connects to "DrumKit-ESP32" WiFi hotspot
- [ ] Safari opens http://192.168.4.1 and loads web app
- [ ] WebSocket connects (status indicator shows connected)
- [ ] Press button → phone plays drum sound with no USB cable
- [ ] All 7 buttons trigger correct sounds on phone
- [ ] Latency feels acceptable (< 20ms perceived)
- [ ] No audio glitches after 5 min continuous play

### Phase 4+ (future)
- [ ] Button → audible sound in < 10ms (hard requirement)
- [ ] 4 simultaneous buttons all produce sound
- [ ] No clipping, pops, or crackling
- [ ] System stable after 30min continuous use
