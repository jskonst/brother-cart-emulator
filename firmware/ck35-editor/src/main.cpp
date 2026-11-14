#include <Arduino.h>

#include "pattern_editor.h"

// CK-35 pattern editor only. This firmware does not drive a cartridge bus
// and does not program the embroidery NOR flash in legacy/code-esp32.
void setup() {
  Serial.begin(115200);
  patternEditor.begin();
}

void loop() {
  patternEditor.loop();
  delay(1);
}
