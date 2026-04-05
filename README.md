# ESP32 Digital Drum Kit

A low-latency, polyphonic digital drum kit built on the ESP32-M1 DevKit. Press physical buttons to trigger drum sounds played back via I2S audio through a MAX98357A amplifier.

## Features
- 8 drum pads (Kick, Snare, Hi-Hat Closed/Open, Low Tom, Mid Tom, Crash, Ride)
- Polyphonic playback — up to 4–8 simultaneous voices
- < 10ms latency from button press to sound
- WAV samples stored on SD card or internal SPIFFS flash
- Optional OLED display, LED feedback, and multiple kit switching

## Hardware
| Component | Details |
|-----------|---------|
| MCU | ESP32-M1 DevKit |
| Amplifier | MAX98357A I2S |
| Speaker | 3W 4Ω/8Ω or 3.5mm jack |
| Input | 8 tactile push buttons |
| Storage | SD card (SPI) or SPIFFS |
| Display | 0.96" OLED I2C (optional) |

## Repository Structure
```
firmware/   ← ESP32 Arduino/PlatformIO source
web_app/    ← Chrome companion app (Web MIDI/Audio)
docs/       ← System requirements and project context
```

## Quick Start
1. Wire hardware per the GPIO map in `docs/system_requirements.md`
2. Convert drum samples to 22050Hz 16-bit mono WAV and copy to SD card
3. Open `firmware/` in VS Code with PlatformIO
4. Build and upload to ESP32
5. Press buttons to play

## Documentation
- [System Requirements](docs/system_requirements.md) — full hardware and firmware spec
- [Project Context](docs/project_context.md) — background, goals, decisions

## License
MIT
