# ESP32 Digital Drum Kit — Claude Code Pair Programming Guide

## TL;DR — What We're Building
A progressive drum kit project. We start with zero extra hardware (just ESP32 + USB cable + browser) and add hardware phase by phase until we have a standalone physical instrument.

**Current phase: Phase 0 — UART + Web Browser MVP**  
**Current branch: `phase-0-mvp`**

---

## Phase Overview

| Phase | Name | Branch | Hardware | Status |
|-------|------|--------|----------|--------|
| 0 | UART + Browser MVP | `phase-0-mvp` | ESP32 + USB only | In Progress |
| 1 | Physical Buttons → UART → Browser | `phase-1-buttons` | + 8 buttons | Not started |
| 2 | On-Device I2S Audio | `phase-2-i2s-audio` | + MAX98357A + SD card + speaker | Not started |
| 3 | Polyphony + FreeRTOS | `phase-3-polyphony` | Same as Phase 2 | Not started |
| 4 | OLED + Kit Switching | `phase-4-display` | + OLED | Not started |
| 5 | Enclosure + Final Build | `phase-5-enclosure` | Full BOM | Not started |

---

## Phase 0 — UART + Web Browser MVP (CURRENT)

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

### What the ESP32 firmware does (Phase 0)
- Listens on Serial (115200 baud)
- Receives single-character or keyword commands (e.g. `1`, `K`, or `KICK`)
- Echoes back a canonical drum command string: `KICK`, `SNARE`, `HIHAT_CLOSED`, etc.
- Later phases: firmware will also read GPIO buttons and send the same commands

### What the web app does (Phase 0)
- Runs in Chrome (Web Serial API is Chrome-only, no install needed)
- User clicks "Connect" → browser asks permission to open the serial port
- Reads incoming lines from ESP32
- Maps command strings → drum sound
- Plays sound via Web Audio API (AudioContext + AudioBuffer)
- Sounds are either: bundled base64 WAV blobs, or fetched from a local `/samples/` folder
- Shows a visual drum pad UI that lights up on hit

### Phase 0 File Layout
```
firmware/
└── phase0/
    └── phase0.ino          ← Arduino sketch (or main.cpp for PlatformIO)

web_app/
└── phase0/
    ├── index.html          ← Single HTML file, self-contained
    ├── app.js              ← Web Serial + Web Audio logic
    ├── styles.css          ← Drum pad UI
    └── samples/            ← WAV files (22050Hz 16-bit mono)
        ├── kick.wav
        ├── snare.wav
        ├── hihat_closed.wav
        ├── hihat_open.wav
        ├── tom_low.wav
        ├── tom_mid.wav
        ├── crash.wav
        └── ride.wav
```

### Command Protocol (Phase 0)
Simple newline-terminated strings over Serial at 115200 baud:

| Command sent by ESP32 | Drum sound triggered |
|-----------------------|---------------------|
| `KICK` | Kick drum |
| `SNARE` | Snare drum |
| `HIHAT_CLOSED` | Closed hi-hat |
| `HIHAT_OPEN` | Open hi-hat |
| `TOM_LOW` | Low tom |
| `TOM_MID` | Mid tom |
| `CRASH` | Crash cymbal |
| `RIDE` | Ride cymbal |

Keep commands uppercase, no spaces, newline-terminated (`\n`). The web app splits on `\n` and trims whitespace.

### Phase 0 — How to Run
1. Flash `firmware/phase0/phase0.ino` to ESP32 via Arduino IDE or PlatformIO
2. Open `web_app/phase0/index.html` in Chrome (must be Chrome — Web Serial is Chrome-only)
3. Click "Connect to ESP32" in the web app
4. Select the ESP32 COM port
5. Open Arduino Serial Monitor, type `KICK`, hit enter → hear kick drum in browser
6. Or press the on-screen drum pads to trigger sounds without ESP32 (standalone test mode)

---

## Phase 1 — Physical Buttons (FUTURE)

### What changes from Phase 0
- Firmware adds GPIO interrupt handlers for 8 buttons
- Each button maps to one drum command
- Software debounce: 30ms minimum between triggers
- Browser app unchanged — same command protocol

### GPIO Pin Map (Phase 1+)
| GPIO | Button | Command |
|------|--------|---------|
| 4 | Button 1 | `KICK` |
| 5 | Button 2 | `SNARE` |
| 12 | Button 3 | `HIHAT_CLOSED` |
| 13 | Button 4 | `HIHAT_OPEN` |
| 14 | Button 5 | `TOM_LOW` |
| 15 | Button 6 | `TOM_MID` |
| 18 | Button 7 | `CRASH` |
| 19 | Button 8 | `RIDE` |

> Avoid GPIO 6–11 (internal flash). GPIO 34–39 are input-only.

---

## Phase 2 — On-Device I2S Audio (FUTURE)

### What changes from Phase 1
- Add MAX98357A I2S amplifier (GPIO 25/26/22)
- Add SD card SPI module (GPIO 23/21/20/16)
- WAV files on SD card, streamed and mixed on ESP32
- Web app becomes optional — ESP32 plays audio standalone
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
- No heap allocation in DMA callback (Phase 2+)
- No dynamic allocation in audio hot path — pre-allocate at boot

## What NOT to Do
- Do not use `delay()` in ISRs or audio tasks
- Do not call `Serial.print()` inside ISRs
- Do not allocate heap inside DMA callback
- Do not use GPIO 6–11
- Do not block AudioMixTask with SD card reads (use a dedicated read task)
- Do not merge phase branches out of order — each phase builds on the previous

## Branching Strategy
- `main` — stable, tagged releases only
- `phase-0-mvp` — Phase 0 development (current)
- `phase-1-buttons` — branches from `phase-0-mvp` when Phase 0 is complete
- Each subsequent phase branches from the previous completed phase
- PRs go: `phase-N` → `main` when phase is fully working and tested

## Audio Sample Spec (Phase 0: sourced from web, Phase 2+: on SD card)
- Format: WAV, PCM, uncompressed
- Sample rate: 22050 Hz
- Bit depth: 16-bit
- Channels: Mono
- Files: kick, snare, hihat_closed, hihat_open, tom_low, tom_mid, crash, ride
- Free sources: freesound.org, sampleswap.org
- Conversion: Audacity → Export as WAV → resample to 22050Hz mono

## Testing Checklist by Phase

### Phase 0
- [ ] Web app connects to ESP32 serial port in Chrome
- [ ] Typing `KICK` in Serial Monitor plays kick sound in browser
- [ ] All 8 command strings trigger correct sounds
- [ ] On-screen pads work without ESP32 (standalone test mode)
- [ ] No audio glitches or latency > 200ms (browser is relaxed vs hardware)

### Phase 1 (future)
- [ ] All 8 buttons register within 1ms of press
- [ ] No double-trigger at realistic press speed (debounce working)
- [ ] Button → browser sound in < 50ms total

### Phase 2+ (future)
- [ ] Button → audible sound in < 10ms (hard requirement)
- [ ] 4 simultaneous buttons all produce sound
- [ ] No clipping, pops, or crackling
- [ ] System stable after 30min continuous use
