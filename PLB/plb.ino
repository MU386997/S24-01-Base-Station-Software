// Feather9x_TX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (transmitter)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example Feather9x_RX

#include <stdlib.h>
#include <SPI.h>
#include <RH_RF95.h>
#include "TinyGPS++.h"

//includes for battery and display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <vector>

#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

#define BEACON_ID "1"

//more includes for batteries and delay
#define VBATPIN A5

float measuredvbat = 0;
float highvoltage = 4.3;
float lowvoltage = 3.25;
//int counter = 0;

Adafruit_SSD1306 myDisplay = Adafruit_SSD1306(128,64,&Wire, -1);


int valbutton = 0;
int valpower = 0;


int panicstate = 0;
int panicbounce = 0;

int powerstate = 0;
int gps_counter = 0;

char panic_state[] = "0";
char gps_lat[] = "37.227726000000";  //default lat and long values for testing
char gps_long[] = "-80.42221600000";
float batpercent = 100;

bool inPanic = false;
bool prevPanicState = false;

#define panicpin A0
#define panicpinled 16

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);
TinyGPSPlus gps;

void setup() 
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  //Serial.begin(9600);  //comment this back in to allow print statements
  Serial1.begin(9600);
  // while (!Serial || !Serial1) {
  //   delay(1);
  // }
  while(!Serial1) {
    delay(10);
  }

  delay(100);

  //intitate pins on the feather for I/O
  pinMode(panicpin, INPUT);

  pinMode(A1, INPUT);

  pinMode(panicpinled, OUTPUT);

  pinMode(A3, OUTPUT);

  pinMode(1, OUTPUT);

  //for battery read
  pinMode(VBATPIN, INPUT_PULLUP);
  //for display
  myDisplay.begin(SSD1306_SWITCHCAPVCC, 0x3C);



  // Serial.println("Feather LoRa TX Test!");

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    // Serial.println("LoRa radio init failed");
    // Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    while (1);
  }
  // Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    // Serial.println("setFrequency failed");
    while (1);
  }
  // Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
}

int16_t packetnum = 0;  // packet counter, we increment per xmission

void loop()
{
  // Serial.println("Transmitting..."); // Send a message to rf95_server

  //initialize panic state and gps values
  
/**  COMMENTED OUT FOR SMART DELAY
  if(Serial1.available()){
    char c = Serial1.read();
    gps.encode(c);
  }
  **/
smartDelay(100); //Spends one second filling recieve buffer
  
  if(gps.location.isValid()){
    
    Serial.println("done");
    Serial.print("Latitude:         ");
    Serial.println(gps.location.lat(), 6);
    float gps_lat_float = gps.location.lat();  
    String gps_lat_string = "";
    gps_lat_string += String(int(gps_lat_float)) + "." + String(getDecimal(gps_lat_float)); //combining both whole and decimal part in string with a fullstop between them
    // Serial.print("stringVal: ");Serial.println(gps_lat);              //display string value
    gps_lat_string.toCharArray(gps_lat, gps_lat_string.length()+1);
    while(strlen(gps_lat) < 15) {
      strcat(gps_lat, "0");
    }

    float gps_long_float = gps.location.lng();
    String gps_long_string = "";
    gps_long_string += String(int(gps_long_float)) + "." + String(getDecimal(gps_long_float)); //combining both whole and decimal part in string with a fullstop between them
    // Serial.print("stringVal: ");Serial.println(gps_lat); 
    gps_long_string.toCharArray(gps_long, gps_long_string.length()+1);
    while(strlen(gps_long) < 15) {
      strcat(gps_long, "0");
    }

  }

  //panic button
  valbutton = analogRead(panicpin);

  //power switch
  valpower = analogRead(A1);

  // Serial.println(valbutton); 

   if (valbutton > 1000) {
    if (panicbounce == 0)
      if (panicstate == 0){
        panicstate = 1;
        panicbounce = 1;

      }
      else if (panicstate == 1){
        panicstate = 0;
        panicbounce = 1;
      }
  }
  else {
    panicbounce = 0;
  }
  //sense panic button - hen its greater than 1000 the button is sensed...
  if (panicstate == 1) {
    //led on
    digitalWrite(panicpinled, HIGH);;
    panic_state[0] = '1';
    if (prevPanicState == false) {
      prevPanicState = true;
      gps_counter = 100;
    }

  } else {
    //led off
    digitalWrite(panicpinled, LOW);
    panic_state[0] = '0';
    if (prevPanicState == true) {
      prevPanicState = false;
      gps_counter = 100;
    }
  }
  //this is using switch button - greater than 1000 when swich is down
  if (valpower > 1000) {

    digitalWrite(A3, LOW);
    //Serial.println("Device powered on!");
  } else {
    //led off
    digitalWrite(A3, HIGH);

  }

  //CODE FOR BATTERY READ AND OLED DISPLAY
  if (gps_counter >= 200){
  measuredvbat = analogRead(VBATPIN);
  // Serial.println(measuredvbat);
  measuredvbat = 750;
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /=  1024; // convert to voltage
  
  batpercent = ((measuredvbat - lowvoltage) / (highvoltage - lowvoltage)) * 100;  //linear battery relationship - does not perfectly follow model of battery
  // Serial.println("batpercent: " + String(batpercent));  

  //***OLED DISPLAY CODE - UNCOMMENT TO USE DISPLAY***
  //note that uncommenting this code will slow down the time between trasnmits
  // myDisplay.clearDisplay();
  // myDisplay.setTextColor(WHITE);

  // myDisplay.setTextSize(2);
  // myDisplay.setCursor(0,0);
  // myDisplay.print("Beacon #");
  // myDisplay.print(BEACON_ID);

  // myDisplay.setTextSize(1);
  // myDisplay.setCursor(0,20);
  // myDisplay.print("BAT:");
  // String batstr = String(measuredvbat, 2);
  // myDisplay.print(batstr);
  // myDisplay.print("V|");

  // myDisplay.setTextSize(2);
  // String batpercentstr = String(batpercent, 1);
  // myDisplay.print(batpercentstr);
  // myDisplay.print("%");

  // myDisplay.setTextSize(2);
  // myDisplay.setCursor(0,40);
  // myDisplay.print("Panic ");
  // if (panicstate){
  //   myDisplay.print("ON");
  // }
  // else {
  //   myDisplay.print("OFF");
  // }

  // myDisplay.display();

  // }
  

  //convert gps data to an array of chars

  //Beacon ID - 2, lat - 12, long - 12, panic state - 1
  if(gps_counter >= 100) {
    
    char radiopacket[35] = "";
    strcat(radiopacket, BEACON_ID);
    strcat(radiopacket, gps_lat);
    strcat(radiopacket, gps_long);

    //***dummy battery reading for testing - uncomment if there are issues with the actual battery reading below
    // String bat_string = "";
    // char bat_chars[] = "097";   

    //***actaul battery reading - if there are issues, comment out and use dummy reading above
    bat_string += String(int(ceil(batpercent)));
    Serial.println("string batpercent: " + bat_string);

    if(bat_string.length() == 2) {
      bat_chars[0] = '0';
    }
    if(bat_string.length() == 1) {
      bat_chars[0] = '0';
      bat_chars[1] = '0';
    }
     
    // // Serial.print("stringVal: ");Serial.println(gps_lat); 
    // bat_string.toCharArray(bat_chars, bat_string.length()+1);
    strcat(radiopacket, bat_chars);
    strcat(radiopacket, panic_state);
    //Serial.print("Sending "); Serial.println(radiopacket);
    rf95.send((uint8_t *)radiopacket, 36);
    gps_counter = 0;
  }
  gps_counter += 1;

  delay(50);

}

//function to extract decimal part of float
long getDecimal(float val)
{
  int intPart = int(val);
  long decPart = 1000000000*(val-intPart); //I am multiplying by 1000 assuming that the foat values will have a maximum of 3 decimal places. 
                                    //Change to match the number of decimal places you need
  if(decPart>0)return(decPart);           //return the decimal part of float number if it is available 
  else if(decPart<0)return((-1)*decPart); //if negative, multiply by -1
  else if(decPart=0)return(00);           //return 0 if decimal part of float number is not available
  return 0;

}

static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (Serial1.available())
      gps.encode(Serial1.read());
  } while (millis() - start < ms);
}
