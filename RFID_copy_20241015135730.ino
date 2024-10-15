#include <MFRC522.h>
#include <LiquidCrystal.h>
#include <SPI.h>

LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

#define SS_PIN 10
#define RST_PIN 9
 
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key;

int code[] = {203, 158, 65, 2}; // This is the stored UID
int codeRead = 0;
String uidString;

unsigned long startTime;
bool timerRunning = true;
#define COUNTDOWN_TIME 600 // 10 minutes in seconds
unsigned long lastPrintTime = 0;

void setup() {
  lcd.begin(16, 2);
  Serial.begin(9600);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 
  pinMode(8, OUTPUT); // LED pin for unauthorized card
  pinMode(A0, OUTPUT); // LED pin for authorized card

  // Start the timer
  startTime = millis();
}

void loop() {
  // Display timer countdown if running
  if (timerRunning) {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = (currentTime - startTime) / 1000;

    if (elapsedTime >= COUNTDOWN_TIME) {
      timerRunning = false;
      Serial.println("Time's up!");
      lcd.clear();
      lcd.print("Time's up!");
      flashLed(); // Flash LED when time is up
    } else {
      unsigned long remainingTime = COUNTDOWN_TIME - elapsedTime;
      unsigned long minutes = remainingTime / 60;
      unsigned long seconds = remainingTime % 60;

      // Print the countdown timer to the Serial console every minute
      if (currentTime - lastPrintTime >= 60000 || lastPrintTime == 0) {
        Serial.print("Time Remaining: ");
        Serial.print(minutes);
        Serial.print(":");
        Serial.println(seconds);
        lastPrintTime = currentTime;
      }

      // Display on LCD
      lcd.setCursor(0, 0);
      lcd.print("Time Remaining:");
      lcd.setCursor(0, 1);
      lcd.print(minutes);
      lcd.print(":");
      if (seconds < 10) {
        lcd.print("0"); // Add leading zero for seconds
      }
      lcd.print(seconds);
    }
  }

  // Check for RFID input to disarm
  if (rfid.PICC_IsNewCardPresent()) {
    readRFID();
  }

  delay(1000); // Update every second
  lcd.clear();
}

void readRFID() {
  rfid.PICC_ReadCardSerial();
  Serial.print(F("\nPICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check if the PICC is of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  clearUID();
 
  Serial.println("Scanned PICC's UID:");
  printDec(rfid.uid.uidByte, rfid.uid.size);

  uidString = String(rfid.uid.uidByte[0]) + " " + String(rfid.uid.uidByte[1]) + " " + String(rfid.uid.uidByte[2]) + " " + String(rfid.uid.uidByte[3]);
  
  printUID();

  int i = 0;
  boolean match = true;
  while (i < rfid.uid.size) {
    if (!(rfid.uid.uidByte[i] == code[i])) {
      match = false;
    }
    i++;
  }

  if (match) {
    Serial.println("\nI know this card!");
    lcd.clear();
    lcd.print(uidString);
    printUnlockMessage();
    timerRunning = false; // Stop the timer if correct card is scanned
    digitalWrite(A0, HIGH); // Turn on LED for authorized card
  } else {
    lcd.clear();
    lcd.print(uidString);
    lcd.setCursor(0, 1);
    digitalWrite(8, HIGH); // Turn on LED for unauthorized card
    lcd.print("Card Not Allowed");
    delay(2000);
    digitalWrite(8, LOW); // Turn off LED for unauthorized card
    lcd.clear();
    Serial.println("\nUnknown Card");
  }

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}

void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}

void clearUID() {
  lcd.print(uidString); 
}

void printUID() {
  // Placeholder function, can be expanded if needed
}

void printUnlockMessage() {
  lcd.setCursor(0, 1);
  lcd.print("Unlocked");
  digitalWrite(A0, HIGH);
  delay(2000);
  lcd.clear();
  lcd.print("Locked");
  digitalWrite(A0, LOW);
}

void flashLed() {
  for (int i = 0; i < 10; i++) { // Flash LED 10 times
    digitalWrite(A0, HIGH);
    delay(500);
    digitalWrite(A0, LOW);
    delay(500);
  }
}
