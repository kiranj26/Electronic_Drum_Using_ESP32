#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

// ============================================================
// ESP32 Digital Drum Kit — Phase 4a
// SD Card WAV Loading
//
// Goal: prove that all 8 drum WAV files can be read from an
// Adafruit microSD breakout over SPI and decoded into memory
// at boot. No audio output — purely validates the data pipeline.
//
// What changed from Phase 3:
//   - SNARE remapped GPIO 5  → GPIO 33  (GPIO 5  = SD CS)
//   - CRASH remapped GPIO 18 → GPIO 32  (GPIO 18 = SPI SCK)
//   - SD card mounted on boot via SD.h
//   - All 8 WAV files opened, header parsed, PCM data loaded
//   - Button presses confirm correct GPIO remap in Serial Monitor
//
// SD wiring (Adafruit MicroSD Breakout+):
//   CS   → GPIO  5
//   DI   → GPIO 23  (MOSI)
//   DO   → GPIO 19  (MISO)
//   CLK  → GPIO 18  (SCK)
//   3V   → 3V3
//   GND  → GND
//
// SD card: FAT32, WAV files in root directory
// WAV format: 22050Hz, 16-bit, mono, PCM uncompressed
// ============================================================

// ── SD card ──────────────────────────────────────────────────
#define SD_CS_PIN     5   // Chip select

// ── Button GPIO assignments ───────────────────────────────────
#define BTN_KICK          4   // GPIO4  — unchanged
#define BTN_SNARE        33   // GPIO33 — remapped from 5 (freed for SD CS)
#define BTN_HIHAT_CLOSED 12   // GPIO12 — unchanged
#define BTN_HIHAT_OPEN   13   // GPIO13 — unchanged
#define BTN_TOM_LOW      14   // GPIO14 — unchanged
#define BTN_TOM_MID      15   // GPIO15 — unchanged
#define BTN_CRASH        32   // GPIO32 — remapped from 18 (freed for SPI SCK)

#define DEBOUNCE_MS   10
#define NUM_BUTTONS    7

// ── FreeRTOS task config ──────────────────────────────────────
#define INPUT_TASK_CORE      1
#define INPUT_TASK_PRIORITY 20
#define INPUT_TASK_STACK   2048

// ── WAV file list ─────────────────────────────────────────────
#define NUM_SAMPLES 8

struct WavSample {
  const char* filename;   // path on SD card
  int16_t*    buffer;     // PCM data loaded into heap
  uint32_t    num_samples;// number of int16_t samples
  bool        loaded;     // true if successfully loaded at boot
};

static WavSample SAMPLES[NUM_SAMPLES] = {
  { "/kick.wav",         nullptr, 0, false },
  { "/snare.wav",        nullptr, 0, false },
  { "/hihat_closed.wav", nullptr, 0, false },
  { "/hihat_open.wav",   nullptr, 0, false },
  { "/tom_low.wav",      nullptr, 0, false },
  { "/tom_mid.wav",      nullptr, 0, false },
  { "/crash.wav",        nullptr, 0, false },
  { "/ride.wav",         nullptr, 0, false },
};

// ── Button definitions ────────────────────────────────────────
struct Button {
  const uint8_t pin;
  const char*   command;
};

static const Button BUTTONS[NUM_BUTTONS] = {
  { BTN_KICK,          "KICK"         },
  { BTN_SNARE,         "SNARE"        },
  { BTN_HIHAT_CLOSED,  "HIHAT_CLOSED" },
  { BTN_HIHAT_OPEN,    "HIHAT_OPEN"   },
  { BTN_TOM_LOW,       "TOM_LOW"      },
  { BTN_TOM_MID,       "TOM_MID"      },
  { BTN_CRASH,         "CRASH"        },
};

// ── ISR state ─────────────────────────────────────────────────
volatile bool     trigger_flags[NUM_BUTTONS]   = { false };
volatile uint32_t last_trigger_ms[NUM_BUTTONS] = { 0 };

// ── ISRs ──────────────────────────────────────────────────────
#define MAKE_ISR(idx)                                         \
  void IRAM_ATTR isr_btn_##idx() {                            \
    uint32_t now = millis();                                  \
    if (now - last_trigger_ms[idx] >= DEBOUNCE_MS) {         \
      last_trigger_ms[idx] = now;                             \
      trigger_flags[idx] = true;                              \
    }                                                         \
  }

MAKE_ISR(0) MAKE_ISR(1) MAKE_ISR(2) MAKE_ISR(3)
MAKE_ISR(4) MAKE_ISR(5) MAKE_ISR(6)

static void (*ISR_HANDLERS[NUM_BUTTONS])() = {
  isr_btn_0, isr_btn_1, isr_btn_2, isr_btn_3,
  isr_btn_4, isr_btn_5, isr_btn_6,
};

// ── WAV header parser ─────────────────────────────────────────
// Standard PCM WAV layout (44 bytes):
//   Offset  0: "RIFF"
//   Offset  4: file size - 8
//   Offset  8: "WAVE"
//   Offset 12: "fmt "
//   Offset 16: fmt chunk size (16 for PCM)
//   Offset 20: audio format (1 = PCM)
//   Offset 22: num channels
//   Offset 24: sample rate
//   Offset 28: byte rate
//   Offset 32: block align
//   Offset 34: bits per sample
//   Offset 36: "data"
//   Offset 40: data chunk size (num bytes of PCM)
//   Offset 44: PCM data begins
struct WavHeader {
  uint16_t audio_format;   // 1 = PCM
  uint16_t num_channels;   // 1 = mono
  uint32_t sample_rate;    // 22050
  uint16_t bits_per_sample;// 16
  uint32_t data_size;      // bytes of PCM data
  uint32_t data_offset;    // byte offset where PCM starts
  bool     valid;
};

WavHeader parse_wav_header(File& f) {
  WavHeader hdr = { 0, 0, 0, 0, 0, 44, false };

  uint8_t buf[44];
  if (f.read(buf, 44) != 44) return hdr;

  // Validate RIFF + WAVE magic bytes
  if (buf[0]!='R'||buf[1]!='I'||buf[2]!='F'||buf[3]!='F') return hdr;
  if (buf[8]!='W'||buf[9]!='A'||buf[10]!='V'||buf[11]!='E') return hdr;

  hdr.audio_format    = buf[20] | (buf[21] << 8);
  hdr.num_channels    = buf[22] | (buf[23] << 8);
  hdr.sample_rate     = buf[24] | (buf[25]<<8) | (buf[26]<<16) | (buf[27]<<24);
  hdr.bits_per_sample = buf[34] | (buf[35] << 8);
  hdr.data_size       = buf[40] | (buf[41]<<8) | (buf[42]<<16) | (buf[43]<<24);
  hdr.valid           = true;
  return hdr;
}

// ── Load a single WAV file from SD into heap ──────────────────
bool load_wav(WavSample& s) {
  File f = SD.open(s.filename);
  if (!f) {
    Serial.printf("  ERROR: cannot open %s\n", s.filename);
    return false;
  }

  WavHeader hdr = parse_wav_header(f);
  if (!hdr.valid) {
    Serial.printf("  ERROR: %s — invalid WAV header\n", s.filename);
    f.close();
    return false;
  }

  // Validate format
  if (hdr.audio_format != 1) {
    Serial.printf("  ERROR: %s — not PCM (format=%u)\n",
                  s.filename, hdr.audio_format);
    f.close();
    return false;
  }
  if (hdr.num_channels != 1) {
    Serial.printf("  ERROR: %s — not mono (%u channels)\n",
                  s.filename, hdr.num_channels);
    f.close();
    return false;
  }
  if (hdr.sample_rate != 22050) {
    Serial.printf("  ERROR: %s — wrong sample rate (%luHz, need 22050)\n",
                  s.filename, hdr.sample_rate);
    f.close();
    return false;
  }
  if (hdr.bits_per_sample != 16) {
    Serial.printf("  ERROR: %s — not 16-bit (%u-bit)\n",
                  s.filename, hdr.bits_per_sample);
    f.close();
    return false;
  }

  // Allocate heap buffer and read PCM data
  uint32_t num_samples = hdr.data_size / 2;  // 2 bytes per int16_t
  s.buffer = (int16_t*)malloc(hdr.data_size);
  if (!s.buffer) {
    Serial.printf("  ERROR: %s — malloc failed (%lu bytes)\n",
                  s.filename, hdr.data_size);
    f.close();
    return false;
  }

  size_t bytes_read = f.read((uint8_t*)s.buffer, hdr.data_size);
  f.close();

  if (bytes_read != hdr.data_size) {
    Serial.printf("  ERROR: %s — short read (%u of %lu bytes)\n",
                  s.filename, bytes_read, hdr.data_size);
    free(s.buffer);
    s.buffer = nullptr;
    return false;
  }

  s.num_samples = num_samples;
  s.loaded      = true;

  Serial.printf("  OK  : %-22s  22050Hz 16-bit mono  %lu samples  (%lu KB)\n",
                s.filename, num_samples, hdr.data_size / 1024);
  return true;
}

// ── Core 1 — Input Task ───────────────────────────────────────
void input_task(void* pv) {
  Serial.println("# InputTask → Core 1");
  while (true) {
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
      if (trigger_flags[i]) {
        trigger_flags[i] = false;
        Serial.printf("▶ %s\n", BUTTONS[i].command);
      }
    }
    vTaskDelay(1);
  }
}

// ── Setup ─────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }

  Serial.println("# ESP32 Drum Kit — Phase 4a booting...");
  Serial.println("# Goal: SD card mount + WAV file load verification");
  Serial.println();

  // ── Mount SD card ─────────────────────────────────────────
  Serial.println("# Mounting SD card...");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("# ERROR: SD mount failed!");
    Serial.println("#   Check wiring: CS=GPIO5 MOSI=GPIO23 MISO=GPIO19 SCK=GPIO18");
    Serial.println("#   Check card is inserted and FAT32 formatted");
    return;
  }

  uint64_t card_size_mb = SD.cardSize() / (1024 * 1024);
  Serial.printf("# SD mounted OK  —  card size: %lluMB\n", card_size_mb);
  Serial.println();

  // ── Load all WAV files ────────────────────────────────────
  Serial.println("# Loading WAV files from SD card...");
  uint8_t loaded_count = 0;
  for (uint8_t i = 0; i < NUM_SAMPLES; i++) {
    if (load_wav(SAMPLES[i])) loaded_count++;
  }

  Serial.println();
  Serial.printf("# WAV load result: %u / %u files loaded\n",
                loaded_count, NUM_SAMPLES);

  // ── Heap report ────────────────────────────────────────────
  Serial.printf("# Free heap after load: %lu bytes\n", ESP.getFreeHeap());
  Serial.println();

  if (loaded_count == NUM_SAMPLES) {
    Serial.println("# All samples loaded — SD pipeline verified!");
  } else {
    Serial.printf("# WARNING: %u file(s) failed to load\n",
                  NUM_SAMPLES - loaded_count);
  }

  // ── Init buttons ──────────────────────────────────────────
  Serial.println();
  Serial.println("# Initialising buttons...");
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTONS[i].pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTONS[i].pin),
                    ISR_HANDLERS[i], FALLING);
  }
  Serial.println("# Buttons ready");
  Serial.println("#   SNARE → GPIO33  (remapped from GPIO5)");
  Serial.println("#   CRASH → GPIO32  (remapped from GPIO18)");
  Serial.println();

  // ── Launch input task ─────────────────────────────────────
  xTaskCreatePinnedToCore(
    input_task,
    "InputTask",
    INPUT_TASK_STACK,
    NULL,
    INPUT_TASK_PRIORITY,
    NULL,
    INPUT_TASK_CORE
  );

  Serial.println("# Ready — press buttons to verify GPIO remap");
  Serial.println("# ---");
}

// ── Loop — intentionally empty ────────────────────────────────
void loop() {
  vTaskDelay(portMAX_DELAY);
}
