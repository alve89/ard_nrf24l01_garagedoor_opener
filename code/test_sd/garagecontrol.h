#ifndef garagedooropener_h
    #define garagedooropener_h

#include <Arduino.h>
#include<avr/wdt.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <SD.h>
#include <SHA256.h>
#include <CryptoCstm.h>
#include <SpeckTiny.h>
#include <string.h>
#include <Ethernet.h>
#include <MQTT.h>


#define KEY_SIZE            32
#define STRING_SIZE         16
#define SENDING             1
#define ACK                 2
#define NO_ACK              4
#define DFLT                0
#define BY_RF               1
#define BY_MQTT             2
#define BY_SENSORS          3
#define BY_BUTTON           4
#define BY_KEY              5
#define ACT_OPEN_DOOR       1
#define ACT_CLOSE_DOOR      2
#define ACT_STOP            3


const uint16_t _STATUS_SENDING_LED_DURAT  = 1000;
const uint16_t _STATUS_NO_ACK_LED_DURAT  = 1000;
const uint16_t _STATUS_ACK_LED_DURAT  = 1000;

const char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

struct CipherVector {
  const char *name;
  const char *cKey;
  byte bKey[KEY_SIZE];
  byte bPlaintext[STRING_SIZE];
  byte bCiphertext[STRING_SIZE];
  char cPlaintext[STRING_SIZE];
};









class sensor {
  public:
    sensor();
    sensor(String);
    bool hasStateChanged();
    bool state();
    bool lastState = false;
    bool currentState = false;


    String mqttStateTopic;
    String mqttPayload;
};

String pinNotFound(String);


class config {
    public:
        class mqtt;
        class pin;
        class rf;


        class mqtt {
            public:
                uint16_t maxPayloadSize = 1024; //MAX_MQTT_PAYLOAD_SIZE
                String hostAddress = "homecontrol.lan.k4";
                String user = "test";
                String pwd = "qoibOIBbfoqib38bqucv3u89qv";
                String clientId = "garage_sensors";


                String getHostAddress() {
                    return hostAddress;
                }

                String getClientId() {
                    return clientId;
                }

                String getUser() {
                    return user;
                }

                String getPwd() {
                    return pwd;
                }


        };

        class rf {
            public:
                uint16_t timeout          = 3000; // milliseconds => time between sending the string and marking this try as failure because of no answer
                const byte address           = "00001";
                uint8_t readingPipe             = 0;
                uint8_t channel                 = 0;
                unsigned long sendTime          = 0;
                bool dynamicPayloadSize         = false;
                uint8_t waitForNextRFSending    = 5;
                unsigned long maxInitTime       = 5000;
                bool state = false;
                bool lastState = false;
                config::pin* csn;
                config::pin* ce;


                void setTimeout(uint16_t);
                uint16_t getTimeout();

                config::pin* getCSNPin();
                config::pin* getCEPin();



                rf();
                void init();
                bool stateChanged();
        };

        class pin {
            public:
                uint16_t number = 99;
                String name = "noNameAsDefault";
                bool inverted = false;
                bool state = false;

                pin();
                pin(String, uint16_t, bool);

                bool isValidPin();
                bool isInverted();
                uint8_t getNumber();
                String getName();
                bool getState();
        };


        config();

        bool use_logging = true;
        bool use_network = true;
        bool testing_only = false;
        bool dhcpInitSuccessful = false;
        String logfile = "log.txt";
        uint8_t doorAreaClearingTime = 25; // seconds
        uint8_t RFAreaClearingTime = 30; // seconds
        uint8_t rebootCycle = 2; // hours
        float ldrTolerance         = 30; // integer, will be transformed to percentage
        unsigned long bootingTime = 0;
        pin pins[20];
        pin* getPinByName(String);

        rf RF;
        mqtt MQTT;

        
        void newPin(String, uint16_t, bool = false);




}; 

extern config CONFIG;
extern EthernetClient net;
extern MQTTClient client;
extern SpeckTiny speckTiny;
extern RF24 radio;

extern String changedTopic;
extern String changedTopicPayload;
extern String garageDoorCommandTopic;
extern String garageDoorCommandPayloadOpen;
extern String garageDoorCommandPayloadClose;
extern String garageDoorCommandPayloadStop;
extern String garageDoorLogTopic;
extern String garageButtonLogTopic;
extern String garageKeyLogTopic;

extern bool cipherSuccessful;
extern bool ackReceived;
extern bool answerReceived;
extern bool answerIsValid;

extern bool triggerDetected;

extern byte triggerType;
extern byte triggeredAction;
extern String triggerName;

extern unsigned long timerForLeavingCar;
extern unsigned long lightbarrierEnabledTime;
extern unsigned long lightbarrierDisabledTime;

extern unsigned long lastOccupancyTime;
extern unsigned long lastOpeningTime;
extern unsigned long lastClosingTime;
extern unsigned long lastOpenedTime;
extern unsigned long lastClosedTime;

extern bool lightbarrierStatus;
extern float lightbarrierValue1;
extern float lightbarrierValue2;

extern unsigned long delayTime;

extern sensor isGarageDoorClosed;
extern sensor isGarageDoorOpen;
extern sensor garageOccupancy;

extern CipherVector cipherVector;

extern byte BUFFER[STRING_SIZE];
extern File logFile;


bool handleCipher(BlockCipher *, const struct CipherVector *, size_t, bool = true);
void generateNewString(const struct CipherVector *);
void displayKey(const struct CipherVector *);
void displayRawString(const struct CipherVector *);
void displayReceivedData(byte *, size_t = STRING_SIZE);
void openDoor(uint8_t = CONFIG.getPinByName("relay")->getNumber(), bool = false);
void closeDoor(uint8_t = CONFIG.getPinByName("relay")->getNumber(), bool = false);
void triggerDoorRelay(uint8_t = CONFIG.getPinByName("relay")->getNumber(), bool = false);
bool isDoorClosed();
bool isGarageOccupied();
bool isTimeout(unsigned long, unsigned long);
void resetRF();
void toggleStatusLed(uint8_t, uint16_t = 5000);
void messageReceived(String&, String&);
void checkIfSensorsChanged();
bool handleNewRFCommuncation();
void checkConfig();
void reboot(String);



void publishConfig();
void connect();
bool isDoorOpen();
void checkChangedMqttTopic();
void newTriggerDetected(byte, byte);

void isrKey();
void isrButton();

void handleGarage();

void displayEncryptedString(const struct CipherVector*);

void log(String);



#endif