#include "rda.h"
#include <Wire.h>

RDA5807M radio;
int currentVolume = 5;

void RadioInit(uint16_t freq){
  //-----------------------//
  Serial.println("Radio Init");
  radio.initWire(Wire);
  delay(50);
  //-----------------------//
  //radio.debugEnable(true);
  //-----------------------//
  Serial.println("Radio set band freq");
  radio.setBandFrequency(RADIO_BAND_FM, freq);
  delay(50);
  //-----------------------//
  radio.debugRadioInfo();
  //-----------------------//
  Serial.println("Radio set Volume");
  radio.setVolume(currentVolume);
  delay(50);
  //-----------------------//
  Serial.println("Radio set Mono");
  radio.setMono(false);
  delay(50);
  //-----------------------//
  Serial.println("Radio set Mute");
  radio.setMute(false);
  delay(50);
  //-----------------------//
  Serial.println("Radio Shutdown");
  pinMode(SHUTDOWN_PIN, OUTPUT);
  digitalWrite(SHUTDOWN_PIN, LOW);
  delay(50);
  //-----------------------//
  Serial.println("Radio init done!");
}

int getRSSI(){
  RADIO_INFO info;
  radio.getRadioInfo(&info);
  return info.rssi;
}

void RadioSetFreq(uint16_t freq) {
  radio.setBandFrequency(RADIO_BAND_FM, freq);
}

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

