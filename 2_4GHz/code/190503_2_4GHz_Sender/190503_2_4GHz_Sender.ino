#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <SHA256.h>
#include <Crypto.h>
#include <SpeckTiny.h>
#include <string.h>




byte RADIO_ADDRESS[6] = {0x30, 0x30, 0x30, 0x30, 0x31}; // 00001
uint8_t RADIO_READINGPIPE = 0;


const bool useButton              = false;
const size_t KEY_SIZE             = 32;
const size_t STRING_SIZE          = 16;
const uint8_t _PIN_RELAY          = 1;
const uint8_t _PIN_RF_CE          = 7;
const uint8_t _PIN_RF_CSN         = 8;
const uint8_t _PIN_BUTTON         = 4;
bool BUTTON_INVERTED              = false;
uint8_t MAX_RECEIVE_ATTEMPTS      = 10;
const byte GREETING_MSG[STRING_SIZE]  = {0x48, 0x69, 0x6D, 0x4F, 0x4E, 0x79, 0x49, 0x5A, 0x6B, 0x6D, 0x41, 0x6D, 0x68, 0x39, 0x77, 0x63}; // HimONyIZkmAmh9wc





// END OF CONFIGURATION PART


struct CipherVector {
  const char *name;
  const char *cKey;
  byte bKey[KEY_SIZE];
  byte bPlaintext[STRING_SIZE];
  byte bCiphertext[STRING_SIZE];
  char cPlaintext[STRING_SIZE];
};

// Define the vector vectors from http://eprint.iacr.org/2013/404
static CipherVector cipherVector = {
    .name        = "Speck-128-ECB",
    .cKey         = "bnbUy90ErlgzkpSrBYWRKhlc5L85Q7BX",
    .bKey         = {0x62, 0x6E, 0x62, 0x55, 0x79, 0x39, 0x30, 0x45, 0x72, 0x6C, 0x67, 0x7A, 0x6B, 0x70, 0x53, 0x72, 0x42, 0x59, 0x57, 0x52, 0x4B, 0x68, 0x6C, 0x63, 0x35, 0x4C, 0x38, 0x35, 0x51, 0x37, 0x42, 0x58},
    .bPlaintext   = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    .bCiphertext  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};


SpeckTiny speckTiny;
RF24 radio(_PIN_RF_CE, _PIN_RF_CSN); // CE, CSN



byte BUFFER[STRING_SIZE];
const char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
uint8_t currentMode = 1;
uint8_t latestPrintedMode = 0;
uint8_t receivingAttempts = 0;

bool handleCipher(BlockCipher *, const struct CipherVector *, size_t, bool = true);
void generateNewString(const struct CipherVector*);
void displayKey(const struct CipherVector*);
void displayRawString(const struct CipherVector*);
void displayReceivedData(byte*, size_t = STRING_SIZE);
void setRadioModeToListening(RF24*, uint8_t = RADIO_READINGPIPE, byte* = RADIO_ADDRESS);
void setRadioModeToWriting(RF24*, uint8_t = RADIO_READINGPIPE, byte* = RADIO_ADDRESS);
void displayCurrentMode();
void openDoor(uint8_t = _PIN_RELAY, bool = false);





/*
1   Generate and encrypt random string
2   Wait for greeting and perform handshake
3   Send encrypted string to handsender
4   Wait for matching encrypted string
5   Open door

*/





void setup() {
  Serial.begin(115200);
  while(!Serial) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
  }
  Serial.println("I am the mobile device - ready!");

  radio.begin();

  // Set device to receive mode to automatically receive the handshake string from the stationary device
  radio.openReadingPipe(RADIO_READINGPIPE, RADIO_ADDRESS);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();

}

/*
1   Wait for button push
x2   Send greeting
3   Wait for string
4   Encrypt string and send encrypted string
*/




void loop() {

  // Step 1 - wait for button push
  if(currentMode == 1) {
    displayCurrentMode();

    if(useButton) {
      if(digitalRead(_PIN_BUTTON) != BUTTON_INVERTED) {
        currentMode = 3;
      }
    }
    else {
      // No button push needed
      currentMode = 3;
    }

  }


  // Step 2 - send greeting
  if(currentMode == 2) {
    // Set RF sending mode
    radio.openWritingPipe(RADIO_ADDRESS);
    radio.setPALevel(RF24_PA_MIN);
    radio.stopListening();
    delay(100);
    radio.write(GREETING_MSG, STRING_SIZE);
    currentMode = 3;
  }

/*
    // Generate new string and encrypt it
    generateNewString(&cipherVector);

    // Show new raw string
    displayRawString(&cipherVector);

    // Encrypt string
    if( handleCipher(&speckTiny, &cipherVector, KEY_SIZE, false)) {
      // Update mode
      currentMode = 2;
    }
    else {
      Serial.println("Resetting mode to 1");
      delay(500);
    }
  }
*/


  // Step 3 - wait for incoming string and store it in the vector
  if(currentMode == 3) {
    displayCurrentMode();

    // Set RF to listening mode
    // setRadioModeToListening(&radio);
    radio.openReadingPipe(RADIO_READINGPIPE, RADIO_ADDRESS);
    radio.setPALevel(RF24_PA_MIN);
    radio.startListening();
    delay(100);

    if(radio.available()) {
      // Received something, now process it

      byte receivedData[STRING_SIZE];

      // Read content of received data
      radio.read(&receivedData, STRING_SIZE);

// Display received data
displayReceivedData(receivedData, STRING_SIZE);



      for(uint8_t i=0; i<STRING_SIZE; i++) {
        cipherVector.bCiphertext[i] = receivedData[i];
      }

      // Compare received data with common greeting
      // if(currentMode == 3 && memcmp(receivedData, GREETING_MSG, STRING_SIZE) == 0) {
      if(memcmp(receivedData, cipherVector.bCiphertext, STRING_SIZE) == 0) {
        // Handshake successful

        // Display received data
        displayReceivedData(receivedData, STRING_SIZE);

        // Update mode
        currentMode = 4;
      }
    }
  }

  // Step 4 - send string to sender
  if(currentMode == 4) {
    displayCurrentMode();

    // Set RF sending mode
    radio.openWritingPipe(RADIO_ADDRESS);
    radio.setPALevel(RF24_PA_MIN);
    radio.stopListening();
    delay(100);

    if( handleCipher(&speckTiny, &cipherVector, KEY_SIZE, false)) {
  //  const char text[] = "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWYXZ";
      radio.write(cipherVector.bCiphertext, STRING_SIZE);
      currentMode = 4;
    }

    // Display encrypted string
    displayEncryptedString(&cipherVector);


    // Reset mode
    currentMode = 1;
  }


}


void generateNewString(const struct CipherVector* vector) {
  Serial.println(F("######## Generating new random string... ########"));
  uint8_t i = 0;
  while(i < STRING_SIZE) {
    char letter = letters[random(0, sizeof(letters)-1)];
    cipherVector.bPlaintext[i] = (byte) letter;
    cipherVector.cPlaintext[i] = letter;
    i++;
  }
}

void displayRawString(const struct CipherVector* vector) {
  Serial.print("String as plaintext: ");
  for(uint8_t i=0; i<STRING_SIZE; i++) {
    Serial.print(vector->cPlaintext[i]);
  }
  Serial.println();


  Serial.print("String as HEX bytes: ");
  for(uint8_t i=0; i<STRING_SIZE; i++) {
    Serial.print("0x");
    if(vector->bPlaintext[i] < 16) {
      Serial.print("0");
    }
    Serial.print(vector->bPlaintext[i], HEX);
    if(i < STRING_SIZE-1) {
      Serial.print(", ");
    }
  }
  Serial.println();
}

void displayEncryptedString(const struct CipherVector* vector) {
  Serial.print("Encrypted string as HEX byte: ");

  for(uint8_t i=0; i<STRING_SIZE; i++) {
    cipherVector.bCiphertext[i] = BUFFER[i];
    Serial.print("0x");

    if(cipherVector.bCiphertext[i] < 16) {
      Serial.print("0");
    }

    Serial.print(cipherVector.bCiphertext[i], HEX);
    if(i < STRING_SIZE-1) {
      Serial.print(", ");
    }
  }
  Serial.println();
}

void displayKey(const struct CipherVector* vector) {
  Serial.print(F("Key as plaintext: ")); Serial.println(vector->cKey);
  Serial.print(F("Key as HEX bytes: "));
  for(uint8_t i=0; i<KEY_SIZE; i++) {
    Serial.print("0x");

    if(vector->bKey[i] < 16) {
      Serial.print("0");
    }

    Serial.print(vector->bKey[i], HEX);
    if(i < KEY_SIZE-1) {
      Serial.print(", ");
    }
  }
  Serial.println();
}

bool handleCipher(BlockCipher *cipher, const struct CipherVector *vector, size_t keySize, bool decryption) {
  Serial.println("######## Encrypting string... ########");

  // Display used key
  displayKey(vector);

  // Display unencrypted string
  displayRawString(vector);
  crypto_feed_watchdog();

  cipher->setKey(vector->bKey, keySize);
  cipher->encryptBlock(BUFFER, vector->bPlaintext);

  // Display encrypted string
  displayEncryptedString(vector);


  Serial.print(vector->name);
  Serial.print(" Encryption ");




  if (memcmp(BUFFER, vector->bCiphertext, STRING_SIZE) == 0) {
    Serial.println("Passed");
    return true;
  }
  else {
    Serial.println("Failed");
    return false;
  }
}

void displayReceivedData(byte *data, size_t dataSize) {
  for(size_t i=0; i<dataSize; i++) {
    Serial.print("0x");
    if(data[i] < 16) {
      Serial.print("0");
    }
    Serial.print(data[i], HEX);
    if(i < dataSize-1) {
      Serial.print(", ");
    }
  }
}


void displayCurrentMode() {
  if(latestPrintedMode != currentMode) {
    Serial.print(F("######################################## Current Mode: ")); Serial.print(currentMode); Serial.println(F(" ########################################"));
    Serial.println(F("# "));
    Serial.print(F("# "));
    switch(currentMode) {
      case 1:
        if(useButton) {
          Serial.print(F("Wait for button to pressed"));
        }
        else {
          Serial.print(F("Skip trigger awaiting"));
        }
        break;
      case 2:
        Serial.print(F("Wait for incoming greeting and perform handshake"));
        break;
      case 3:
        Serial.print(F("Wait for raw string"));
        break;
      case 4:
        Serial.print(F("Encrypt string and send it back"));
        break;
      default:
        Serial.print(F("ERROR, UNDEFINED MODE"));
        break;
    }
    Serial.println();
    Serial.println(F("#"));
    latestPrintedMode = currentMode;
  }
}



/*
void setRadioModeToListening(RF24* rf24radio, uint8_t readingPipe = RADIO_READINGPIPE, byte* radioAddress = RADIO_ADDRESS, size_t addressSize=sizeof(RADIO_ADDRESS)) {
  rf24radio->openReadingPipe(readingPipe, radioAddress);
  rf24radio->setPALevel(RF24_PA_MIN);
  rf24radio->startListening();
}

void setRadioModeToWriting() {

}
*/
