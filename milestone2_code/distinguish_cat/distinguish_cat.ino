#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5
#define RST_PIN 0

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

void setup() {
  Serial.begin(9600);         // Initialize serial communications with the PC
  SPI.begin();                // Init SPI bus
  mfrc522.PCD_Init();         // Init MFRC522 card
  Serial.println("Scan a card");
}

void loop() {
  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  //Show UID on serial monitor
  Serial.print("Card UID:");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  // Check if the card's UID matches the predefined UID of cat1
  if (isCat1(mfrc522.uid.uidByte, mfrc522.uid.size)) {
    Serial.println("This is cat1");
  } else if (isCat2(mfrc522.uid.uidByte, mfrc522.uid.size)) {
    Serial.println("This is cat2");
  } else {
    Serial.println("Unknown card");
  }

  // Halt PICC
  mfrc522.PICC_HaltA();

  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
}

// Function to check if the UID matches cat1's UID
bool isCat1(byte *buffer, byte bufferSize) {
  byte cat1[4] = {0x34, 0x28, 0x65, 0xA7};  // Replace these bytes with cat1's UID
  return compareUid(buffer, cat1, bufferSize);
}

// Function to check if the UID matches cat2's UID
bool isCat2(byte *buffer, byte bufferSize) {
  byte cat2[4] = {0xF3, 0xFD, 0xFE, 0x1A};  // Replace these bytes with cat2's UID
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
