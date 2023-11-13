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
// #include <Base64.h>
// #include <garageOpenerLib.h>

class sensor {
  public:
    sensor();
    sensor(String);
    bool hasStateChanged();
    bool state();
    bool lastState;
    bool currentState;
    String mqttStateTopic;
    String mqttPayload;
};

bool USE_NETWORK                      = true;
bool TESTING_ONLY                     = false;
//byte RADIO_ADDRESS[6] = {0x30, 0x30, 0x30, 0x30, 0x31}; // 00001
const byte RADIO_ADDRESS[6]           = "00001";
uint8_t RADIO_READINGPIPE             = 0;
uint8_t RADIO_CHANNEL                 = 0;
bool RADIO_DYNAMIC_PAYLOAD_SIZE       = false;
const size_t KEY_SIZE                 = 32;
const size_t STRING_SIZE              = 16;


const uint8_t _PIN_GARAGE_OCCUPANCY   = A1;

const uint8_t _PIN_DOOR_CLOSED        = 11;
const uint8_t _PIN_LIGHTBARRIER       = 9;
const uint8_t _PIN_RF_CSN             = 8;
const uint8_t _PIN_RF_CE              = 7;
const uint8_t _PIN_DOOR_OPEN          = 6;
const uint8_t _PIN_RELAY              = 5;
const uint8_t _PIN_BUTTON_HW          = 21;
const uint8_t _PIN_ENABLE_TESTING     = 3;
const uint8_t _PIN_DISABLE_NETWORK    = 2;

const uint8_t _PIN_STATUS_NO_ACK      = A5;
const uint8_t _PIN_STATUS_ACK         = A5;
const uint8_t _PIN_STATUS_SENDING     = A5;
const uint8_t _PIN_UNUSED             = A0;  // For random seed


const uint16_t _STATUS_SENDING_LED_DURAT  = 1000;
const uint16_t _STATUS_NO_ACK_LED_DURAT  = 1000;
const uint16_t _STATUS_ACK_LED_DURAT  = 1000;


const uint16_t MAX_MQTT_PAYLOAD_SIZE  = 1024;
const bool _INVERT_GARAGE_OCCUPATION  = false;
const bool _INVERT_DOOR_OPEN_STATUS   = true;
const bool _INVERT_DOOR_CLOSED_STATUS = true;
const bool _INVERT_ENABLE_TESTING     = true;
const bool _INVERT_DISABLE_NETWORK    = true;
const bool _INVERT_LIGHTBARRIER       = false;
const bool _INVERT_BUTTON_HW          = true;
const uint8_t MAX_RECEIVE_ATTEMPTS    = 10;
const uint8_t MAX_WAIT_DURATION_SEC   = 10;
const uint16_t TIMEOUT                = 3000; // milliseconds => time between sending the string and marking this try as failure because of no answer
const uint8_t WAIT_FOR_NEXT_RF_SENDING= 5;  // seconds
const uint8_t DOOR_AREA_CLEARING_TIME = 15;  // seconds
const uint8_t RF_AREA_CLEARING_TIME   = 25;  // seconds
const float LDR_TOLERANCE             = 30; // integer, will be transformed to percentage
const uint16_t LDR_TRESHOLD           = 600; // value of analoagRead()

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
byte ip[] = {192, 168, 20, 177};  // <- change to match your network
IPAddress myDns(192, 168, 20, 1);
const char mqttHostAddress[] = "homecontrol.lan.k4";
const char mqttUser[] = "test";
const char mqttPwd[] = "qoibOIBbfoqib38bqucv3u89qv";
const char mqttClientId[] = "garage_sensors";

sensor isGarageDoorClosed((String) "homeassistant/cover/garage_door/state");
sensor isGarageDoorOpen((String) "homeassistant/cover/garage_door/state");
sensor garageOccupancy((String) "homeassistant/binary_sensor/garage_occupancy/state");



EthernetClient net;
MQTTClient client(MAX_MQTT_PAYLOAD_SIZE);

const uint8_t SENDING                 = 1;
const uint8_t ACK                     = 2;
const uint8_t NO_ACK                  = 4;

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
unsigned long timerForLeavingCar = 0; // This variable is used to be checked against the DOOR_AREA_CLEARING_TIME
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

const byte DFLT     = 0;
const byte RF       = 1;
const byte MQTT     = 2;
const byte SENSORS  = 3;
const byte BUTTON   = 4;
const byte ACT_OPEN_DOOR = 1;
const byte ACT_CLOSE_DOOR = 2;
const byte ACT_STOP = 3;
byte triggerType = DFLT;
byte triggeredAction = DFLT;
String triggerName = "";

byte openedBy       = DFLT;
byte closedBy       = DFLT;

bool triggerDetected = false;





String changedTopic = "";
String changedTopicPayload = "";
String garageDoorCommandTopic = "homeassistant/cover/garage_door/set";
String garageDoorCommandPayloadOpen = "OPEN";
String garageDoorCommandPayloadClose = "CLOSE";
String garageDoorCommandPayloadStop = "STOP";





  sensor::sensor() {}
  sensor::sensor(String topic) {
      mqttStateTopic = topic;
  }
  bool sensor::state() {
    return currentState;
  }

bool cipherSuccessful = false;
bool ackReceived = false;
bool answerReceived = false;
bool answerIsValid = false;


bool handleCipher(BlockCipher *, const struct CipherVector *, size_t, bool = true);
void generateNewString(const struct CipherVector *);
void displayKey(const struct CipherVector *);
void displayRawString(const struct CipherVector *);
void displayReceivedData(byte *, size_t = STRING_SIZE);
void openDoor(uint8_t = _PIN_RELAY, bool = false);
void closeDoor(uint8_t = _PIN_RELAY, bool = false);
void triggerDoorRelay(uint8_t = _PIN_RELAY, bool = false);
bool isDoorClosed();
bool isGarageOccupied();
bool isTimeout();
void resetRF();
void toggleStatusLed(uint8_t, uint16_t = 5000);
void messageReceived(String&, String&);
void checkIfSensorsChanged();
bool handleNewRFCommuncation();
void handleClosedDoorAndUnoccupiedGarage();
void handleOpenDoorAndOccupiedGarage();
void checkConfig();


void checkConfig() {
  Serial.print(F("checkConfig(): "));
  TESTING_ONLY = _INVERT_ENABLE_TESTING ? !digitalRead(_PIN_ENABLE_TESTING) : digitalRead(_PIN_ENABLE_TESTING);
  USE_NETWORK = _INVERT_DISABLE_NETWORK ? !digitalRead(_PIN_DISABLE_NETWORK) : digitalRead(_PIN_DISABLE_NETWORK);
  Serial.print(F("TESTING_ONLY:")); Serial.print(TESTING_ONLY); Serial.print(F(" / ")); Serial.print(F("USE_NETWORK: ")); Serial.println(USE_NETWORK);
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
    client.publish(F("homeassistant/sensor/garage_current_action/state"), F("Send encrypted string by RF, wait for ack"));
    sendTime = millis();
    if(radio.write(cipherVector.bPlaintext, STRING_SIZE)) {
      ackReceived = true;
      client.publish(F("homeassistant/sensor/garage_current_action/state"), F("Ack successful, wait for answer"));
      Serial.println(F("# String successfully sent"));
      Serial.println(F("#"));
      Serial.println(F(""));
    }

    if(ackReceived) {
      // 4.   Wait until receiving a string or until timeout
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
          client.publish(F("homeassistant/sensor/garage_current_action/state"), F("Answer timed out"));
          
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
        client.publish(F("homeassistant/sensor/garage_current_action/state"), F("Answer received"));
        displayReceivedData(text);
      }

      if(answerReceived) {
        if (memcmp(&text, cipherVector.bCiphertext, STRING_SIZE) == 0) {
          // Receiver sent back a valid encrypted string
          answerIsValid = true;
          Serial.println("Strings match!");
          client.publish(F("homeassistant/sensor/garage_current_action/state"), F("Received string is valid"));
        
          if( isGarageDoorClosed.state() ) {
            newTriggerDetected(RF, ACT_OPEN_DOOR);
          }

          if( isGarageDoorOpen.state() ) {
            newTriggerDetected(RF, ACT_CLOSE_DOOR);
          }

          if( !isGarageDoorClosed.state() && isGarageDoorOpen.state() ) {
            newTriggerDetected(RF, ACT_STOP);
          }

          return true;
        }
        else {
          Serial.println(F("Strings NOT match!"));
          client.publish(F("homeassistant/sensor/garage_current_action/state"), F("Received string is invalid"));
          return false;
        }
      }
    }
    else {
      client.publish(F("homeassistant/sensor/garage_current_action/state"), F("No ack"));
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
    if( !isGarageDoorClosed.currentState) {
      // Door is opening
      isGarageDoorClosed.mqttPayload = "opening";
      // client.publish(F("homeassistant/sensor/garage_current_action/state"), F("Door is opening by changed state"));
      lastOpeningTime = millis();
      // closedBy = DFLT;
      if( triggerType == DFLT ) {
        triggerType = SENSORS;
      }
    }
    else {
      isGarageDoorClosed.mqttPayload = "closed";
      // client.publish(F("homeassistant/sensor/garage_current_action/state"), F("Door is closed by changed state"));
      lastClosedTime = millis();
      // openedBy = DFLT;
    }

    isGarageDoorClosed.lastState = isGarageDoorClosed.currentState;
    if(USE_NETWORK) client.publish(isGarageDoorClosed.mqttStateTopic, isGarageDoorClosed.mqttPayload);

  }

  if(isGarageDoorOpen.lastState != isGarageDoorOpen.currentState) {
    // State has changed, let's check which way
    if( !isGarageDoorOpen.currentState ) {
      // Door is closing
      isGarageDoorOpen.mqttPayload = "closing";
      // client.publish(F("homeassistant/sensor/garage_current_action/state"), F("Door is closing by changed state"));
      lastClosingTime = millis();
      // openedBy = DFLT;
      if( triggerType == DFLT ) {
        triggerType = SENSORS;
      }
    }
    else {
      isGarageDoorOpen.mqttPayload = "open";
      // client.publish(F("homeassistant/sensor/garage_current_action/state"), F("Door is open by changed state"));
      lastOpenedTime = millis();
      // closedBy = DFLT;
    }
    isGarageDoorOpen.lastState = isGarageDoorOpen.currentState;
    if(USE_NETWORK) client.publish(isGarageDoorOpen.mqttStateTopic, isGarageDoorOpen.mqttPayload);
  }

  if(garageOccupancy.lastState != garageOccupancy.currentState) {
    // State has changed, let's check which way
    if(!garageOccupancy.currentState) {
    // Garage is free
    garageOccupancy.mqttPayload = "OFF";
    }
    else {
    // Garage is occupied
    garageOccupancy.mqttPayload = "ON";
    }
    garageOccupancy.lastState = garageOccupancy.currentState;
    if(USE_NETWORK) client.publish(garageOccupancy.mqttStateTopic, garageOccupancy.mqttPayload);
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
          newTriggerDetected(MQTT, ACT_CLOSE_DOOR);

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
          newTriggerDetected(MQTT, ACT_OPEN_DOOR);
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
            newTriggerDetected(MQTT, ACT_STOP);
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
      ledPin = _PIN_STATUS_SENDING;
      ledDuration = _STATUS_SENDING_LED_DURAT;
      break;
    case ACK:
      ledPin = _PIN_STATUS_ACK;
      ledDuration = _STATUS_ACK_LED_DURAT;
      break;
    case NO_ACK:
      ledPin = _PIN_STATUS_NO_ACK;
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
  // "   \"command_topic\":\"homeassistant/text/garage_current_action/set\","
  "   \"unique_id\":\"garage_current_action\","
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
}

void connect() {
   Serial.print(F("connecting..."));
  while (!client.connect(mqttClientId, mqttUser, mqttPwd)) {
     Serial.print(".");
    delay(1000);
  }
   Serial.println(F("\nconnected!"));
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

bool isGarageOccupied() {
  bool status = false;

  if(TESTING_ONLY) {
    Serial.print(F("isGarageOccupied_testing(): "));
     status = _INVERT_GARAGE_OCCUPATION ? !digitalRead(_PIN_GARAGE_OCCUPANCY) : digitalRead(_PIN_GARAGE_OCCUPANCY);
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
    lightbarrierValue1 = analogRead(_PIN_GARAGE_OCCUPANCY);
    delay(10);

    // Now turn the laser on
    digitalWrite(_PIN_LIGHTBARRIER, !_INVERT_LIGHTBARRIER);
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

  lightbarrierValue2 = analogRead(_PIN_GARAGE_OCCUPANCY);

  // Turn laser off
  digitalWrite(_PIN_LIGHTBARRIER, _INVERT_LIGHTBARRIER);
  lightbarrierDisabledTime = millis();
  lightbarrierStatus = false;
  
  //  Serial.print("lightbarrierValue2 (");  Serial.print(lightbarrierValue2);  Serial.print(") >= lightbarrierValue1 (");  Serial.print(lightbarrierValue1);  Serial.print(") x 0.8 ("); Serial.print(lightbarrierValue1*0.8); Serial.print("): ");  Serial.println((lightbarrierValue2 >= (lightbarrierValue1 * 0.8)));
  //  Serial.print("lightbarrierValue2 (");  Serial.print(lightbarrierValue2);  Serial.print(") <= lightbarrierValue1 (");  Serial.print(lightbarrierValue1);  Serial.print(") x 1.2 ("); Serial.print(lightbarrierValue1*1.2); Serial.print("): ");  Serial.println((lightbarrierValue2 <= (lightbarrierValue1 * 1.2)));

  float bottomLimit = lightbarrierValue1 * (1 - LDR_TOLERANCE/100);
  float topLimit = lightbarrierValue1 * (1 + LDR_TOLERANCE/100);

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

  Serial.print(F("isGarageOccupied(): "));
  Serial.print(status); // Serial.print(" [");  Serial.print(value1);  Serial.print(" / ");  Serial.print(value2);  Serial.println("]");
  Serial.println();

  return status;
}

bool isDoorClosed() {
  Serial.print(F("isDoorClosed(): "));
  bool status = _INVERT_DOOR_CLOSED_STATUS ? !digitalRead(_PIN_DOOR_CLOSED) : digitalRead(_PIN_DOOR_CLOSED);
  Serial.println(status);

  return status;
}

bool isDoorOpen() {
  Serial.print(F("isDoorOpen(): "));
  bool status = _INVERT_DOOR_OPEN_STATUS ? !digitalRead(_PIN_DOOR_OPEN) : digitalRead(_PIN_DOOR_OPEN);
  Serial.println(status);

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

void generateNewString(const struct CipherVector *vector) {
   Serial.println(F("######## Generating new random string... ########"));
  client.publish(F("homeassistant/sensor/garage_current_action/state"), F("Generate new string"));
  uint8_t i = 0;
  while (i < STRING_SIZE) {
    char letter = letters[random(0, sizeof(letters) - 1)];
    cipherVector.bPlaintext[i] = (byte)letter;
    cipherVector.cPlaintext[i] = letter;
    i++;
  }
  client.publish(F("homeassistant/sensor/garage_current_action/state"), F("New string generated"));
}

bool handleCipher(BlockCipher *cipher, const struct CipherVector *vector, size_t keySize, bool decryption) {
   Serial.println(F("######## Encrypting string... ########"));
  client.publish(F("homeassistant/sensor/garage_current_action/state"), F("Encrypt string"));


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
    client.publish(F("homeassistant/sensor/garage_current_action/state"), F("Encryption passed"));
    return true;
  } else {
     Serial.println(F("Failed"));
    client.publish(F("homeassistant/sensor/garage_current_action/state"), F("Encryption passed"));
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

void newTriggerDetected(byte type, byte action) {
  triggerDetected = true;
  triggerType = type;
  triggeredAction = action;
}

void isr() {


  Serial.print(F("Button pushed - "));

  if( isGarageDoorClosed.state() ) {
    Serial.println(F("open door"));
    newTriggerDetected(BUTTON, ACT_OPEN_DOOR);
  }
  
  if( isGarageDoorOpen.state() ) {
    Serial.println(F("close door"));
    newTriggerDetected(BUTTON, ACT_CLOSE_DOOR);
  }

  if( !isGarageDoorClosed.state() && !isGarageDoorOpen.state() ) {
    Serial.println(F("stop door"));
    newTriggerDetected(BUTTON, ACT_STOP);
  }

}


void setup() {
  // put your setup code here, to run once:
  // Setup Serial

  Serial.begin(115200);
  while (!Serial) {}
  Serial.println(F("Serial ready"));
  delay(500);



  pinMode(_PIN_DOOR_CLOSED, INPUT_PULLUP);
  pinMode(_PIN_DOOR_OPEN, INPUT_PULLUP);
  pinMode(_PIN_GARAGE_OCCUPANCY, INPUT_PULLUP);
  pinMode(_PIN_ENABLE_TESTING, INPUT_PULLUP);
  pinMode(_PIN_DISABLE_NETWORK, INPUT_PULLUP);
  pinMode(_PIN_BUTTON_HW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(_PIN_BUTTON_HW), isr, LOW);
  pinMode(_PIN_LIGHTBARRIER, OUTPUT);
  pinMode(_PIN_RELAY, OUTPUT);
  pinMode(_PIN_STATUS_ACK, OUTPUT);
  pinMode(_PIN_STATUS_NO_ACK, OUTPUT);
  pinMode(_PIN_STATUS_SENDING, OUTPUT);
  Serial.println(F("GPIOs ready"));
  delay(500);

  checkConfig();

  // Setup RF

  if (!radio.begin()) {
    Serial.println(F("RF module not connected"));
    while (!radio.begin()) {}
  }
  Serial.println(F("RF module ready"));
  delay(500);

  // Set up all relevant settings for the RF module
  resetRF();

  // Generate new seed for more randomness for future
  // randomSeed(esp_random());
  randomSeed(analogRead(_PIN_UNUSED));

  // Setup network
  if(USE_NETWORK) {

    Serial.println(F("Initialize Ethernet with DHCP:"));
    if (Ethernet.begin(mac) == 0) {
      Serial.println(F("Failed to configure Ethernet using DHCP"));
      // Check for Ethernet hardware present
      if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        Serial.println(F("Ethernet shield was not found.  Sorry, can't run without hardware."));
        while (true) {
          delay(1); // do nothing, no point running without Ethernet hardware
        }
      }
      if (Ethernet.linkStatus() == LinkOFF) {
        Serial.println(F("Ethernet cable is not connected."));
      }
      // try to congifure using IP address instead of DHCP:
      Ethernet.begin(mac, ip, myDns);
      Serial.println(F("Started Ethernet with manually assigned IP"));
      
    } else {
      Serial.print(F("  DHCP assigned IP "));
      Serial.println(Ethernet.localIP());
    }

    // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported
    // by Arduino. You need to set the IP address directly.
    client.begin(mqttHostAddress, net);
    client.onMessage(messageReceived);

    connect();
    publishConfig();
    // isGarageDoorClosed.mqttStateTopic = "homeassistant/cover/garage_door/state";
    // isGarageDoorClosed.mqttStateTopic = "homeassistant/cover/garage_door/state";
    // isGarageDoorOpen.mqttStateTopic = "homeassistant/cover/garage_door/state";
  

    isGarageDoorClosed.currentState = isDoorClosed();
    isGarageDoorOpen.currentState = isDoorOpen();
    garageOccupancy.currentState = isGarageOccupied();

    if(garageOccupancy.currentState) {
      garageOccupancy.mqttPayload = "ON";
    }
    else {
      garageOccupancy.mqttPayload = "OFF";
    }

    client.publish(garageOccupancy.mqttStateTopic, garageOccupancy.mqttPayload);


    if(isGarageDoorOpen.currentState) {
      // Door is open
      isGarageDoorOpen.mqttPayload = "open";
      client.publish(isGarageDoorOpen.mqttStateTopic, isGarageDoorOpen.mqttPayload);
    }
    else if(isGarageDoorClosed.currentState) {
      // Door is closed
      isGarageDoorClosed.mqttPayload = "closed";
      client.publish(isGarageDoorClosed.mqttStateTopic, isGarageDoorClosed.mqttPayload);
    }
    else {
      client.publish(isGarageDoorClosed.mqttStateTopic, "Unknown");
    }
  }
  else {
    Serial.println(F("#################################### ! NO NETWORK USAGE ! ####################################"));
  }

  Serial.println(F("I am the stationary device - ready!"));
  client.publish(F("homeassistant/sensor/garage_current_action/state"), F("Device has finished booting, run loop() now"));
  client.publish(F("homeassistant/cover/garage_door/set"), "", true, 0); // First delete this payload
  client.subscribe("homeassistant/cover/garage_door/set"); // Then subscribe to this topic
}




byte currentProcessPhase;

// 0    closed, unoccupied, wait for valid answer
// 1    unclosed, unopen, unoccupied, wait for open door and car to appear
// 2    open, unoccupied, wait for car to appear
// 3    open, occupied, wait for manual closing
// 4    closed, occupied, do nothing
// 5    open, occupied, wait for car to leave
// 6    open, unoccupied, wait for car leaving the door clearing area
// 7    closing, unoccupied, wait for car leaving the RF area
//    back to 0


String getTriggerName(byte type) {
  String triggerName;

  switch( type ) {
    case BUTTON:
      triggerName = "BUTTON";
      break;

    case MQTT:
      triggerName = "MQTT";
      break;
    
    case RF:
      triggerName = "RF";
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
//  triggerType = DFLT;
}

bool newTriggerExists() {
  return triggerDetected;
}



// TODO Add MQTT-STOP-handler
void handleGarage() {
  if( garageOccupancy.state() ) {
    // Garage is occupied => nothing to do
    Serial.println(F("Garage is occupied")); delay(delayTime);

    lastOccupancyTime = millis();

  }





  if( !isGarageDoorOpen.state() ) {
    // Door is not open
    Serial.println(F("Door is not open")); delay(delayTime);

    if( isGarageDoorClosed.state() ) {
      // Door is closed
      Serial.println(F("Door is closed")); delay(delayTime);

      if( newTriggerExists() ) {
        // The door got a trigger from any of the available channels (RF / MQTT / BUTTON)

        if( triggeredAction = ACT_OPEN_DOOR ) {
          // Open the door now

          // Trigger the relay
          openDoor();

          // Wait until the door started movement
          while( isDoorClosed() ) {
            delay(10); // Give the door some time to start the movement
          } 

          delay(100);
          checkIfSensorsChanged();
        
          client.publish(F("homeassistant/sensor/garage_current_action"), "The door was opened by " + getTriggerName(triggerType) );

          // Reset the trigger information
          resetDetectedTrigger();

          return;
        }
      }


      if( !newTriggerExists()
      && !garageOccupancy.state() 
      && (millis() - lastClosingTime > (RF_AREA_CLEARING_TIME*1000)) ) { // TODO Replace lastClosingTime by lastClosedTime
        // Car is far enough away that sending RF strings will not result in an accidentally opened door => generate and send string
        Serial.println(F("Car is far enough away for the next RF string")); delay(delayTime);

        if( millis() - sendTime > (WAIT_FOR_NEXT_RF_SENDING*1000) ) {
          // Last sending was a few seconds before, try again
          Serial.println(F("Last sending was a few seconds before, try again")); delay(delayTime);

          if( handleNewRFCommuncation() ) {
            // RF communication was successfull
            Serial.println(F("RF communication was successfull")); delay(delayTime);
            return;
          }
        }
      }
    }
    else {
      // Door is not closed anymore but still not fully opened => do nothing (until door is fully opened)
      if( newTriggerExists() ) {
        if( triggeredAction == ACT_STOP ) {
          triggerDoorRelay();
          
          client.publish(F("homeassistant/sensor/garage_current_action"), "The door was stopped by " + getTriggerName(triggerType) );

          // Reset the trigger information
          resetDetectedTrigger();
        }
      }
    }
  }
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

        // Wait until the door started movement
        while( isDoorOpen() ) {
          delay(10); // Give the door some time to start the movement
        } 

        delay(100);
        checkIfSensorsChanged();

        client.publish(F("homeassistant/sensor/garage_current_action"), "The door was closed by " + getTriggerName(triggerType) );

        // Reset the trigger information
        resetDetectedTrigger();

        return;
      }
    }


    if( !garageOccupancy.state() ) { // and garage is still OPEN
      // Garage is not occupied
      // Wasn't the car in the garage before
      // or did the car just leave the garage?



      if( triggerType == RF ) {
        // Garage was opened by RF => the car wasn't in the garage before
        if( millis() - lastOpeningTime > (DOOR_AREA_CLEARING_TIME*1000) ) { // TODO Replace lastOpeningTime by lastOpenedTime
          // No car appeared in configured time => close door again
          Serial.println(F("No car appeared")); delay(delayTime);
          client.publish(F("homeassistant/sensor/garage_current_action"), F("No car appeared after opening door by RF"));

          // Trigger the relay
          closeDoor();

          // Wait until the door started movement
          while( isDoorOpen() ) {
            delay(10); // Give the door some time to start the movement
          } 

          delay(100);
          checkIfSensorsChanged(); 

          return;
        }
      }
      else {
        // Door was opened on another way than RF
        if( millis() - lastOccupancyTime > (DOOR_AREA_CLEARING_TIME*1000) 
         && millis() - lastOccupancyTime < (RF_AREA_CLEARING_TIME*1000) ) {
          // Car just left the garage
          Serial.println(F("Car just left the garage")); delay(delayTime);
          client.publish(F("homeassistant/sensor/garage_current_action"), F("Car just left the garage"));

          // Trigger the relay
          closeDoor();

          // Wait until the door started movement
          while( isDoorOpen() ) {
            delay(10); // Give the door some time to start the movement
          } 

          delay(100);
          checkIfSensorsChanged();

          return;
        }
      }
    }
  }
}

/*
External Interrupts: 
2 (interrupt 0), 
3 (interrupt 1), 
18 (interrupt 5), 
19 (interrupt 4), 
20 (interrupt 3), and 
21 (interrupt 2). These pins can be configured to trigger an interrupt on a low value, a rising or falling edge, or a change in value. See the attachInterrupt() function for details.
*/

void loop() {

  if(USE_NETWORK) {
    client.loop();
    if (!client.connected()) {
      connect();
    }
  }

  // Check if any sensor changed its state
  checkIfSensorsChanged();

  // Check if a recently received MQTT message is relevant for the door control
  checkChangedMqttTopic();

  // Handle the sensor/actor controlling
  handleGarage();  
}