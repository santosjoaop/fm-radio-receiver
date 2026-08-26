/**
 * @file btn.h
 * @brief Joystick and button interface for the Digitally Controlled FM Radio Receiver.
 *
 * Defines pin numbers and button identifier constants for the
 * 7-direction joystick module, and declares the public API.
 *
 * @author J.P.Santos 
 * @date 18/02/2026 - 12/06/2026
 */

#ifndef BTN_H
#define BTN_H

#include <Arduino.h>

// Joystick GPIO pin assignments
#define UP 12
#define DWN 11
#define LEF 10
#define RHT 9
#define MID 8
#define SET 7
#define RST 6

// Button identifier constants returned by ReadButton()
#define BTN_NONE 0
#define BTN_UP   1
#define BTN_DWN  2
#define BTN_LEF  3
#define BTN_RHT  4
#define BTN_MID  5
#define BTN_SET  6
#define BTN_RST  7
/** @} */

/**
 * @brief Initialise all joystick GPIO pins as INPUT_PULLUP.
 *
 * Must be called once in setup() before any call to ReadButton().
 */

void JoyStickInit();

/**
 * @brief Read the currently pressed button/direction.
 *
 * Checks each pin in priority order (UP first, RST last).
 * Returns the identifier of the first pin found LOW, or BTN_NONE
 * if no button is currently pressed.
 *
 * @return One of the BTN_* constants defined in this header.
 */
 
int ReadButton();

#endif