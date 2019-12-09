#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <math.h>

Adafruit_BMP280 bmp; // I2C

float sea_level_hpa;

float inline Alt_Eq1 (float P0){  //weather.gov
  float P = bmp.readPressure()/100;
  return (1 - pow((P/P0), 0.190284)) * 145366.45 * 0.3048;
}

float inline Alt_Eq2 (float P0){  //brisbanehotairballooning.com.au
  float P = bmp.readPressure()/100;
  return 0.3048 * (pow(10,(log10(P/P0)/5.2558797))-1)/(-6.8755856*pow(10,-6)); //Altitude = (10^(log(P/P_0)/5.2558797)-1/(-6.8755856*10^-6) 
}

float inline Alt_Eq3 (float P0){  //math24.net
  float P = bmp.readPressure()/1000;
  return 25000/3 * log(P0/P);
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("\nBMP280 test\n"));

  Wire.begin(4, 15);

  if (!bmp.begin()) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  sea_level_hpa = bmp.readPressure()/100;
}

void loop() {
    Serial.print(F("Temperature = "));
    Serial.print(bmp.readTemperature());
    Serial.println(" *C");

    Serial.println();

    Serial.print(F("Pressure = "));
    Serial.print(bmp.readPressure());
    Serial.println(" Pa");

    Serial.println();

    Serial.print(F("Altitude (Builtin DV) = "));
    Serial.print(bmp.readAltitude(1013.25)); /* Adjusted to local forecast! */
    Serial.println(" m");

    Serial.println();

    Serial.print(F("Altitude (Builtin RV = "));
    Serial.print(bmp.readAltitude(sea_level_hpa));
    Serial.println(" m");

    Serial.println();

    Serial.print(F("Altitude (Eq1 DV) = "));
    Serial.print(Alt_Eq1(1013.25)); /* Adjusted to local forecast! */
    Serial.println(" m");

    Serial.println();

    Serial.print(F("Altitude (Eq1 RV = "));
    Serial.print(Alt_Eq1(sea_level_hpa));
    Serial.println(" m");

    Serial.println();

    Serial.print(F("Altitude (Eq2 DV) = "));
    Serial.print(Alt_Eq2(1013.25)); /* Adjusted to local forecast! */
    Serial.println(" m");

    Serial.println();

    Serial.print(F("Altitude (Eq2 RV = "));
    Serial.print(Alt_Eq2(sea_level_hpa));
    Serial.println(" m");

    Serial.println();

    Serial.print(F("Altitude (Eq3 DV) = "));
    Serial.print(Alt_Eq3(1013.25)); /* Adjusted to local forecast! */
    Serial.println(" m");

    Serial.println();

    Serial.print(F("Altitude (Eq3 RV = "));
    Serial.print(Alt_Eq3(sea_level_hpa/10));
    Serial.println(" m");

    Serial.println();
    Serial.println();
    Serial.println();
    
    delay(30000);
}
