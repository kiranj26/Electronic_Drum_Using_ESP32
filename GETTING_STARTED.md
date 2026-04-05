# Getting Started — Phase 0

Exact steps to go from zero to a working browser drum machine triggered by your ESP32 over USB.  
**No extra hardware needed — just your ESP32-M1 DevKit and a USB data cable.**

---

## What You Need Before Starting

| Item | Notes |
|------|-------|
| ESP32-M1 DevKit | Any standard ESP32 DevKit board |
| USB cable | **Must be a data cable** — charge-only cables will not work |
| Mac or Windows PC | These steps show Mac terminal commands |
| VS Code | Download from code.visualstudio.com if not installed |
| Chrome browser | Web Serial API is Chrome-only — Firefox and Safari won't work |

---

## Part 1 — Install PlatformIO in VS Code

> Do this once. Skip if you already have PlatformIO.

1. Open **VS Code**
2. Click the **Extensions** icon on the left sidebar (4-square icon)
3. Search **PlatformIO IDE**
4. Click **Install** — it's the one published by PlatformIO
5. Wait for it to finish. VS Code will prompt you to **Reload** — do it.
6. After reload you'll see a **house icon** in the left sidebar. That's PlatformIO.

---

## Part 2 — Clone or Download This Repository

If you have git:
```bash
git clone <repo-url>
cd Electronic_Drum_Using_ESP32
git checkout phase-0-mvp
```

Or download the ZIP from GitHub and unzip it.

---

## Part 3 — Open the Firmware Project

1. In VS Code: **File → Open Folder**
2. Navigate to and open this specific folder:
   ```
   Electronic_Drum_Using_ESP32/firmware/phase0/
   ```
   Open **that folder** — not the root repo folder.
3. VS Code detects `platformio.ini` and configures the project automatically.
4. First time: a progress bar runs at the bottom while it downloads the ESP32 toolchain. Wait ~1–2 minutes.

---

## Part 4 — Connect the ESP32 and Find Its Port

1. Plug your ESP32-M1 DevKit into your Mac with a USB cable
2. Open Terminal in VS Code: **Terminal → New Terminal**
3. Run:
   ```bash
   ls /dev/cu.*
   ```
4. You should see a new entry like `/dev/cu.usbserial-XXXX` or `/dev/cu.SLAB_USBtoUART`

> **Nothing new appeared?** Your cable is charge-only. Try every cable you have — only data cables work.

---

## Part 5 — Set Your Port in platformio.ini

Open `firmware/phase0/platformio.ini` and update the port to match what you saw:

```ini
upload_port  = /dev/cu.usbserial-XXXX   ← replace with your port
monitor_port = /dev/cu.usbserial-XXXX   ← same value
```

Save the file.

---

## Part 6 — Flash the Firmware

1. Look at the **blue status bar** at the very bottom of VS Code
2. Click the **→ (Upload)** button
   - Or: `Cmd+Shift+P` → type `PlatformIO: Upload` → Enter
3. Watch the terminal. It ends with:
   ```
   ==== [SUCCESS] ====
   ```

> **Upload failed?** Make sure the port in `platformio.ini` is correct and the cable is plugged in.

---

## Part 7 — Verify the Firmware with Serial Monitor

1. In the blue status bar click the **plug icon** (Serial Monitor)
   - Or: `Cmd+Shift+P` → `PlatformIO: Serial Monitor`
2. You should see:
   ```
   # ESP32 Drum Kit — Phase 0 ready
   # Keys: 1=KICK  2=SNARE  3=HIHAT_CLOSED  4=HIHAT_OPEN
   #       5=TOM_LOW  6=TOM_MID  7=CRASH  8=RIDE
   ```
3. Type `1` and press Enter → you should see `KICK` printed back
4. Test a few more: `2` → `SNARE`, `7` → `CRASH`, etc.

> **Nothing appears?** Press the **EN/RST** button on the ESP32 to reboot it.

---

## Part 8 — Get Drum WAV Samples

You need 8 WAV files. Get them free from sampleswap.org:

1. Go to **sampleswap.org → Samples → DRUMS (SINGLE HITS)**
2. Download one file from each category:
   - **Kicks** → rename to `kick.wav`
   - **Snares** → rename to `snare.wav`
   - **Hats** → rename to `hihat_closed.wav`
   - **Hats** (different one) → rename to `hihat_open.wav`
   - **Toms** → rename to `tom_low.wav`
   - **Toms** (different one) → rename to `tom_mid.wav`
   - **Crashes** → rename to `crash.wav`
   - **Rides** → rename to `ride.wav`

3. Place all 8 files here (exact filenames matter):
   ```
   web_app/phase0/samples/kick.wav
   web_app/phase0/samples/snare.wav
   web_app/phase0/samples/hihat_closed.wav
   web_app/phase0/samples/hihat_open.wav
   web_app/phase0/samples/tom_low.wav
   web_app/phase0/samples/tom_mid.wav
   web_app/phase0/samples/crash.wav
   web_app/phase0/samples/ride.wav
   ```

> Any WAV format works. If you want to convert to 22050Hz 16-bit mono: use Audacity → Tracks → Stereo to Mono → Tracks → Resample 22050Hz → File → Export as WAV 16-bit.

---

## Part 9 — Start the Local Web Server

The web app must be served over HTTP (not opened as a file) so Chrome allows audio and serial access.

1. Open a **new terminal** (not the PlatformIO Serial Monitor one)
2. Run:
   ```bash
   cd /path/to/Electronic_Drum_Using_ESP32/web_app/phase0
   python3 -m http.server 8080
   ```
3. You should see:
   ```
   Serving HTTP on :: port 8080 (http://[::]:8080/) ...
   ```

Leave this terminal running.

---

## Part 10 — Open the Web App in Chrome

1. Open **Chrome** (not Firefox, not Safari)
2. Go to:
   ```
   http://localhost:8080
   ```
3. You'll see the drum pad grid with 8 color-coded pads

---

## Part 11 — Connect Chrome to the ESP32

> **Important:** Close the PlatformIO Serial Monitor first — Chrome and Serial Monitor cannot share the same port.

1. In Chrome, click **"Connect to ESP32"**
2. A popup lists available serial ports — select your ESP32 port (e.g. `usbserial-3110`)
3. Click **Connect**
4. The status dot turns **green**

---

## Part 12 — Play Drums

**Browser only (no ESP32 needed):**
- Click pads with your mouse
- Press keyboard keys: `1` `2` `3` `4` `5` `6` `7` `8`

**Full end-to-end (ESP32 → USB → Chrome → sound):**
1. Make sure Chrome is connected (green dot)
2. Open PlatformIO Serial Monitor
3. Type `1`, press Enter
4. Hear kick drum play in Chrome

That's Phase 0 complete.

---

## Keyboard / Command Reference

| Key | Alias | Command | Sound |
|-----|-------|---------|-------|
| `1` | `k` | `KICK` | Kick drum |
| `2` | `s` | `SNARE` | Snare |
| `3` | `h` | `HIHAT_CLOSED` | Closed hi-hat |
| `4` | `H` | `HIHAT_OPEN` | Open hi-hat |
| `5` | `t` | `TOM_LOW` | Low tom |
| `6` | `T` | `TOM_MID` | Mid tom |
| `7` | `c` | `CRASH` | Crash cymbal |
| `8` | `r` | `RIDE` | Ride cymbal |

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| No port appears when ESP32 plugged in | Try a different cable — charge-only cables have no data wires |
| Upload fails with "No serial data received" | Wrong port in `platformio.ini` — check `ls /dev/cu.*` |
| Serial Monitor shows nothing | Press the **EN/RST** button on the ESP32 to reboot |
| Chrome shows no serial ports in popup | Close PlatformIO Serial Monitor first — two apps can't share one port |
| Pads are dim / greyed out | WAV files missing or wrong filename — check `web_app/phase0/samples/` |
| Click pad but no sound | Click anywhere on the page first — Chrome requires a user gesture to unlock audio |
| `python3: command not found` | Install Python from python.org |
| Web Serial API not available | You're not in Chrome — switch from Firefox or Safari |
