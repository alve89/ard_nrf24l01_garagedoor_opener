#include <RCSwitch.h>

#define transmitterData 10   // Stationary transmitter module sends data on this pin
#define receiverPower   11   // Enables the receiver module by powering it with 5 V
#define dataPin         4    // Send data to server and power the relay
#define receiverData    0    // receiverData => pin 2 => interruptPin 0




RCSwitch receiver = RCSwitch();
RCSwitch sender = RCSwitch();

unsigned long msgHash = random(10, 99)*10000000 + random(0, 9999)*1000 + random(1,999);
unsigned int msgGreeting = 1234;

unsigned long currentTime = millis();
unsigned long previousTime = millis();

void setup() {

  pinMode(receiverPower, OUTPUT);
  pinMode(dataPin, OUTPUT);
  
  Serial.begin(9600);
  while(!Serial) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
  }
  Serial.println("Ready to receive data!");

  sender.setRepeatTransmit(10);
  sender.enableTransmit(transmitterData);  // Der Sender wird an Pin 10 angeschlossen
  receiver.enableReceive(receiverData);  // Empfänger ist an Interrupt-Pin "0" - Das ist am UNO der Pin2
}

void loop() {
  digitalWrite(receiverPower, HIGH);

  previousTime = millis();
  while(!receiver.available()) {
    currentTime = millis();
    
    if(currentTime - previousTime > 5000) {
      Serial.println("Waiting for greeting message");
      msgHash = random(10, 99)*10000000 + random(0, 9999)*1000 + random(1,999);
      break;
    }
  }
  
  if (receiver.available()) // Wenn ein Code Empfangen wird...
  {
    if (receiver.getReceivedValue() == 0) // Wenn die Empfangenen Daten "0" sind, wird "Unbekannter Code" angezeigt.
    {
      Serial.println("Unbekannter Code");
    }

    else // Wenn der Empfangene Code brauchbar ist, wird er hier an den Serial Monitor gesendet.
    {
      if(receiver.getReceivedValue() == msgGreeting) {
        Serial.print("Greeting received... Sending hash: ");
        Serial.println(msgHash);
        digitalWrite(receiverPower, LOW);
        delay(1000);
        sender.send(msgHash, 32); // Der 433mhz Sender versendet die Dezimalzahl „1234“
        Serial.println("Waiting for response");
        previousTime = millis();
        digitalWrite(receiverPower, HIGH);
        receiver.resetAvailable(); // Hier wird der Empfänger "resettet"
      }
      else if(receiver.getReceivedValue() == handleHash(msgHash)) {
        Serial.println("Result hash received... Opening door.");
        digitalWrite(dataPin, HIGH);
        delay(1000);
        digitalWrite(dataPin, LOW);
        receiver.resetAvailable(); // Hier wird der Empfänger "resettet" 
        msgHash = random(10, 99)*10000000 + random(0, 9999)*1000 + random(1,999);
      }
    }    
  }
}

unsigned long handleHash(unsigned long hash) {

  unsigned short hashId;
  unsigned int rndNo, hashType ; 
  unsigned short a,b,c,d,e,f,g,h,i,j;
  unsigned long result, returnResult;
  
  a= (hash / 100000000 ) % 10;
  b= (hash / 10000000 ) % 10;
  c= (hash / 1000000 ) % 10;
  d= (hash / 100000 ) % 10;
  e= (hash / 10000 ) % 10;
  f= (hash / 1000 ) % 10;
  g= (hash / 100 ) % 10;
  h= (hash / 10) % 10;
  i= hash % 10;  
  
  hashId = 10*a+1*b;
  rndNo = 1000*c + 100*d + 10*e + 1*f;
  hashType = g;
  
  result = 0;
  returnResult = 0;
  
  switch(hashType) {
   case 0:
     result = (rndNo - g) + ( h % i) - (h * abs(i-g));
     break;
  
   case 1:
     result = (rndNo - g) + ( h % i) - (h * abs(i-g));
     break;
  
   case 2:
     result = (rndNo - g) + ( h % i) - (h * abs(i-g));
     break;
      
   case 3:
     result = (rndNo + g) - abs( h % i) + (h * abs(i+g));
     break;
  
   case 4:
     result = (rndNo + h) - abs( i % g) + (i * (g+h));
     break;
  
   case 5:
     result = (rndNo + i) - abs( g % h) + (g * (h+i));
     break;
      
   case 6:
     result = abs(rndNo - g) + abs( h % i) + (h * abs(i+g));
     break;
  
   case 7:
     result = abs(rndNo - h) + abs( i % g) + (i * (g+h));
     break;
  
   case 8:
     result = abs(rndNo - i) + abs( g % h) + (g * (h+i));
     break;
      
   case 9:
     result = abs(rndNo - g) + abs( h % i) - (h * abs(i+g));
     break;
      
   default:
       break;
  }

  returnResult = hashId *10000000 + result * 1000 + 123;  
  return returnResult;
}
