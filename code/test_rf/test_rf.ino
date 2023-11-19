#include "garagecontrol.h"



const byte RADIO_ADDRESS[6]           = "00001";
uint8_t RADIO_READINGPIPE             = 0;
uint8_t RADIO_CHANNEL                 = 0;
bool RADIO_DYNAMIC_PAYLOAD_SIZE       = false;



const uint8_t _PIN_RF_CSN             = 8;
const uint8_t _PIN_RF_CE              = 7;
const uint8_t _PIN_UNUSED             = A0;  // For random seed

const uint16_t TIMEOUT                = 3000; // milliseconds => time between sending the string and marking this try as failure because of no answer
const uint8_t WAIT_FOR_NEXT_RF_SENDING= 5;  // seconds



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
unsigned long sendTime = 0;



bool cipherSuccessful = false;
bool ackReceived = false;
bool answerReceived = false;
bool answerIsValid = false;


bool handleCipher(BlockCipher *, const struct CipherVector *, size_t, bool = true);
void generateNewString(const struct CipherVector *);
void displayKey(const struct CipherVector *);
void displayRawString(const struct CipherVector *);
void displayReceivedData(byte *, size_t = STRING_SIZE);
bool isTimeout();
void resetRF();
bool handleNewRFCommuncation();
void handleClosedDoorAndUnoccupiedGarage();
void handleOpenDoorAndOccupiedGarage();

bool handleNewRFCommuncation() {

  resetRF();

  // 1.   Generate new string
  generateNewString(&cipherVector);

  // Send generated string
  // Handle the RF communication
  if(handleCipher(&speckTiny, &cipherVector, KEY_SIZE, false)) cipherSuccessful = true;

  if(cipherSuccessful) {
    // 3.   Send encrypted string and check if there's an ACK
    
    sendTime = millis();
    if(radio.write(cipherVector.bPlaintext, STRING_SIZE)) {
      ackReceived = true;
      
      Serial.println(F("# String successfully sent"));
      Serial.println(F("#"));
      Serial.println(F(""));
    }

    if(ackReceived) {
      // 4.   Wait until receiving a string or until timeout
      Serial.println(F("Ack received"));
      radio.setChannel(0);
      radio.openReadingPipe(RADIO_READINGPIPE, RADIO_ADDRESS);
      radio.startListening();
      if (RADIO_DYNAMIC_PAYLOAD_SIZE) {
        radio.enableDynamicPayloads();
      }
      else {
        radio.setPayloadSize(STRING_SIZE);
      }

      while (!radio.available()) {
        if (isTimeout()) {
          Serial.println(F("############# TIMEOUT ############"));

          // Set all relevant settings for the RF module
          // Set it back to transmitter mode
          resetRF();
          return false;
        }
        delay(1);
      }

      byte len = RADIO_DYNAMIC_PAYLOAD_SIZE ? radio.getDynamicPayloadSize() : STRING_SIZE;
      byte text[STRING_SIZE] = "";
      
      if (radio.available()) {
        answerReceived = true;
        // byte len = RADIO_DYNAMIC_PAYLOAD_SIZE ? radio.getDynamicPayloadSize() : STRING_SIZE;
        // byte text[STRING_SIZE] = "";
        
        radio.read(&text, len);
        displayReceivedData(text);
      }

      if(answerReceived) {
        if (memcmp(&text, cipherVector.bCiphertext, STRING_SIZE) == 0) {
          // Receiver sent back a valid encrypted string
          answerIsValid = true;
          Serial.println("Strings match!");

          return true;
        }
        else {
          Serial.println(F("Strings NOT match!"));
          return false;
        }
      }
    }
    else {
      Serial.println(F("# Error while sending string"));
      Serial.println(F("#"));
      Serial.println(F(""));
    }
  }
  return false;
}

void resetRF() {
  Serial.println("resetRF()");
  sendTime = 0;

    /* Set the data rate:
   * RF24_250KBPS: 250 kbit per second
   * RF24_1MBPS:   1 megabit per second (default)
   * RF24_2MBPS:   2 megabit per second
   */
  radio.setDataRate(RF24_250KBPS);


  /* Set the power amplifier level rate:
   * RF24_PA_MIN:   -18 dBm
   * RF24_PA_LOW:   -12 dBm
   * RF24_PA_HIGH:   -6 dBm
   * RF24_PA_MAX:     0 dBm (default)
   */
  radio.setPALevel(RF24_PA_MAX);


  /* Set the channel x with x = 0...125 => 2400 MHz + x MHz 
   * Default: 76 => Frequency = 2476 MHz
   * use getChannel to query the channel
   */
  radio.setChannel(RADIO_CHANNEL);


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
  if (RADIO_DYNAMIC_PAYLOAD_SIZE) {
    radio.enableDynamicPayloads();
  } else {
    radio.setPayloadSize(STRING_SIZE);
  }
}

bool isTimeout() {
  return (millis() - sendTime > TIMEOUT) ? true : false;
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
   Serial.println(F("######## Encrypting string... ########"));

  // Display used key
  displayKey(vector);

  // Display unencrypted string
  displayRawString(vector);
  crypto_feed_watchdog();

  cipher->setKey(vector->bKey, keySize);
  cipher->encryptBlock(BUFFER, vector->bPlaintext);

  // Display encrypted string
  displayEncryptedString(vector);

   Serial.print(F("    "));
   Serial.print(vector->name);
   Serial.print(F(" Encryption "));

  if (memcmp(BUFFER, vector->bCiphertext, STRING_SIZE) == 0) {
    Serial.println(F("Passed"));
    return true;
  } else {
     Serial.println(F("Failed"));
    return false;
  }
}

void displayReceivedData(char *data, size_t dataSize = STRING_SIZE) {
   Serial.print(F("Received data as char: "));
   Serial.println(data);
   Serial.print(F("Received data as HEX: "));
  for (size_t i = 0; i < dataSize; i++) {
     Serial.print(F("0x"));
    if (data[i] < 16) {
       Serial.print(F("0"));
    }
     Serial.print(data[i], HEX);
    if (i < dataSize - 1) {
       Serial.print(F(", "));
    }
  }
   Serial.println(F(""));
}

void displayReceivedData(byte *data, size_t dataSize) {
   Serial.print(F("    Received data as char: '"));
  for (size_t i = 0; i < dataSize; i++) {
     Serial.print(data[i]);
  }
   Serial.println(F("'"));

   Serial.print(F("    Received data as HEX: '"));
  for (size_t i = 0; i < dataSize; i++) {
     Serial.print(F("0x"));
    if (data[i] < 16) {
       Serial.print(F("0"));
    }
     Serial.print(data[i], HEX);
    if (i < dataSize - 1) {
       Serial.print(F(", "));
    }
  }
   Serial.println(F("'"));
}

void displayEncryptedString(const struct CipherVector *vector) {
   Serial.print(F("    Encrypted string as HEX byte: "));

  for (uint8_t i = 0; i < STRING_SIZE; i++) {
    cipherVector.bCiphertext[i] = BUFFER[i];
     Serial.print(F("0x"));

    if (cipherVector.bCiphertext[i] < 16) {
       Serial.print(F("0"));
    }

     Serial.print(cipherVector.bCiphertext[i], HEX);
    if (i < STRING_SIZE - 1) {
       Serial.print(F(", "));
    }
  }
   Serial.println();
}

void displayKey(const struct CipherVector *vector) {
   Serial.print(F("    Key as plaintext: "));
   Serial.println(vector->cKey);
   Serial.print(F("    Key as HEX bytes: "));
  for (uint8_t i = 0; i < KEY_SIZE; i++) {
     Serial.print(F("0x"));

    if (vector->bKey[i] < 16) {
       Serial.print(F("0"));
    }

     Serial.print(vector->bKey[i], HEX);
    if (i < KEY_SIZE - 1) {
       Serial.print(F(", "));
    }
  }
   Serial.println();
}

void displayRawString(const struct CipherVector *vector) {
   Serial.print(F("    String as plaintext: "));
  for (uint8_t i = 0; i < STRING_SIZE; i++) {
     Serial.print(vector->cPlaintext[i]);
  }
   Serial.println();


   Serial.print(F("    String as HEX bytes: "));
  for (uint8_t i = 0; i < STRING_SIZE; i++) {
     Serial.print(F("0x"));
    if (vector->bPlaintext[i] < 16) {
       Serial.print(F("0"));
    }
     Serial.print(vector->bPlaintext[i], HEX);
    if (i < STRING_SIZE - 1) {
       Serial.print(F(", "));
    }
  }
   Serial.println();
}



void setup() {
  // put your setup code here, to run once:
  // Setup Serial

  Serial.begin(115200);
  while (!Serial) {}
  Serial.println(F("Serial ready"));
  delay(500);



  // Setup RF

  if (!radio.begin()) {
    Serial.println(F("RF module not connected"));
    while (!radio.begin()) {}
  }
  Serial.println(F("RF module ready"));
  delay(500);

  // Set up all relevant settings for the RF module
  resetRF();
}




void loop() {

  handleNewRFCommuncation();
  delay(5000);


}