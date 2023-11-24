#include "garagecontrol.h"


config CONFIG;
EthernetClient net;
MQTTClient client(1024);
RF24 radio(7, 8);  // CE, CSN
SpeckTiny speckTiny;

sensor isGarageDoorClosed((String) "homeassistant/cover/garage_door/state");
sensor isGarageDoorOpen((String) "homeassistant/cover/garage_door/state");
sensor garageOccupancy((String) "homeassistant/binary_sensor/garage_occupancy/state");




//byte CONFIG.RF.address[6] = {0x30, 0x30, 0x30, 0x30, 0x31}; // 00001
// const byte RADIO_ADDRESS[6]           = "00001";
// uint8_t CONFIG.RF.readingPipe             = 0;
// uint8_t RADIO_CHANNEL                 = 0;
// bool RADIO_DYNAMIC_PAYLOAD_SIZE       = false;




// const bool _INVERT_GARAGE_OCCUPATION  = false;
// const bool _INVERT_DOOR_OPEN_STATUS   = true;
// const bool _INVERT_DOOR_CLOSED_STATUS = true;
// const bool _INVERT_ENABLE_TESTING     = true;
// const bool _INVERT_DISABLE_NETWORK    = true;
// const bool _INVERT_LIGHTBARRIER       = false;
// const bool _INVERT_BUTTON_HW          = true;
// const bool _INVERT_KEY_HW             = true;




// const uint8_t _PIN_GARAGE_OCCUPANCY   = A1;
// const uint8_t _PIN_DOOR_CLOSED        = 11;
// const uint8_t _PIN_LIGHTBARRIER       = 9;
// const uint8_t _PIN_RF_CSN             = 8;
// const uint8_t _PIN_RF_CE              = 7;
// const uint8_t _PIN_DOOR_OPEN          = 6;
// const uint8_t _PIN_RELAY              = 5;
// const uint8_t _PIN_BUTTON_HW          = 21;
// const uint8_t _PIN_KEY_HW             = 20;
// const uint8_t _PIN_ENABLE_TESTING     = 3;
// const uint8_t _PIN_DISABLE_NETWORK    = 2;

// const uint8_t _PIN_STATUS_NO_ACK      = A5;
// const uint8_t _PIN_STATUS_ACK         = A5;
// const uint8_t _PIN_STATUS_SENDING     = A5;
// const uint8_t _PIN_UNUSED             = A0;  // For random seed







// const uint16_t TIMEOUT                = 3000; 
// const uint8_t WAIT_FOR_NEXT_RF_SENDING= 5;  // seconds
// const uint8_t DOOR_AREA_CLEARING_TIME = 15;  // seconds
// const uint8_t RF_AREA_CLEARING_TIME   = 25;  // seconds
// const float LDR_TOLERANCE             = 30; // integer, will be transformed to percentage
// const uint16_t LDR_TRESHOLD           = 600; // value of analoagRead()

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
byte ip[] = {192, 168, 20, 177};  // <- change to match your network
IPAddress myDns(192, 168, 20, 1);
byte BUFFER[STRING_SIZE];

// char mqttHostAddress[] = "homecontrol.lan.k4";
// const char mqttUser[] = "test";
// const char mqttPwd[] = "qoibOIBbfoqib38bqucv3u89qv";
// const char mqttClientId[] = "garage_sensors";



// Define the vector vectors from http://eprint.iacr.org/2013/404
static CipherVector cipherVector = {
  .name = "Speck-128-ECB",
  .cKey = "bnbUy90ErlgzkpSrBYWRKhlc5L85Q7BX",
  .bKey = { 0x62, 0x6E, 0x62, 0x55, 0x79, 0x39, 0x30, 0x45, 0x72, 0x6C, 0x67, 0x7A, 0x6B, 0x70, 0x53, 0x72, 0x42, 0x59, 0x57, 0x52, 0x4B, 0x68, 0x6C, 0x63, 0x35, 0x4C, 0x38, 0x35, 0x51, 0x37, 0x42, 0x58 },
  .bPlaintext = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  .bCiphertext = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};












String changedTopic = "";
String changedTopicPayload = "";
String garageDoorCommandTopic = "homeassistant/cover/garage_door/set";
String garageDoorCommandPayloadOpen = "OPEN";
String garageDoorCommandPayloadClose = "CLOSE";
String garageDoorCommandPayloadStop = "STOP";
String garageDoorLogTopic = "homeassistant/sensor/garage_current_action/state";
String garageButtonLogTopic = "homeassistant/sensor/garage_button_log/state";
String garageKeyLogTopic = "homeassistant/sensor/garage_key_log/state";
String garageRFStateTopic = "homeassistant/binary_sensor/garage_rf_state/state";










void setup() {

  CONFIG.bootingStartTime = millis();


  wdt_disable();  /* Disable the watchdog and wait for more than 2 seconds */
  delay(3000);  /* Done so that the Arduino doesn't keep resetting infinitely in case of wrong configuration */
//  wdt_enable(WDTO_2S);  /* Enable the watchdog with a timeout of 2 seconds */

  // put your setup code here, to run once:
  // Setup Serial

  Serial.begin(115200);
  while (!Serial) {}
  Serial.println(F("Serial ready"));
  delay(500);













CONFIG.MQTT.maxPayloadSize  = 1024;

CONFIG.newPin("garageOccupancy", A1, false);
CONFIG.newPin("laser", 9, false);
CONFIG.newPin("doorClosed", 11, true);
CONFIG.newPin("doorOpen", 6, true);
CONFIG.newPin("relay", 5, false);
CONFIG.newPin("button", 21, true);
CONFIG.newPin("key", 20, true);
CONFIG.newPin("enableTesting", 3, true);
CONFIG.newPin("disableNetwork", 2, true);
CONFIG.newPin("ledNoAck", A5, false);
CONFIG.newPin("ledAck", A5, false);
CONFIG.newPin("ledSending", A5, false);
CONFIG.newPin("unused", A0, false);

CONFIG.newPin("rf_csn", 8);
CONFIG.newPin("rf_ce", 7);




  pinMode(CONFIG.getPinByName("doorClosed")->getNumber(), INPUT_PULLUP);
  pinMode(CONFIG.getPinByName("doorOpen")->getNumber(), INPUT_PULLUP);
  pinMode(CONFIG.getPinByName("garageOccupancy")->getNumber(), INPUT_PULLUP);
  pinMode(CONFIG.getPinByName("enableTesting")->getNumber(), INPUT_PULLUP);
  pinMode(CONFIG.getPinByName("disableNetwork")->getNumber(), INPUT_PULLUP);
  pinMode(CONFIG.getPinByName("button")->getNumber(), INPUT_PULLUP);
  pinMode(CONFIG.getPinByName("key")->getNumber(), INPUT_PULLUP);
  /*
    External Interrupts: 
    2 (interrupt 0), 
    3 (interrupt 1), 
    18 (interrupt 5), 
    19 (interrupt 4), 
    20 (interrupt 3), and 
    21 (interrupt 2). These pins can be configured to trigger an interrupt on a low value, a rising or falling edge, or a change in value. See the attachInterrupt() function for details.
  */
  attachInterrupt(digitalPinToInterrupt(CONFIG.getPinByName("key")->getNumber()), isrKey, CHANGE );
  attachInterrupt(digitalPinToInterrupt(CONFIG.getPinByName("button")->getNumber()), isrButton, CHANGE );
  pinMode(CONFIG.getPinByName("laser")->getNumber(), OUTPUT);
  pinMode(CONFIG.getPinByName("relay")->getNumber(), OUTPUT);
  pinMode(CONFIG.getPinByName("ledAck")->getNumber(), OUTPUT);
  pinMode(CONFIG.getPinByName("ledNoAck")->getNumber(), OUTPUT);
  pinMode(CONFIG.getPinByName("ledSending")->getNumber(), OUTPUT);
  Serial.println(F("GPIOs ready"));
  delay(500);

  checkConfig();





  // Setup RF
  CONFIG.RF.init();
  delay(500);

  // Set up all relevant settings for the RF module
  resetRF();

  // Generate new seed for more randomness for future
  // randomSeed(esp_random());
  randomSeed(analogRead(CONFIG.getPinByName("unused")->getNumber()));

  // Setup network
  if(CONFIG.use_network) {

    Serial.println(F("Initialize Ethernet with DHCP:"));
    if (Ethernet.begin(mac) == 0) {
      Serial.println(F("Failed to configure Ethernet using DHCP"));
      // Check for Ethernet hardware present
      if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        Serial.println(F("Ethernet shield was not found.  Sorry, can't run without hardware."));
        unsigned long ethernetStartTime = millis();
        while (true) {
          if( isTimeout(ethernetStartTime, 5000)) CONFIG.use_network = false;
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
  const char hostAddressTemp[CONFIG.MQTT.getHostAddress().length()+1] = "";
  CONFIG.MQTT.getHostAddress().toCharArray(hostAddressTemp, CONFIG.MQTT.getHostAddress().length()+1);
  client.begin(hostAddressTemp, net);
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


  if(CONFIG.use_network) {
    client.publish(garageDoorLogTopic, F("Device has finished booting, run loop() now"));
    client.publish(garageDoorCommandTopic, "", true, 0); // First delete this payload
    client.subscribe(garageDoorCommandTopic); // Then subscribe to this topic



  }


}




void loop() {


  if(CONFIG.use_network) {
    client.loop();
    if (!client.connected()) {
      connect();
    }


    CONFIG.RF.init();


    if( CONFIG.RF.stateChanged() ) {
      if(CONFIG.RF.state) {
        client.publish(garageRFStateTopic, F("ON"));
      }
      else {
        client.publish(garageRFStateTopic, F("OFF"));
      }
    }    

    
  }

  // Check if any sensor changed its state
  checkIfSensorsChanged();

  // Check if a recently received MQTT message is relevant for the door control
  checkChangedMqttTopic();

  // Handle the sensor/actor controlling
  handleGarage();
}