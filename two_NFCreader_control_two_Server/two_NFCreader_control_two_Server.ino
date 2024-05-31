#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

// NFC Reader 1
#define SS_PIN1  5  // Slave Select pin for reader 1
#define RST_PIN1 4  // Reset pin for reader 1
#define SERVO_PIN1 13  // Pin connected to the first servo motor

// NFC Reader 2
#define SS_PIN2  16  // Slave Select pin for reader 2
#define RST_PIN2 17  // Reset pin for reader 2
#define SERVO_PIN2 12  // Pin connected to the second servo motor

MFRC522 mfrc522_1(SS_PIN1, RST_PIN1);  // Create MFRC522 instance for reader 1
MFRC522 mfrc522_2(SS_PIN2, RST_PIN2);  // Create MFRC522 instance for reader 2

Servo servo1;  // Create servo object to control the first servo
Servo servo2;  // Create servo object to control the second servo

void setup() {
  delay(1000);               // Add delay to ensure serial communication initializes
  Serial.begin(9600);        // Initialize serial communications with the PC

  Serial.println("Initializing SPI...");
  SPI.begin();               // Init SPI bus

  // Initialize NFC Reader 1
  Serial.println("Initializing NFC Reader 1...");
  mfrc522_1.PCD_Init();         // Init MFRC522 card for reader 1
  servo1.attach(SERVO_PIN1);    // Attaches the first servo on SERVO_PIN1
  servo1.write(180);  // Move servo 1 to 180 degrees

  // Initialize NFC Reader 2
  Serial.println("Initializing NFC Reader 2...");
  mfrc522_2.PCD_Init();         // Init MFRC522 card for reader 2
  servo2.attach(SERVO_PIN2);    // Attaches the second servo on SERVO_PIN2
  servo2.write(0);  // Move servo 2 to 0 degrees

  Serial.println("Setup complete. Scan a card.");
}

void loop() {
  handleReader(mfrc522_1, servo1, "Reader 1", true, true); // Add a parameter to indicate the rotation direction for servo1
  handleReader(mfrc522_2, servo2, "Reader 2", false, false); // Add a parameter to indicate the rotation direction for servo2
}

void handleReader(MFRC522 &mfrc522, Servo &servo, const char* readerName, bool isCat1Reader, bool reverse) {
  static bool lastTagWasNotCat1OrCat2 = false; // State to remember if the last tag was not Cat1 or Cat2

  // Look for new cards
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    Serial.print(readerName);
    Serial.print(": UID tag:");
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println();
    
    if ((isCat1Reader && isCat1(mfrc522.uid.uidByte, mfrc522.uid.size)) || 
        (!isCat1Reader && isCat2(mfrc522.uid.uidByte, mfrc522.uid.size))) {
      Serial.print(readerName);
      Serial.println(isCat1Reader ? ": This is cat1" : ": This is cat2");
      servo.write(reverse ? 180 : 0); // Move servo to 180 degrees if reverse, else to 0 degrees
      lastTagWasNotCat1OrCat2 = false;
    } else {
      Serial.print(readerName);
      Serial.println(isCat1Reader ? ": This is not cat1" : ": This is not cat2");
      if (!lastTagWasNotCat1OrCat2) {
        servo.write(reverse ? 0 : 180); // Rotate servo to 0 degrees if reverse, else to 180 degrees
        delay(1900); // Adjust time as needed for your specific servo
        delay(3000); // Keep the servo at position for 30 seconds
        servo.write(reverse ? 180 : 0); // Rotate servo back to 180 degrees if reverse, else to 0 degrees
        delay(2200); // Adjust time as needed to reverse the previous rotation
        lastTagWasNotCat1OrCat2 = true;
      }
    }
  } else if (lastTagWasNotCat1OrCat2) {
    lastTagWasNotCat1OrCat2 = false;
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

bool isCat2(byte *buffer, byte bufferSize) {
  byte cat2[4] = {0xF3, 0xFD, 0xFE, 0x1A}; // UID of cat2
  return compareUid(buffer, cat2, bufferSize);
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

