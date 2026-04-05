# ESP32 Digital Drum Kit — Claude Code Guide

## Project Summary
An ESP32-based digital drum kit that maps 8 push buttons to drum sounds, plays polyphonic WAV samples via I2S audio output through a MAX98357A amplifier. This is a hardware + firmware project with an optional Chrome Web MIDI companion app.

## Repository Layout
```
Electronic_Drum_Using_ESP32/
├── CLAUDE.md                  ← You are here (Claude reads this automatically)
├── README.md                  ← Public project description
├── docs/
│   ├── system_requirements.md ← Full hardware + software spec
│   └── project_context.md     ← Goals, decisions, background context
├── firmware/                  ← ESP32 Arduino/PlatformIO source code
│   └── (source files go here)
└── web_app/                   ← Chrome drum companion app (Web MIDI / Web Audio)
    └── (source files go here)
```

## Hardware Platform
- **MCU:** ESP32-M1 DevKit (Dual-core Xtensa LX6 @ 240MHz, 520KB SRAM, 4MB Flash)
- **Audio output:** MAX98357A I2S amplifier → 3W speaker or 3.5mm jack
- **Input:** 8 tactile push buttons (active LOW, INPUT_PULLUP)
- **Storage:** SD card (SPI) for WAV samples, or internal SPIFFS fallback
- **Optional:** 0.96" OLED display (I2C), WS2812B LEDs, rotary encoder

## GPIO Pin Map
| GPIO | Function |
|------|----------|
| 4 | Button 1 — Kick |
| 5 | Button 2 — Snare |
| 12 | Button 3 — Hi-Hat Closed |
| 13 | Button 4 — Hi-Hat Open |
| 14 | Button 5 — Low Tom |
| 15 | Button 6 — Mid Tom |
| 18 | Button 7 — Crash |
| 19 | Button 8 — Ride |
| 25 | I2S LRCLK (WS) |
| 26 | I2S BCLK |
| 22 | I2S DOUT |
| 23 | SD MOSI |
| 21 | SD MISO |
| 20 | SD SCK |
| 16 | SD CS |

> Avoid GPIO 6–11 (internal flash), GPIO 34–39 are input-only.

## Firmware Architecture (6 modules)
1. **Button Input Manager** — ISR on FALLING edge, volatile bitmask, INPUT_PULLUP
2. **Debounce Engine** — per-button timestamp, 20–50ms minimum gap
3. **Sample Manager** — loads WAV headers on boot, streams PCM chunks on trigger
4. **Polyphonic Mixer** — 4–8 Voice structs, 32-bit accumulation → 16-bit clip, fills I2S DMA
5. **I2S Audio Driver** — DMA-buffered, double-buffered, callback-based refill
6. **Kit Manager** (optional) — multiple kit configs, OLED display, toggle button

## FreeRTOS Task Layout
| Task | Core | Priority |
|------|------|----------|
| AudioMixTask | Core 0 | 20 (HIGH) |
| InputTask | Core 1 | 19 (HIGH) |
| UITask | Core 1 | 5 (LOW) |

## Audio Spec
- Format: WAV, PCM, 16-bit mono, 22050 Hz
- Sample files: kick, snare, hihat_closed, hihat_open, tom_low, tom_mid, crash, ride
- Total size: ~730KB (fits in SPIFFS)
- I2S: I2S_NUM_0, Master TX, DMA 8 buffers × 512 samples

## Latency Target
Button press → audible sound in **< 10ms** total. This is hard real-time — do not compromise.

## Development Environment
- Framework: Arduino-ESP32 (espressif/arduino-esp32)
- IDE: VS Code + PlatformIO (preferred) or Arduino IDE 2.x
- Language: C++ (Arduino framework)
- Board: ESP32 Dev Module, Flash 80MHz, Upload 921600 baud
- Partition: Default 4MB with SPIFFS

## Key Libraries
- `Arduino-ESP32 I2S` — built-in
- `SD.h` — built-in
- `SPIFFS.h` — built-in
- `Wire.h` — built-in
- `Adafruit SSD1306` — Library Manager

## Development Phases
| Phase | Goal |
|-------|------|
| 1 | Hardware wiring, I2S test, play single WAV |
| 2 | Button input + debounce |
| 3 | Polyphonic mixer (4+ voices) |
| 4 | All 8 buttons + all samples |
| 5 | OLED display + kit switching |
| 6 | Enclosure build |
| 7 | Latency testing + tuning |

## Code Style Conventions
- Use `snake_case` for variables and functions
- Use `UPPER_SNAKE_CASE` for constants and `#define`
- Keep ISRs minimal — set flags only, never do I/O or Serial in an ISR
- Use `IRAM_ATTR` for all ISR functions
- Comment GPIO pin numbers in pin definitions, e.g. `#define BTN_KICK 4 // GPIO4`
- No dynamic allocation in audio hot path — pre-allocate voice buffers at boot

## What NOT to Do
- Do not use `delay()` in the audio path
- Do not call `Serial.print()` inside ISRs
- Do not allocate heap inside the DMA callback
- Do not use GPIO 6–11
- Do not block the AudioMixTask with SD card reads — use a separate read task

## Testing Checklist
- [ ] All 8 buttons register within 1ms
- [ ] No double-trigger at realistic press speed
- [ ] Audible sound < 10ms from press
- [ ] 4 simultaneous buttons all produce sound
- [ ] No clipping, pops, or crackling
- [ ] System stable after 30min continuous use
