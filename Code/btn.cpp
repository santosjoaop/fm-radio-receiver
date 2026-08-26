/**
 * @file btn.cpp
 * @brief Implementation of the joystick and button interface.
 *
 * @author J.P.Santos
 * @date 18/02/2026 - 12/06/2026
 */

#include "btn.h"

/**
 * @brief Initialise all joystick GPIO pins as INPUT_PULLUP.
 *
 * Uses internal pull-up resistors so the pins read HIGH when idle
 * and LOW when the corresponding button is pressed.
 */

void JoyStickInit() {
  pinMode(UP, INPUT_PULLUP);
  pinMode(DWN, INPUT_PULLUP);
  pinMode(LEF, INPUT_PULLUP);
  pinMode(RHT, INPUT_PULLUP);
  pinMode(MID, INPUT_PULLUP);
  pinMode(SET, INPUT_PULLUP);
  pinMode(RST, INPUT_PULLUP);
}

/**
 * @brief Read the currently pressed button/direction.
 *
 * Checks each pin in priority order. The first pin found LOW wins —
 * simultaneous presses are not supported.
 *
 * @return One of the BTN_* constants, or BTN_NONE if nothing is pressed.
 */
 
int ReadButton() {
  if(digitalRead(UP) == LOW) return BTN_UP;
  if(digitalRead(DWN) == LOW) return BTN_DWN;
  if(digitalRead(LEF) == LOW) return BTN_LEF;
  if(digitalRead(RHT) == LOW) return BTN_RHT;
  if(digitalRead(MID) == LOW) return BTN_MID;
  if(digitalRead(SET) == LOW) return BTN_SET;
  if(digitalRead(RST) == LOW) return BTN_RST;
  
  return BTN_NONE; // Nenhum "botão" pressionado
}