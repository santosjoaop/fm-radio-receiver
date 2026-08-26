/**
 * @file pfc_main.ino
 * @brief Main firmware file for the Digitally Controlled FM Radio Receiver project.
 *
 * This file implements the main state machine, scan routines and
 * serial command handler for an FM radio built with an Arduino Nano ESP32
 * and an RDA5807M module. It communicates with a Python dashboard via UART.
 *
 * @author J.P.Santos
 * @date 18/02/2026 - 12/06/2026
 */

#include "rda.h"
#include "display.h"
#include "btn.h"
#include <Wire.h>

/// Timestamp of the last display refresh (used to avoid refreshing every loop tick)
unsigned long ultimoTempoEcra = 0;

/**
 * @brief System states for the main state machine.
 *
 * - RADIO            Normal radio playback mode.
 * - MENU_MAIN        Top-level scan type selection menu.
 * - CONFIG_TOTAL     Configuration screen for a full-band scan.
 * - SELECT_CENTER_FREQ  Screen to pick the centre frequency for a central scan.
 * - CONFIG_CENTER    Configuration screen for a central scan.
 */
enum Estado { RADIO, MENU_MAIN, CONFIG_TOTAL, SELECT_CENTER_FREQ, CONFIG_CENTER };

/// Currently active system state.
Estado estadoAtual = RADIO;

/// Currently highlighted menu option (0 = Total scan, 1 = Central scan).
int menuSelecao = 0;

/// Currently selected configuration field (0 = Step, 1 = Samples).
int campoSelecionado = 0;

/// Step size for the scan in RDA units (1 unit = 10 kHz).
int saltoScan = 10;

/// Number of RSSI samples to average at each frequency point.
int amostrasScan = 10;

/// Centre frequency for a central scan, in RDA units (9320 = 93.2 MHz).
uint16_t freqCentro = 9320;

/// Currently tuned frequency in RDA units (9320 = 93.2 MHz).
uint16_t frequency = 9320; 

/**
 * @brief Perform a centred frequency scan around a given centre frequency.
 *
 * Scans 10 steps to the left and 10 steps to the right of @p centro,
 * for a total of 21 frequency points. At each point it collects
 * @p numAmostras RSSI readings, computes a logarithmic average and
 * prints the result to Serial in the format expected by the Python dashboard:
 * @code
 * SCAN_INFO: tipo=Central centro=9320 passo=10 amostras=10
 * Freq: 93.2 MHz | RSSI: 15, 16, 14 | Media RSSI: 15
 * @endcode
 *
 * @param centro      Centre frequency in RDA units (e.g. 9320 = 93.2 MHz).
 * @param passo       Step size in RDA units (e.g. 10 = 100 kHz).
 * @param numAmostras Number of RSSI readings to average at each point.
 */
void ScanCentered(uint16_t centro, uint16_t passo, int numAmostras) {
  // Informa o Python do tipo de scan e parâmetros antes de começar
  Serial.print("SCAN_INFO: tipo=Central centro=");
  Serial.print(centro);
  Serial.print(" passo=");
  Serial.print(passo);
  Serial.print(" amostras=");
  Serial.println(numAmostras);

  // 10 saltos para a esquerda e 10 para a direita (total 21 pontos)
  for (int i = -10; i <= 10; i++) {
    int32_t f_atual = centro + (i * passo);
    
    // limites da banda 80-108 MHz
    if (f_atual < 8000 || f_atual > 10800) continue;

    RadioSetFreq((uint16_t)f_atual);
    DisplayWrite(f_atual / 100.0, getRSSI());
    delay(100);

    Serial.print("Freq: ");
    Serial.print(f_atual / 100.0, 1);
    Serial.print(" MHz | RSSI: ");

    float soma = 0.0;
    
    // Faz as leituras 
    for (int a = 0; a < numAmostras; a++) {
      int rssi_log = getRSSI();
      Serial.print(rssi_log);
      
      // Coloca vírgula e espaço, exceto no último número
      if (a < (numAmostras - 1)) {
        Serial.print(", ");
      }
      
      soma += pow(10.0, (rssi_log / 10.0));
      delay(20);
    }
    
    // Calcula a média real
    float mediaLinear = soma / numAmostras;
    float mediaLog = 10.0 * log10(mediaLinear);
    int mediaFinal = round(mediaLog);

    Serial.print(" | Média RSSI: "); 
    Serial.println(mediaFinal);
  }
  
  RadioSetFreq(frequency);
}

/**
 * @brief Perform a full FM band scan from 80.0 MHz to 108.0 MHz.
 *
 * Steps through the entire FM band in increments of @p passo RDA units.
 * At each step it collects @p numAmostras RSSI readings, computes a
 * logarithmic average and prints the result to Serial:
 * @code
 * SCAN_INFO: tipo=Total passo=10 amostras=10
 * Freq: 88.0 MHz | RSSI: 15, 16, 14 | Media RSSI: 15
 * @endcode
 *
 * @param passo       Step size in RDA units (e.g. 10 = 100 kHz).
 * @param numAmostras Number of RSSI readings to average at each point.
 */
void ScanTotal(uint16_t passo, int numAmostras) {
  // Informa o Python do tipo de scan e parâmetros antes de começar
  Serial.print("SCAN_INFO: tipo=Total passo=");
  Serial.print(passo);
  Serial.print(" amostras=");
  Serial.println(numAmostras);

  for (uint16_t f = 8000; f <= 10800; f += passo) {
    RadioSetFreq(f);
    DisplayWrite(f / 100.0, getRSSI());
    delay(100);

    Serial.print("Freq: ");
    Serial.print(f / 100.0, 1);
    Serial.print(" MHz | RSSI: ");

    float soma = 0.0;
    
    // Faz as leituras
    for (int i = 0; i < numAmostras; i++) {
      int rssi_log = getRSSI();
      Serial.print(rssi_log);

      // Coloca vírgula e espaço, exceto no último número
      if (i < (numAmostras - 1)) {
        Serial.print(", ");
      }

      soma += pow(10.0, (rssi_log / 10.0));
      delay(20);
    }

    // Calcula a média real
    float mediaLinear = soma / numAmostras;
    float mediaLog = 10.0 * log10(mediaLinear);
    int mediaFinal = round(mediaLog);

    Serial.print(" | Média RSSI: ");
    Serial.println(mediaFinal);
  }
  
  RadioSetFreq(frequency);
}

/**
 * @brief Arduino setup routine — runs once at power-on or reset.
 *
 * Initialises serial communication, the OLED display, the RDA5807M radio
 * module and the joystick/button inputs.
 */
void setup() {
  Serial.begin(115200);
  DisplayInit();
  RadioInit(frequency);
  JoyStickInit();
}

/**
 * @brief Parse and execute a command string received from Python via UART.
 *
 * Supported commands:
 * | Command                    | Action                              |
 * |----------------------------|-------------------------------------|
 * | VOL+                       | Increase volume by 1 step           |
 * | VOL-                       | Decrease volume by 1 step           |
 * | MUTE                       | Toggle mute on/off                  |
 * | FREQ:\<xxxx\>              | Tune to frequency in RDA units      |
 * | SCAN_TOTAL:\<pp\>:\<aa\>   | Full band scan, step pp, aa samples |
 * | SCAN_CENTER:\<ff\>:\<pp\>:\<aa\> | Central scan around ff      |
 *
 * After each command the Arduino replies with a confirmation line:
 * VOL:\<n\>, MUTE:ON/OFF, FREQ_OK:\<xxxx\> or FREQ_ERR:out of range.
 *
 * @param cmd The raw command string (trailing whitespace is trimmed).
 */
void handleSerialCommand(String cmd) {
  cmd.trim();

  if (cmd == "VOL+") {
    RadioVolUp();
    Serial.print("VOL:"); Serial.println(currentVolume);

  } else if (cmd == "VOL-") {
    RadioVolDown();
    Serial.print("VOL:"); Serial.println(currentVolume);

  } else if (cmd == "MUTE") {
    // Toggle mute: if volume > 0 mute, if muted restore
    RadioMuteToggle();

  } else if (cmd.startsWith("FREQ:")) {
    uint16_t f = (uint16_t)cmd.substring(5).toInt();
    if (f >= 8000 && f <= 10800) {
      frequency = f;
      RadioSetFreq(frequency);
      DisplayWrite(frequency / 100.0, getRSSI());
      Serial.print("FREQ_OK:"); Serial.println(frequency);
    } else {
      Serial.println("FREQ_ERR:out of range");
    }

  } else if (cmd.startsWith("SCAN_TOTAL:")) {
    
    int sep1 = cmd.indexOf(':', 11);
    if (sep1 > 0) {
      saltoScan    = cmd.substring(11, sep1).toInt();
      amostrasScan = cmd.substring(sep1 + 1).toInt();
      estadoAtual  = RADIO;
      ScanTotal(saltoScan, amostrasScan);
      DisplayWrite(frequency / 100.0, getRSSI());
    }

  } else if (cmd.startsWith("SCAN_CENTER:")) {

    int sep1 = cmd.indexOf(':', 12);
    int sep2 = cmd.indexOf(':', sep1 + 1);
    if (sep1 > 0 && sep2 > 0) {
      freqCentro   = (uint16_t)cmd.substring(12, sep1).toInt();
      saltoScan    = cmd.substring(sep1 + 1, sep2).toInt();
      amostrasScan = cmd.substring(sep2 + 1).toInt();
      estadoAtual  = RADIO;
      ScanCentered(freqCentro, saltoScan, amostrasScan);
      DisplayWrite(frequency / 100.0, getRSSI());
    }
  }
}
/**
 * @brief Arduino main loop — runs continuously after setup().
 *
 * Each iteration:
 * 1. Checks for incoming serial commands from Python and handles them.
 * 2. Reads the joystick/button state.
 * 3. Drives the state machine (RADIO, MENU_MAIN, CONFIG_TOTAL,
 *    SELECT_CENTER_FREQ, CONFIG_CENTER).
 * 4. Applies a debounce delay (200 ms on button press, 50 ms idle).
 */
void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    handleSerialCommand(cmd);
  }

  int btn = ReadButton();

  switch (estadoAtual) {
    case RADIO:
      if (btn == BTN_SET) { 
        estadoAtual = MENU_MAIN; 
        DisplayMenuMain(menuSelecao); 
      }
      else {
        if (btn == BTN_UP) RadioVolUp();
        else if (btn == BTN_DWN) RadioVolDown();
        else if (btn == BTN_RHT) { frequency += 10; RadioSetFreq(frequency); }
        else if (btn == BTN_LEF) { frequency -= 10; RadioSetFreq(frequency); }
        
        if (btn != BTN_NONE || (millis() - ultimoTempoEcra > 500)) {
          DisplayWrite(frequency / 100.0, getRSSI());
          ultimoTempoEcra = millis();
        }
      }
      break;

    case MENU_MAIN:

      if (btn == BTN_SET) {
        estadoAtual = (menuSelecao == 0) ? CONFIG_TOTAL : SELECT_CENTER_FREQ;
        
        // Desenha o próximo ecrã instantaneamente
        if (estadoAtual == CONFIG_TOTAL) 
          DisplayMenuConfig("Config Total", saltoScan, amostrasScan, campoSelecionado);
        else DisplayMenuFreq(freqCentro);
      }
      // Volta ao rádio
      else if (btn == BTN_RST) {
        estadoAtual = RADIO;
        DisplayWrite(frequency / 100.0, getRSSI());
      }

      else if (btn == BTN_UP || btn == BTN_DWN) {
        menuSelecao = (btn == BTN_DWN) ? 1 : 0;
        DisplayMenuMain(menuSelecao);
      }
      break;

    case SELECT_CENTER_FREQ:
      // configurações
      if (btn == BTN_SET) {
        estadoAtual = CONFIG_CENTER;
        DisplayMenuConfig("Config Central", saltoScan, amostrasScan, campoSelecionado);
      }
      // Volta atrás
      else if (btn == BTN_RST) {
        estadoAtual = MENU_MAIN;
        DisplayMenuMain(menuSelecao);
      }

      else if (btn == BTN_UP || btn == BTN_DWN) {
        if (btn == BTN_UP) freqCentro += 10;
        else if (btn == BTN_DWN) freqCentro -= 10;
        DisplayMenuFreq(freqCentro);
      }
      break;

    case CONFIG_TOTAL:
    case CONFIG_CENTER:
      // Arranca o Scan
      if (btn == BTN_SET) {
        
        if (estadoAtual == CONFIG_TOTAL) ScanTotal(saltoScan, amostrasScan);
        else ScanCentered(freqCentro, saltoScan, amostrasScan);
        
        // No fim, regressa ao rádio
        estadoAtual = RADIO;
        DisplayWrite(frequency / 100.0, getRSSI()); 
      }
      // Volta atrás
      else if (btn == BTN_RST) {
        if (estadoAtual == CONFIG_TOTAL) {
          estadoAtual = MENU_MAIN;
          DisplayMenuMain(menuSelecao);
        } else {
          estadoAtual = SELECT_CENTER_FREQ;
          DisplayMenuFreq(freqCentro);
        }
      }
      // Só redesenha as configurações se usares as setas
      else if (btn == BTN_LEF || btn == BTN_RHT || btn == BTN_UP || btn == BTN_DWN) {
        if (btn == BTN_LEF) campoSelecionado = 0;
        else if (btn == BTN_RHT) campoSelecionado = 1;
        else if (btn == BTN_UP) {
          if (campoSelecionado == 0) saltoScan += 5; else amostrasScan += 5;
        }
        else if (btn == BTN_DWN) {
          if (campoSelecionado == 0 && saltoScan > 5) saltoScan -= 5; 
          else if (campoSelecionado == 1 && amostrasScan > 5) amostrasScan -= 5;
        }
        const char* t = (estadoAtual == CONFIG_TOTAL) ? "Config Total" : "Config Central";
        DisplayMenuConfig(t, saltoScan, amostrasScan, campoSelecionado);
      }
      break;
  }
  
  if (btn != BTN_NONE) delay(200); else delay(50);
}