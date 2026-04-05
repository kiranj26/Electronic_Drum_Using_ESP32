#include <Arduino.h>

// ============================================================
// ESP32 Digital Drum Kit — Phase 0
// Reads key presses from Serial Monitor, sends drum commands
// to the Chrome web app over USB (UART at 115200 baud).
// ============================================================

#define BAUD_RATE 115200

// Maps a single key character to a drum command string
struct DrumMapping {
  char        key;
  const char* command;
};

static const DrumMapping DRUM_MAP[] = {
  { '1', "KICK"         },
  { 'k', "KICK"         },
  { '2', "SNARE"        },
  { 's', "SNARE"        },
  { '3', "HIHAT_CLOSED" },
  { 'h', "HIHAT_CLOSED" },
  { '4', "HIHAT_OPEN"   },
  { 'H', "HIHAT_OPEN"   },
  { '5', "TOM_LOW"      },
  { 't', "TOM_LOW"      },
  { '6', "TOM_MID"      },
  { 'T', "TOM_MID"      },
  { '7', "CRASH"        },
  { 'c', "CRASH"        },
  { '8', "RIDE"         },
  { 'r', "RIDE"         },
};

static const uint8_t DRUM_MAP_SIZE = sizeof(DRUM_MAP) / sizeof(DRUM_MAP[0]);

// Returns true if the string is already a valid drum command
bool is_valid_command(const String& s) {
  return s == "KICK"         ||
         s == "SNARE"        ||
         s == "HIHAT_CLOSED" ||
         s == "HIHAT_OPEN"   ||
         s == "TOM_LOW"      ||
         s == "TOM_MID"      ||
         s == "CRASH"        ||
         s == "RIDE";
}

void setup() {
  Serial.begin(BAUD_RATE);
  while (!Serial) { ; }  // wait until Serial port is ready

  // Lines starting with # are ignored by the web app
  Serial.println("# ESP32 Drum Kit — Phase 0 ready");
  Serial.println("# Keys: 1=KICK  2=SNARE  3=HIHAT_CLOSED  4=HIHAT_OPEN");
  Serial.println("#       5=TOM_LOW  6=TOM_MID  7=CRASH  8=RIDE");
  Serial.println("# (aliases: k s h H t T c r)");
}

void loop() {
  if (!Serial.available()) return;

  String input = Serial.readStringUntil('\n');
  input.trim();

  if (input.length() == 0) return;

  // Accept full command strings typed directly (e.g. KICK)
  if (is_valid_command(input)) {
    Serial.println(input);
    return;
  }

  // Accept single-character shortcuts
  if (input.length() == 1) {
    char key = input.charAt(0);
    for (uint8_t i = 0; i < DRUM_MAP_SIZE; i++) {
      if (DRUM_MAP[i].key == key) {
        Serial.println(DRUM_MAP[i].command);
        return;
      }
    }
  }

  // Unknown input — echo as a comment so it shows in the log but doesn't trigger a sound
  Serial.print("# unknown input: ");
  Serial.println(input);
}
