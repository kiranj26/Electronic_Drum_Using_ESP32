# ESP32 Digital Drum Kit — Project Context

## What This Project Is
A physical electronic drum kit using an ESP32-M1 DevKit as the brain. The user presses tactile push buttons wired to GPIO pins. Each button press triggers a WAV drum sample that is mixed polyphonically and output through an I2S amplifier (MAX98357A) to a speaker.

The goal is a real instrument — latency must stay under 10ms end-to-end, which is the threshold where human musicians perceive delay as "wrong."

## Why This Approach
- **ESP32** chosen for dual-core architecture: one core handles audio DMA, other handles button input. No scheduling conflicts.
- **I2S + MAX98357A** chosen over PWM audio for cleaner analog output and DMA support (zero CPU load for audio streaming).
- **SD card** preferred over internal SPIFFS for sample storage — gives room to expand sample library later without reflashing.
- **Interrupt-driven buttons** (not polled) to minimize input latency to <1ms.
- **Software debounce** preferred over hardware RC filter for flexibility and zero extra BOM cost.

## Current Status
- [ ] Phase 1: Hardware wiring + single WAV playback test
- [ ] Phase 2: Button input + debounce
- [ ] Phase 3: Polyphonic mixer (4+ voices)
- [ ] Phase 4: All 8 buttons + all 8 samples mapped
- [ ] Phase 5: OLED display + kit switching
- [ ] Phase 6: Enclosure build
- [ ] Phase 7: Latency testing + tuning

## Key Technical Decisions Made
| Decision | Choice | Reason |
|----------|--------|--------|
| Audio interface | I2S (not PWM, not DAC) | Clean audio, DMA support |
| Amplifier | MAX98357A | Cheap, easy, I2S native |
| Sample format | 22050Hz 16-bit mono WAV | Fits in SPIFFS, low CPU decode |
| Input method | ISR FALLING edge | Lowest latency |
| Debounce | Software (20–50ms) | No extra components |
| Polyphony | 32-bit accumulator mix | Prevents clipping on simultaneous hits |
| RTOS | FreeRTOS (built into ESP32) | Audio Core 0, Input Core 1 |

## Constraints
- Must work on ESP32-M1 DevKit (not ESP32-S2, S3, or C3 — those have different I2S and GPIO)
- Total RAM budget: 520KB SRAM. Voice buffers + WAV headers must fit.
- Target cost: ~$28–40 total BOM
- Target latency: < 10ms button press to audible sound (hard requirement)

## Drum Pad Layout (8 buttons)
```
[Crash]  [Ride]
[HH-O]  [HH-C]
[Tom-L] [Tom-M]
[Snare]  [Kick]
```

## Sample Files Required
Place on SD card root or SPIFFS root:
- `kick.wav`
- `snare.wav`
- `hihat_closed.wav`
- `hihat_open.wav`
- `tom_low.wav`
- `tom_mid.wav`
- `crash.wav`
- `ride.wav`

Format: WAV, PCM, 16-bit, mono, 22050 Hz. Convert with Audacity if needed.

## Pair Programming Notes (for Claude)
- The developer is building this from scratch — start with Phase 1 and work sequentially.
- Always check the GPIO pin map before suggesting wiring or code pin assignments.
- Keep ISRs minimal (`IRAM_ATTR`, flag-only, no Serial or I/O).
- The `Voice` struct is the core audio primitive — don't change its shape without considering the mixer.
- When writing firmware code, default to PlatformIO project structure unless told otherwise.
- Do not use `delay()` in any audio or input path.
