#include <RCSwitch.h>

#define transmitterData   7   // ATtiny sends data to the stationary receiver module on this pin
#define receiverPower     3   // Enables the receiver module by powering it with 5 V
#define transmitterPower  8   
#define receiverData      2    // ATtiny receives data from the stationary transmitter module on this pin (interrupt pin!)
#define button            4    // Pushing this button sends the initial handshake message via 433 MHz


unsigned long currentTime = millis();
unsigned long previousTime = millis();

unsigned long msgGreeting = 1234;



RCSwitch sender = RCSwitch();
RCSwitch receiver = RCSwitch();


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

/*
blinkLed(unsigned short frequency) {
  digitalWrite(statusLed, HIGH);
  delay(frequency);
  digitalWrite(statusLed, LOW);
  delay(frequency);
}
*/

void setup() {
  Serial.begin(9600);
  //while(!Serial);
  Serial.println("Ready to send data!");
  pinMode(receiverPower, OUTPUT);
  pinMode(transmitterPower, OUTPUT);
  pinMode(button, INPUT_PULLUP);
  
  
  sender.enableTransmit(transmitterData);  // Der Sender wird an Pin 10 angeschlossen
  sender.setRepeatTransmit(10);

  receiver.enableReceive(receiverData);
}

void loop() {
  Serial.println("Waiting for button push");
  digitalWrite(transmitterPower, LOW);
  while(digitalRead(button));
  digitalWrite(transmitterPower, HIGH);
  currentTime = millis();
  previousTime = millis();

  Serial.print("Sending greeting: ");
  Serial.println(msgGreeting);
  sender.send(msgGreeting, 24); // Der 433mhz Sender versendet die Dezimalzahl „1234“

 
  digitalWrite(transmitterPower, LOW);
  digitalWrite(receiverPower, HIGH);
  
  Serial.println("Warte auf Response");

  
  while(!receiver.available() ) {
    currentTime = millis();
    
    if(currentTime - previousTime > 5000) {
      return;
    }
  }
//  digitalWrite(statusLed, LOW);
  
  if (receiver.available()) // Wenn ein Code Empfangen wird...
  {
    if(receiver.getReceivedValue() == 0) // Wenn die Empfangenen Daten "0" sind, wird "Unbekannter Code" angezeigt.
    {
      Serial.println("Unbekannter Code");
    }

    else // Wenn der Empfangene Code brauchbar ist, wird er hier an den Serial Monitor gesendet.
    {
      Serial.print("Received: ");
      Serial.println( receiver.getReceivedValue() );
      Serial.println("Processing hash");
      delay(1000);
      digitalWrite(transmitterPower, HIGH);
      delay(50);
      Serial.print("Sending result: ");
      Serial.println(handleHash(receiver.getReceivedValue()));
      sender.send(handleHash(receiver.getReceivedValue()), 32); // Send back new hash
    }

    receiver.resetAvailable(); // Hier wird der Empfänger "resettet"
  }
  digitalWrite(receiverPower, LOW);
}  
