#include <Arduino.h>

// ============================================================
// ESP32 Digital Drum Kit — Phase 1
// Physical buttons → UART → Chrome Web Audio
//
// 7 buttons wired to GPIO pins (INPUT_PULLUP, active LOW).
// Each button press sends a drum command string over Serial
// at 115200 baud to the Chrome web app.
//
// ISR sets a flag — main loop reads flags and sends commands.
// Debounce: 30ms minimum between triggers per button.
//
// Wiring (each button):
//   GPIO pin → button left pin
//   button right pin → GND
//   No resistors needed (INPUT_PULLUP handles it)
//
// GPIO map:
//   GPIO 4  → KICK
//   GPIO 5  → SNARE
//   GPIO 12 → HIHAT_CLOSED
//   GPIO 13 → HIHAT_OPEN
//   GPIO 14 → TOM_LOW
//   GPIO 15 → TOM_MID
//   GPIO 18 → CRASH
// ============================================================

#define BAUD_RATE    115200
#define DEBOUNCE_MS  10
#define NUM_BUTTONS  7

// ── Pin and command definitions ──────────────────────────────
struct Button {
  const uint8_t  pin;
  const char*    command;
};

static const Button BUTTONS[NUM_BUTTONS] = {
  {  4, "KICK"         },  // GPIO4
  {  5, "SNARE"        },  // GPIO5
  { 12, "HIHAT_CLOSED" },  // GPIO12
  { 13, "HIHAT_OPEN"   },  // GPIO13
  { 14, "TOM_LOW"      },  // GPIO14
  { 15, "TOM_MID"      },  // GPIO15
  { 18, "CRASH"        },  // GPIO18
};

// ── State ────────────────────────────────────────────────────
// volatile because written in ISR, read in main loop
volatile bool trigger_flags[NUM_BUTTONS] = { false };
volatile uint32_t last_trigger_ms[NUM_BUTTONS] = { 0 };

// ── ISRs (one per button) ─────────────────────────────────────
// ISRs must be IRAM_ATTR — they run from IRAM, not flash
// Keep them minimal: just check debounce and set flag

// Macro to generate ISR for each button index
#define MAKE_ISR(idx)                                          \
  void IRAM_ATTR isr_btn_##idx() {                             \
    uint32_t now = millis();                                   \
    if (now - last_trigger_ms[idx] >= DEBOUNCE_MS) {          \
      last_trigger_ms[idx] = now;                              \
      trigger_flags[idx] = true;                               \
    }                                                          \
  }

MAKE_ISR(0)
MAKE_ISR(1)
MAKE_ISR(2)
MAKE_ISR(3)
MAKE_ISR(4)
MAKE_ISR(5)
MAKE_ISR(6)

// Array of ISR function pointers for easy registration
static void (*ISR_HANDLERS[NUM_BUTTONS])() = {
  isr_btn_0, isr_btn_1, isr_btn_2, isr_btn_3,
  isr_btn_4, isr_btn_5, isr_btn_6,
};

// ── Setup ─────────────────────────────────────────────────────
void setup() {
  Serial.begin(BAUD_RATE);
  while (!Serial) { ; }

  // Init each button pin and attach ISR on FALLING edge
  // FALLING = button press (pin goes HIGH → LOW with INPUT_PULLUP)
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTONS[i].pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTONS[i].pin),
                    ISR_HANDLERS[i],
                    FALLING);
  }

  Serial.println("# ESP32 Drum Kit — Phase 1 ready");
  Serial.println("# Press a button to trigger a drum sound");
  Serial.println("# GPIO4=KICK GPIO5=SNARE GPIO12=HIHAT_CLOSED");
  Serial.println("# GPIO13=HIHAT_OPEN GPIO14=TOM_LOW GPIO15=TOM_MID GPIO18=CRASH");
}

// ── Main loop ─────────────────────────────────────────────────
void loop() {
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    if (trigger_flags[i]) {
      trigger_flags[i] = false;           // clear flag first
      Serial.println(BUTTONS[i].command); // send command to Chrome
    }
  }
}
