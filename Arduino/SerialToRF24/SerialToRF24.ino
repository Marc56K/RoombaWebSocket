
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <SoftwareSerial.h>

int ddPin=5;
int rxPin=2; //10
int txPin=9; //11
int redLedPin=10;
int greenLedPin=6;
unsigned long lastKeepAwake = 0;
unsigned long ledsOffUntil = 0;

SoftwareSerial Roomba(rxPin, txPin);

const byte address[6] = "00001";
RF24 radio(7,8);

struct Message
{
  byte cmd;
  byte dataSize;
  byte data[30];
};

void updateLeds(bool isConnected, bool messageReceived)
{
  unsigned long now = millis();

  if (messageReceived && now > ledsOffUntil)
  {
    ledsOffUntil = now + 30;
  }
  
  if (now < ledsOffUntil)
  {
    digitalWrite(redLedPin, LOW);
    digitalWrite(greenLedPin, LOW);
  }
  else
  {
    digitalWrite(redLedPin, isConnected ? LOW : HIGH);
    digitalWrite(greenLedPin, isConnected ? HIGH : LOW);
  }
}

int readByte(unsigned long timeoutInMs)
{
    unsigned long start = millis();
    while (!Roomba.available())
    {
      if (millis() - start > timeoutInMs)
      {
        Serial.println("timeout");
        return -1;
      }
    }

    int value = Roomba.read();
    return value;
}

void reduceBaudRate ()
{
  Serial.println("switching baud rate");

  Roomba.end();
  delay(100);
  Roomba.begin(19200);
  delay(1000);
  
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

  Roomba.flush();
}

int getOIMode()
{
  Roomba.write(149); // query
  Roomba.write(1);
  Roomba.write(35); // OI mode
  int mode = readByte(1000);
  if (mode < 1 || mode > 4)
  {
    return -1;
  }
  return mode;
}

bool connect()
{
  bool isConnected = false;

  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);

  if (getOIMode() > -1)
  {
      isConnected = true;
  }
  else
  {
    Roomba.write(128);
    if (getOIMode() > -1)
    {
      isConnected = true;
    }
  }

  if (!isConnected)
  {
    digitalWrite(redLedPin, HIGH);
    digitalWrite(greenLedPin, LOW);
    reduceBaudRate ();
    delay(1000);
    Roomba.write(128);
    isConnected =  getOIMode() > -1;

    if (isConnected)
    {
      Serial.println("connected");
    }
  }

  updateLeds(isConnected, false);

  return isConnected;
}

bool isCharging()
{
  Roomba.write(149); // query
  Roomba.write(1);
  Roomba.write(34); // OI mode
  while (!Roomba.available());
  return readByte(1000) > 0;
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

  int oiMode = getOIMode();
  switch (oiMode)
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
      Serial.print("unknown-mode: ");
      Serial.println(oiMode);
      break;
  }
}

void keepAwake()
{
  unsigned long now = millis();
  if (lastKeepAwake == 0 || now - lastKeepAwake > 120000) // 2 min
  {
    lastKeepAwake = now;
    
    Roomba.write(128);  //Start -> Passive mode

    if (isCharging())
    {
      Serial.println("keep awake");
      
      delay(500);
      Roomba.write(131);  //Safe mode
      delay(500);
      Roomba.write(128);  //Start -> Passive mode
    }

    printState();
  }
}

void setup()
{
  Serial.begin(19200);

  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MAX);
  radio.startListening();

  //Roomba.write(128); // start  
  //Roomba.write(135); // clean
}

void loop()
{  
  if (!connect())
  {
    lastKeepAwake = 0;
    Serial.println("not connected");
    return;
  }
  
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

    updateLeds(true, true);
  }
}
