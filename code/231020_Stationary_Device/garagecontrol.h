#ifndef garagedooropener_h
    #define garagedooropener_h

#include <Arduino.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <SHA256.h>
#include <CryptoCstm.h>
#include <SpeckTiny.h>
#include <string.h>
#include <Ethernet.h>
#include <MQTT.h>


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
                String hostAddress = "test"; // homecontrol.lan.k4
                String user = "{ 0 }"; //"test";
                String pwd = "{ 0 }"; // "qoibOIBbfoqib38bqucv3u89qv";
                String clientId = "{ 0 }"; //"garage_sensors";
                char buffer[37];

                void resetBuffer() {
                    for(uint8_t i=0; i<sizeof(buffer); i++) {
                        buffer[i] = {0};
                    }
                }

                char* getHostAddress() {
                    resetBuffer();
                    hostAddress.toCharArray(buffer, hostAddress.length());
                    return buffer;
                }

                char* getClientId() {
                    resetBuffer();
                    clientId.toCharArray(buffer, clientId.length());
                    return buffer;
                }

                char* getUser() {
                    resetBuffer();
                    user.toCharArray(buffer, user.length());
                    return buffer;
                }

                char* getPwd() {
                    resetBuffer();
                    pwd.toCharArray(buffer, pwd.length());
                    return buffer;
                }


        };

        class rf {
            public:
                uint16_t timeout          = 3000; // milliseconds => time between sending the string and marking this try as failure because of no answer
                uint64_t address           = "00001";
                uint8_t readingPipe             = 0;
                uint8_t channel                 = 0;
                bool dynamicPayloadSize         = false;
                uint8_t waitForNextRFSending = 5;
                config::pin* csn;
                config::pin* ce;


                void setTimeout(uint16_t);
                bool getTimeout();
                rf();
        };

        class pin {
            public:
                uint8_t number;
                String name;
                bool inverted = false;

                pin(String = "noNamePin", uint8_t = 99, bool = false);

                bool isValidPin();
                bool isInverted();
                uint8_t getNumber();
                String getName();
        };


        config();


        bool use_network = true;
        bool testing_only = false;
        uint8_t doorAreaClearingTime = 25; // seconds
        uint8_t RFAreaClearingTime = 30; // seconds
        float ldrTolerance         = 30; // integer, will be transformed to percentage
        pin pins[10];
        pin* getPinByName(String);

        rf RF;
        mqtt MQTT;

        
        void newPin(String, uint8_t, bool = false);




};

extern config CONFIG;
extern EthernetClient net;
extern MQTTClient client;



#endif