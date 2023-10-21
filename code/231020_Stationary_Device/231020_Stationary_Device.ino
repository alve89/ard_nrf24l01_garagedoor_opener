#include <Arduino.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <SHA256.h>
#include <CryptoCstm.h>
#include <SpeckTiny.h>
#include <string.h>
#include <Base64.h>

//byte RADIO_ADDRESS[6] = {0x30, 0x30, 0x30, 0x30, 0x31}; // 00001
const byte RADIO_ADDRESS[6] = "00001";
uint8_t RADIO_READINGPIPE = 0;
const size_t KEY_SIZE = 32;
const size_t STRING_SIZE = 16;
const uint8_t _PIN_UNUSED = A0;  // For random seed
const uint8_t _PIN_RELAY = 4;
const uint8_t _PIN_RF_CE = 8;
const uint8_t _PIN_RF_CSN = 7;
const uint8_t _PIN_GARAGE_OCCUPATION = A1;
const uint8_t _PIN_DOOR_STATUS = 6;
const uint8_t _PIN_LIGHTBARRIER = 9;
const bool _INVERT_GARAGE_OCCUPATION = false;
const bool _INVERT_DOOR_STATUS = false;
const bool _INVERT_LIGHTBARRIER = false;
const uint8_t MAX_RECEIVE_ATTEMPTS = 10;
const uint8_t MAX_WAIT_DURATION_SEC = 10;
const uint8_t TIMEOUT = 3000;                // milliseconds
const uint8_t DOOR_AREA_CLEARING_TIME = 10;  // seconds
const uint8_t RF_AREA_CLEARING_TIME = 10;  // seconds
const uint8_t LDR_TOLERANCE = 5; // integer, will be transformed to percentage
const uint8_t LDR_TRESHOLD = 600; // value of analoagRead()

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
  .name = "Speck-128-ECB",
  .cKey = "bnbUy90ErlgzkpSrBYWRKhlc5L85Q7BX",
  .bKey = { 0x62, 0x6E, 0x62, 0x55, 0x79, 0x39, 0x30, 0x45, 0x72, 0x6C, 0x67, 0x7A, 0x6B, 0x70, 0x53, 0x72, 0x42, 0x59, 0x57, 0x52, 0x4B, 0x68, 0x6C, 0x63, 0x35, 0x4C, 0x38, 0x35, 0x51, 0x37, 0x42, 0x58 },
  .bPlaintext = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  .bCiphertext = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};


SpeckTiny speckTiny;
RF24 radio(_PIN_RF_CE, _PIN_RF_CSN);  // CE, CSN

byte BUFFER[STRING_SIZE];
const char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
unsigned long sendTime = 0;

bool handleCipher(BlockCipher *, const struct CipherVector *, size_t, bool = true);
void generateNewString(const struct CipherVector *);
void displayKey(const struct CipherVector *);
void displayRawString(const struct CipherVector *);
void displayReceivedData(byte *, size_t = STRING_SIZE);
void openDoor(uint8_t = _PIN_RELAY, bool = false);
bool isDoorClosed();
bool isGarageOccupied();
bool isTimeout();
void reset();


void reset() {
  Serial.println("reset()");
  sendTime = 0;
  radio.openWritingPipe(RADIO_ADDRESS);
  radio.stopListening();
  radio.setAutoAck(true);
  radio.enableDynamicAck();
  radio.setRetries(5, 15);
  radio.enableDynamicPayloads();
}


bool isTimeout() {
  return (millis() - sendTime > TIMEOUT) ? true : false;
}

bool isGarageOccupied() {
  Serial.print("isGarageOccupied(): ");
  
  //bool status = _INVERT_GARAGE_OCCUPATION ? abs(digitalRead(_PIN_GARAGE_OCCUPATION)) : digitalRead(_PIN_GARAGE_OCCUPATION);
  bool status = false;

  // First meausure the current value of the sensor
  uint8_t value1 = analogRead(_PIN_GARAGE_OCCUPATION);
  
  // Now turn the laser on (or off)
  digitalWrite(_PIN_LIGHTBARRIER, !_INVERT_LIGHTBARRIER);
  delay(1000);

  // Give the sensor a second to adapt to the new brightness
  uint8_t value2 = analogRead(_PIN_GARAGE_OCCUPATION);
  digitalWrite(_PIN_LIGHTBARRIER, _INVERT_LIGHTBARRIER);
  
  // Compare both values
  if(value1 > value2 * (1-LDR_TRESHOLD/100)
  || value1 < value1 * (1+LDR_TRESHOLD/100)) {
    // The sensors value didn't change during measurement => no change in the occupation status
    status = true;
  }
  else {
    // The sensors value changed by more than X percent
    // => this means, the laser hit the LDR before the second measurement
    // => garage is not occupied
    status = false;
  }

  Serial.println(status);
  return status;
}

bool isDoorClosed() {
  Serial.print("isDoorClosed(): ");
  bool status = _INVERT_DOOR_STATUS ? abs(digitalRead(_PIN_DOOR_STATUS)) : digitalRead(_PIN_DOOR_STATUS);
  Serial.println(status);
  return status;
}

void openDoor(uint8_t relayPin, bool inverted) {
  digitalWrite(relayPin, !inverted);
  delay(1000);
  digitalWrite(relayPin, inverted);
  Serial.println("openDoor()");
}

void generateNewString(const struct CipherVector *vector) {
  Serial.println(F("######## Generating new random string... ########"));
  uint8_t i = 0;
  while (i < STRING_SIZE) {
    char letter = letters[random(0, sizeof(letters) - 1)];
    cipherVector.bPlaintext[i] = (byte)letter;
    cipherVector.cPlaintext[i] = letter;
    i++;
  }
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

  Serial.print("    ");
  Serial.print(vector->name);
  Serial.print(" Encryption ");




  if (memcmp(BUFFER, vector->bCiphertext, STRING_SIZE) == 0) {
    Serial.println("Passed");
    return true;
  } else {
    Serial.println("Failed");
    return false;
  }
}

void displayReceivedData(char *data, size_t dataSize = STRING_SIZE) {
  Serial.print("Received data as char: ");
  Serial.println(data);
  Serial.print("Received data as HEX: ");
  for (size_t i = 0; i < dataSize; i++) {
    Serial.print("0x");
    if (data[i] < 16) {
      Serial.print("0");
    }
    Serial.print(data[i], HEX);
    if (i < dataSize - 1) {
      Serial.print(", ");
    }
  }
  Serial.println("");
}

void displayReceivedData(byte *data, size_t dataSize = STRING_SIZE) {
  Serial.print("    Received data as char: '");
  for (size_t i = 0; i < dataSize; i++) {
    Serial.print(data[i]);
  }
  Serial.println("'");

  Serial.print("    Received data as HEX: '");
  for (size_t i = 0; i < dataSize; i++) {
    Serial.print("0x");
    if (data[i] < 16) {
      Serial.print("0");
    }
    Serial.print(data[i], HEX);
    if (i < dataSize - 1) {
      Serial.print(", ");
    }
  }
  Serial.println("'");
}


void displayEncryptedString(const struct CipherVector *vector) {
  Serial.print("    Encrypted string as HEX byte: ");

  for (uint8_t i = 0; i < STRING_SIZE; i++) {
    cipherVector.bCiphertext[i] = BUFFER[i];
    Serial.print("0x");

    if (cipherVector.bCiphertext[i] < 16) {
      Serial.print("0");
    }

    Serial.print(cipherVector.bCiphertext[i], HEX);
    if (i < STRING_SIZE - 1) {
      Serial.print(", ");
    }
  }
  Serial.println();
}

void displayKey(const struct CipherVector *vector) {
  Serial.print(F("    Key as plaintext: "));
  Serial.println(vector->cKey);
  Serial.print(F("    Key as HEX bytes: "));
  for (uint8_t i = 0; i < KEY_SIZE; i++) {
    Serial.print("0x");

    if (vector->bKey[i] < 16) {
      Serial.print("0");
    }

    Serial.print(vector->bKey[i], HEX);
    if (i < KEY_SIZE - 1) {
      Serial.print(", ");
    }
  }
  Serial.println();
}

void displayRawString(const struct CipherVector *vector) {
  Serial.print("    String as plaintext: ");
  for (uint8_t i = 0; i < STRING_SIZE; i++) {
    Serial.print(vector->cPlaintext[i]);
  }
  Serial.println();


  Serial.print("    String as HEX bytes: ");
  for (uint8_t i = 0; i < STRING_SIZE; i++) {
    Serial.print("0x");
    if (vector->bPlaintext[i] < 16) {
      Serial.print("0");
    }
    Serial.print(vector->bPlaintext[i], HEX);
    if (i < STRING_SIZE - 1) {
      Serial.print(", ");
    }
  }
  Serial.println();
}

void setup() {
  // put your setup code here, to run once:
  // Setup Serial

  Serial.begin(115200);
  while (!Serial) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
  }
  Serial.println("Serial ready");
  delay(500);



  pinMode(_PIN_DOOR_STATUS, INPUT_PULLUP);
  pinMode(_PIN_GARAGE_OCCUPATION, INPUT_PULLUP);
  pinMode(_PIN_LIGHTBARRIER, OUTPUT);
  Serial.println("GPIOs ready");
  delay(500);


  // Setup RF

  if (!radio.begin()) {
    Serial.println("RF module not connected");
    while (!radio.begin()) {}
  }
  Serial.println("RF module ready");
  delay(500);


  /* Set the data rate:
   * RF24_250KBPS: 250 kbit per second
   * RF24_1MBPS:   1 megabit per second (default)
   * RF24_2MBPS:   2 megabit per second
   */
  radio.setDataRate(RF24_2MBPS);
  /* Set the power amplifier level rate:
   * RF24_PA_MIN:   -18 dBm
   * RF24_PA_LOW:   -12 dBm
   * RF24_PA_HIGH:   -6 dBm
   * RF24_PA_MAX:     0 dBm (default)
   */
  radio.setPALevel(RF24_PA_LOW);  // sufficient for tests side by side
  /* Set the channel x with x = 0...125 => 2400 MHz + x MHz 
   * Default: 76 => Frequency = 2476 MHz
   * use getChannel to query the channel
   */
  radio.setChannel(0);

  radio.openWritingPipe(RADIO_ADDRESS);  // set the address
  radio.stopListening();                 // set as transmitter
  /* You can choose if acknowlegdements shall be requested (true = default) or not (false) */
  radio.setAutoAck(true);

  /* with this you are able to choose if an acknowledgement is requested for 
   * INDIVIDUAL messages.
   */
  radio.enableDynamicAck();


  /* setRetries(byte delay, byte count) sets the number of retries until the message is
   * successfully sent. 
   * Delay time = 250 µs + delay * 250 µs. Default delay = 5 => 1500 µs. Max delay = 15.
   * Count: number of retries. Default = Max = 15. 
   */
  radio.setRetries(5, 15);
  /* The default payload size is 32. You can set a fixed payload size which must be the
   * same on both the transmitter (TX) and receiver (RX)side. Alternatively, you can use 
   * dynamic payloads, which need to be enabled on RX and TX. 
   */
  //radio.setPayloadSize(11);
  radio.enableDynamicPayloads();



  // Generate new seed for more randomness for future
  // randomSeed(esp_random());
  randomSeed(analogRead(_PIN_UNUSED));

  Serial.println("I am the stationary device - ready!");
}

void loop() {
  // put your main code here, to run repeatedly:
  if (isDoorClosed() && !isGarageOccupied()) {

    // 1.   Generate new string
    generateNewString(&cipherVector);

    // 2.   Encrypt generated string
    if (handleCipher(&speckTiny, &cipherVector, KEY_SIZE, false)) {


      // 3.   Send encrypted string and check if there's an ACK
      if (radio.write(cipherVector.bPlaintext, STRING_SIZE)) {
        sendTime = millis();
        Serial.println("# String successfully sent");
        Serial.println("#");
        Serial.println("");


        // 4.   Wait until receiving a string or until timeout
        radio.setChannel(0);
        radio.openReadingPipe(RADIO_READINGPIPE, RADIO_ADDRESS);
        radio.startListening();
        radio.enableDynamicPayloads();

        while (!radio.available()) {
          if (isTimeout()) {
            Serial.println("############# TIMEOUT ############");
            sendTime = 0;
            break;
          }
          delay(1);
        }

        if (radio.available()) {
          byte len = radio.getDynamicPayloadSize();
          Serial.println(len);
          byte text[len] = "";
          radio.read(&text, len);
          displayReceivedData(text);

          if (memcmp(&text, cipherVector.bCiphertext, STRING_SIZE) == 0) {
            Serial.println("Strings match!");
            openDoor();

            // Give the door some time to open
            Serial.println("Give the door some time to open");
            delay(3000);

            unsigned long triggerTime = millis();

            // Wait until car is in the garage
            while (!isGarageOccupied()) {
              // If no car appears after a while close the door
              if(millis() - triggerTime > RF_AREA_CLEARING_TIME*1000) {
                Serial.println("No car appeared - close the door");
                break;
              }
              delay(1);
            }
            while (!isDoorClosed()) {
              delay(1);
            }
            reset();
          } else {
            Serial.println("Strings NOT match!");
          }
        }

      } else {
        Serial.println("# Error while sending string");
        Serial.println("#");
        Serial.println("");
      }
    }
  } else if (!isDoorClosed() && isGarageOccupied()) {
    // Car is in the garage and the door is (already) open
    Serial.println("Waiting for the car to leave the garage");
    while (isGarageOccupied()) {
      // Wait until the car left the garage or until the door is manually closed
      if(isDoorClosed()) {
        return;
      }
      delay(1);
    }
    // Car has left the garage => wait 10 more seconds until closing the door
    Serial.print("Waiting for the car to be away from the door - wait ");
    Serial.print(DOOR_AREA_CLEARING_TIME);
    Serial.println(" seconds");
    for (uint8_t i = DOOR_AREA_CLEARING_TIME; i > 0; i--) {
      Serial.print(i);
      Serial.print("...");
      delay(1000);
    }
    Serial.println();
    openDoor();

    // Wait until the door is closed
    while (!isDoorClosed()) {
      delay(1);
    }

    // Wait until the car is far enough away
    Serial.println("Wait until the car is far enough away");
    delay(RF_AREA_CLEARING_TIME*1000);
  }
  delay(5000);
}
