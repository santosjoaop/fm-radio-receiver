/**
 * @file rda.cpp
 * @brief Implementation of the RDA5807M FM radio module interface.
 *
 * @author J.P.Santos
 * @date 18/02/2026 - 12/06/2026
 */

#include "rda.h"
#include <Wire.h>

/// Global RDA5807M radio object from the radio library.
RDA5807M radio;

/// Current volume level (0–15). Initialised to 5.
int currentVolume = 5;

/// Mute state flag. false = audio on, true = muted.
bool isMuted = false;

/**
 * @brief Initialise the RDA5807M radio module.
 *
 * @param freq Initial frequency in RDA units.
 */
void RadioInit(uint16_t freq){
  Serial.println("Radio Init");
  radio.initWire(Wire);
  delay(50);

  Serial.println("Radio set band freq");
  radio.setBandFrequency(RADIO_BAND_FM, freq);
  delay(50);

  radio.debugRadioInfo();

  Serial.println("Radio set Volume");
  radio.setVolume(currentVolume);
  delay(50);

  Serial.println("Radio set Mono");
  radio.setMono(false);
  delay(50);

  Serial.println("Radio set Mute");
  radio.setMute(false);
  delay(50);

  Serial.println("Radio Shutdown");
  pinMode(SHUTDOWN_PIN, OUTPUT);
  digitalWrite(SHUTDOWN_PIN, LOW);
  delay(50);

  Serial.println("Radio init done!");
}

/**
 * @brief Read the current RSSI from the RDA5807M.
 *
 * @return Raw RSSI value (0–63 typical range).
 */
int getRSSI(){
  RADIO_INFO info;
  radio.getRadioInfo(&info);
  return info.rssi;
}

/**
 * @brief Tune the radio to a specific frequency.
 *
 * @param freq Target frequency in RDA units.
 */
void RadioSetFreq(uint16_t freq) {
  radio.setBandFrequency(RADIO_BAND_FM, freq);
}

/**
 * @brief Increase the output volume by one step (max 15).
 *
 * If coming from volume 0, re-enables the amplifier before increasing.
 */
void RadioVolUp() {
  if (currentVolume < 15) {

    if (currentVolume == 0){
      digitalWrite(SHUTDOWN_PIN, LOW);
      delay(10);
      radio.setMute(false);
    }
      currentVolume++;
      radio.setVolume(currentVolume);
    

    Serial.print("Volume: "); 
    Serial.println(currentVolume);

  }
}

/**
 * @brief Decrease the output volume by one step (min 0).
 *
 * When volume reaches 0, mutes the radio and asserts the amplifier
 * shutdown pin to cut power to the speaker stage.
 */
void RadioVolDown() {
  if (currentVolume > 0) {
    currentVolume--;
    
    if (currentVolume == 0){
      radio.setMute(true);
      digitalWrite(SHUTDOWN_PIN, HIGH);
    } else{
      radio.setVolume(currentVolume);
    }
   
    Serial.print("Volume: "); 
    Serial.println(currentVolume);
  }
}

/**
 * @brief Toggle mute on and off using a dedicated flag.
 *
 * Tracks mute state in isMuted independently of currentVolume so that
 * unmuting always restores the previous volume level.
 * Sends "MUTE:ON" or "MUTE:OFF" to Serial after each toggle.
 */
void RadioMuteToggle() {
  isMuted = !isMuted;
  if (isMuted) {
    radio.setMute(true);
    digitalWrite(SHUTDOWN_PIN, HIGH);
    Serial.println("MUTE:ON");
  } else {
    radio.setMute(false);
    digitalWrite(SHUTDOWN_PIN, LOW);
    Serial.println("MUTE:OFF");
  }
}


