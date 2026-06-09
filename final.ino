#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10
#define RST_PIN 9
#define BUZZER_PIN 5

MFRC522 rfid(SS_PIN, RST_PIN);

// ============================
// 74HC595 PINS
// ============================
const int DATA_PIN  = 2;
const int LATCH_PIN = 3;
const int CLOCK_PIN = 4;

// ============================
// SYSTEM MODES
// ============================
#define MODE_NORMAL  0
#define MODE_FUNCTION 1
#define MODE_ADMIN    2

byte systemMode = MODE_NORMAL;

// ============================
// RELAY STATE (ACTIVE LOW)
// ============================
uint16_t outputs = 0xFFFF;

// ============================
// FLOOR TIMER
// ============================
unsigned long floorTimer = 0;
byte activeFloor = 0;
bool floorActive = false;

// ============================
// SPECIAL CARDS
// ============================
byte installCard[4] = {0x7B, 0x44, 0x29, 0xD5}; // FUNCTION CARD

byte adminCard[4]   = {0xAA, 0xBB, 0xCC, 0xDD}; // CHANGE THIS!

// ============================
// SERVICE CARDS
// ============================
byte serviceCards[][4] = {
  {0x47,0xC6,0xBE,0xAD},
  {0xE7,0xD3,0xB0,0xAD},
  {0x97,0xE2,0x75,0xAD},
  {0xF7,0xEE,0xB0,0xAD}
};

// ============================
// FLOOR DATABASE (unchanged)
// ============================
byte floor1[][4] = {{0xD7,0x6D,0x85,0xAD},{0x67,0x0B,0xD4,0xAD}};
byte floor2[][4] = {{0x73,0xF2,0x8E,0x05},{0x79,0x3C,0x8F,0x05}};
byte floor3[][4] = {{0x7A,0x5E,0x8E,0x05},{0x15,0xD6,0x8E,0x05}};
byte floor4[][4] = {{0x17,0x70,0xA8,0xAD},{0x07,0xEE,0x67,0xAD}};
byte floor5[][4] = {{0x04,0x58,0x8F,0x05},{0x04,0xEC,0x8E,0x05}};
byte floor6[][4] = {{0xE3,0xCE,0xF5,0x27},{0x67,0xEB,0xB5,0xAD}};
byte floor7[][4] = {{0x97,0xE5,0xBF,0xAD},{0xD7,0x8A,0x88,0xAD}};
byte floor8[][4] = {{0xA7,0x88,0xD5,0xAD},{0x67,0x0D,0x5E,0xAD}};
byte floor9[][4] = {{0xD7,0xF4,0x8F,0xAD},{0xE7,0x7D,0xA8,0xAD}};
byte floor10[][4] = {{0x77,0x4D,0xCC,0xAD},{0x37,0x28,0xB7,0xAD}};
byte floor11[][4] = {{0x77,0xAC,0xC8,0xAD},{0x27,0xC4,0x62,0xAD}};
byte floor12[][4] = {{0x97,0x5B,0x7D,0xAD},{0xC7,0xC8,0x8C,0xAD}};

// ============================
// UTIL
// ============================
bool compare(byte *a, byte *b, byte len) {
  for (byte i = 0; i < len; i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

// ============================
// SETUP
// ============================
void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();

  pinMode(DATA_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  updateShiftRegister();

  Serial.println("=== SYSTEM READY ===");
}

// ============================
// LOOP
// ============================
void loop() {

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  byte *uid = rfid.uid.uidByte;
  byte len = rfid.uid.size;

  // ============================
  // ADMIN CARD
  // ============================
  if (compare(uid, adminCard, len)) {
    beep();
    systemMode = MODE_ADMIN;
    Serial.println("ADMIN MODE ACTIVATED");
    rfid.PICC_HaltA();
    return;
  }

  // ============================
  // FUNCTION CARD TOGGLE
  // ============================
  if (compare(uid, installCard, len)) {
    beep();

    if (systemMode == MODE_NORMAL) {
      systemMode = MODE_FUNCTION;
      Serial.println("FUNCTION MODE ON");
    } else {
      systemMode = MODE_NORMAL;
      Serial.println("FUNCTION MODE OFF");
    }

    rfid.PICC_HaltA();
    return;
  }

  // ============================
  // ADMIN MODE ACTION
  // ============================
  if (systemMode == MODE_ADMIN) {
    Serial.println("ADMIN ACCESS → ALL FLOORS ON");

    outputs = 0x0000;
    updateShiftRegister();

    delay(3000);

    outputs = 0xFFFF;
    updateShiftRegister();

    systemMode = MODE_NORMAL;

    rfid.PICC_HaltA();
    return;
  }

  // ============================
  // NORMAL + FUNCTION MODE ACCESS
  // ============================
  if (systemMode == MODE_NORMAL || systemMode == MODE_FUNCTION) {

    if (checkFloor(uid, floor1, 6, len)) granted(1);
    else if (checkFloor(uid, floor2, 6, len)) granted(2);
    else if (checkFloor(uid, floor3, 6, len)) granted(3);
    else if (checkFloor(uid, floor4, 6, len)) granted(4);
    else if (checkFloor(uid, floor5, 6, len)) granted(5);
    else if (checkFloor(uid, floor6, 6, len)) granted(6);
    else if (checkFloor(uid, floor7, 6, len)) granted(7);
    else if (checkFloor(uid, floor8, 6, len)) granted(8);
    else if (checkFloor(uid, floor9, 6, len)) granted(9);
    else if (checkFloor(uid, floor10, 6, len)) granted(10);
    else if (checkFloor(uid, floor11, 6, len)) granted(11);
    else if (checkFloor(uid, floor12, 6, len)) granted(12);
    else error();
  }

  rfid.PICC_HaltA();
}

// ============================
// GRANTED
// ============================
void granted(byte floor) {

  beep();

  Serial.print("ACCESS GRANTED → FLOOR ");
  Serial.println(floor);

  outputs &= ~(1 << (floor - 1));
  updateShiftRegister();

  activeFloor = floor;
  floorTimer = millis();
  floorActive = true;
}

// ============================
// ERROR
// ============================
void error() {
  Serial.println("ACCESS DENIED");
  errorBeep();
}

// ============================
// FLOOR CHECK
// ============================
bool checkFloor(byte *uid, byte floor[][4], int size, byte len) {
  for (int i = 0; i < size; i++) {
    if (compare(uid, floor[i], len)) return true;
  }
  return false;
}

// ============================
// SHIFT REGISTER
// ============================
void updateShiftRegister() {
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, highByte(outputs));
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, lowByte(outputs));
  digitalWrite(LATCH_PIN, HIGH);
}

// ============================
// BEEP
// ============================
void beep() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(120);
  digitalWrite(BUZZER_PIN, LOW);
}

void errorBeep() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(400);
  digitalWrite(BUZZER_PIN, LOW);
}
