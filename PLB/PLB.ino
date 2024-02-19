// includes
#include <stdlib.h>
#include <SPI.h>
#include <RH_RF95.h>
#include "TinyGPS++.h"
#include <Wire.h>
#include <vector>


// pins
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

#define STANDBY_BUTTON_PIN 13
#define PANIC_BUTTON_PIN 12
#define POWER_LED_PIN 11
#define STANDBY_LED_PIN 10
#define PANIC_LED_PIN 9
#define NOISE_SEED_PIN A4 //note this pin is not connected to anything, we need the noise from the open connection


// constants
#define RF95_FREQ 915.0
#define RF95_TX_POWER 23
#define RADIO_ID 1
#define MESSAGE_SIZE_BYTES 16

#define SLEEP_TIME 10000
#define SLEEP_TIME_VARIANCE 2000

#define DEBUG_BAUD 9600
#define GPS_BAUD 9600
#define SERIAL_DEBUG true
#define DEBUG_START_DELAY_SEC 10


// Singletons
RH_RF95 rf95(RFM95_CS, RFM95_INT);
TinyGPSPlus gps;
uint8_t message[MESSAGE_SIZE_BYTES];
int8_t messageID = 0;
bool panicState = false;
bool standbyState = false; 

void setup() 
{
  // start serial
  if (SERIAL_DEBUG)
  {
    Serial.begin(DEBUG_BAUD);
    
    //countdown startup
    for (int i=DEBUG_START_DELAY_SEC; i>0; i--)
    {
      serialLog("Starting in ", i, "");
      delay(1000);
    }
  }
  Serial1.begin(GPS_BAUD);

  // set pinmodes  
  pinMode(RFM95_RST, OUTPUT);
  pinMode(STANDBY_BUTTON_PIN, INPUT);
  pinMode(PANIC_BUTTON_PIN, INPUT);
  pinMode(POWER_LED_PIN, OUTPUT);
  pinMode(STANDBY_LED_PIN, OUTPUT);
  pinMode(PANIC_LED_PIN, OUTPUT);
  pinMode(NOISE_SEED_PIN, INPUT);

  // manually reset rf95
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);  

  // init rf95
  serialLog("Initializing RF95...");
  if (!rf95.init())
  {
    serialLog("Failed to initialize RF95");
    errorLoop();
  }

  serialLog("Setting frequency to ", RF95_FREQ, " MHZ...");
  if (!rf95.setFrequency(RF95_FREQ))
  {
    serialLog("Failed to set frequency");
    errorLoop();
  }
  serialLog("Setting TX power to ", RF95_TX_POWER, " dBm...");
  rf95.setTxPower(RF95_TX_POWER, false);

  serialLog("RF95 initialized.");

  //set random seed using noise from analoge
  uint32_t noise = analogRead(NOISE_SEED_PIN);
  randomSeed(noise);
  serialLog("Setting random seed with noise: ", noise, "");

  digitalWrite(POWER_LED_PIN, HIGH);
  serialLog("Setup Complete\n");
}

void loop()
{
  /*
  packet structure:
  - 16 bit radio ID
  - 1 bit panic state
  - 7 bit message ID
  - 32 bit latitude
  - 32 bit longitude
  - 8 bit battery life
  - 32 bit UTC

  Total size: 128 bits or 16 bytes
  */

  // init data for message
  float gpsLat = 0;
  float gpsLong = 0;
  uint8_t batteryLife;
  int32_t utc;

  // read gps data from Serial1
  if (gps.location.isValid())
  {
    gpsLat = gps.location.lat();
    gpsLong = gps.location.lng();
  }
  else
  {
    serialLog("GPS location invalid");
  }

  if (gps.time.isValid())
  {
    utc = gps.time.value();
  }
  else
  {
    serialLog("GPS time invalid");
  }
  if (gps.satellites.isValid())
  {
    serialLog("Number of GPS satellites: ", gps.satellites.value(),""); 
  }
  else
  {
    serialLog("GPS satellites invalid");
  }

  // send message
  encodeMessage(RADIO_ID, panicState, messageID, gpsLat, gpsLong, batteryLife, utc);
  rf95.send(message, MESSAGE_SIZE_BYTES);

  // noisy wait
  messageID++;
  long waitTime =  random(SLEEP_TIME - SLEEP_TIME_VARIANCE, SLEEP_TIME + SLEEP_TIME_VARIANCE);
  serialLog("Waiting for ", waitTime, " ms");
  serialLog("");
  smartDelay(waitTime);
}

// delay funciton that reads from GPS serial while waiting
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  while (millis() - start < ms)
  {
    if (Serial1.available())
    {
      gps.encode(Serial1.read());
    }
  }
}

void encodeMessage(uint16_t radioID, bool panicState, int8_t messageID, float gpsLat, float gpsLong, uint8_t batteryLife, int32_t utc)
{
  int byteIndex = 0;

  // encode radio ID
  for (int i=sizeof(radioID)-1; i>=0; i--)
  {
    //cast data to byte array then get byte by index
    message[byteIndex] = ((uint8_t*)&radioID)[i];
    byteIndex++;
  }

  // encode panicState and messageID
  uint8_t panicMask = 0b00000000;
  if (panicState)
  {
    panicMask = 0b10000000;
  }
  message[byteIndex] = (messageID & 0b01111111) | panicMask;
  byteIndex++;

  // encode gpsLat
  for (int i = sizeof(gpsLat)-1; i>=0; i--)
  {
    //cast data to byte array then get byte by index
    message[byteIndex] = ((uint8_t*)&gpsLat)[i];
    byteIndex++;
  }

  // encode gpsLong
  for (int i = sizeof(gpsLong)-1; i>=0; i--)
  {
    //cast data to byte array then get byte by index
    message[byteIndex] = ((uint8_t*)&gpsLong)[i];
    byteIndex++;
  }

  // encode batteryLife
  message[byteIndex] = batteryLife;
  byteIndex++;

  // encode utc
  for (int i = sizeof(utc)-1; i>=0; i--)
  {
    //cast data to byte array then get byte by index
    message[byteIndex] = ((uint8_t*)&utc)[i];
    byteIndex++;
  }
}

void serialLog(String message)
{
  if (SERIAL_DEBUG)
  {
    Serial.println(message);
  }
}

// overloaded for inserting float value
void serialLog(String preMessage, double value, String postMessage)
{
  if (SERIAL_DEBUG)
  {
    Serial.print(preMessage);
    Serial.print(value);
    Serial.println(postMessage);
  }
}

void errorLoop()
{
  digitalWrite(POWER_LED_PIN, LOW);
  digitalWrite(STANDBY_BUTTON_PIN, LOW);

  while (true)
  {
    digitalWrite(PANIC_LED_PIN, HIGH);
    delay(500);
    digitalWrite(PANIC_LED_PIN, LOW);
    delay(500);
  }
}