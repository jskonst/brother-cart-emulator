// CK-35 / Stitch Pattern Cartridge read-only dumper.
// MIT for this sketch. Pin names come from seto/brother (CERN OHL 1.2):
// knitting-cart/brothercart.sch and the emulator rename in emulator/emulator.sch.
//
// This sketch never drives a write. Pin 31 (P31 / WERAM1) is not a GPIO:
// the board ties it to GND. Pin 34 (P34 / ~WERAM2) is held HIGH.
// Pin 40 (VMEM) is only measured through a divider. It is not powered.

static const uint8_t ADDR[15] = {
    22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36};
static const uint8_t DATA[8] = {37, 38, 39, 40, 41, 42, 43, 44};

static const uint8_t PIN_CEROM = 46;  // active low, edge pin 14
static const uint8_t PIN_OEROM = 47;  // active low, edge pin 20
static const uint8_t PIN_CSRAM = 48;  // active low, edge pin 30
static const uint8_t PIN_P18 = 49;    // emulator name ~OERAM1, edge pin 18
static const uint8_t PIN_P33 = 50;    // emulator name ~OERAM2, edge pin 33
static const uint8_t PIN_P34 = 51;    // emulator name ~WERAM2, edge pin 34, always HIGH
static const uint8_t PIN_P38 = 52;    // emulator name LRAM, edge pin 38, 74HC74 clock
static const uint8_t PIN_VMEM = A0;   // edge pin 40, sense only

static const uint16_t DUMP_LEN = 32768;

static void setAddr(uint16_t addr) {
  for (uint8_t i = 0; i < 15; i++) {
    digitalWrite(ADDR[i], (addr >> i) & 1);
  }
}

static uint8_t readData() {
  uint8_t value = 0;
  for (uint8_t i = 0; i < 8; i++) {
    if (digitalRead(DATA[i])) value |= (1 << i);
  }
  return value;
}

static void idleStrobes() {
  digitalWrite(PIN_CEROM, HIGH);
  digitalWrite(PIN_OEROM, HIGH);
  digitalWrite(PIN_CSRAM, HIGH);
  digitalWrite(PIN_P18, HIGH);
  digitalWrite(PIN_P33, HIGH);
  digitalWrite(PIN_P34, HIGH);
  digitalWrite(PIN_P38, LOW);
}

static void pulseLatch() {
  // brothercart.sch: 74HC74 clocks A13/A14 into A13RAM/A14RAM on rising P38.
  // That selects a RAM address. It is not a write while ~WERAM stays high.
  digitalWrite(PIN_P38, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_P38, HIGH);
  delayMicroseconds(2);
  digitalWrite(PIN_P38, LOW);
}

static uint8_t readRomByte(uint16_t addr) {
  setAddr(addr);
  digitalWrite(PIN_CSRAM, HIGH);
  digitalWrite(PIN_P18, HIGH);
  digitalWrite(PIN_P33, HIGH);
  digitalWrite(PIN_P34, HIGH);
  digitalWrite(PIN_P38, LOW);
  digitalWrite(PIN_CEROM, LOW);
  digitalWrite(PIN_OEROM, LOW);
  delayMicroseconds(5);
  uint8_t value = readData();
  digitalWrite(PIN_CEROM, HIGH);
  digitalWrite(PIN_OEROM, HIGH);
  return value;
}

static uint8_t readRamByte(uint16_t addr) {
  setAddr(addr);
  pulseLatch();
  digitalWrite(PIN_CEROM, HIGH);
  digitalWrite(PIN_OEROM, HIGH);
  // Hypothesis from brothercart.sch, not confirmed on B442076-1:
  // ~OERAM = P18 OR P33 OR an unwired gate input. Both low lets the RAM drive.
  digitalWrite(PIN_P18, LOW);
  digitalWrite(PIN_P33, LOW);
  digitalWrite(PIN_P34, HIGH);
  digitalWrite(PIN_CSRAM, LOW);
  delayMicroseconds(5);
  uint8_t value = readData();
  digitalWrite(PIN_CSRAM, HIGH);
  digitalWrite(PIN_P18, HIGH);
  digitalWrite(PIN_P33, HIGH);
  return value;
}

static void dumpRegion(const char* name, uint8_t (*reader)(uint16_t)) {
  Serial.print("BEGIN ");
  Serial.print(name);
  Serial.print(" ");
  Serial.println(DUMP_LEN);
  Serial.flush();
  for (uint32_t addr = 0; addr < DUMP_LEN; addr++) {
    Serial.write(reader((uint16_t)addr));
  }
  Serial.flush();
  Serial.print("END ");
  Serial.println(name);
  Serial.flush();
}

static float readVmem() {
  // 100k / 100k divider. 5 V at the pin reads about 2.5 V on A0.
  long sum = 0;
  for (uint8_t i = 0; i < 8; i++) sum += analogRead(PIN_VMEM);
  float volts = (sum / 8.0f) * (5.0f / 1023.0f) * 2.0f;
  return volts;
}

void setup() {
  for (uint8_t i = 0; i < 15; i++) {
    pinMode(ADDR[i], OUTPUT);
    digitalWrite(ADDR[i], LOW);
  }
  for (uint8_t i = 0; i < 8; i++) pinMode(DATA[i], INPUT);
  pinMode(PIN_CEROM, OUTPUT);
  pinMode(PIN_OEROM, OUTPUT);
  pinMode(PIN_CSRAM, OUTPUT);
  pinMode(PIN_P18, OUTPUT);
  pinMode(PIN_P33, OUTPUT);
  pinMode(PIN_P34, OUTPUT);
  pinMode(PIN_P38, OUTPUT);
  idleStrobes();

  Serial.begin(115200);
  analogReference(DEFAULT);
  delay(200);
  idleStrobes();

  Serial.println("CK35-DUMP 1");
  Serial.println("READ ONLY. Pin 31 tied to GND. Pin 34 held high. Pin 40 is sense only.");
  float vmem = readVmem();
  Serial.print("VMEM_EST_V ");
  Serial.println(vmem, 2);
  if (vmem < 2.0f) {
    Serial.println("WARN battery looks low or pin 40 is open. RAM may already be empty. Do not replace the cell to chase a dump.");
  }
  Serial.println("Send G to dump. Dump the cartridge before it goes back into a CK-35 or a PPD.");
}

void loop() {
  if (Serial.available() && Serial.read() == 'G') {
    while (Serial.available()) Serial.read();
    idleStrobes();
    dumpRegion("ROM", readRomByte);
    dumpRegion("RAM", readRamByte);
    idleStrobes();
    Serial.println("DONE");
  }
}
