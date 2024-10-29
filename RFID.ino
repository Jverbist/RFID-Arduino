#include <MFRC522.h>
#include <SPI.h>

#define SS_PIN 10
#define RST_PIN 9
 
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key;

int code[] = {203, 158, 65, 2}; // This is the stored UID
int codeRead = 0;
String uidString;

unsigned long startTime;
bool timerRunning = true;
bool pumpRelayActivated = false;
bool alarmRelayActivated = false;
#define COUNTDOWN_TIME 1800 // 60 seconds for testing
unsigned long lastPrintTime = 0;

// Init array that will store new NUID 
byte nuidPICC[4];

void setup() {
  Serial.begin(9600);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 
  pinMode(8, OUTPUT); // LED pin for unauthorized card
  pinMode(A0, OUTPUT); // LED pin for authorized card
  pinMode(4, OUTPUT); // Relay pin for pump
  pinMode(5, OUTPUT); // Relay pin for alarm light
  pinMode(7, OUTPUT); // Zoomer pin
  digitalWrite(4, LOW); // Ensure pump relay is off initially
  digitalWrite(5, LOW); // Ensure alarm relay is off initially

  // Start the timer
  startTime = millis();

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("This code scans the MIFARE Classic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);
}

void loop() {
  unsigned long currentTime = millis();

  // Display timer countdown if running
  if (timerRunning) {
    unsigned long elapsedTime = (currentTime - startTime) / 1000;

    if (elapsedTime >= COUNTDOWN_TIME) {
      timerRunning = false;
      Serial.println("Time's up!");
      flashLed(); // Flash LED when time is up
      activateZoomer(); // Activate zoomer when time is up
      if (!alarmRelayActivated) {
        digitalWrite(5, HIGH); // Activate alarm relay when time is up
        alarmRelayActivated = true;
      }
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
    }
  }

  // Keep alarm relay on if timer has ended
  if (alarmRelayActivated) {
    digitalWrite(5, HIGH);
  } else {
    digitalWrite(5, LOW);
  }

  // Check for RFID input to disarm
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Serial.println("Card detected!");
    Serial.print(F("PICC type: "));
    MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
    Serial.println(rfid.PICC_GetTypeName(piccType));

    // Check if the PICC is of Classic MIFARE type
    if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
        piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
        piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
      Serial.println(F("Your tag is not of type MIFARE Classic."));
      return;
    }

    if (rfid.uid.uidByte[0] != nuidPICC[0] || 
        rfid.uid.uidByte[1] != nuidPICC[1] || 
        rfid.uid.uidByte[2] != nuidPICC[2] || 
        rfid.uid.uidByte[3] != nuidPICC[3] ) {
      Serial.println(F("A new card has been detected."));

      // Store NUID into nuidPICC array
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

    readRFID();
  }
}

void readRFID() {
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
    printUnlockMessage();
    timerRunning = false; // Stop the timer if correct card is scanned
    digitalWrite(A0, HIGH); // Turn on LED for authorized card
    digitalWrite(4, HIGH); // Activate pump relay for 5 seconds
    delay(5000); // Keep pump relay on for 5 seconds
    digitalWrite(4, LOW); // Deactivate pump relay
    noTone(7); // Turn off zoomer if disarmed
  } else {
    digitalWrite(8, HIGH); // Turn on LED for unauthorized card
    Serial.println("\nUnknown Card");
    delay(2000);
    digitalWrite(8, LOW); // Turn off LED for unauthorized card
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

void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void clearUID() {
  Serial.print(uidString); 
}

void printUID() {
  // Placeholder function, can be expanded if needed
}

void printUnlockMessage() {
  Serial.println("Unlocked");
  digitalWrite(A0, HIGH);
  delay(2000);
  digitalWrite(A0, LOW);
}

void flashLed() {
  for (int i = 0; i < 10; i++) { // Flash unauthorized LED 10 times
    digitalWrite(8, HIGH);
    delay(500);
    digitalWrite(8, LOW);
    delay(500);
  }
}

void activateZoomer() {
  // Activate alarm relay for alarm light
  digitalWrite(5, HIGH);

  // Play new "Game Over" song
  int melody[] = {440, 392, 349, 330, 294, 262, 294, 330, 349, 392, 440};
  int noteDurations[] = {200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 400};

  for (int thisNote = 0; thisNote < 11; thisNote++) {
    tone(7, melody[thisNote], noteDurations[thisNote]);
    delay(noteDurations[thisNote]);
  }
  noTone(7); // Turn off zoomer at the end
}
