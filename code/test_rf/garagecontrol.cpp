#include "garagecontrol.h"

config::pin::pin() {}


config::config() {}

config::rf::rf() {};

sensor::sensor() {}

sensor::sensor(String topic) {
    mqttStateTopic = topic;
}

bool sensor::state() {
  return currentState;
}

void config::newPin(String pinName, uint16_t pinNumber, bool pinInverted) {
//   try {
    for(uint8_t p=0; p<sizeof(pins); p++) {
        if( CONFIG.pins[p].isValidPin() ) continue;

        pins[p] = pin(pinName, pinNumber, pinInverted);
    //   pins[p].name = pinName;
    //   pins[p].number = pinNumber;
    //   pins[p].inverted = pinInverted;
        return;
    }
    // throw pinNotFound(String("This pin was not found!"));
    return;
//   }
//   catch (String pinNotFound) {
//     String msg = "EXC### pin not found while initializing _" + pinName + "_";

//     Serial.println(msg);
//     return NULL;
//   }
//   return NULL;
}


config::pin* config::getPinByName(String pinName) {
//   try {
    for(uint8_t p=0; p<10; p++) {
      if( pins[p].getName() == pinName ) {
        pin* foundPin = &pins[p];
        //return &leds[l];
        return foundPin;
      }
    }
    // throw ledNotFound(String("This LED was not found!"));
    return NULL;
//   }
//   catch (String ledNotFound) {
//     String msg = "EXC### LED not found _" + ledName + "_";

//     Serial.println(msg);
//     helper::writeLogsToSdCard(msg);
//     return NULL;
//   }
//   return NULL;
}



config::pin::pin(String pinName, uint16_t pinNumber, bool pinInverted) {
    name = pinName;
    number = pinNumber;
    inverted = pinInverted;
}


uint8_t config::pin::getNumber() {
    return number;
}

String config::pin::getName() {
    return name;
}


bool config::pin::isInverted() {
    return inverted;
}


bool config::pin::isValidPin() {
  return ( number != 99 ? true : false );
}


void config::rf::setTimeout(uint16_t time) {
    timeout = time;
}


bool config::rf::getTimeout() {
    return timeout;
}

config::pin* getCSNPin() {
    CONFIG.getPinByName("rf_csn");
}
config::pin* getCEPin() {
    CONFIG.getPinByName("rf_ce");
}




