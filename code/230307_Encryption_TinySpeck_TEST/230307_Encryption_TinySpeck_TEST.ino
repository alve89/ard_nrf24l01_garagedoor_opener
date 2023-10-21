#include <Crypto.h>
#include <SpeckTiny.h>
#include <string.h>


const size_t KEY_SIZE    = 32;
const size_t STRING_SIZE = 16;
const uint8_t _PIN_UNUSED = A0;
const uint8_t _PIN_RELAY  = 1;
const uint8_t _PIN_RF_CE  = 7;
const uint8_t _PIN_RF_CSN = 8; 






// END OF CONFIGURATION PART


struct CipherVector {
  const char *name;
  const char *cKey;
  byte bKey[KEY_SIZE];
  byte bPlaintext[STRING_SIZE];
  byte ciphertext[STRING_SIZE];
  char cPlaintext[STRING_SIZE];
};

// Define the vector vectors from http://eprint.iacr.org/2013/404
static CipherVector cipherVector = {
    .name        = "Speck-128-ECB",
    .cKey         = "bnbUy90ErlgzkpSrBYWRKhlc5L85Q7BX",
    .bKey         = {0x62, 0x6E, 0x62, 0x55, 0x79, 0x39, 0x30, 0x45, 0x72, 0x6C, 0x67, 0x7A, 0x6B, 0x70, 0x53, 0x72, 0x42, 0x59, 0x57, 0x52, 0x4B, 0x68, 0x6C, 0x63, 0x35, 0x4C, 0x38, 0x35, 0x51, 0x37, 0x42, 0x58},
    .bPlaintext   = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    .ciphertext  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};


SpeckTiny speckTiny;
byte buffer[STRING_SIZE];
const char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

void handleCipher(BlockCipher *, const struct CipherVector *, size_t, bool = true);
void generateNewString(const struct CipherVector*, size_t);
void displayKey(const struct CipherVector*);
void displayRawString(const struct CipherVector*);




/*
  Receiver generates random string
  Receiver waits for incoming greeting
  Sender waits for button push
  Sends sends greeting
  Sender waits for incoming string
  Receiver sends plaintext string
  If sender receives string
    Sender encrypts string
    Sender sends encrypted string to receiver
    Receiver compares both encrypted strings (bytewise)
    Receiver opens door or not
  Else
    Start over


*/


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while(!Serial);
  Serial.println("Device ready");

  // Generate new seed for more randomness for future 
  randomSeed(analogRead(_PIN_UNUSED));

  // Serial.println("Key used for encryption");
  // displayKey(&cipherVector);


}








void loop() {
  // put your main code here, to run repeatedly:


  // Generate new string
  generateNewString(&cipherVector);
  displayRawString(&cipherVector);
  
  // Encrypt generated string
  handleCipher(&speckTiny, &cipherVector, KEY_SIZE, false);


  Serial.println();
  Serial.println();
  Serial.println();
  delay(20000);
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
    cipherVector.ciphertext[i] = buffer[i];
    Serial.print("0x");

    if(cipherVector.ciphertext[i] < 16) {
      Serial.print("0");  
    } 

    Serial.print(cipherVector.ciphertext[i], HEX);
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

void handleCipher(BlockCipher *cipher, const struct CipherVector *vector, size_t keySize, bool decryption) {
  Serial.println("######## Encrypting string... ########");
  displayKey(vector);

  // Serial.print(F("Used string: ")); Serial.println(vector->cPlaintext);
  displayRawString(vector);
  crypto_feed_watchdog();

  cipher->setKey(vector->bKey, keySize);
  cipher->encryptBlock(buffer, vector->bPlaintext);


  displayEncryptedString(vector);


  Serial.print(vector->name);
  Serial.print(" Encryption ... ");




  if (memcmp(buffer, vector->ciphertext, STRING_SIZE) == 0)
      Serial.println("Passed");
  else
      Serial.println("Failed");

  if (!decryption)
      return;

  Serial.print(vector->name);
  Serial.print(" Decryption ... ");
  cipher->decryptBlock(buffer, vector->ciphertext);
  if (memcmp(buffer, vector->bPlaintext, 16) == 0)
      Serial.println("Passed");
  else
      Serial.println("Failed");


  Serial.println();

  Serial.print("Ciphertext: ");
  for(size_t i=0; i< sizeof(vector->ciphertext); i++) {
    Serial.print("0x");Serial.print(vector->ciphertext[i], HEX);Serial.print(", ");
  }
  Serial.println();

  Serial.print("Ciphertext buffer: "); 
  for(size_t i=0; i< sizeof(buffer); i++) {
    if(i > 0) {
      Serial.print(", ");
    }
    Serial.print("0x");
    if(buffer[i] < 16) {
      Serial.print("0");  
    } 
    Serial.print(buffer[i], HEX);
    

  }
  Serial.println();
  Serial.println();
}



