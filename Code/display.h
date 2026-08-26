/**
 * @file display.h
 * @brief OLED display interface for the Digitally Controlled FM Radio Receiver.
 *
 * Wraps the Adafruit SSD1306 library and provides simple screen
 * drawing functions for the radio, menus and scan configuration screens.
 *
 * @author J.P.Santos
 * @date 18/02/2026 - 12/06/2026
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define DISPLAY_W 128             ///< Display width in pixels
#define DISPLAY_H 64              ///< Display height in pixels
#define DISPLAY_RESET -1          ///< Reset Pin
#define I2C_DISPLAY_ADDR 0x3C     ///< I2C address of the SSD1306 module

/**
 * @brief Initialise the SSD1306 display and show the splash screen.
 *
 * Attempts to start the display over I2C. If initialisation fails the
 * function halts in an infinite loop and prints an error to Serial.
 * On success it shows the project name and author for 2 seconds.
 */
void DisplayInit();

/**
 * @brief Refresh the main radio screen with current frequency and RSSI.
 *
 * Draws:
 *  - A header bar with the project name.
 *  - The tuned frequency in MHz (large text).
 *  - The current RSSI value and volume (small text in the footer).
 *
 * @param freq Tuned frequency in MHz (e.g. 93.2).
 * @param rssi Current RSSI value returned by the RDA5807M.
 */
void DisplayWrite(float freq, int rssi);

/**
 * @brief Draw the top-level scan type selection menu.
 *
 * Shows two options — "Banda Total" and "Freq. Central" — with a
 * ">" cursor next to the currently highlighted option.
 *
 * @param selecao Currently highlighted option index (0 = Total, 1 = Central).
 */
void DisplayMenuMain(int selecao);

/**
 * @brief Draw the scan configuration screen (step and samples side by side).
 *
 * Displays two editable fields — step size (kHz) and number of samples —
 * with a ">" cursor on the field currently selected for editing.
 *
 * @param titulo           Screen title string (e.g. "Config Total").
 * @param salto            Current step size value (in RDA units × 10 kHz).
 * @param amostras         Current number of samples.
 * @param campoSelecionado Index of the active field (0 = step, 1 = samples).
 */
void DisplayMenuConfig(const char* titulo, int salto, int amostras, int campoSelecionado);

/**
 * @brief Draw the centre-frequency selection screen.
 *
 * Shows the currently selected centre frequency in MHz in large text.
 *
 * @param freq Centre frequency in RDA units (e.g. 9320 = 93.2 MHz).
 */
void DisplayMenuFreq(uint16_t freq);

#endif