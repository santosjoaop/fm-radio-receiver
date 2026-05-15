#include "rda.h"
#include "display.h"
#include "btn.h"

#include <Wire.h>

uint16_t frequency = 9320; // Corresponde a 93.20 MHz

//Enumeração para cada "estado"

// void ScanCenter(xxxx){}

// MUDAR PARA TER PARAMETROS

void ScanFrequencies() {
  // 80MHz = 8000 | 108MHz = 10800 | Salto de 100kHz (0.1MHz) = 10
  for (uint16_t f = 8000; f <= 10800; f += 10) {
    RadioSetFreq(f);
    
    // Atualiza o display
    int currentRSSI = getRSSI();
    DisplayWrite(f / 100.0, currentRSSI); 
    delay(100); 

    Serial.print("Freq: ");
    Serial.print(f / 100.0, 1);
    Serial.print(" MHz | RSSI: ");

    float soma = 0.0;

    // 10 amostras de RSSI
    for (int i = 0; i < 10; i++) {
      int rssi_log = getRSSI();

      Serial.print(rssi_log);

      if (i < 9) {
        Serial.print(", ");
      }
      soma += pow(10.0, (rssi_log/10.0));
      delay(20);
    }
    
    float mediaLinear = soma / 10.0;

    float mediaLog = 10.0 * log10(mediaLinear);

    int mediaFinal = round(mediaLog);

    Serial.print(" | Média RSSI: ");
    Serial.println(mediaFinal);
  }
  
  // No final do Scan, volta a sintonizar a frequência onde estavas antes
  RadioSetFreq(frequency);
}

void setup() {
  //-----------------------//
  Serial.begin(115200);
  delay(1000);
  //-----------------------//
  DisplayInit();
  //-----------------------//
  RadioInit(frequency);
  //-----------------------//
  JoyStickInit();
  //-----------------------//
}

void loop() {

  //Fazer máquina de estados (switch case)
  
  int btn = ReadButton();

  if (btn == BTN_UP) {
    RadioVolUp();
  } 
  else if (btn == BTN_DWN) {
    RadioVolDown();
  } 
  else if (btn == BTN_RHT) {
    frequency += 10; 
    RadioSetFreq(frequency);
  } 
  else if (btn == BTN_LEF) {
    frequency -= 10; 
    RadioSetFreq(frequency);
  }
  else if (btn == BTN_SET) {
    ScanFrequencies();
  }

  int rssi = getRSSI(); 
  float laydOutFreq = frequency / 100.0;
  DisplayWrite(laydOutFreq, rssi);
  
  // Debounce
  if (btn != BTN_NONE) {
    delay(200); 
  } else {
    delay(50);  
  }
}
