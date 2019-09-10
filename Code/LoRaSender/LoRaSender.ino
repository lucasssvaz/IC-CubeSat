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

//Term Settings (OneWire)
const int ONEWIRE_PIN = 33;

// Payload Bytes
const int SYNC_BYTE = 0x55;
const int PACKAGE_TYPE = 0x66;

//RS8 Settings
#define MM     (8)
#define NN     (255)
#define NROOTS (32)
#define FCR    (112)
#define PRIM   (11)
#define IPRIM  (116)

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define A0       (NN) /* Special reserved value encoding zero in index form */

PROGMEM const uint8_t alpha_to[] = {
0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x87,0x89,0x95,0xAD,0xDD,0x3D,0x7A,0xF4,
0x6F,0xDE,0x3B,0x76,0xEC,0x5F,0xBE,0xFB,0x71,0xE2,0x43,0x86,0x8B,0x91,0xA5,0xCD,
0x1D,0x3A,0x74,0xE8,0x57,0xAE,0xDB,0x31,0x62,0xC4,0x0F,0x1E,0x3C,0x78,0xF0,0x67,
0xCE,0x1B,0x36,0x6C,0xD8,0x37,0x6E,0xDC,0x3F,0x7E,0xFC,0x7F,0xFE,0x7B,0xF6,0x6B,
0xD6,0x2B,0x56,0xAC,0xDF,0x39,0x72,0xE4,0x4F,0x9E,0xBB,0xF1,0x65,0xCA,0x13,0x26,
0x4C,0x98,0xB7,0xE9,0x55,0xAA,0xD3,0x21,0x42,0x84,0x8F,0x99,0xB5,0xED,0x5D,0xBA,
0xF3,0x61,0xC2,0x03,0x06,0x0C,0x18,0x30,0x60,0xC0,0x07,0x0E,0x1C,0x38,0x70,0xE0,
0x47,0x8E,0x9B,0xB1,0xE5,0x4D,0x9A,0xB3,0xE1,0x45,0x8A,0x93,0xA1,0xC5,0x0D,0x1A,
0x34,0x68,0xD0,0x27,0x4E,0x9C,0xBF,0xF9,0x75,0xEA,0x53,0xA6,0xCB,0x11,0x22,0x44,
0x88,0x97,0xA9,0xD5,0x2D,0x5A,0xB4,0xEF,0x59,0xB2,0xE3,0x41,0x82,0x83,0x81,0x85,
0x8D,0x9D,0xBD,0xFD,0x7D,0xFA,0x73,0xE6,0x4B,0x96,0xAB,0xD1,0x25,0x4A,0x94,0xAF,
0xD9,0x35,0x6A,0xD4,0x2F,0x5E,0xBC,0xFF,0x79,0xF2,0x63,0xC6,0x0B,0x16,0x2C,0x58,
0xB0,0xE7,0x49,0x92,0xA3,0xC1,0x05,0x0A,0x14,0x28,0x50,0xA0,0xC7,0x09,0x12,0x24,
0x48,0x90,0xA7,0xC9,0x15,0x2A,0x54,0xA8,0xD7,0x29,0x52,0xA4,0xCF,0x19,0x32,0x64,
0xC8,0x17,0x2E,0x5C,0xB8,0xF7,0x69,0xD2,0x23,0x46,0x8C,0x9F,0xB9,0xF5,0x6D,0xDA,
0x33,0x66,0xCC,0x1F,0x3E,0x7C,0xF8,0x77,0xEE,0x5B,0xB6,0xEB,0x51,0xA2,0xC3,0x00,
};

PROGMEM const uint8_t index_of[] = {
0xFF,0x00,0x01,0x63,0x02,0xC6,0x64,0x6A,0x03,0xCD,0xC7,0xBC,0x65,0x7E,0x6B,0x2A,
0x04,0x8D,0xCE,0x4E,0xC8,0xD4,0xBD,0xE1,0x66,0xDD,0x7F,0x31,0x6C,0x20,0x2B,0xF3,
0x05,0x57,0x8E,0xE8,0xCF,0xAC,0x4F,0x83,0xC9,0xD9,0xD5,0x41,0xBE,0x94,0xE2,0xB4,
0x67,0x27,0xDE,0xF0,0x80,0xB1,0x32,0x35,0x6D,0x45,0x21,0x12,0x2C,0x0D,0xF4,0x38,
0x06,0x9B,0x58,0x1A,0x8F,0x79,0xE9,0x70,0xD0,0xC2,0xAD,0xA8,0x50,0x75,0x84,0x48,
0xCA,0xFC,0xDA,0x8A,0xD6,0x54,0x42,0x24,0xBF,0x98,0x95,0xF9,0xE3,0x5E,0xB5,0x15,
0x68,0x61,0x28,0xBA,0xDF,0x4C,0xF1,0x2F,0x81,0xE6,0xB2,0x3F,0x33,0xEE,0x36,0x10,
0x6E,0x18,0x46,0xA6,0x22,0x88,0x13,0xF7,0x2D,0xB8,0x0E,0x3D,0xF5,0xA4,0x39,0x3B,
0x07,0x9E,0x9C,0x9D,0x59,0x9F,0x1B,0x08,0x90,0x09,0x7A,0x1C,0xEA,0xA0,0x71,0x5A,
0xD1,0x1D,0xC3,0x7B,0xAE,0x0A,0xA9,0x91,0x51,0x5B,0x76,0x72,0x85,0xA1,0x49,0xEB,
0xCB,0x7C,0xFD,0xC4,0xDB,0x1E,0x8B,0xD2,0xD7,0x92,0x55,0xAA,0x43,0x0B,0x25,0xAF,
0xC0,0x73,0x99,0x77,0x96,0x5C,0xFA,0x52,0xE4,0xEC,0x5F,0x4A,0xB6,0xA2,0x16,0x86,
0x69,0xC5,0x62,0xFE,0x29,0x7D,0xBB,0xCC,0xE0,0xD3,0x4D,0x8C,0xF2,0x1F,0x30,0xDC,
0x82,0xAB,0xE7,0x56,0xB3,0x93,0x40,0xD8,0x34,0xB0,0xEF,0x26,0x37,0x0C,0x11,0x44,
0x6F,0x78,0x19,0x9A,0x47,0x74,0xA7,0xC1,0x23,0x53,0x89,0xFB,0x14,0x5D,0xF8,0x97,
0x2E,0x4B,0xB9,0x60,0x0F,0xED,0x3E,0xE5,0xF6,0x87,0xA5,0x17,0x3A,0xA3,0x3C,0xB7,
};

PROGMEM const uint8_t poly[] = {
0x00,0xF9,0x3B,0x42,0x04,0x2B,0x7E,0xFB,0x61,0x1E,0x03,0xD5,0x32,0x42,0xAA,0x05,
0x18,0x05,0xAA,0x42,0x32,0xD5,0x03,0x1E,0x61,0xFB,0x7E,0x2B,0x04,0x42,0x3B,0xF9,
0x00,
};


//=============================================================================================== VARIABLES

int counter = 0;
char *StrCounter;

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

inline int mod255(int x)
{
  while(x >= 255)
  {
    x -= 255;
    x = (x >> 8) + (x & 255);
  }
  return(x);
}

//----------------------------------------------------------------------------------

void encode_rs_8(uint8_t *data, uint8_t *parity, int pad)
{
  int i, j;
  uint8_t feedback;
  
  memset(parity, 0, NROOTS * sizeof(uint8_t));
  
  for(i = 0; i < NN - NROOTS - pad; i++)
  {
    feedback = pgm_read_byte(&index_of[data[i] ^ parity[0]]);
    if(feedback != A0) /* feedback term is non-zero */
    {
      for(j = 1; j < NROOTS; j++)
        parity[j] ^= pgm_read_byte(&alpha_to[mod255(feedback + pgm_read_byte(&poly[NROOTS - j]))]);
    }
    
    /* Shift */
    memmove(&parity[0], &parity[1], sizeof(uint8_t) * (NROOTS - 1));
    if(feedback != A0)
      parity[NROOTS - 1] = pgm_read_byte(&alpha_to[mod255(feedback + pgm_read_byte(&poly[0]))]);
    else
      parity[NROOTS - 1] = 0;
  }
}

//--------------------------------------------------------------------------------------------------

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
  delay(1000);
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

  RH_RF95::ModemConfig myconfig =  {RH_RF95_BW_62_5KHZ, RH_RF95_CODING_RATE_4_5, RH_RF95_SPREADING_FACTOR_128CPS};
  rf95.setModemRegisters(&myconfig);

  float Freq = LORA_FREQ;

  if (!rf95.setFrequency(LORA_FREQ))
    DEBUG_PRINTLN("LoRa Radio: WARNING! setFrequency failed.");
  else
    DEBUG_PRINTF("LoRa Radio: Freqency set to %.1f MHz\n", Freq);

  DEBUG_PRINTF("LoRa Radio: Max Msg size: %u Bytes\n", RH_RF95_MAX_MESSAGE_LEN);

  rf95.setModeTx();
  rf95.setTxPower(17, false);
  rf95.setSpreadingFactor(LORA_SF);
  rf95.setSignalBandwidth(LORA_BANDWIDTH);
  rf95.setCodingRate4(LORA_CODING_RATE);
  rf95.setPayloadCRC(true);

  //rf95.printRegisters();
  
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
  
  DEBUG_PRINTLN("Starting...");

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
  
  String aux = "hello " + String(StrCounter);
  aux.toCharArray(lora_buf,RH_RF95_MAX_MESSAGE_LEN);
  rf95.send((uint8_t *)lora_buf, lora_len);
  rf95.waitPacketSent();

  //DEBUG_PRINTLN("Sent: hello " + String(counter));

  counter++;

  //getTemp();

  delay(1000);
}
