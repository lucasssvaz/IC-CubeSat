#include "rs8.h"
#include "images.h"
#include <Arduino.h>
#include <RHSoftwareSPI.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <SD.h>
#include <SSD1306.h>
#include <pgmspace.h>
#include <string.h>

#define DEBUG

#ifdef DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTLN2(x,y) Serial.println(x,y)
  #define DEBUG_PRINTF(x,y) printf(x,y)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTLN2(x,y)
  #define DEBUG_PRINTF(x,y)
#endif

//========================================================================================================= CONSTANTS

#define CALLSIGN "HABT"

// Serial Settings
const int SERIAL_BR = 57600;

// Display Settings (I2C)
const int DISPLAY_ADDRESS = 0x3c;
const int DISPLAY_SDA = 4;
const int DISPLAY_SCL = 15;
const int DISPLAY_RST = 16;
const int FONT_HEIGHT = 10;

// LoRa Settings (SPI)
const int LORA_SCK = 5;
const int LORA_MISO = 19;
const int LORA_MOSI = 27;
const int LORA_SS = 18;
const int LORA_RST = 15;
const int LORA_DI00 = 26;
const float LORA_FREQ = 915.0;
const int LORA_SF = 7;
const int LORA_CODING_RATE = 5;
const int LORA_BANDWIDTH = 62.5E3;

// Payload Bytes
const int SYNC_BYTE = 0x55;
const int PACKAGE_TYPE = 0x66;


//=============================================================================================== VARIABLES

SSD1306 display(DISPLAY_ADDRESS, DISPLAY_SDA, DISPLAY_SCL);
SPIClass sd_spi(HSPI);

RHSoftwareSPI sx1278_spi;
RH_RF95 rf95(LORA_SS, LORA_DI00, sx1278_spi);
uint8_t lora_len = RH_RF95_MAX_MESSAGE_LEN;

uint8_t lora_buf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t lora_tk[RH_RF95_MAX_MESSAGE_LEN];

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
  display.setFont(ArialMT_Plain_10);
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

  rf95.setModeRx();
  
  //rf95.setSpreadingFactor(LORA_SF);
  //rf95.setSignalBandwidth(LORA_BANDWIDTH);
  //rf95.setCodingRate4(LORA_CODING_RATE);
  //rf95.setPayloadCRC(true);

  //rf95.printRegisters();
  
}

//==================================================================================================== MAIN

void setup() 
{
  Serial.begin(SERIAL_BR);
  
  DEBUG_PRINTLN("Starting Receiver...");

  displayInit();
  loraInit();
}

//------------------------------------------------------------------------------

void loop() 
{  

  int line = 0;

  display.display();

  lora_len = RH_RF95_MAX_MESSAGE_LEN;
    
  if(rf95.recv(lora_buf, &lora_len))
  {
    //RH_RF95::printBuffer("request: ", lora_buf, lora_len);

    memcpy(lora_tk, lora_buf, lora_len);

    String C_Temp = strtok((char*)lora_tk, " ");
    String C_Pres = strtok(NULL, " ");
    String Def_DV = strtok(NULL, " ");
    String Def_RV = strtok(NULL, " ");
    String Eq1_DV = strtok(NULL, " ");
    String Eq1_RV = strtok(NULL, " ");
    String Eq2_DV = strtok(NULL, " ");
    String Eq2_RV = strtok(NULL, " ");
    String Eq3_DV = strtok(NULL, " ");
    String Eq3_RV = strtok(NULL, " ");
    String Eq4_DV = strtok(NULL, " ");
    String Eq4_RV = strtok(NULL, " ");

    DEBUG_PRINT("Got message: ");
    DEBUG_PRINTLN((char*)lora_buf);

    display.clear();

    display.drawString(0, line * FONT_HEIGHT, "Temp: " + C_Temp + " Pres: " + C_Pres );

    line++;

    display.drawString(0, line * FONT_HEIGHT, "DV0: " + Def_DV + " RV0: " + Def_RV );

    line++;

    display.drawString(0, line * FONT_HEIGHT, "DV1: " + Eq1_DV + " RV1: " + Eq1_RV );

    line++;

    display.drawString(0, line * FONT_HEIGHT, "DV2: " + Eq2_DV + " RV2: " + Eq2_RV );

    line++;

    display.drawString(0, line * FONT_HEIGHT, "DV3: " + Eq3_DV + " RV3: " + Eq3_RV );

    line++;
    
    display.drawString(0, line * FONT_HEIGHT, "DV4: " + Eq4_DV + " RV4: " + Eq4_RV );

    Serial.flush();
  }

}
