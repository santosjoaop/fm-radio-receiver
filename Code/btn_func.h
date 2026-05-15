#ifndef BTN_H
#define BTN_H

#include <Arduino.h>

#define UP 12
#define DWN 11
#define LEF 10
#define RHT 9
#define MID 8
#define SET 7
#define RST 6

// Identificadores de cada "botão"
#define BTN_NONE 0
#define BTN_UP   1
#define BTN_DWN  2
#define BTN_LEF  3
#define BTN_RHT  4
#define BTN_MID  5
#define BTN_SET  6
#define BTN_RST  7

void JoyStickInit();
int ReadButton();

#endif
