#ifndef PATTERN_EDITOR_H
#define PATTERN_EDITOR_H

#include <Arduino.h>

// Wi-Fi grid editor for the CK-35 project.
// Stores the editor's own JSON in LittleFS. It does not write a cartridge image.
class PatternEditor {
 public:
  void begin();
  void loop();
};

extern PatternEditor patternEditor;

// TODO(CK-35 encoder): turn the editor JSON into a Brother Pattern Memory image.
// The byte layout (directory, block table, bitmap) is not known yet.
// Leave this empty until a read-only dump of a real CK-35 cartridge confirms it.
// Do not invent offsets, signatures, or colour packing here.
void todoEncodeCk35(const uint8_t* json, size_t len);

#endif
