#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <SHA256.h>
#include <CryptoCstm.h>
#include <SpeckTiny.h>
#include <string.h>
// #include <Base64.h>


const byte RADIO_ADDRESS[6]             = "00001";
uint8_t RADIO_READINGPIPE               = 0;
uint8_t RADIO_CHANNEL                   = 0;
const size_t KEY_SIZE                   = 32;
const size_t STRING_SIZE                = 16;
const uint8_t _PIN_RF_CE                = 7; //D4;
const uint8_t _PIN_RF_CSN               = 8; //D2; 
// const uint8_t _PIN_BUTTON               = 4;


struct CipherVector {
  const char *name;
  const char *cKey;
  byte bKey[KEY_SIZE];
  byte bPlaintext[STRING_SIZE+1];
  byte bCiphertext[STRING_SIZE+1];
  char cPlaintext[STRING_SIZE+1];
};

// Define the vector vectors from http://eprint.iacr.org/2013/404
static CipherVector cipherVector = {
    .name        = "Speck-128-ECB",
    .cKey         = "bnbUy90ErlgzkpSrBYWRKhlc5L85Q7BX",
    .bKey         = {0x62, 0x6E, 0x62, 0x55, 0x79, 0x39, 0x30, 0x45, 0x72, 0x6C, 0x67, 0x7A, 0x6B, 0x70, 0x53, 0x72, 0x42, 0x59, 0x57, 0x52, 0x4B, 0x68, 0x6C, 0x63, 0x35, 0x4C, 0x38, 0x35, 0x51, 0x37, 0x42, 0x58},
    .bPlaintext   = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    .bCiphertext  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    .cPlaintext    = "1234567890123456"
};


SpeckTiny speckTiny;
RF24 radio(_PIN_RF_CE, _PIN_RF_CSN); // CE, CSN
byte BUFFER[STRING_SIZE];
const char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";






bool handleCipher(BlockCipher *, const struct CipherVector *, size_t, bool = true);
void displayKey(const struct CipherVector*);
void displayRawString(const struct CipherVector*);
void displayReceivedData(byte*, size_t = STRING_SIZE);
void displayEncryptedString(const struct CipherVector*);




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
  }
  else {
    Serial.println("Failed");
    return false;
  }
}

void displayEncryptedString(const struct CipherVector* vector) {

  Serial.print("    Encrypted string as char: '");
  for(uint8_t i=0; i<STRING_SIZE; i++) {
    Serial.print(cipherVector.bCiphertext[i]);
  }
  Serial.println("'");

  Serial.print("    Encrypted string as HEX: '");
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
  Serial.println("'");
  Serial.println();
}

void displayKey(const struct CipherVector* vector) {
  Serial.print(F("    Key as plaintext: ")); Serial.println(vector->cKey);
  Serial.print(F("    Key as HEX bytes: "));
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
  Serial.println();
}

void displayRawString(const struct CipherVector* vector) {
  Serial.print("    String as plaintext: '");
  for(uint8_t i=0; i<STRING_SIZE; i++) {
    Serial.print(vector->cPlaintext[i]);
  }
  Serial.println("'");


  Serial.print("    String as HEX bytes: '");
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
  Serial.println("'");
  Serial.println();
}

void displayReceivedData(char *data, size_t dataSize = STRING_SIZE) {
  Serial.print("Received data as char: ");
  Serial.println(data);
  Serial.print("Received data as HEX: ");
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
  Serial.println("");
}

void reset() {
  Serial.println("reset(): Set as Receiver.");

  /* Set the data rate:
   * RF24_250KBPS: 250 kbit per second
   * RF24_1MBPS:   1 megabit per second
   * RF24_2MBPS:   2 megabit per second
   */
  radio.setDataRate(RF24_2MBPS);

  /* Set the power amplifier level rate:
   * RF24_PA_MIN:   -18 dBm
   * RF24_PA_LOW:   -12 dBm
   * RF24_PA_HIGH:   -6 dBm
   * RF24_PA_MAX:     0 dBm (default)
   */
  radio.setPALevel(RF24_PA_LOW); // sufficient for tests side by side

   /* Set the channel x with x = 0...125 => 2400 MHz + x MHz 
   * Default: 76 => Frequency = 2476 MHz
   * use getChannel to query the channel
   */
  radio.setChannel(RADIO_CHANNEL);

  // Set the address
  radio.openReadingPipe(RADIO_READINGPIPE, RADIO_ADDRESS); 

  // Set as receiver
  radio.startListening();
  /* The default payload size is 32. You can set a fixed payload size which 
   * must be the same on both the transmitter (TX) and receiver (RX)side. 
   * Alternatively, you can use dynamic payloads, which need to be enabled 
   * on RX and TX. 
   */
  // radio.enableDynamicPayloads();
  radio.setPayloadSize(STRING_SIZE);

  // Go!
  radio.startListening();
}




void setup() {
  Serial.begin(115200);
  while(!Serial) {}
  Serial.println("Serial ready");
  if(!radio.begin()){
    Serial.println("nRF24L01 module not connected!");
    while(1){}
  }
  else 
    Serial.println("nRF24L01 module connected!");

  // Setup the RF module
  reset();

}






void loop() {
  if(radio.available()){
    byte len = radio.getDynamicPayloadSize();
//    Serial.println(len); //just for information
    char text[STRING_SIZE+1] = {0}; 
    radio.read(&text, STRING_SIZE);
//    Serial.println(text);
    displayReceivedData(text);
    Serial.println();
    for(uint8_t i=0; i<STRING_SIZE; i++) {
      cipherVector.cPlaintext[i] = text[i];
      cipherVector.bPlaintext[i] = (byte) text[i];
    }
//    displayRawString(&cipherVector);
    if(handleCipher(&speckTiny, &cipherVector, KEY_SIZE, false)) {
      radio.setChannel(RADIO_CHANNEL);
      radio.openWritingPipe(RADIO_ADDRESS);
      radio.stopListening();
      radio.setAutoAck(true);



  // int encodedLength = Base64.encodedLength(STRING_SIZE);
  // char encodedString[encodedLength];
  // Base64.encode(encodedString, cipherVector.bCiphertext, STRING_SIZE);
  // Serial.print("BASE64: "); Serial.println(encodedString);


  // int decodedLength = Base64.decodedLength(encodedString, encodedLength);
  // char decodedString[decodedLength];
  // Base64.decode(decodedString, encodedString, encodedLength);

  // Serial.print("Decoded: "); Serial.println(decodedString);



      if(radio.write(cipherVector.bCiphertext, STRING_SIZE)) {
        Serial.println("# String successfully sent");
        Serial.println("#");
        Serial.println("");
      }
      else {
        Serial.println("# Error while sending string");
        Serial.println("#");
        Serial.println("");
      }
    }
    reset();
    
  }
}

