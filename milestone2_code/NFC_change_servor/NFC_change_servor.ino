#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

#define SS_PIN  5  // Slave Select pin
#define RST_PIN 0  // Reset pin
#define SERVO_PIN 13  // Pin connected to the servo motor

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
Servo servo;  // Create servo object to control a servo

void setup() {
  Serial.begin(9600);         // Initialize serial communications with the PC
  SPI.begin();                // Init SPI bus
  mfrc522.PCD_Init();         // Init MFRC522 card
  servo.attach(SERVO_PIN);    // Attaches the servo on the SERVO_PIN to the servo object
  servo.writeMicroseconds(1500);  // Stop servo
  Serial.println("Scan a card");
}

void loop() {
  static bool lastTagWasNotCat1 = false; // State to remember if the last tag was not Cat1

  // Look for new cards
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    if (isCat1(mfrc522.uid.uidByte, mfrc522.uid.size)) {
      Serial.println("This is cat1");
      servo.writeMicroseconds(1500); // Stop servo
      lastTagWasNotCat1 = false;
    } else {
      Serial.println("This is not cat1");
      if (!lastTagWasNotCat1) {
        servo.writeMicroseconds(1600); // Rotate servo to open position
        delay(1900); // Adjust time as needed for your specific servo
        servo.writeMicroseconds(1500); // Stop servo at open position
        delay(10000); // Keep the servo at open position for 30 seconds
        servo.writeMicroseconds(1400); // Rotate servo to close position
        delay(2200); // Adjust time as needed to reverse the previous rotation
        servo.writeMicroseconds(1500); // Stop servo
        lastTagWasNotCat1 = true;
      }
    }
  } else if (lastTagWasNotCat1) {
    lastTagWasNotCat1 = false;
    Serial.println("Card removed - reset to closed position");
  }

  // Halt PICC
  mfrc522.PICC_HaltA();

  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
}

bool isCat1(byte *buffer, byte bufferSize) {
  byte cat1[4] = {0x34, 0x28, 0x65, 0xA7}; // UID of cat1
  return compareUid(buffer, cat1, bufferSize);
}

// Helper function to compare two UIDs
bool compareUid(byte *buffer1, byte *buffer2, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    if (buffer1[i] != buffer2[i]) {
      return false;
    }
  }
  return true;
}

