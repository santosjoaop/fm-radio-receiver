/**
 * @file display.cpp
 * @brief Implementation of the OLED display interface.
 *
 * @author J.P.Santos
 * @date 18/02/2026 - 12/06/2026
 */

#include "display.h"

/// Current volume level — declared in rda.cpp, used here for the footer display.
extern int currentVolume;

/// Global Adafruit SSD1306 display object.
Adafruit_SSD1306 display(DISPLAY_W, DISPLAY_H, &Wire, DISPLAY_RESET);

/**
 * @brief Initialise the SSD1306 display and show the splash screen.
 *
 * Halts with an infinite loop if the display cannot be found on I2C.
 */
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

/**
 * @brief Refresh the main radio screen with current frequency and RSSI.
 *
 * @param freq Tuned frequency in MHz.
 * @param rssi Current RSSI value.
 */
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

/**
 * @brief Draw the top-level scan type selection menu.
 *
 * @param selecao Highlighted option (0 = Banda Total, 1 = Freq. Central).
 */
void DisplayMenuMain(int selecao) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("Tipo de Scan:"));
  display.drawLine(0, 10, 128, 10, WHITE);

  display.setCursor(0, 30);
  if (selecao == 0) display.print(F("> ")); else display.print(F("  "));
  display.print(F("Banda Total"));

  display.setCursor(0, 50);
  if (selecao == 1) display.print(F("> ")); else display.print(F("  "));
  display.print(F("Freq. Central"));
  display.display();
}

/**
 * @brief Draw the scan configuration screen.
 *
 * @param titulo           Screen title.
 * @param salto            Step size in RDA units.
 * @param amostras         Number of samples.
 * @param campoSelecionado Active field index (0 = step, 1 = samples).
 */

void DisplayMenuConfig(const char* titulo, int salto, int amostras, int campoSelecionado) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(titulo);
  display.drawLine(0, 10, 128, 10, WHITE);

  // Coluna Salto
  display.setCursor(0, 25);
  if (campoSelecionado == 0) display.print(F(">")); else display.print(F(" "));
  display.print(F("Span"));
  display.setCursor(6, 40); 
  display.print(salto * 10); display.print(F("kHz"));

  // Coluna Amostras
  display.setCursor(64, 25);
  if (campoSelecionado == 1) display.print(F(">")); else display.print(F(" "));
  display.print(F("Amostras"));
  display.setCursor(70, 40);
  display.print(amostras);

  display.display();
}

/**
 * @brief Draw the centre-frequency selection screen.
 *
 * @param freq Centre frequency in RDA units (e.g. 9320 = 93.2 MHz).
 */
void DisplayMenuFreq(uint16_t freq) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("Freq. Central:"));
  display.drawLine(0, 10, 128, 10, WHITE);

  display.setTextSize(2);
  display.setCursor(25, 30);
  display.print(freq / 100.0, 1);
  display.print(F(" MHz"));
  display.display();
} 
