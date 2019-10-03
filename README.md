# Tycho Satellite
[![Maintenance](https://img.shields.io/badge/Maintained%3F-Yes-brightgreen.svg)](https://GitHub.com/lucasssvaz/IC-CubeSat/graphs/commit-activity)
[![Latest](https://img.shields.io/github/v/release/lucasssvaz/IC-CubeSat?include_prereleases&label=Release)](https://github.com/lucasssvaz/IC-CubeSat/releases/latest)
[![Downloads](https://img.shields.io/github/downloads/lucasssvaz/IC-CubeSat/total?label=Downloads)](https://github.com/lucasssvaz/IC-CubeSat/releases/latest)
[![GitHub license](https://img.shields.io/github/license/lucasssvaz/IC-CubeSat?color=yellow&label=License)](https://github.com/lucasssvaz/IC-CubeSat/blob/master/LICENSE)
[![saythanks](https://img.shields.io/badge/Say-Thanks-ff69b4.svg)](https://saythanks.io/to/lucasssvaz)
[![translate](https://img.shields.io/badge/Help%20Us-Translate-blue.svg)](https://gitlocalize.com/repo/3252)

### Table of Contents

1. [Introduction](#introduction)
2. [Objectives](#objectives)
3. [Development](#development)
4. [Getting Started](#getting-started)
	1. [Hardware](#hardware)
	2. [Dependencies](#dependencies)
5. [Usage](#usage)
6. [Future Improvements](#future-improvements)
7. [Troubleshooting Log](#troubleshooting-log)
8. [Authors](#authors)
9. [License](#license)
10. [Acknowledgments](#scknowledgments)
  

## Introduction
This project focus is to design and build a High Altitude Balloon (HAB) Satellite using commercial off-the-shelf (COTS) and low-cost hardware implementing the LoRa radio technology.
The Satellite is named after the danish astronomer Tycho Brahe (1546-1601).

The project was oriented by [Prof. Dr. Lauro Paulo da Silva Neto](http://lattes.cnpq.br/3979447098275675) during scientific research at the Federal University of São Paulo (Brazil). 

## Objectives

The main objective of this assignment is to create a low-cost satellite using COTS components applying the LoRa technology as the communication system.

## Development

## Prerequisites

### Hardware

### Dependencies
- [RadioHead](http://www.airspayce.com/mikem/arduino/RadioHead/) ([Source](https://github.com/PaulStoffregen/RadioHead))
- OneWire ([Source](https://github.com/PaulStoffregen/OneWire)) [Arduino IDE]
- DallasTemperature ([Source](https://github.com/milesburton/Arduino-Temperature-Control-Library)) [Arduino IDE]
- RTClib ([Source](https://github.com/adafruit/RTClib)) [Arduino IDE]
- ESP8266 and ESP32 Oled Driver for SSD1306 display ([Source](https://github.com/ThingPulse/esp8266-oled-ssd1306)) [Arduino IDE]

## Usage

## Future Improvements

## Troubleshooting Log

| Problem: | Reproduction: | Reason: | Development: | Solution: |
|----------|---------------|---------|--------------|-----------|
Can’t connect LoRa and microSD module at the same time. Only one of them works.|Use the “LoRa.h”, “SPI.h” and “SD.h” libraries together, using the same SPI bus with different SS pins.|Unknown.|The SPI and SD libraries work at different frequencies (1 MHz vs 4 MHz). Changing both libraries to the same frequency still doesn’t work. Creating a software simulated SPI works, but requires using the LoRa library “RadioHead”.|Replacing the LoRa library with the “RadioHead” library and using its Software SPI implementation to simulate a separate SPI bus. Use the hardware implemented SPI with the microSD module and the software simulated SPI with the LoRa module.


## Authors

- [Lucas Saavedra Vaz](http://lattes.cnpq.br/9960344593786290)
- [Prof. Dr. Lauro Paulo da Silva Neto](http://lattes.cnpq.br/3979447098275675)

## License

## Acknowledgments