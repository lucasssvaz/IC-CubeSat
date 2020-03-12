#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <math.h>
#include <RHSoftwareSPI.h>
#include <RH_RF95.h>

#define DEBUG_PRINTLN
#define DEBUG_PRINTF

Adafruit_BMP280 bmp; // I2C

float sea_level_hpa;

float sea_level_temp;

float last_alt = 0;

float C_Pres, C_Temp, Def_DV, Def_RV, Eq1_DV, Eq1_RV, Eq2_DV, Eq2_RV, Eq3_DV, Eq3_RV, Eq4_DV, Eq4_RV;
String Payload;

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

RHSoftwareSPI sx1278_spi;
RH_RF95 rf95(LORA_SS, LORA_DI00, sx1278_spi);
uint8_t lora_len = RH_RF95_MAX_MESSAGE_LEN;

char lora_buf[RH_RF95_MAX_MESSAGE_LEN];

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

float inline Alt_Eq1 (float P0){  //weather.gov
  float res;
  float P = bmp.readPressure()/100;
  res = (1 - pow((P/P0), 0.190284)) * 145366.45 * 0.3048;
  return res;
}

float inline Alt_Eq2 (float P0){  //brisbanehotairballooning.com.au
  float P = bmp.readPressure()/100;
  float res;
  res = 0.3048 * (pow(10,(log10(P/P0)/5.2558797))-1)/(-6.8755856*pow(10,-6)); //Altitude = (10^(log(P/P_0)/5.2558797)-1/(-6.8755856*10^-6) 
  return res;
}

float inline Alt_Eq3 (float P0){  //math24.net
  float P = bmp.readPressure()/1000;
  float res;
  res = 25000/3 * log(P0/P);
  return res;
}

float inline Alt_Eq4 (float P0, float H0){  //mide.com
  float P = bmp.readPressure();
  if (last_alt < 11000){
    last_alt = H0 + (sea_level_temp+273.15)/-0.0065 * ( pow(P/P0, (-8.31432 * -0.0065)/(9.80665 * 0.0289644)) - 1 );
  } else {
    last_alt = H0 + (8.31432 * (sea_level_temp+273.15) * log(P/P0)) / (-9.80665 * 0.0289644);
  }
  return last_alt;
}

void setup() {
  Serial.begin(57600);
  Serial.println(F("\nBMP280 test\n"));

  pinMode(23, OUTPUT);

  Wire.begin(21, 22);

  if (!bmp.begin(0x76)) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    digitalWrite(23, HIGH);
    while (1);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  sea_level_hpa = bmp.readPressure()/100;

  sea_level_temp = bmp.readTemperature();

  loraInit();
  
}



void loop() {
    Serial.print(F("Temperature = "));
    C_Temp = bmp.readTemperature();
    Serial.print(C_Temp);
    Serial.println(" *C");

    Serial.println();

    Serial.print(F("Pressure = "));
    C_Pres = bmp.readPressure();
    Serial.print(C_Pres);
    Serial.println(" Pa");

    Serial.println();

    Serial.print(F("Altitude (Builtin DV) = "));
    Def_DV = bmp.readAltitude(1013.25);
    Serial.print(Def_DV); /* Adjusted to local forecast! */
    Serial.println(" m");

    Serial.println();

    Serial.print(F("Altitude (Builtin RV) = "));
    Def_RV = bmp.readAltitude(sea_level_hpa);
    Serial.print(Def_RV);
    Serial.println(" m");

    Serial.println();

    Serial.print(F("Altitude (Eq1 DV) = "));
    Eq1_DV = Alt_Eq1(1013.25);
    Serial.print(Eq1_DV); /* Adjusted to local forecast! */
    Serial.println(" m");

    Serial.println();

    Serial.print(F("Altitude (Eq1 RV) = "));
    Eq1_RV = Alt_Eq1(sea_level_hpa);
    Serial.print(Eq1_RV);
    Serial.println(" m");

    Serial.println();

    Serial.print(F("Altitude (Eq2 DV) = "));
    Eq2_DV = Alt_Eq2(1013.25);
    Serial.print(Eq2_DV); /* Adjusted to local forecast! */
    Serial.println(" m");

    Serial.println();

    Serial.print(F("Altitude (Eq2 RV) = "));
    Eq2_RV = Alt_Eq2(sea_level_hpa);
    Serial.print(Eq2_RV);
    Serial.println(" m");

    Serial.println();

    Serial.print(F("Altitude (Eq3 DV) = "));
    Eq3_DV = Alt_Eq3(1013.25/10);
    Serial.print(Eq3_DV); /* Adjusted to local forecast! */
    Serial.println(" m");

    Serial.println();

    Serial.print(F("Altitude (Eq3 RV) = "));
    Eq3_RV = Alt_Eq3(sea_level_hpa/10);
    Serial.print(Eq3_RV);
    Serial.println(" m");

    Serial.println();

    Serial.print(F("Altitude (Eq4 DV) = "));
    Eq4_DV = Alt_Eq4(101325, 0);
    Serial.print(Eq4_DV); /* Adjusted to local forecast! */
    Serial.println(" m");

    Serial.println();

    Serial.print(F("Altitude (Eq4 RV) = "));
    Eq4_RV = Alt_Eq4(sea_level_hpa*100, 0);
    Serial.print(Eq4_RV);
    Serial.println(" m");

    Serial.println();
    Serial.println();
    

    Payload = String(C_Temp) + " " + String(C_Pres) + " " + String(Def_DV) + " " + String(Def_RV) + " " + String(Eq1_DV) + " " + String(Eq1_RV)  + " " + String(Eq2_DV) + " " + String(Eq2_RV)  + " " + String(Eq3_DV) + " " + String(Eq3_RV)  + " " + String(Eq4_DV) + " " + String(Eq4_RV);
    Serial.println(Payload);

    Payload.toCharArray(lora_buf,RH_RF95_MAX_MESSAGE_LEN); 
    rf95.send((uint8_t *)lora_buf, lora_len);
    rf95.waitPacketSent();
    Serial.println("Payload Sent");
    Serial.println();
    
    delay(15000);
}
