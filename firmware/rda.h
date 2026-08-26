/**
 * @file rda.h
 * @brief RDA5807M FM radio module interface.
 *
 * Declares the public API for initialising and controlling the RDA5807M
 * FM receiver via the radio library. Also exports the current volume level
 * so other modules (e.g. display.cpp) can read it.
 *
 * @author J.P.Santos
 * @date 18/02/2026 - 12/06/2026
 */

#ifndef RDA_H
#define RDA_H

#define SHUTDOWN_PIN 19     ///< GPIO pin connected to the shutdown 

#include <radio.h>
#include <RDA5807M.h>
#include <Arduino.h>

/// Current volume level (0–15). Defined in rda.cpp, extern here for display use.
extern int currentVolume;

/// Mute state flag. true = muted, false = unmuted.
extern bool isMuted;

/**
 * @brief Initialise the RDA5807M radio module.
 *
 * Starts I2C communication, tunes to @p freq, sets the default volume,
 * disables mono mode, unmutes the output and configures the amplifier
 * shutdown pin.
 *
 * @param freq Initial frequency in RDA units (e.g. 9320 = 93.2 MHz).
 */
void RadioInit(uint16_t freq);

/**
 * @brief Read the current RSSI (Received Signal Strength Indicator).
 *
 * Queries the RDA5807M radio info struct and returns the raw RSSI value.
 *
 * @return RSSI value as reported by the RDA5807M (typically 0–63).
 */
int getRSSI();

/**
 * @brief Tune the radio to a specific frequency.
 *
 * @param freq Target frequency in RDA units (e.g. 9320 = 93.2 MHz).
 */
void RadioSetFreq(uint16_t freq);

/**
 * @brief Increase the output volume by one step.
 *
 * Clamps at the maximum volume of 15. If the volume was 0 (muted via
 * volume control) the amplifier is re-enabled before increasing.
 */
void RadioVolUp();

/**
 * @brief Decrease the output volume by one step.
 *
 * Clamps at 0. When volume reaches 0 the radio is muted and the
 * amplifier shutdown pin is asserted.
 */
void RadioVolDown();

/**
 * @brief Toggle mute on and off.
 *
 * Uses the isMuted flag to track state independently of the volume level,
 * so unmuting restores audio without changing the volume setting.
 * Sends "MUTE:ON" or "MUTE:OFF" to Serial after toggling.
 */
void RadioMuteToggle();

#endif