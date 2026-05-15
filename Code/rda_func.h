#ifndef RDA_H
#define RDA_H

#define SHUTDOWN_PIN 19

#include <radio.h>
#include <RDA5807M.h>
#include <Arduino.h>

extern int currentVolume;

void RadioInit(uint16_t freq);
int getRSSI();

// Novas funções
void RadioSetFreq(uint16_t freq);
void RadioVolUp();
void RadioVolDown();

#endif
