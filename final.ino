#include <SPI.h>
#include <MFRC522.h>
#include <avr/wdt.h>
#define SS_PIN 10
#define RST_PIN 8

MFRC522 mfrc522(SS_PIN, RST_PIN);

// ============================
// BUZZER
// ============================
#define BUZZER_PIN 6

// ============================
// RELAY PINS
// ============================
const int groundRelay = 22;

const int floorRelay[12] = {
  23, 24, 25, 26, 27, 28,
  29, 30, 31, 32, 33, 34
};

// ============================
// RELAY TYPES (UNCHANGED LOGIC)
// ============================
const bool FLOOR_ACTIVE_LOW = true;
const bool GROUND_ACTIVE_HIGH = true;

// ============================
// ACCESS TIMER
// ============================
const unsigned long ACCESS_TIME = 3000;
unsigned long accessStart = 0;
bool accessActive = false;
unsigned long lastRC522Check = 0;
const unsigned long RC522_CHECK_INTERVAL = 5000;

int activeFloor = -1;
bool serviceMode = false;


//byte installCard = {0,0,0,0};
// ============================
// SERVICE CARDS
// ============================''

byte serviceCards[5][4] = {
  {0x82,0x50,0x41,0x20},
  {0x47,0xC6,0xBE,0xAD},
  {0xE7,0xD3,0xB0,0xAD},
  {0x97,0xE2,0x75,0xAD},
  {0xF7,0xEE,0xB0,0xAD}
};

// ============================
// FLOOR CARDS
// ============================

byte floorCards[12][24][4] = {
  {{0xD7,0x6D,0x85,0xAD},{0x67,0x0B,0xD4,0xAD},{0x47,0x40,0xC2,0xAD},{0xA7,0xF6,0x8F,0xAD},{0x57,0xC5,0xB2,0xAD},{0x07,0x22,0x8B,0xAD},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}},
  //2nd Floor
  {{0x73,0xF2,0x8E,0x05},{0x79,0x3C,0x8F,0x05},{0xEC,0x51,0x8E,0x05},{0x27,0xFE,0x89,0xAD},{0xE7,0x78,0xA8,0xAD},{0x67,0xC7,0x69,0xAD},{0xBA,0x43,0x52,0x07},{0x6A,0xFC,0x4F,0x07},{0x85,0x4F,0x52,0x07},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}},
  //3rd Floor
  {{0x7A,0x5E,0x8E,0x05},{0x15,0xD6,0x8E,0x05},{0x69,0xCB,0x8F,0x05},{0x6E,0xF2,0x8E,0x05},{0x91,0x2B,0x8F,0x05},{0x6B,0x82,0x8E,0x05},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}},
  //4th Floor 
  {{0x17,0x70,0xA8,0xAD},{0x07,0xEE,0x67,0xAD},{0x07,0xB3,0x9D,0xAD},{0x4B,0x14,0x8F,0x05},{0xD7,0x20,0x78,0xAD},{0xF7,0xBA,0xC4,0xAD},{0x1E,0x3A,0x50,0x07},{0xC3,0x32,0x51,0x07},{0x13,0x43,0x50,0x07},{0xCA,0x48,0x52,0x07},{0xA2,0x1C,0x50,0x07},{0x72,0x4C,0x50,0x07},{0x76,0x4C,0x52,0x07},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}},
  //5th Floor 
  {{0x04,0x58,0x8F,0x05},{0x04,0xEC,0x8E,0x05},{0x7B,0x66,0x8E,0x05},{0x27,0x1B,0x69,0xAD},{0x87,0x24,0xC2,0xAD},{0xB4,0x80,0x8E,0x05},{0xD2,0x36,0x50,0x07},{0xB7,0xA4,0x51,0x07},{0xCD,0x7C,0x50,0x07},{0x3E,0xBF,0x50,0x07},{0x6A,0x0C,0x50,0x07},{0xF6,0x3B,0x50,0x07},{0xE5,0x01,0x50,0x07},{0x53,0x50,0x51,0x07},{0xBC,0x92,0x50,0x07},{0x54,0x21,0x52,0x07},{0xDC,0x76,0x52,0x07},{0xED,0x76,0x52,0x07},{0x02,0xB9,0x51,0x07},{0xC7,0x41,0x52,0x07},{0x2B,0x0A,0x51,0x07},{0x71,0x43,0x50,0x07},{0xEE,0x7F,0x50,0x07},{0x1B,0x9B,0x51,0x07}},
  //6th Floor
  {{0xE3,0xCE,0xF5,0x27},{0x67,0xEB,0xB5,0xAD},{0x47,0xF3,0x6A,0xAD},{0xC7,0x69,0xA2,0xAD},{0x27,0x9D,0xB9,0xAD},{0x67,0x4C,0xB9,0xAD},{0x54,0x1C,0x50,0x07},{0x4B,0x0B,0x50,0x07},{0x32,0x60,0x52,0x07},{0x27,0x60,0x51,0x07},{0xE4,0x39,0x50,0x07},{0x42,0x69,0x52,0x07},{0x49,0x14,0x50,0x07},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}},
  //7th Floor
  {{0x97,0xE5,0xBF,0xAD},{0xD7,0x8A,0x88,0xAD},{0x87,0x1F,0xC5,0xAD},{0xA7,0x4F,0x82,0xAD},{0xC7,0xB9,0x94,0xAD},{0xE7,0xA9,0x58,0xAD},{0x13,0x19,0x50,0x07},{0x32,0x62,0x51,0x07},{0xD5,0x60,0x52,0x07},{0xE5,0x9E,0x50,0x07},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}},
  //8th Floor --> 
  {{0xA7,0x88,0xD5,0xAD},{0x67,0x0D,0x5E,0xAD},{0x87,0x0D,0x72,0xAD},{0xD7,0xC6,0xCC,0xAD},{0x47,0xF7,0xC9,0xAD},{0x47,0x67,0xAC,0xAD},{0x9B,0x64,0x51,0x07},{0x25,0x6E,0x52,0x07},{0x7B,0xEF,0x51,0x07},{0x5B,0xCD,0x51,0x07},{0x1A,0x10,0x50,0x07},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}},
  //9th Floor --> 
  {{0xD7,0xF4,0x8F,0xAD},{0xE7,0x7D,0xA8,0xAD},{0x1E,0x95,0x6A,0x05},{0x49,0x4F,0x8E,0x05},{0xC6,0x00,0x71,0x05},{0x1D,0x47,0x8E,0x05},{0x72,0x00,0x52,0x07},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}},
  //10th Floor --> 
  {{0x77,0x4D,0xCC,0xAD},{0x37,0x28,0xB7,0xAD},{0x17,0xB9,0xD0,0xAD},{0xF7,0x7A,0xB5,0xAD},{0x67,0xE5,0x8E,0x05},{0xB9,0x5E,0x8E,0x05},{0x25,0x55,0x52,0x07},{0x46,0x69,0x52,0x07},{0x2B,0x0B,0x50,0x07},{0xD1,0xFB,0x51,0x07},{0x6A,0x8C,0x52,0x07},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}},
  //11st Floor --> 
  {{0x77,0xAC,0xC8,0xAD},{0x27,0xC4,0x62,0xAD},{0xC7,0x6A,0x8E,0xAD},{0xD7,0xC9,0x87,0xAD},{0xC7,0x80,0x89,0xAD},{0x17,0xF0,0xB2,0xAD},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}},
  //12nd Floor --> 
  {{0x97,0x5B,0x7D,0xAD},{0xC7,0xC8,0x8C,0xAD},{0xF7,0xE4,0x8E,0xAD},{0xD7,0x41,0x59,0xAD},{0x17,0x72,0x57,0xAD},{0x77,0x12,0x56,0xAD},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}
};

// ============================
// RELAY CONTROL FUNCTIONS (UNCHANGED)
// ============================
void floorON(int pin) {
  if (FLOOR_ACTIVE_LOW)
    digitalWrite(pin, LOW);
  else
    digitalWrite(pin, HIGH);
}

void floorOFF(int pin) {
  if (FLOOR_ACTIVE_LOW)
    digitalWrite(pin, HIGH);
  else
    digitalWrite(pin, LOW);
}

void groundON() {
  if (GROUND_ACTIVE_HIGH)
    digitalWrite(groundRelay, HIGH);
  else
    digitalWrite(groundRelay, LOW);
}

void groundOFF() {
  if (GROUND_ACTIVE_HIGH)
    digitalWrite(groundRelay, LOW);
  else
    digitalWrite(groundRelay, HIGH);
}

// ============================
// BUZZER FUNCTIONS (ADDED ONLY)
// ============================
void beep(int t) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(t);
  digitalWrite(BUZZER_PIN, LOW);
}

void beepSuccess() {
  beep(100);
}

void beepService() {
  beep(100);
  delay(100);
  beep(100);
}

void beepError() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(600);
  digitalWrite(BUZZER_PIN, LOW);
}

// ============================
// UID MATCH
// ============================
bool matchUID(byte *a, byte *b) {
  for (byte i = 0; i < 4; i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}


bool isEmpty(byte *card) {
  for (int i = 0; i < 4; i++) {
    if (card[i] != 0) return false;
  }
  return true;
}

// ============================
// FIND FLOOR
// ============================
int getFloor(byte *uid) {
  for (int f = 0; f < 12; f++) {
    for (int c = 0; c < 20; c++) {
      if (!isEmpty(floorCards[f][c]) && matchUID(uid, floorCards[f][c])) {
        return f;
      }
    }
  }
  return -1;
}
/*

void InstallMode(byte *id){
  for(int f=0; f<12; f++){
    for(int c=0; c<20; c++){
      if(id == installCard ){

      }
    }
  }
}
*/
// ============================
// SERVICE CHECK
// ============================
bool isService(byte *uid) {
  for (int i = 0; i < 5; i++) {
    if (matchUID(uid, serviceCards[i])) return true;
  }
  return false;
  }

  // RC522 STABILITY ONLY
  void recoverRC522() {

  Serial.println("RC522 RECOVERY");

  digitalWrite(RST_PIN, LOW);
  delay(100);

  digitalWrite(RST_PIN, HIGH);
  delay(100);

  SPI.end();
  delay(50);

  SPI.begin();
  delay(50);

  mfrc522.PCD_Init();
  delay(100);

  byte version = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);

  Serial.print("RC522 VERSION: 0x");
  Serial.println(version, HEX);

  if (version == 0x00 || version == 0xFF) {

    Serial.println("RC522 FAILED -> MEGA RESET");

    wdt_enable(WDTO_15MS);

    while (1) {
    }
  }

  Serial.println("RC522 OK");
}

// ============================
// SETUP
// ============================
void setup() {
  Serial.begin(9600);

  delay(2000);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(groundRelay, OUTPUT);
  groundOFF();

  for (int i = 0; i < 12; i++) {
    pinMode(floorRelay[i], OUTPUT);
    floorOFF(floorRelay[i]);
  }


  SPI.begin();
  delay(200);
  mfrc522.PCD_Init();
  delay(200);


  Serial.println("ELEVATOR READY");
}

// ============================
// LOOP
// ============================
void loop() {

  if (millis() - lastRC522Check > RC522_CHECK_INTERVAL) {

  lastRC522Check = millis();

  byte version =
      mfrc522.PCD_ReadRegister(MFRC522::VersionReg);

  if (version == 0x00 || version == 0xFF) {

    Serial.println("RC522 COMMUNICATION ERROR");

    recoverRC522();
  }
  
}
  if (accessActive && millis() - accessStart > ACCESS_TIME) {

    groundOFF();

    for (int i = 0; i < 12; i++) {
      floorOFF(floorRelay[i]);
    }

    accessActive = false;
    serviceMode = false;
    activeFloor = -1;

    Serial.println("ACCESS EXPIRED");
  }

  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  byte *uid = mfrc522.uid.uidByte;

  int floor = getFloor(uid);

  if (isService(uid)) {

    Serial.println("SERVICE MODE");
    beepService();

    groundON();

    for (int i = 0; i < 12; i++) {
      floorON(floorRelay[i]);
    }

    serviceMode = true;
  }
  else if (floor != -1) {

    Serial.print("FLOOR ACCESS: ");
    Serial.println(floor + 1);

    beepSuccess();

    groundON();
    floorON(floorRelay[floor]);

    activeFloor = floor;
  }
  else {

    Serial.println("ACCESS DENIED");
    beepError();
    return;
  }

  accessActive = true;
  accessStart = millis();

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  wdt_reset();
}
