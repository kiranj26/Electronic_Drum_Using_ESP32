#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <driver/i2s.h>

// ============================================================
// ESP32 Digital Drum Kit — Phase 4b
// I2S Amp + Speaker Audio
//
// Builds on Phase 4a — same SD card WAV loading.
// Adds I2S output through MAX98357A amplifier to speaker.
// No phone, no laptop, no WiFi — fully standalone.
//
// Architecture:
//   Core 0 — AudioTask:  mixes active voices → writes I2S DMA
//   Core 1 — InputTask:  reads ISR flags → triggers voices
//   ISRs:                IRAM_ATTR, set volatile flags only
//
// SD wiring (unchanged from Phase 4a):
//   CS=GPIO5  MOSI=GPIO23  MISO=GPIO19  SCK=GPIO18
//
// I2S wiring (MAX98357A):
//   BCLK → GPIO 26
//   LRC  → GPIO 25
//   DIN  → GPIO 22
//   VIN  → ESP32 VIN (5V)
//   GND  → GND
//
// Button GPIO map (unchanged from Phase 4a):
//   KICK=4  SNARE=33  HIHAT_CLOSED=12  HIHAT_OPEN=13
//   TOM_LOW=14  TOM_MID=15  CRASH=32
// ============================================================

// ── SD card ───────────────────────────────────────────────────
#define SD_CS_PIN     5

// ── I2S config ────────────────────────────────────────────────
#define I2S_PORT          I2S_NUM_0
#define I2S_BCLK_PIN      26
#define I2S_LRC_PIN       25
#define I2S_DOUT_PIN      22
#define I2S_SAMPLE_RATE   22050
#define I2S_DMA_BUF_COUNT 8
#define I2S_DMA_BUF_LEN   512   // samples per DMA buffer

// ── Voice mixer ───────────────────────────────────────────────
#define MAX_VOICES  4

struct Voice {
  int16_t* buffer;      // pointer to WAV PCM data in heap
  uint32_t position;    // current playback sample index
  uint32_t length;      // total samples in this WAV
  bool     active;      // true = currently playing
};

static Voice voices[MAX_VOICES] = { {nullptr, 0, 0, false} };
static portMUX_TYPE voice_mux = portMUX_INITIALIZER_UNLOCKED;

// ── Button GPIO assignments ───────────────────────────────────
#define BTN_KICK          4
#define BTN_SNARE        33
#define BTN_HIHAT_CLOSED 12
#define BTN_HIHAT_OPEN   13
#define BTN_TOM_LOW      14
#define BTN_TOM_MID      15
#define BTN_CRASH        32

#define DEBOUNCE_MS   10
#define NUM_BUTTONS    7

// ── FreeRTOS task config ──────────────────────────────────────
#define AUDIO_TASK_CORE      0
#define INPUT_TASK_CORE      1
#define AUDIO_TASK_PRIORITY  19
#define INPUT_TASK_PRIORITY  20
#define AUDIO_TASK_STACK     4096
#define INPUT_TASK_STACK     2048

// ── WAV file list ─────────────────────────────────────────────
#define NUM_SAMPLES 8

struct WavSample {
  const char* filename;
  int16_t*    buffer;
  uint32_t    num_samples;
  bool        loaded;
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
  const uint8_t  pin;
  const uint8_t  sample_index;  // index into SAMPLES[]
};

static const Button BUTTONS[NUM_BUTTONS] = {
  { BTN_KICK,          0 },  // kick.wav
  { BTN_SNARE,         1 },  // snare.wav
  { BTN_HIHAT_CLOSED,  2 },  // hihat_closed.wav
  { BTN_HIHAT_OPEN,    3 },  // hihat_open.wav
  { BTN_TOM_LOW,       4 },  // tom_low.wav
  { BTN_TOM_MID,       5 },  // tom_mid.wav
  { BTN_CRASH,         6 },  // crash.wav
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

// ── Trigger a voice ───────────────────────────────────────────
// Called from InputTask when a button fires.
// Finds a free voice slot — if all full, steals slot 0 (oldest).
void trigger_voice(uint8_t sample_idx) {
  if (!SAMPLES[sample_idx].loaded) return;

  portENTER_CRITICAL(&voice_mux);

  // Find a free slot
  int8_t slot = -1;
  for (uint8_t i = 0; i < MAX_VOICES; i++) {
    if (!voices[i].active) { slot = i; break; }
  }
  // All slots busy — steal slot 0
  if (slot < 0) slot = 0;

  voices[slot].buffer   = SAMPLES[sample_idx].buffer;
  voices[slot].length   = SAMPLES[sample_idx].num_samples;
  voices[slot].position = 0;
  voices[slot].active   = true;

  portEXIT_CRITICAL(&voice_mux);
}

// ── WAV header parser ─────────────────────────────────────────
struct WavHeader {
  uint16_t audio_format;
  uint16_t num_channels;
  uint32_t sample_rate;
  uint16_t bits_per_sample;
  uint32_t data_size;
  bool     valid;
};

WavHeader parse_wav_header(File& f) {
  WavHeader hdr = { 0, 0, 0, 0, 0, false };
  uint8_t buf[44];
  if (f.read(buf, 44) != 44) return hdr;
  if (buf[0]!='R'||buf[1]!='I'||buf[2]!='F'||buf[3]!='F') return hdr;
  if (buf[8]!='W'||buf[9]!='A'||buf[10]!='V'||buf[11]!='E') return hdr;
  hdr.audio_format    = buf[20] | (buf[21] << 8);
  hdr.num_channels    = buf[22] | (buf[23] << 8);
  hdr.sample_rate     = buf[24]|(buf[25]<<8)|(buf[26]<<16)|(buf[27]<<24);
  hdr.bits_per_sample = buf[34] | (buf[35] << 8);
  hdr.data_size       = buf[40]|(buf[41]<<8)|(buf[42]<<16)|(buf[43]<<24);
  hdr.valid           = true;
  return hdr;
}

bool load_wav(WavSample& s) {
  File f = SD.open(s.filename);
  if (!f) {
    Serial.printf("  ERROR: cannot open %s\n", s.filename);
    return false;
  }
  WavHeader hdr = parse_wav_header(f);
  if (!hdr.valid || hdr.audio_format != 1 ||
      hdr.num_channels != 1 || hdr.sample_rate != 22050 ||
      hdr.bits_per_sample != 16) {
    Serial.printf("  ERROR: %s — bad format\n", s.filename);
    f.close();
    return false;
  }
  s.buffer = (int16_t*)malloc(hdr.data_size);
  if (!s.buffer) {
    Serial.printf("  ERROR: %s — malloc failed\n", s.filename);
    f.close();
    return false;
  }
  f.read((uint8_t*)s.buffer, hdr.data_size);
  f.close();
  s.num_samples = hdr.data_size / 2;
  s.loaded      = true;
  Serial.printf("  OK  : %-22s  %lu samples  (%lu KB)\n",
                s.filename, s.num_samples, hdr.data_size / 1024);
  return true;
}

// ── I2S init ──────────────────────────────────────────────────
void init_i2s() {
  i2s_config_t cfg = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate          = I2S_SAMPLE_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = I2S_DMA_BUF_COUNT,
    .dma_buf_len          = I2S_DMA_BUF_LEN,
    .use_apll             = false,
    .tx_desc_auto_clear   = true,
  };
  i2s_driver_install(I2S_PORT, &cfg, 0, NULL);

  i2s_pin_config_t pins = {
    .bck_io_num   = I2S_BCLK_PIN,
    .ws_io_num    = I2S_LRC_PIN,
    .data_out_num = I2S_DOUT_PIN,
    .data_in_num  = I2S_PIN_NO_CHANGE,
  };
  i2s_set_pin(I2S_PORT, &pins);
  i2s_zero_dma_buffer(I2S_PORT);
  Serial.printf("# I2S ready — BCLK=GPIO%d  LRC=GPIO%d  DOUT=GPIO%d\n",
                I2S_BCLK_PIN, I2S_LRC_PIN, I2S_DOUT_PIN);
}

// ── Core 0 — Audio Task ───────────────────────────────────────
// Runs continuously. Every iteration mixes all active voices
// into a DMA-sized buffer and writes it to I2S.
void audio_task(void* pv) {
  Serial.println("# AudioTask → Core 0");

  // Stereo buffer: I2S sends L+R pairs even in mono mode.
  // We duplicate each mono sample to both channels.
  static int32_t mix_buf[I2S_DMA_BUF_LEN];       // mix in 32-bit to avoid overflow
  static int16_t out_buf[I2S_DMA_BUF_LEN * 2];   // L+R interleaved for I2S

  while (true) {
    // Zero the mix buffer
    memset(mix_buf, 0, sizeof(mix_buf));

    portENTER_CRITICAL(&voice_mux);

    // Mix all active voices
    for (uint8_t v = 0; v < MAX_VOICES; v++) {
      if (!voices[v].active) continue;

      for (uint16_t s = 0; s < I2S_DMA_BUF_LEN; s++) {
        if (voices[v].position < voices[v].length) {
          mix_buf[s] += voices[v].buffer[voices[v].position++];
        } else {
          voices[v].active = false;
          break;
        }
      }
    }

    portEXIT_CRITICAL(&voice_mux);

    // Clip to int16 range and interleave L+R
    for (uint16_t s = 0; s < I2S_DMA_BUF_LEN; s++) {
      int32_t sample = mix_buf[s];
      if (sample >  32767) sample =  32767;
      if (sample < -32768) sample = -32768;
      out_buf[s * 2]     = (int16_t)sample;  // Left channel
      out_buf[s * 2 + 1] = (int16_t)sample;  // Right channel (same — mono)
    }

    // Write to I2S — blocks until DMA buffer accepts data
    size_t bytes_written = 0;
    i2s_write(I2S_PORT, out_buf, sizeof(out_buf), &bytes_written, portMAX_DELAY);
  }
}

// ── Core 1 — Input Task ───────────────────────────────────────
void input_task(void* pv) {
  Serial.println("# InputTask → Core 1");
  while (true) {
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
      if (trigger_flags[i]) {
        trigger_flags[i] = false;
        trigger_voice(BUTTONS[i].sample_index);
        Serial.printf("▶ %s\n",
          SAMPLES[BUTTONS[i].sample_index].filename);
      }
    }
    vTaskDelay(1);
  }
}

// ── Setup ─────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }
  Serial.println("# ESP32 Drum Kit — Phase 4b booting...");
  Serial.println("# Standalone audio: SD card + I2S + MAX98357A + speaker");
  Serial.println();

  // Mount SD card
  Serial.println("# Mounting SD card...");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("# ERROR: SD mount failed — check wiring");
    return;
  }
  Serial.printf("# SD mounted OK  —  card size: %lluMB\n",
                SD.cardSize() / (1024 * 1024));
  Serial.println();

  // Load WAV files
  Serial.println("# Loading WAV files...");
  uint8_t loaded = 0;
  for (uint8_t i = 0; i < NUM_SAMPLES; i++) {
    if (load_wav(SAMPLES[i])) loaded++;
  }
  Serial.printf("# WAV load result: %u / %u\n", loaded, NUM_SAMPLES);
  Serial.printf("# Free heap: %lu bytes\n", ESP.getFreeHeap());
  Serial.println();

  // Init I2S
  init_i2s();
  Serial.println();

  // Init buttons
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTONS[i].pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTONS[i].pin),
                    ISR_HANDLERS[i], FALLING);
  }
  Serial.println("# Buttons ready");
  Serial.println();

  // Launch tasks
  xTaskCreatePinnedToCore(
    audio_task, "AudioTask", AUDIO_TASK_STACK,
    NULL, AUDIO_TASK_PRIORITY, NULL, AUDIO_TASK_CORE
  );
  xTaskCreatePinnedToCore(
    input_task, "InputTask", INPUT_TASK_STACK,
    NULL, INPUT_TASK_PRIORITY, NULL, INPUT_TASK_CORE
  );

  Serial.println("# Tasks launched");
  Serial.println("# Press any button — sound plays through speaker");
  Serial.println("# ---");
}

// ── Loop — intentionally empty ────────────────────────────────
void loop() {
  vTaskDelay(portMAX_DELAY);
}
