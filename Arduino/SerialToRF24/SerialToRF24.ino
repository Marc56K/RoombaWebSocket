
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <SoftwareSerial.h>

int ddPin=5;
int rxPin=2; //10
int txPin=9; // 11
SoftwareSerial Roomba(rxPin, txPin);

const byte address[6] = "00001";
RF24 radio(7,8);

struct Message
{
  byte cmd;
  byte dataSize;
  byte data[30];
};

void reduceBaudRate (void)
{
  pinMode(ddPin, OUTPUT);
  digitalWrite(ddPin, HIGH);
  delay(100);
  digitalWrite(ddPin, LOW);
  delay(500);
  digitalWrite(ddPin, HIGH);
  delay(2000);

  for (int i = 0; i < 3; i++)
  {
      digitalWrite(ddPin, LOW);
      delay(100);
      digitalWrite(ddPin, HIGH);
      delay(100);
  }
}

int getOIMode()
{
  Roomba.write(149); // query
  Roomba.write(1);
  Roomba.write(35); // OI mode
  while (!Roomba.available());
  return (int)Roomba.read();
}

bool isCharging()
{
  Roomba.write(149); // query
  Roomba.write(1);
  Roomba.write(34); // OI mode
  while (!Roomba.available());
  return Roomba.read() != 0;
}

void printState()
{
  if (isCharging())
  {
    Serial.print("charging");
  }
  else
  {
    Serial.print("not charging");
  }

  Serial.print(", ");
  
  switch (getOIMode())
  {
    case 1:
      Serial.println("off-mode");
      break;
    case 2:
      Serial.println("passive-mode");
      break;
    case 3:
      Serial.println("safe-mode");
      break;
    case 4:
      Serial.println("full-mode");
      break;
    default:
      Serial.println("unknown-mode");
      break;
  }
}

void setup()
{
  Roomba.begin(19200);
  Serial.begin(19200);

  reduceBaudRate();

  delay(1000);
  
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MAX);
  radio.startListening();

  Roomba.write(128); // start
  
  //Roomba.write(135); // clean
}

unsigned long lastKeepAwake = 0;
void keepAwake()
{
  unsigned long now = millis();
  if (lastKeepAwake == 0 || now - lastKeepAwake > 270000) // 4,5 min TODO check if in dock
  {
    Roomba.write(128);  //Start -> Passive mode
    
    if (isCharging())
    {
      lastKeepAwake = now;
      Serial.println("keep awake");
      
      delay(500);
      Roomba.write(131);  //Safe mode
      delay(500);
      Roomba.write(128);  //Start -> Passive mode
    }

    printState();
  }
}

void loop()
{  
  keepAwake();
 
  while (radio.available())
  {
    Message msg = {};
    radio.read(&msg, sizeof(Message));
    Serial.println(msg.cmd, DEC);

    Roomba.write(msg.cmd);
    for (int i = 0; i < msg.dataSize; i++)
    {
      Roomba.write(msg.data[i]);
    }
  }
}
