# ESP32 Digital Drum Kit — Full System Requirements
### Embedded Project Specification Document

---

## 1. PROJECT OVERVIEW

**Project Name:** ESP32 Digital Drum Kit  
**Platform:** ESP32-M1 DevKit  
**Interface:** Physical push buttons (switches)  
**Output:** Audio via I2S digital-to-analog pipeline  
**Goal:** A low-latency, polyphonic, button-triggered drum machine capable of real-time performance

---

## 2. SYSTEM ARCHITECTURE OVERVIEW

```
[Buttons / Switches]
        |
        v
[GPIO Input Layer] — Interrupt-driven or polled
        |
        v
[Debounce Logic] — Software debounce per pin
        |
        v
[Trigger Engine] — Maps button → drum channel
        |
        v
[Sample Playback Engine] — Polyphonic WAV mixing
        |
        v
[I2S Audio Output Driver] — DMA buffered
        |
        v
[MAX98357A I2S Amplifier]
        |
        v
[Speaker / 3.5mm Output]
```

---

## 3. HARDWARE REQUIREMENTS

### 3.1 Microcontroller
| Parameter | Specification |
|-----------|--------------|
| Board | ESP32-M1 DevKit |
| CPU | Dual-core Xtensa LX6 @ 240MHz |
| RAM | 520KB SRAM (internal) |
| Flash | 4MB (minimum) |
| GPIO | 30+ usable pins |
| Operating Voltage | 3.3V logic |

### 3.2 Input: Push Buttons / Switches
| Parameter | Specification |
|-----------|--------------|
| Type | Tactile momentary push button (SPST NO) |
| Count | 8 buttons minimum (expandable to 12) |
| Pull configuration | Internal pull-up (INPUT_PULLUP) on ESP32 GPIO |
| Logic | Active LOW (button press = LOW signal) |
| Recommended switch | 12x12mm or arcade-style 24mm for better feel |
| Debounce time | 20–50ms in software |

**Button Mapping (8-pad layout):**
1. Kick Drum (Bass Drum)
2. Snare Drum
3. Hi-Hat Closed
4. Hi-Hat Open
5. Low Tom
6. Mid Tom
7. Crash Cymbal
8. Ride Cymbal

### 3.3 Audio Output Hardware
| Component | Part | Purpose |
|-----------|------|---------|
| I2S Amplifier | MAX98357A | Converts I2S digital audio to analog amplified output |
| Speaker | 3W 4Ω or 8Ω | Audio output |
| OR 3.5mm jack | Stereo TRS | Line out to external speaker/headphones |
| Decoupling caps | 100nF + 10µF | Power filtering on amp VCC |

### 3.4 Storage: Audio Samples
| Option | Details | Pros | Cons |
|--------|---------|------|------|
| Internal Flash (SPIFFS / LittleFS) | ~2MB usable after firmware | No extra hardware | Limited sample count/quality |
| SD Card Module | SPI interface, FAT32 formatted | Unlimited samples, easy to swap | Extra wiring, SPI bus sharing |
| **Recommended** | **SD Card (SPI)** | Best flexibility | Minor added cost |

### 3.5 Optional / Expansion Hardware
| Component | Purpose |
|-----------|---------|
| 0.96" OLED (I2C, SSD1306) | Display current kit, BPM, mode |
| WS2812B LED strip | Visual feedback per hit |
| Rotary encoder | Volume/BPM control |
| Toggle switch | Kit selector (Kit A / Kit B) |
| 5V USB power bank | Portable power |

---

## 4. GPIO PIN ALLOCATION

| GPIO Pin | Function | Notes |
|----------|----------|-------|
| GPIO 4 | Button 1 — Kick | INPUT_PULLUP |
| GPIO 5 | Button 2 — Snare | INPUT_PULLUP |
| GPIO 12 | Button 3 — Hi-Hat Closed | INPUT_PULLUP |
| GPIO 13 | Button 4 — Hi-Hat Open | INPUT_PULLUP |
| GPIO 14 | Button 5 — Low Tom | INPUT_PULLUP |
| GPIO 15 | Button 6 — Mid Tom | INPUT_PULLUP |
| GPIO 18 | Button 7 — Crash | INPUT_PULLUP |
| GPIO 19 | Button 8 — Ride | INPUT_PULLUP |
| GPIO 25 | I2S — LRCLK (WS) | Audio word select |
| GPIO 26 | I2S — BCLK | Audio bit clock |
| GPIO 22 | I2S — DOUT (DIN on amp) | Audio data out |
| GPIO 23 | SD Card — MOSI | SPI |
| GPIO 21 | SD Card — MISO | SPI |
| GPIO 20 | SD Card — SCK | SPI |
| GPIO 16 | SD Card — CS | SPI chip select |
| GPIO 21 | OLED SDA (I2C) | Optional display |
| GPIO 22 | OLED SCL (I2C) | Optional display |
| GND | Common ground | Shared across all |
| 3.3V | Pull-up rail | Button pull-ups |
| VIN / 5V | MAX98357 VCC | Amp power |

> Avoid GPIO 6–11 (used internally by flash), GPIO 34–39 are input-only.

---

## 5. AUDIO SYSTEM REQUIREMENTS

### 5.1 Sample Format Specification
| Parameter | Requirement |
|-----------|------------|
| Format | WAV (PCM, uncompressed) |
| Sample Rate | 22050 Hz or 44100 Hz |
| Bit Depth | 16-bit |
| Channels | Mono (convert stereo to mono to save RAM) |
| File size per sample | 50KB – 200KB recommended |
| Total samples (8 drums) | ~1–2MB for 22050Hz, ~2–4MB for 44100Hz |

### 5.2 I2S Configuration
| Parameter | Value |
|-----------|-------|
| I2S Port | I2S_NUM_0 |
| Mode | Master TX |
| Sample Rate | 22050 or 44100 Hz |
| Bits per sample | 16 |
| DMA buffer count | 8 |
| DMA buffer length | 512 samples |
| Channel format | Mono (right channel only for MAX98357) |

### 5.3 Polyphony Requirements
- **Minimum 4-voice polyphony** — pressing kick + snare + hi-hat simultaneously must all play
- Voices are mixed in software before sending to I2S buffer
- Each active voice tracks: sample pointer, remaining length, volume
- Mixing is done as 32-bit integer accumulation, then scaled back to 16-bit to prevent clipping

### 5.4 Latency Budget
| Stage | Target Latency |
|-------|---------------|
| Button press detected | < 1ms (interrupt-driven) |
| Sample load begins | < 2ms |
| First audio sample to I2S | < 5ms |
| Audible sound from speaker | < 10ms total |

> 10ms is the threshold for perceptible latency in drum performance. Must stay under this.

---

## 6. FIRMWARE / SOFTWARE REQUIREMENTS

### 6.1 Development Environment
| Tool | Specification |
|------|--------------|
| IDE | Arduino IDE 2.x or VS Code + PlatformIO |
| Framework | Arduino-ESP32 (espressif/arduino-esp32) |
| Language | C++ (Arduino framework) |
| Board definition | ESP32 Dev Module |
| Upload speed | 921600 baud |
| Flash frequency | 80MHz |
| Partition scheme | Default 4MB with SPIFFS |

### 6.2 Required Libraries
| Library | Purpose | Source |
|---------|---------|--------|
| Arduino-ESP32 I2S | I2S audio driver | Built into ESP32 Arduino core |
| SD.h | SD card file access | Built-in Arduino |
| SPIFFS.h | Internal flash filesystem | Built-in ESP32 Arduino core |
| Wire.h | I2C for OLED | Built-in |
| Adafruit SSD1306 | OLED display driver | Arduino Library Manager |
| ESP32-audioI2S | High-level audio streaming (optional) | GitHub: schreibfaul1 |

### 6.3 Core Software Modules

**Module 1: Button Input Manager**
- Initializes all GPIO pins as INPUT_PULLUP
- Uses interrupt service routines (ISR) on FALLING edge
- ISR sets a flag in a volatile bitmask
- Main loop reads and clears the bitmask each cycle
- Software debounce: timestamp per button, minimum 20ms between triggers

**Module 2: Debounce Engine**
- Per-button last-trigger timestamp stored in array
- On interrupt: check `millis() - lastTrigger[btn] > DEBOUNCE_MS`
- If passed: set trigger flag, update timestamp
- If not passed: ignore (hardware bounce)

**Module 3: Sample Manager**
- On boot: loads all WAV file headers from SD card into RAM
- Stores: filename, sample rate, bit depth, data offset, total length
- On trigger: opens file, reads raw PCM data in chunks into active voice buffer
- Manages 4–8 voice slots simultaneously

**Module 4: Polyphonic Mixer**
- Maintains array of active Voice structs:
  ```
  struct Voice {
    int16_t* buffer;
    uint32_t position;
    uint32_t length;
    float volume;
    bool active;
  }
  ```
- Each I2S DMA callback: loops through active voices, sums samples
- Clips output to int16 range (-32768 to 32767)
- Writes mixed buffer to I2S TX

**Module 5: I2S Audio Driver**
- Configures ESP32 I2S peripheral
- Uses DMA (Direct Memory Access) for zero-CPU-load audio streaming
- Callback-based: fires when DMA buffer needs refill
- Double-buffered to prevent underruns

**Module 6: Kit Manager (Optional)**
- Stores multiple kit configurations (Rock, Electronic, Jazz)
- Each kit = different set of WAV file paths
- Toggle button cycles through kits
- OLED displays current kit name

### 6.4 RTOS Task Structure (FreeRTOS)
The ESP32 runs FreeRTOS under the hood. Recommended task layout:

| Task | Core | Priority | Purpose |
|------|------|----------|---------|
| AudioMixTask | Core 0 | HIGH (20) | Fills I2S DMA buffer |
| InputTask | Core 1 | HIGH (19) | Reads buttons, fires triggers |
| UITask | Core 1 | LOW (5) | Updates OLED display |
| IdleTask | Both | 0 | System idle |

> Separating audio on Core 0 and input on Core 1 gives maximum responsiveness.

---

## 7. AUDIO SAMPLE REQUIREMENTS

### 7.1 Sample Sourcing
- **Free sources:** freesound.org, sampleswap.org, splice.com (free tier)
- **Format needed:** WAV, 22050Hz, 16-bit mono
- **Conversion tool:** Audacity (free) — File > Export > WAV, resample to 22050Hz

### 7.2 Recommended Sample Set (8 files)
| Filename | Description | Target Size |
|----------|-------------|-------------|
| kick.wav | Deep bass kick drum | ~80KB |
| snare.wav | Sharp snare crack | ~60KB |
| hihat_closed.wav | Tight closed hi-hat | ~30KB |
| hihat_open.wav | Open sloshy hi-hat | ~80KB |
| tom_low.wav | Low floor tom | ~70KB |
| tom_mid.wav | Mid rack tom | ~60KB |
| crash.wav | Crash cymbal (full decay) | ~200KB |
| ride.wav | Ride cymbal ping | ~150KB |

**Total: ~730KB** — fits comfortably in internal SPIFFS flash.

---

## 8. POWER REQUIREMENTS

| Component | Current Draw | Voltage |
|-----------|-------------|---------|
| ESP32 (active Wi-Fi off) | ~80–150mA | 3.3V |
| MAX98357A (idle) | ~3mA | 5V |
| MAX98357A (full output) | ~600mA peak | 5V |
| OLED 0.96" | ~20mA | 3.3V |
| 8x Buttons (LEDs, if used) | ~160mA | 3.3V |
| **Total estimated** | **~350–500mA** | **5V USB** |

> Standard USB 5V 1A power supply is sufficient. USB power bank works for portability.

---

## 9. ENCLOSURE & PHYSICAL DESIGN

### 9.1 Layout Considerations
- Buttons arranged in arc or grid mimicking a real drum kit layout
- Kick button larger / different color (bass drum = most important)
- Color coding: Red = kick, White = snare, Yellow = hi-hat, Blue = toms, Orange = cymbals
- Speaker grille or 3.5mm jack on side panel

### 9.2 Enclosure Options
| Option | Cost | Difficulty |
|--------|------|-----------|
| 3D printed box | Low | Medium |
| Laser-cut acrylic | Medium | Low |
| Project box (Hammond) | Low | Easy |
| Wooden box (handmade) | Low | Medium |

---

## 10. TESTING REQUIREMENTS

| Test | Pass Criteria |
|------|--------------|
| Button detection | All 8 buttons register within 1ms of press |
| Debounce | No double-trigger at any realistic press speed |
| Latency | Audible sound within 10ms of button press |
| Polyphony | 4 simultaneous buttons all produce sound |
| Sample integrity | No clipping, no pops, no crackling |
| Continuous play | System stable after 30min continuous use |
| Power stability | No resets or brownouts during peak audio output |

---

## 11. DEVELOPMENT PHASES

| Phase | Tasks | Est. Time |
|-------|-------|-----------|
| Phase 1 | Hardware wiring, I2S test, play single WAV | 1–2 days |
| Phase 2 | Button input + debounce working | 1 day |
| Phase 3 | Polyphonic mixer — 4+ voices simultaneously | 2–3 days |
| Phase 4 | All 8 buttons + all samples mapped | 1 day |
| Phase 5 | OLED display + kit switching | 1–2 days |
| Phase 6 | Enclosure + physical build | 2–3 days |
| Phase 7 | Testing + tuning latency | 1–2 days |

---

## 12. RISKS & MITIGATIONS

| Risk | Likelihood | Mitigation |
|------|-----------|-----------|
| I2S audio glitches / crackling | Medium | Increase DMA buffer size, separate audio task to Core 0 |
| High latency (>10ms) | Low | Use ISR-based input, pre-load samples into RAM |
| Not enough RAM for polyphony | Medium | Use 22050Hz mono samples, stream from SD rather than full load |
| Button bounce causing double hits | High | Software debounce + optional 100nF cap across button |
| SD card SPI conflicts | Low | SD and I2S on separate buses, careful SPI timing |
| Clipping / distortion on loud hits | Medium | Normalize samples, scale mixer output by number of active voices |

---

## 13. BILL OF MATERIALS (BOM)

| Item | Qty | Est. Cost |
|------|-----|----------|
| ESP32-M1 DevKit | 1 | (you have this) |
| MAX98357A I2S Amp module | 1 | $3 |
| Push buttons (tactile) | 8–12 | $3 |
| SD Card module (SPI) | 1 | $2 |
| MicroSD card (4–8GB) | 1 | $5 |
| Speaker 3W 8Ω | 1 | $4 |
| OLED 0.96" I2C | 1 | $4 |
| Resistors, caps, wires | — | $2 |
| Project box / enclosure | 1 | $5–15 |
| **Total** | | **~$28–40** |

---

*Document Version: 1.0 | Project: ESP32 Digital Drum Kit | Platform: ESP32-M1 DevKit*
