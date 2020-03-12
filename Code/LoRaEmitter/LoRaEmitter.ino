#include "rs8.h"
#include "images.h"
#include <Arduino.h>
#include <DallasTemperature.h>
#include <FS.h>
#include <OneWire.h>
#include <RHSoftwareSPI.h>
#include <RH_RF95.h>
#include <RTClib.h>
#include <SD.h>
#include <SPI.h>
#include <SSD1306.h>
#include <pgmspace.h>
#include <string.h>

#define DEBUG

#ifdef DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(x,y) printf(x,y)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(x,y)
#endif

//========================================================================================================= CONSTANTS

#define CALLSIGN "HABT"

// Serial Settings
const int SERIAL_BR = 57600;

// RTC Settings (I2C)
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// SD Settings (SPI)
const int SD_SCK = 17;
const int SD_MISO = 13;
const int SD_MOSI = 12;
const int SD_SS = 23;

// Display Settings (I2C)
const int DISPLAY_ADDRESS = 0x3c;
const int DISPLAY_SDA = 4;
const int DISPLAY_SCL = 15;
const int DISPLAY_RST = 16;
const int FONT_HEIGHT = 16;

// LoRa Settings (SPI)
const int LORA_SCK = 5;
const int LORA_MISO = 19;
const int LORA_MOSI = 27;
const int LORA_SS = 18;
const int LORA_RST = 15;
const int LORA_DI00 = 26;
const int LORA_FREQ = 915.0;
const int LORA_SF = 7;
const int LORA_CODING_RATE = 5;
const int LORA_BANDWIDTH = 62.5E3;
const int LORA_TXPWR = 20;

//Term Settings (OneWire)
const int ONEWIRE_PIN = 33;

// Payload Bytes
const int SYNC_BYTE = 0x55;
const int PACKAGE_TYPE = 0x66;

//=============================================================================================== VARIABLES

int counter = 0;
int TxTime = 0;

RTC_DS3231 rtc;
SSD1306 display(DISPLAY_ADDRESS, DISPLAY_SDA, DISPLAY_SCL);
SPIClass sd_spi(HSPI);

RHSoftwareSPI sx1278_spi;
RH_RF95 rf95(LORA_SS, LORA_DI00, sx1278_spi);
uint8_t lora_len = RH_RF95_MAX_MESSAGE_LEN;

char lora_buf[RH_RF95_MAX_MESSAGE_LEN];

OneWire oneWire(ONEWIRE_PIN);
DallasTemperature term(&oneWire);

//=============================================================================================== FUNCTIONS

inline void displayInit()
{
  pinMode(DISPLAY_RST, OUTPUT);
  digitalWrite(DISPLAY_RST, LOW);
  delay(1);
  digitalWrite(DISPLAY_RST, HIGH);
  delay(1);
  if(!display.init())
    DEBUG_PRINTLN("Display: WARNING! Init failed!");
  else
    DEBUG_PRINTLN("Display: Init OK!"); 
  displayConfig();
}

//------------------------------------------------------------------------------------------------

inline void displayConfig()
{
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawXbm(0, 0, logo_width, logo_height, (uint8_t*) logo_bits);
  display.display();
  delay(2000);
}

//------------------------------------------------------------------------------------------------

inline void loraInit()
{
  pinMode(LORA_RST, OUTPUT);
  digitalWrite(LORA_RST, LOW);
  delay(100);
  digitalWrite(LORA_RST, HIGH);

  sx1278_spi.setPins(LORA_MISO, LORA_MOSI, LORA_SCK);

  if (!rf95.init()) 
    DEBUG_PRINTLN("LoRa Radio: WARNING! Init failed.");
  else
    DEBUG_PRINTLN("LoRa Radio: Init OK!");

  RH_RF95::ModemConfig myconfig =  {0x72, 0x74, 0x04}; //125 KHz, 4/5 CR, SF7, CRC On, Low Data Rate Op. Off, AGC On
  rf95.setModemRegisters(&myconfig);

  float Freq = LORA_FREQ;

  if (!rf95.setFrequency(LORA_FREQ))
    DEBUG_PRINTLN("LoRa Radio: WARNING! setFrequency failed.");
  else
    DEBUG_PRINTF("LoRa Radio: Freqency set to %.1f MHz\n", Freq);

  DEBUG_PRINTF("LoRa Radio: Max Msg size: %u Bytes\n", RH_RF95_MAX_MESSAGE_LEN);

  rf95.setModeTx();
  rf95.setTxPower(LORA_TXPWR, false);

  #ifdef DEBUG
  	rf95.printRegisters();
  #endif
}

//---------------------------------------------------------------------------------------------------------

inline void SDInit()
{  
  sd_spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_SS);
  
  if(!SD.begin(SD_SS, sd_spi))
    DEBUG_PRINTLN("SD Card: WARNING! Card Mount Failed");
  else
    DEBUG_PRINTLN("SD Card: Card Mount OK!");
    
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE)
    DEBUG_PRINTLN("SD Card: WARNING! No SD card attached");
  else
    DEBUG_PRINTLN("SD Card: SD card detected!");
}

//---------------------------------------------------------------------------------------------------------

void currentTime()
{
  DateTime now = rtc.now();
    
  DEBUG_PRINTLN("Current Date & Time: ");
  
  char dateBuffer[25];

  sprintf(dateBuffer,"%02u/%02u/%04u (%s)",now.day(),now.month(),now.year(),daysOfTheWeek[now.dayOfTheWeek()]);
  DEBUG_PRINTLN(dateBuffer);
  sprintf(dateBuffer,"%02u:%02u:%02u ",now.hour(),now.minute(),now.second());
  DEBUG_PRINTLN(dateBuffer);
}

//---------------------------------------------------------------------------------------------------------

inline void RTCInit()
{  
  if (!rtc.begin())
    DEBUG_PRINTLN("RTC: WARNING! Couldn't find RTC");
  else
    DEBUG_PRINTLN("RTC: Init OK!");

  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  if (rtc.lostPower()) {
    DEBUG_PRINTLN("RTC lost power, setting time...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  currentTime();
}

//----------------------------------------------------------------------------------------------------------

inline void termInit()
{
  DEBUG_PRINTLN("Initializing DS18B20 Temp. Sensor...");
  term.begin();
  term.requestTemperatures();
  DEBUG_PRINT("Current Temp: ");
  float tempC = term.getTempCByIndex(0);
  DEBUG_PRINT(tempC);
  DEBUG_PRINTLN("°C");

  (tempC >= 0 && tempC <= 40) ? DEBUG_PRINTLN("DS18B20: Init OK! Check Readings.") : DEBUG_PRINTLN("DS18B20: WARNING! Init failed. Inaccurate readings detected, check sensor connections.");
  
}

//-----------------------------------------------------------------------------------------------------------

void getTemp()
{
  term.requestTemperatures();
  DEBUG_PRINT("Current Temp: ");
  float tempC = term.getTempCByIndex(0);
  DEBUG_PRINT(tempC);
  DEBUG_PRINTLN("°C");
}

//==================================================================================================== MAIN

void setup() 
{
  Serial.begin(SERIAL_BR);
  
  DEBUG_PRINTLN("Starting Emitter...");

  displayInit();
  loraInit();
  SDInit();
  RTCInit();
  termInit();
}

//------------------------------------------------------------------------------

void loop() 
{  

  int line = 0;
  
  display.clear();
  display.drawString(0, line, "Sending packet: ");
  line++;
  display.drawString(0, line * FONT_HEIGHT, String(counter));

  display.display();
  
  String aux = "hello " + String(counter) + " " + String(TxTime);
  aux.toCharArray(lora_buf,RH_RF95_MAX_MESSAGE_LEN);
  int TxBegin = millis(); 
  rf95.send((uint8_t *)lora_buf, lora_len);
  rf95.waitPacketSent();
  int TxEnd = millis();

  TxTime = TxEnd - TxBegin;

  DEBUG_PRINTLN("Sent: hello " + String(counter) + " " + String(TxTime));

  counter++;

  //currentTime();
  //getTemp();
  //DEBUG_PRINTLN("");

  delay(1000);
}
