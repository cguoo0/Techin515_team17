#include <Arduino.h>
#include <HX711.h>
#include <SPI.h>
#include <MFRC522.h>

// IC卡 (NFC)
#define SS_PIN 21   // NFC SS
#define RST_PIN 15  // NFC RST

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
byte nuidPICC[4];

// HX711电路接线 (Load cell connections)
const int LOADCELL_DOUT_PIN_a = 26;  // 猫1 DOUT
const int LOADCELL_SCK_PIN_a = 27;   // 猫1 SCK
const int LOADCELL_DOUT_PIN_c = 32;  // 盘1 DOUT
const int LOADCELL_SCK_PIN_c = 33;   // 盘1 SCK

HX711 scale_a, scale_c;

#define CALIBRATION_FACTOR 426
int reading_a, reading_c;

// 舵机 (Servo motor)
const int servoPin_a = 17;  // 舵机1 GPIO pin
const int servoPin_c = 16;  // 舵机2 GPIO pin
int freq = 50;
int channel_a = 0;
int channel_c = 1;
int resolution = 16;
bool rotateCW = true;  // State variable to control servo rotation direction

void setup() {
  Serial.begin(115200); // Set baud rate to 115200

  // 舵机初始化 (Servo motor setup)
  ledcSetup(channel_a, freq, resolution);
  ledcSetup(channel_c, freq, resolution);

  ledcAttachPin(servoPin_a, channel_a);
  ledcAttachPin(servoPin_c, channel_c);

  // 初始化称重模块 (Initialize scales)
  scale_a.begin(LOADCELL_DOUT_PIN_a, LOADCELL_SCK_PIN_a);
  scale_a.set_scale(CALIBRATION_FACTOR);
  scale_a.tare();

  scale_c.begin(LOADCELL_DOUT_PIN_c, LOADCELL_SCK_PIN_c);
  scale_c.set_scale(CALIBRATION_FACTOR);
  scale_c.tare();

  // 初始化IC卡 (Initialize NFC)
  SPI.begin();
  rfid.PCD_Init();

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;  // Default key
  }

  Serial.println(F("This code scans the MIFARE Classic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);

  // Initialize servos to stop position
  ledcWrite(channel_a, microsecondsToDuty(1500));
  ledcWrite(channel_c, microsecondsToDuty(1500));
}

void loop() {
  // 称重模块 (Read scales)
  reading_a = round(scale_a.get_units(10)); // Average 10 readings for stability
  reading_c = round(scale_c.get_units(10));

  Serial.print("猫1: ");
  Serial.print(reading_a);
  Serial.println("g");
  Serial.print("盘1: ");
  Serial.print(reading_c);
  Serial.println("g");

  // Power management for HX711
  scale_a.power_down();
  scale_c.power_down();
  delay(2000);
  scale_a.power_up();
  scale_c.power_up();

  // 舵机1 控制逻辑 (Servo motor 1 control logic)
  if (reading_a > 250) {
    if (rotateCW) {
      // Rotate servo clockwise for one full turn
      ledcWrite(channel_a, microsecondsToDuty(1600)); // Move servo CW
      delay(4720); // Time for servo to complete one full turn (adjust as necessary)
      rotateCW = false;
    } else {
      // Rotate servo counterclockwise for one full turn
      ledcWrite(channel_a, microsecondsToDuty(1400)); // Move servo CCW
      delay(6400); // Time for servo to complete one full turn
      rotateCW = true;
    }
    ledcWrite(channel_a, microsecondsToDuty(1500)); // Stop servo

    // Move second servo for 5 seconds
    ledcWrite(channel_c, microsecondsToDuty(1600)); // Move servo CW
    delay(5000); // Rotate for 5 seconds
    ledcWrite(channel_c, microsecondsToDuty(1500)); // Stop servo
  }

  // IC卡 (NFC Handling)
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  if (rfid.uid.uidByte[0] != nuidPICC[0] ||
      rfid.uid.uidByte[1] != nuidPICC[1] ||
      rfid.uid.uidByte[2] != nuidPICC[2] ||
      rfid.uid.uidByte[3] != nuidPICC[3]) {
    Serial.println(F("A new card has been detected."));

    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }

    Serial.println(F("The NUID tag is:"));
    Serial.print(F("In hex: "));
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
    Serial.print(F("In dec: "));
    printDec(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
  } else {
    Serial.println(F("Card read previously."));
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

uint32_t microsecondsToDuty(int microseconds) {
    return (microseconds * pow(2, resolution) / 20000); // Convert microseconds to duty cycle for 16-bit resolution
}

void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}
