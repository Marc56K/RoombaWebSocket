
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <SoftwareSerial.h>

int ddPin=5;
int rxPin=10;
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

void pulseBrcPin (void)
{
    digitalWrite(ddPin, LOW);
    delay(100);
    digitalWrite(ddPin, HIGH);
    delay(100);
}

void reset(void)
{
  Roomba.write(7);
}

void startSafe()
{  
  Roomba.write(128);  //Start
  Roomba.write(131);  //Safe mode
  delay(1000);
}

void setup()
{
  Roomba.begin(19200);
  Serial.begin(19200);

  delay(1000);

  reduceBaudRate();

  delay(1000);
  
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MAX);
  radio.startListening();
  
  //reset();
  //startSafe();
  //Roomba.write(135); // clean
}

unsigned long lastLoop = 0;
unsigned long lastPing = 0;
unsigned long nextHigh = 0;

void ping()
{
  unsigned long now = millis();
  if (now - lastPing > 60000)
  {
    Serial.println("ping begin");
    lastPing = now;
    digitalWrite(ddPin, LOW);
    nextHigh = now + 1000;
  }

  if (nextHigh != 0 && now > nextHigh)
  {
    Serial.println("ping end");
    nextHigh = 0;
    digitalWrite(ddPin, HIGH);
  }  
}

void loop()
{
  ping();
  
  while (radio.available())
  {
    Message msg = {};
    radio.read(&msg, sizeof(Message));
    Serial.println(msg.cmd, DEC);

    switch(msg.cmd)
    {
      case 1:
      {
        Serial.println("reduceBaudRate begin");
        reduceBaudRate();
        Serial.println("reduceBaudRate end");
        break;
      }
      case 2:
      {
        Serial.println("pulse begin");
        pulseBrcPin();
        Serial.println("pulse end");
        break;
      }
      default:
      {
        Roomba.write(msg.cmd);
        for (int i = 0; i < msg.dataSize; i++)
        {
          Roomba.write(msg.data[i]);
        }
      }
    }
  }
}
