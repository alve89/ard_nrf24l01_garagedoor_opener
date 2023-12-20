#include "garagecontrol.h"

config CONFIG;
File logFile;


void setup() {

    CONFIG.bootingTime = millis();

    Serial.begin(115200);
    while (!Serial) {}
    Serial.println(F("Serial ready"));
    delay(500);



    Serial.println("Initialisiere SD-Karte");
    bool sdState = false; 
    unsigned long sdInitStartTime = millis();
    if(!SD.begin(5)) {
    while(true) {
        if( SD.begin(5) ) {                                     // Wenn die SD-Karte nicht (!SD.begin) gefunden werden kann, ...
        sdState = true;    // ... soll eine Fehlermeldung ausgegeben werden. ....
        break;
        }
        if( isTimeout(sdInitStartTime, 3000)) {
        sdState = false;
        break;
        }
    }
    }

    if( !sdState ) {
        reboot(F("SD card not initialized"));
    }
    else {
        Serial.println(F("SD card successfully initialized"));
    }


    if( CONFIG.use_logging ) log(F("Config - Begin pin configuration"));

    logFile = SD.open("log.txt");

    while (logFile.available()) {
      Serial.write(logFile.read());
    }
    // close the file:
    logFile.close();


}

void loop() {

}