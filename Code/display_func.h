#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define DISPLAY_W 128
#define DISPLAY_H 64
#define DISPLAY_RESET -1
#define I2C_DISPLAY_ADDR 0x3C

void DisplayInit();
void DisplayWrite(float freq, int rssi);

#endif
