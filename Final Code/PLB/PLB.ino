// includes
#include <LoRa.h>
#include <TinyGPS++.h>
#include <TimeLib.h>


// pins
#define RFM95_CS 8                  // this pin is wired on the feather PCB and is out of our control
#define RFM95_RST 4                 // this pin is wired on the feather PCB and is out of our control
#define RFM95_INT 3                 // this pin is wired on the feather PCB and is out of our control

#define VBAT_PIN A7                 // this pin is wired on the feather PCB and is out of our control
#define PANIC_BUTTON_PIN 13
#define STANDBY_BUTTON_PIN 12
#define POWER_LED_PIN 11
#define STANDBY_LED_PIN 10
#define PANIC_LED_PIN 9
#define NOISE_SEED_PIN A4           // this pin is not connected to anything, we need the noise from the open connection


// constants
#define RF95_FREQ 915E6
#define RF95_BANDWIDTH 125E3
#define RF95_TX_POWER 1
#define RADIO_ID 1
#define PACKET_SIZE_BYTES 16

#define SLEEP_TIME 10000            // time between transmissions
#define SLEEP_TIME_VARIANCE 2000    // randomness in sleep time to prevent two transmitters from consistently colliding
#define GPS_TIME_ALLOWABLE_AGE 500  // how old (in ms) the GPS time is allowed to be when syncing with the system clock

#define DEBUG_BAUD 9600
#define GPS_BAUD 9600
#define SERIAL_DEBUG true           // whether to display debugging info on the USB Serial
#define DEBUG_START_DELAY_SEC 10    // starting delay to allow the USB Serial to be connected before running (a low number may miss some information)

#define BATTERY_MIN_THRESHOLD 496   // calculated from 3.2 volts according to https://learn.adafruit.com/adafruit-feather-m0-adalogger/power-management
#define BATTERY_MAX_THRESHOLD 652   // calculated from 4.2 volts according to https://learn.adafruit.com/adafruit-feather-m0-adalogger/power-management


// Singletons
TinyGPSPlus gps;
uint8_t message[PACKET_SIZE_BYTES];
int8_t messageID = 0;
bool panicState = false;

void setup() 
{
  // start serial
  Serial1.begin(GPS_BAUD);
  if (SERIAL_DEBUG)
  {
    Serial.begin(DEBUG_BAUD);
    
    //countdown startup
    for (int i=DEBUG_START_DELAY_SEC; i>0; i--)
    {
      serialLogInteger("Starting in:", i);
      smartDelay(1000);
    }
  }

  // set pinmodes  
  pinMode(VBAT_PIN, INPUT);
  pinMode(STANDBY_BUTTON_PIN, INPUT);
  pinMode(PANIC_BUTTON_PIN, INPUT);
  pinMode(POWER_LED_PIN, OUTPUT);
  pinMode(STANDBY_LED_PIN, OUTPUT);
  pinMode(PANIC_LED_PIN, OUTPUT);
  pinMode(NOISE_SEED_PIN, INPUT);

  // init rf95
  serialLog("Initializing RF95...");
  LoRa.setPins(RFM95_CS, RFM95_RST, RFM95_INT);
  LoRa.setSignalBandwidth(RF95_BANDWIDTH);
  LoRa.setTxPower(RF95_TX_POWER);
  LoRa.enableCrc();
  if (!LoRa.begin(RF95_FREQ))
  {
    serialLog("Failed to initialize RF95");
    errorLoop();
  }

  serialLog("RF95 initialized.");

  //set random seed using noise from analoge
  uint32_t noise = analogRead(NOISE_SEED_PIN);
  randomSeed(noise);
  serialLogInteger("Setting random seed with noise:", noise);

  digitalWrite(POWER_LED_PIN, HIGH);
  serialLog("Setup Complete\n");
}

void loop()
{
  bool standbyMode = digitalRead(STANDBY_BUTTON_PIN);
  if (standbyMode)
  {
    // standby mode
    LoRa.sleep();
    Serial.println("Standby...");
  }
  else
  {
    // activeMode
    activeMode();
  }

  // noisy wait
  uint16_t waitTime =  random(SLEEP_TIME - SLEEP_TIME_VARIANCE, SLEEP_TIME + SLEEP_TIME_VARIANCE);
  serialLogInteger("Waiting for", waitTime, "ms\n");
  smartDelay(waitTime);
}

// delay funciton that reads from GPS serial while waiting. Also exited by panic state
static void smartDelay(uint16_t ms)
{
  unsigned long start = millis();
  while ((millis() - start) < ms)
  {
    bool newPanicState = digitalRead(PANIC_BUTTON_PIN);
    if (!panicState && newPanicState)
    {
      activeMode();
      panicState = true;
    }
    else if (panicState && !newPanicState)
    {
      panicState = false;
    }

    if (Serial1.available())
    {
      gps.encode(Serial1.read());
    }
  }
}

void activeMode()
{
  // read battery life
  long batteryReading = analogRead(VBAT_PIN);
  uint8_t batteryPercent = map(batteryReading, BATTERY_MIN_THRESHOLD, BATTERY_MAX_THRESHOLD, 0, 100); // map to uint8_t
  batteryPercent = max(batteryReading, 0); // trim negative overflow 
  batteryPercent = min(batteryReading, 100); // trim positive overflow

  // read gps data from Serial1
  float gpsLat = 0;
  float gpsLong = 0;
  if (gps.location.isValid())
  {
    gpsLat = gps.location.lat();
    gpsLong = gps.location.lng();
  }
  else
  {
    serialLog("GPS location invalid");
  }

  if (gps.time.isValid() && gps.time.age() < GPS_TIME_ALLOWABLE_AGE && gps.date.isValid())
  {
    //resync the system clock with the gps
    setTime(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.date.day(), gps.date.month(), gps.date.year());
  }
  else
  {
    serialLog("Unable to sync time with GPS");
  }

  if (gps.satellites.isValid())
  {
    serialLogInteger("Number of GPS satellites:", gps.satellites.value()); 
  }
  else
  {
    serialLog("GPS satellites invalid");
  }

  if (gps.location.isValid() && timeStatus() != timeNotSet)
  {
    int32_t utc = now();

    //log packet contents before send
    serialLogInteger("Radio ID:", RADIO_ID);
    serialLogInteger("Panic State:", panicState);
    serialLogInteger("Message ID:", messageID);
    serialLogDouble("GPS latitude:", gpsLat);
    serialLogDouble("GPS longitude:", gpsLong);
    serialLogInteger("Battery Percent:", batteryPercent, "%");
    serialLogInteger("Timestamp:", utc);

    // send message
    encodeMessage(RADIO_ID, messageID, gpsLat, gpsLong, batteryPercent, utc);
    LoRa.beginPacket();
    LoRa.write(message, PACKET_SIZE_BYTES);
    LoRa.endPacket();
    messageID = (messageID + 1) & 0b01111111; // increment without consuming most significant bit (reserved for panic state)
  }
}

void encodeMessage(uint16_t radioID, int8_t messageID, float gpsLat, float gpsLong, uint8_t batteryPercent, int32_t utc)
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

  int byteIndex = 0;

  // encode radio ID
  for (int i=sizeof(radioID)-1; i>=0; i--)
  {
    //cast data to byte array then get byte by index
    message[byteIndex] = ((uint8_t*)&radioID)[i];
    byteIndex++;
  }

  // encode panicState and messageID
  uint8_t panicMask = panicState ? 0b10000000 : 0b00000000;
  message[byteIndex] = messageID | panicMask;
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

  // encode batteryPercent
  message[byteIndex] = batteryPercent;
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

void serialLogInteger(String prefix, long intValue)
{
  if (SERIAL_DEBUG)
  {
    Serial.print(prefix);
    Serial.print(" ");
    Serial.println(intValue);
  }
}

void serialLogInteger(String prefix, long intValue, String postfix)
{
  if (SERIAL_DEBUG)
  {
    Serial.print(prefix);
    Serial.print(" ");
    Serial.print(intValue);
    Serial.print(" ");
    Serial.println(postfix);
  }
}

void serialLogDouble(String prefix, double decimalValue)
{
  if (SERIAL_DEBUG)
  {
    Serial.print(prefix);
    Serial.print(" ");
    Serial.println(decimalValue);
  }
}

void serialLogDouble(String prefix, double decimalValue, String postfix)
{
  if (SERIAL_DEBUG)
  {
    Serial.print(prefix);
    Serial.print(" ");
    Serial.print(decimalValue);
    Serial.print(" ");
    Serial.println(postfix);
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