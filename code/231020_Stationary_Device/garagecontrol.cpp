#include "garagecontrol.h"




const byte RADIO_ADDRESS[6]           = "00001";




config::pin::pin() {}


config::config() {}

config::rf::rf() {};

sensor::sensor() {}


void config::rf::init() {
  unsigned long rfInitStartTime = millis();
  lastState = state;
  if (!radio.begin()) {
    while (true) {
      if( radio.begin() ) {
        state = true;
        break;
      }
      if( isTimeout(rfInitStartTime, maxInitTime)) {
        state = false;
        break;
      }
    }
  }
  else {
    state = true;
  }
  
  if( stateChanged() ) {
    Serial.print(F("State changed! New state: ")); Serial.println(state);
  }
}

bool config::rf::stateChanged() {

  return ( state != lastState ) ? true : false;

}



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


bool config::pin::getState() {
    bool tempState = digitalRead(number);
    state = isInverted() ? !tempState : tempState;
    return state;
}


void config::rf::setTimeout(uint16_t newTimeout) {
    timeout = newTimeout;
}

uint16_t config::rf::getTimeout() {
    return timeout;
}

config::pin* getCSNPin() {
    CONFIG.getPinByName("rf_csn");
}

config::pin* getCEPin() {
    CONFIG.getPinByName("rf_ce");
}





/// @brief Callback function that is called when a new message was published in a subscribed MQTT topic
/// @param topic Subscribed topic by MQTTT::subscribe()
/// @param payload Received payload
void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  changedTopic = topic;
  changedTopicPayload = payload;


  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
}

void toggleStatusLed(uint8_t led, uint16_t duration) {
  uint8_t ledPin;
  uint16_t ledDuration;
 switch(led) {
    case SENDING:
      ledPin = CONFIG.getPinByName("ledSending")->getNumber();
      ledDuration = _STATUS_SENDING_LED_DURAT;
      break;
    case ACK:
      ledPin = CONFIG.getPinByName("ledAck")->getNumber();
      ledDuration = _STATUS_ACK_LED_DURAT;
      break;
    case NO_ACK:
      ledPin = CONFIG.getPinByName("ledNoAck")->getNumber();
      ledDuration = _STATUS_NO_ACK_LED_DURAT;
      break;
    default:
      ledPin = led;
      ledDuration = duration;
      break;
  }

  digitalWrite(ledPin, HIGH);
  delay(1000);
  digitalWrite(ledPin, LOW);
}

void publishConfig() {
  
  String garage_door = F("{"
  "  \"name\":\"Garage Door\","
  "  \"device_class\":\"garage\","
  "  \"retain\": true,"
  "  \"payload_open\": \"OPEN\","
  "  \"payload_close\": \"CLOSE\","
  "  \"state_topic\":\"homeassistant/cover/garage_door/state\","
  "  \"command_topic\": \"homeassistant/cover/garage_door/set\","
  "  \"state_closed\": \"closed\","
  "  \"state_open\": \"open\","
  "  \"state_opening\": \"opening\","
  "  \"state_closing\": \"closing\","
  "  \"unique_id\":\"garage_door\","
  "  \"device\": {"
  "    \"identifiers\":["
  "      \"garage_sensors/config\""
  "      ],"
  "    \"name\":\"Garage Sensors\""
  "  }"
  "}");

    String garage_occupancy = F("{"
  "   \"retain\":true,"  
  "  \"name\":\"Garage Occupancy\","
  "  \"device_class\":\"occupancy\","
  "  \"state_topic\":\"homeassistant/binary_sensor/garage_occupancy/state\","
  "  \"unique_id\":\"garage_occupancy\","
  "  \"action_topic\":\"homeassistant/binary_sensor/garage_occupancy/action\","
  "  \"device\": {"
  "    \"identifiers\":["
  "      \"garage_sensors/config\""
  "      ],"
  "    \"name\":\"Garage Sensors\""
  "  }"
  "}");

  String garage_sensors_action = F("{"
  "   \"retain\":true,"  
  "   \"name\":\"Garage Current Action\","
  "   \"state_topic\":\"homeassistant/sensor/garage_current_action/state\","
  "   \"unique_id\":\"garage_current_action\","
  "   \"device\": {"
  "     \"identifiers\":["
  "       \"garage_sensors/config\""
  "       ],"
  "      \"name\":\"Garage Sensors\""
  "   }"
  "}" );

    String garage_button_log = F("{"
  "   \"retain\":true,"  
  "   \"name\":\"Garage Button Log\","
  "   \"state_topic\":\"homeassistant/sensor/garage_button_log/state\","
  "   \"unique_id\":\"garage_button_log\","
  "   \"device\": {"
  "     \"identifiers\":["
  "       \"garage_sensors/config\""
  "       ],"
  "      \"name\":\"Garage Sensors\""
  "   }"
  "}" );

    String garage_key_log = F("{"
  "   \"retain\":true,"  
  "   \"name\":\"Garage Key Log\","
  "   \"state_topic\":\"homeassistant/sensor/garage_key_log/state\","
  "   \"unique_id\":\"garage_key_log\","
  "   \"device\": {"
  "     \"identifiers\":["
  "       \"garage_sensors/config\""
  "       ],"
  "      \"name\":\"Garage Sensors\""
  "   }"
  "}" );
  
      String garage_rf_state = F("{"
  "   \"retain\":true,"  
  "   \"name\":\"Garage RF State\","
  "   \"state_topic\":\"homeassistant/binary_sensor/garage_rf_state/state\","
  "   \"unique_id\":\"garage_rf_state\","
  "   \"device\": {"
  "     \"identifiers\":["
  "       \"garage_sensors/config\""
  "       ],"
  "      \"name\":\"Garage Sensors\""
  "   }"
  "}" );
 

  client.publish(F("homeassistant/binary_sensor/garage_occupancy/config"), garage_occupancy);
  client.publish(F("homeassistant/cover/garage_door/config"), garage_door);
  client.publish(F("homeassistant/sensor/garage_current_action/config"), garage_sensors_action);
  
  client.publish(F("homeassistant/sensor/garage_button_log/config"), garage_button_log);
  client.publish(F("homeassistant/sensor/garage_key_log/config"), garage_key_log);

  client.publish(F("homeassistant/binary_sensor/garage_rf_state/config"), garage_rf_state);
} 

void connect() {

  // TODO: Check if initializers work (added after last test), see also INO L243
  const char userTemp[CONFIG.MQTT.getUser().length()+1] = "";
  const char pwdTemp[CONFIG.MQTT.getPwd().length()+1] = "";
  const char clientIdTemp[CONFIG.MQTT.getClientId().length()+1] = "";

  CONFIG.MQTT.getUser().toCharArray(userTemp, CONFIG.MQTT.getUser().length()+1);
  CONFIG.MQTT.getPwd().toCharArray(pwdTemp, CONFIG.MQTT.getPwd().length()+1);
  CONFIG.MQTT.getClientId().toCharArray(clientIdTemp, CONFIG.MQTT.getClientId().length()+1);

  unsigned long connectingStartTime = millis();
  while(!client.connect(clientIdTemp, userTemp, pwdTemp)) {
    Serial.println(client.lastError());
    if(isTimeout(connectingStartTime, 10000)) reboot();
  }

  Serial.println(F("Connected to MQTT server"));

}

void resetRF() {
   Serial.println("resetRF()");
  CONFIG.RF.sendTime = 0;

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
  radio.setChannel(CONFIG.RF.channel);

  
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
  if (CONFIG.RF.dynamicPayloadSize) {
    radio.enableDynamicPayloads();
  } else {
    radio.setPayloadSize(STRING_SIZE);
  }
}

bool isTimeout(unsigned long timeToCheck, unsigned long timeoutTime) {
  return (millis() - timeToCheck > timeoutTime) ? true : false;
}

bool isGarageOccupied() {
  bool status = false;

  if(CONFIG.testing_only) {
    Serial.print(F("isGarageOccupied_testing(): "));
    status = CONFIG.getPinByName("garageOccupancy")->getState();
    Serial.println(status);
    return status;
  }
  
  // Since we don't know when the sensor got light for the last time,
  // give it some cool down time (1s)
  if(millis() - lightbarrierDisabledTime > 1000) {
  }
  else {
    // Serial.println(F("Canceled due to DisabledTime"));
    return garageOccupancy.state();
  }

  if( !lightbarrierStatus ) {
    // First measurement when the laser is off
    lightbarrierValue1 = analogRead(CONFIG.getPinByName("garageOccupancy")->getNumber());
    delay(10);

    // Now turn the laser on
    digitalWrite(CONFIG.getPinByName("laser")->getNumber(), !CONFIG.getPinByName("laser")->isInverted());
    lightbarrierEnabledTime = millis();
    lightbarrierStatus = true;
  }
  
  // Give the sensor a second to adapt to the new brightness
  if(millis() - lightbarrierEnabledTime > 2000) {
  }
  else {
    // Serial.println(F("Canceled due to EnabledTime"));
    return garageOccupancy.state();
  }

  lightbarrierValue2 = analogRead(CONFIG.getPinByName("garageOccupancy")->getNumber());

  // Turn laser off
  digitalWrite(CONFIG.getPinByName("laser")->getNumber(), CONFIG.getPinByName("laser")->isInverted());
  lightbarrierDisabledTime = millis();
  lightbarrierStatus = false;
  
  //  Serial.print("lightbarrierValue2 (");  Serial.print(lightbarrierValue2);  Serial.print(") >= lightbarrierValue1 (");  Serial.print(lightbarrierValue1);  Serial.print(") x 0.8 ("); Serial.print(lightbarrierValue1*0.8); Serial.print("): ");  Serial.println((lightbarrierValue2 >= (lightbarrierValue1 * 0.8)));
  //  Serial.print("lightbarrierValue2 (");  Serial.print(lightbarrierValue2);  Serial.print(") <= lightbarrierValue1 (");  Serial.print(lightbarrierValue1);  Serial.print(") x 1.2 ("); Serial.print(lightbarrierValue1*1.2); Serial.print("): ");  Serial.println((lightbarrierValue2 <= (lightbarrierValue1 * 1.2)));

  float bottomLimit = lightbarrierValue1 * (1 - CONFIG.ldrTolerance/100);
  float topLimit = lightbarrierValue1 * (1 + CONFIG.ldrTolerance/100);

  // Compare both values
  if( (lightbarrierValue2 >= bottomLimit)
  &&  (lightbarrierValue2 <= topLimit)) {
    // The sensors value didn't change during measurement => no change in the occupation status
    // => garage is occupied
    status = true;
  }
  else if(lightbarrierValue2 == lightbarrierValue1) {
    status = true;
  }
  else {
    // The sensors value changed by more than X percent
    // => this means, the laser hit the LDR before the second measurement
    // => garage is not occupied
    status = false;
  }

  // Serial.print(F("isGarageOccupied(): "));
  // Serial.print(status); // Serial.print(" [");  Serial.print(value1);  Serial.print(" / ");  Serial.print(value2);  Serial.println("]");
  // Serial.println();

  return status;
}

bool isDoorClosed() {
  // Serial.print(F("isDoorClosed(): "));
  bool status = CONFIG.getPinByName("doorClosed")->getState();
  // Serial.println(status);

  return status;
}

bool isDoorOpen() {
//  Serial.print(F("isDoorOpen(): "));
  bool status = CONFIG.getPinByName("doorOpen")->getState();
//  Serial.println(status);

  return status;
}

void triggerDoorRelay(uint8_t relayPin, bool inverted)  {
  digitalWrite(relayPin, !inverted);
  delay(1000);
  digitalWrite(relayPin, inverted);
}

void openDoor(uint8_t relayPin, bool inverted) {
  triggerDoorRelay(relayPin, inverted);
}

void closeDoor(uint8_t relayPin, bool inverted) {
  Serial.println(F("closeDoor()"));
  // client.publish(F("homeassistant/cover/garage_door/state"), F("closing"));
  triggerDoorRelay(relayPin, inverted);
}



void checkConfig() {
  Serial.print(F("checkConfig(): "));
  CONFIG.testing_only = CONFIG.getPinByName("enableTesting")->getState();
  CONFIG.use_network = CONFIG.getPinByName("disableNetwork")->getState();
  Serial.print(F("CONFIG.testing_only:")); Serial.print(CONFIG.testing_only); Serial.print(F(" / ")); Serial.print(F("CONFIG.use_network: ")); Serial.println(CONFIG.use_network);
  delay(2000);
}

bool handleNewRFCommuncation() {

  resetRF();

  // 1.   Generate new string
  generateNewString(&cipherVector);

  // Send generated string
  // Handle the RF communication
  if(handleCipher(&speckTiny, &cipherVector, KEY_SIZE, false)) cipherSuccessful = true;

  if(cipherSuccessful) {
    // 3.   Send encrypted string and check if there's an ACK
    if(CONFIG.use_network) client.publish(garageDoorLogTopic, F("Send encrypted string by RF, wait for ack"));
    CONFIG.RF.sendTime = millis();
    if(radio.write(cipherVector.bPlaintext, STRING_SIZE)) {
      ackReceived = true;
      if(CONFIG.use_network) client.publish(garageDoorLogTopic, F("Ack successful, wait for answer"));
      Serial.println(F("# String successfully sent"));
      Serial.println(F("#"));
      Serial.println(F(""));
    }

    if(ackReceived) {
      // 4.   Wait until receiving a string or until timeout
      radio.setChannel(0);
      radio.openReadingPipe(CONFIG.RF.readingPipe, RADIO_ADDRESS);
      radio.startListening();
      if (CONFIG.RF.dynamicPayloadSize) {
        radio.enableDynamicPayloads();
      }
      else {
        radio.setPayloadSize(STRING_SIZE);
      }

      while (!radio.available()) {

        if (isTimeout(CONFIG.RF.sendTime, CONFIG.RF.getTimeout())) {
          Serial.println(F("############# TIMEOUT ############"));
          if(CONFIG.use_network) client.publish(garageDoorLogTopic, F("Answer timed out"));
          
          // Set all relevant settings for the RF module
          // Set it back to transmitter mode
          resetRF();
          return false;
        }
      }

      byte len = CONFIG.RF.dynamicPayloadSize ? radio.getDynamicPayloadSize() : STRING_SIZE;
      byte text[STRING_SIZE] = "";
      
      if (radio.available()) {
        answerReceived = true;
        // byte len = CONFIG.RF.dynamicPayloadSize ? radio.getDynamicPayloadSize() : STRING_SIZE;
        // byte text[STRING_SIZE] = "";
        radio.read(&text, len);
        if(CONFIG.use_network) client.publish(garageDoorLogTopic, F("Answer received"));
        displayReceivedData(text);
      }

      if(answerReceived) {
        if (memcmp(&text, cipherVector.bCiphertext, STRING_SIZE) == 0) {
          // Receiver sent back a valid encrypted string
          answerIsValid = true;
          Serial.println("Strings match!");
          if(CONFIG.use_network) client.publish(garageDoorLogTopic, F("Received string is valid"));

          return true;
        }
        else {
          Serial.println(F("Strings NOT match!"));
          if(CONFIG.use_network) client.publish(garageDoorLogTopic, F("Received string is invalid"));
          return false;
        }
      }
    }
    else {
      if(CONFIG.use_network) client.publish(garageDoorLogTopic, F("No ack"));
      Serial.println(F("# Error while sending string"));
      Serial.println(F("#"));
      Serial.println(F(""));
    }
  }
  return false;
}

void checkIfSensorsChanged() {

  delay(10);

  isGarageDoorClosed.currentState = isDoorClosed();
  isGarageDoorOpen.currentState = isDoorOpen();
  garageOccupancy.currentState = isGarageOccupied();
  

  if( isGarageDoorClosed.lastState != isGarageDoorClosed.currentState ) {
    // State has changed, let's check which way

    Serial.print(F("Door status changed. It's now: "));

    if( !isGarageDoorClosed.currentState) {
      // Door is opening
      isGarageDoorClosed.mqttPayload = "opening";
      // client.publish(garageDoorLogTopic, F("Door is opening by changed state"));
      lastOpeningTime = millis();

      Serial.println(F("opening"));
      // closedBy = DFLT;
      if( triggerType == DFLT ) {
        triggerType = BY_SENSORS;
      }
    }
    else {
      isGarageDoorClosed.mqttPayload = "closed";
      // client.publish(garageDoorLogTopic, F("Door is closed by changed state"));
      Serial.println(F("closed"));
      lastClosedTime = millis();
      // openedBy = DFLT;
    }

    isGarageDoorClosed.lastState = isGarageDoorClosed.currentState;
    if(CONFIG.use_network) client.publish(isGarageDoorClosed.mqttStateTopic, isGarageDoorClosed.mqttPayload);

  }

  if(isGarageDoorOpen.lastState != isGarageDoorOpen.currentState) {
    // State has changed, let's check which way
    if( !isGarageDoorOpen.currentState ) {
      // Door is closing
      isGarageDoorOpen.mqttPayload = "closing";
      Serial.println(F("closing"));
      // client.publish(garageDoorLogTopic, F("Door is closing by changed state"));
      lastClosingTime = millis();
      // openedBy = DFLT;
      if( triggerType == DFLT ) {
        triggerType = BY_SENSORS;
      }
    }
    else {
      isGarageDoorOpen.mqttPayload = "open";
      Serial.println(F("open"));
      // client.publish(garageDoorLogTopic, F("Door is open by changed state"));
      lastOpenedTime = millis();
      // closedBy = DFLT;
    }
    isGarageDoorOpen.lastState = isGarageDoorOpen.currentState;
    if(CONFIG.use_network) client.publish(isGarageDoorOpen.mqttStateTopic, isGarageDoorOpen.mqttPayload);
  }


  if(garageOccupancy.lastState != garageOccupancy.currentState) {
    // State has changed, let's check which way

    Serial.print(F("Occupancy state changed. It's now: "));

    if(!garageOccupancy.currentState) {
    // Garage is free
    garageOccupancy.mqttPayload = "OFF";
    Serial.println(F("free"));
    }
    else {
    // Garage is occupied
    garageOccupancy.mqttPayload = "ON";
    Serial.println(F("occupied"));
    }
    garageOccupancy.lastState = garageOccupancy.currentState;
    if(CONFIG.use_network) client.publish(garageOccupancy.mqttStateTopic, garageOccupancy.mqttPayload);
  }
}

void checkChangedMqttTopic() {
  if(changedTopic == garageDoorCommandTopic) {
      // Change the state of the garage door
      if( isGarageDoorOpen.state() ) {
        // Garage is open
        if(changedTopicPayload == garageDoorCommandPayloadOpen) {
          // Do nothing, door is already open
        }
        else if(changedTopicPayload == garageDoorCommandPayloadClose) {
          // Store information about new trigger
          newTriggerDetected(BY_MQTT, ACT_CLOSE_DOOR);

//          closedBy = MQTT;
        }
        else if(changedTopicPayload == garageDoorCommandPayloadStop) {
          // Do nothing, door is open and not in movement
        }
        else {
            // Do nothing, unknown command
        }
      }
      else if( isGarageDoorClosed.state() ) {
        // Garage is closed
        if( changedTopicPayload == garageDoorCommandPayloadOpen ) {
            // openDoor();
//            openedBy = MQTT;
          // Store information about new trigger
          newTriggerDetected(BY_MQTT, ACT_OPEN_DOOR);
        }
        else if(changedTopicPayload == garageDoorCommandPayloadClose) {
            // Do nothing, door is already closed
        }
        else if(changedTopicPayload == garageDoorCommandPayloadStop) {
            // Do nothing, door is open and not in movement
        }
        else {
            // Do nothing, unknown command
        }
      }
      else {
      // Door is either closing or opening => in movement
      // or door is between open and closed => not in movement
      if(changedTopicPayload == garageDoorCommandPayloadOpen) {
          // Do nothing, the doors state is unknown
      }
      else if(changedTopicPayload == garageDoorCommandPayloadClose) {
          // Do nothing, the doors state is unknown
      }
      else if(changedTopicPayload == garageDoorCommandPayloadStop) {
          // Assuming (!) the door is between open and closed and it is in movement => stop the movement
          if( !isGarageDoorOpen.state() && !isGarageDoorClosed.state() ) {
            // Store information about new trigger
            newTriggerDetected(BY_MQTT, ACT_STOP);
//            triggerDoorRelay();
          }
      }
      else {
          // Do nothing, unknown command
      }
    }
  }

  // Reset MQTT commands
  changedTopic = "";
  changedTopicPayload = "";
}

void newTriggerDetected(byte type, byte action) {
  triggerDetected = true;
  triggerType = type;
  triggeredAction = action;
}

void generateNewString(const struct CipherVector *vector) {
   Serial.println(F("######## Generating new random string... ########"));
  client.publish(garageDoorLogTopic, F("Generate new string"));
  uint8_t i = 0;
  while (i < STRING_SIZE) {
    char letter = letters[random(0, sizeof(letters) - 1)];
    cipherVector.bPlaintext[i] = (byte)letter;
    cipherVector.cPlaintext[i] = letter;
    i++;
  }
  client.publish(garageDoorLogTopic, F("New string generated"));
}


bool handleCipher(BlockCipher *cipher, const struct CipherVector *vector, size_t keySize, bool decryption) {
   Serial.println(F("######## Encrypting string... ########"));
  client.publish(garageDoorLogTopic, F("Encrypt string"));


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
    client.publish(garageDoorLogTopic, F("Encryption passed"));
    return true;
  } else {
     Serial.println(F("Failed"));
    client.publish(garageDoorLogTopic, F("Encryption passed"));
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


void isrButton() {

  if( CONFIG.use_network ) {
    bool status = CONFIG.getPinByName("button")->getState();
    String state = status ? "ON" : "OFF";
    client.publish(garageButtonLogTopic, state);
  }

  return; // TODO Remove when fixed the button electronic

  if( isGarageDoorClosed.state() ) {
    newTriggerDetected(BY_BUTTON, ACT_OPEN_DOOR);
  }
  
  if( isGarageDoorOpen.state() ) {
    newTriggerDetected(BY_BUTTON, ACT_CLOSE_DOOR);
  }

  if( !isGarageDoorClosed.state() && !isGarageDoorOpen.state() ) {
    newTriggerDetected(BY_BUTTON, ACT_STOP);
  }


}

void isrKey() {
  if( CONFIG.use_network ) {
    bool status = CONFIG.getPinByName("key")->getState();
    String state = status ? "ON" : "OFF";
    client.publish(garageKeyLogTopic, state);
  }

  return; // TODO Remove when fixed the key electronic


  if( isGarageDoorClosed.state() ) {
    newTriggerDetected(BY_KEY, ACT_OPEN_DOOR);
  }
  
  if( isGarageDoorOpen.state() ) {
    newTriggerDetected(BY_KEY, ACT_CLOSE_DOOR);
  }

  if( !isGarageDoorClosed.state() && !isGarageDoorOpen.state() ) {
    newTriggerDetected(BY_KEY, ACT_STOP);
  }

}

String getTriggerName(byte type) {
  String triggerName;

  switch( type ) {
    case BY_BUTTON:
      triggerName = "BUTTON";
      break;

    case BY_MQTT:
      triggerName = "MQTT";
      break;
    
    case BY_RF:
      triggerName = "RF";
      break;

    case BY_SENSORS:
      triggerName = "SENORS";
      break;

    case BY_KEY:
      triggerName = "KEY";
      break;

    default:
      triggerName = "changed sensor state";
      break;

  }
  
  return triggerName;
}

void resetDetectedTrigger() {
  triggerDetected = false;
  triggeredAction = DFLT;
  triggerType = DFLT;
}

bool newTriggerExists() {
  return triggerDetected;
}

// TODO Add MQTT-STOP-handler
void handleGarage() {

  if( garageOccupancy.state() ) {
    // Garage is occupied => nothing to do
    // Serial.println(F("Garage is occupied")); delay(delayTime);

    lastOccupancyTime = millis();

  }





  if( !isGarageDoorOpen.state() ) {
    // Door is not open
    // Serial.println(F("Door is not open")); delay(delayTime);

    if( isGarageDoorClosed.state() ) {
      // Door is closed
      // Serial.println(F("Door is closed")); delay(delayTime);

      if( newTriggerExists() ) {
        // The door got a trigger from any of the available channels (RF / MQTT / BUTTON)

        if( triggeredAction = ACT_OPEN_DOOR ) {
          // Open the door now

          // Trigger the relay
          openDoor();

          // Wait until the door started movement
          unsigned long doorMovementStartTime = millis();
          while( isDoorClosed() ) {
            delay(1); // Give the door some time to start the movement
            if( isTimeout(doorMovementStartTime, 5000)) reboot();
          } 
        
          client.publish(garageDoorLogTopic, "The door was opened by " + getTriggerName(triggerType) );

          // Reset the trigger information
          if( triggerType == BY_RF ) {
            resetDetectedTrigger();
            triggerType = BY_RF; // Keep this information for automatic closing the door if no car appears
          }
          else {
            resetDetectedTrigger();
          }

          return;
        }
      }

      // Door is closed
      if( !newTriggerExists()
      && !garageOccupancy.state() 
      && CONFIG.RF.state
      && (millis() - lastClosedTime > (CONFIG.RFAreaClearingTime*1000)) ) {
        // Car is far enough away that sending RF strings will not result in an accidentally opened door => generate and send string
        // Serial.println(F("Car is far enough away for the next RF string")); delay(delayTime);

        if( millis() - CONFIG.RF.sendTime > (CONFIG.RF.waitForNextRFSending*1000) ) {
          // Last sending was a few seconds before, try again
          Serial.println(F("Last sending was a few seconds before, try again")); delay(delayTime);

          if( handleNewRFCommuncation() ) {
            // RF communication was successful
            Serial.println(F("RF communication was successful")); delay(delayTime);
            
            if( isGarageDoorClosed.state() ) {
              newTriggerDetected(BY_RF, ACT_OPEN_DOOR);
            }
            return;
          }
        }
      }
    } // isGarageDoorClosed.state()
    else {
      // Door is not closed anymore but still not fully opened => do nothing (until door is fully opened)
      if( newTriggerExists() ) {
        if( triggeredAction == ACT_STOP ) {
          triggerDoorRelay();
          
          if(CONFIG.use_network) client.publish(garageDoorLogTopic, "The door was stopped by " + getTriggerName(triggerType) );

          // Reset the trigger information
          resetDetectedTrigger();
        }
      }
    }
  } // ! isGarageDoorOpen.state()
  else {
    // Garage is open

      // Either car just left the garage and is still in front of the garage (DOOR_AREA_CLEAR_TIME)
      // or car left the garage a few seconds before and already drove away (RF_AREA_CLEAR_TIME)
      // or the garage was opened by button (car inside or not)
      // or the garage was opened by MQTT (car inside or not)

    if( newTriggerExists() ) {
      if( triggeredAction == ACT_CLOSE_DOOR ) {

        // Trigger the relay
        closeDoor();

        unsigned doorMovementStartTime = millis();
        // Wait until the door started movement
        while( isDoorOpen() ) {
          delay(1); // Give the door some time to start the movement
          if( isTimeout(doorMovementStartTime, 5000)) reboot();
        } 

        delay(100);
        checkIfSensorsChanged();

        if(CONFIG.use_network) client.publish(garageDoorLogTopic, "The door was closed by " + getTriggerName(triggerType) );

        // Reset the trigger information
        resetDetectedTrigger();

        return;
      }
    }

    // else: no trigger to close
    if( !garageOccupancy.state() ) { // and garage is still OPEN
      // Garage is not occupied
      // Wasn't the car in the garage before
      // or did the car just leave the garage?



      if( triggerType == BY_RF ) {
        // Garage was opened by RF => the car wasn't in the garage before

        if( millis() - lastOpeningTime > (CONFIG.doorAreaClearingTime*1000) ) {
          // No car appeared in configured time => close door again

          Serial.println(F("No car appeared")); delay(delayTime);
          if(CONFIG.use_network) client.publish(garageDoorLogTopic, F("No car appeared after opening door by RF"));

          // Trigger the relay
          closeDoor();

          // Wait until the door started movement
          unsigned long doorMovementStartTime = millis();
          while( isDoorOpen() ) {
            delay(1); // Give the door some time to start the movement
            if( isTimeout(doorMovementStartTime, 5000)) reboot();
          } 

          delay(100);
          checkIfSensorsChanged();

          return;
        }
      } // ! garageOccupancy.state()
      else {
        // Door was opened on another way than RF
        if( millis() - lastOccupancyTime >= (CONFIG.doorAreaClearingTime*1000) 
         && millis() - lastOccupancyTime <= (CONFIG.RFAreaClearingTime*1000) ) {
          // Car just left the garage

          Serial.println(F("Car just left the garage")); delay(delayTime);
          if(CONFIG.use_network) client.publish(garageDoorLogTopic, F("Car just left the garage, wait for it to get far enough away."));

          // Trigger the relay
          closeDoor();

          // Wait until the door started movement
          unsigned long doorMovementStartTime = millis();
          while( isDoorOpen() ) {
            delay(10); // Give the door some time to start the movement
            if( isTimeout(doorMovementStartTime, 5000)) reboot();
          } 

          delay(100);
          checkIfSensorsChanged();

          return;
        }
      }
    } // ! garageOccupancy.state()
  }
}


void reboot() {
  Serial.println("REBOOT!");
  wdt_enable(WDTO_1S);
  while(true) {}
}





unsigned long timerForLeavingCar = 0; // This variable is used to be checked against the CONFIG.doorAreaClearingTime
unsigned long lightbarrierEnabledTime = 0;
unsigned long lightbarrierDisabledTime = 0;

unsigned long lastOccupancyTime = 0;
unsigned long lastOpeningTime   = 0;
unsigned long lastClosingTime   = 0;
unsigned long lastOpenedTime    = 0;
unsigned long lastClosedTime    = 0;

bool lightbarrierStatus = false;
float lightbarrierValue1 = 0;
float lightbarrierValue2 = 0;

unsigned long delayTime = 0;


byte triggerType = DFLT;
byte triggeredAction = DFLT;
String triggerName = "";

bool triggerDetected = false;


bool cipherSuccessful = false;
bool ackReceived = false;
bool answerReceived = false;
bool answerIsValid = false;



