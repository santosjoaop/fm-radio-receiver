#include "display.h"

extern int currentVolume;

Adafruit_SSD1306 display(DISPLAY_W, DISPLAY_H, &Wire, DISPLAY_RESET);

void DisplayInit() {

  if(!display.begin(SSD1306_SWITCHCAPVCC, I2C_DISPLAY_ADDR)){
    Serial.println(F("Display Inicialization ERROR!"));
    while(true){
      delay(100);
    }
  }

  //Inicialization
  //-----------------------//
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.drawLine(0,5,128,5, WHITE);
  display.drawLine(0,55,128,55, WHITE);
  display.setCursor(5, 15);
  display.println("Projeto 8");
  display.setCursor(0, 35);
  display.print("J.P.Santos");
  display.display();
  delay(2000);
}


void DisplayWrite(float freq, int rssi) {
  display.clearDisplay();
  
  // Cabeçalho
  //-----------------------//
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("Projeto - P08"));
  display.drawLine(0,10,128,10, WHITE);
  
  // Frequência
  //-----------------------//
  display.setTextSize(2);
  display.setCursor(40, 15);
  display.println(F("Freq:"));
  display.setCursor(20, 35);  
  display.print(freq, 1);
  display.println(F(" MHz"));
  
  // RSSI
  //-----------------------//
  display.setTextSize(1);
  display.setCursor(0, 55);
  display.print(F("RSSI: "));
  display.print(rssi);
  
  // Volume
  //-----------------------//
  display.setTextSize(1);
  display.setCursor(80, 55);
  display.print("Vol: ");
  display.print(currentVolume);

  display.display();
}
