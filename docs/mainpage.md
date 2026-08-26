# Project Overview

![Circuit Photo](images/foto_projeto.png)

## Summary

This project implements a **Digitally Controlled FM Radio Receiver** based on the **Arduino Nano ESP32** and the **RDA5807M** module.
It features an OLED display, joystick control, and a **Python** dashboard for remote control via UART, real-time signal strength (RSSI) visualization, and frequency scans (full scan and center scan).

---

## Authorship

**Name:** João Pedro Santos  
**Institution:** ISEL (_Instituto Superior de Engenharia de Lisboa_)  
**Course:** Electronics, Telecommunications and Computer Engineering  
**Year:** 2025/2026

---

## Project Jury

| Role | Name |
|---|---|
| Advisor (_Orientador_) | Professor Vítor Fialho, Ph.D. |
| Examination Committee — Arguente | Professor Luís Pires |
| Examination Committee — Presidente | Professor Fernando Fortes, Ph.D. |

---

## Block Diagram

![Project Block Diagram](images/diagrama_blocos.png)

---

## Code Structure

| Module | Description |
|---|---|
| @ref pfc_main.ino | Main file - state machine and main loop |
| @ref btn.h "btn.h / btn.cpp" | Joystick/buttons interface |
| @ref display.h "display.h / display.cpp" | OLED display interface |
| @ref rda.h "rda.h / rda.cpp" | RDA5807M radio module interface |
| @ref fm_scanner.py | Python dashboard and graphic visualizer |

---

## How to use

1. Upload the firmware `pfc.ino` to the Arduino Nano ESP32.
2. Connect the Arduino to the PC via USB.
3. Run the Python script:
   ```bash
   python fm_scanner.py
   ```
4. Use the dashboard to control the radio (volume, frequency, scans)
   or the physical joystick.

---

## Features

- Manual and dashboard tuning.
- Volume and mute control.
- Full scan (80-108 MHz) and center scan (± 10 steps).
- Real-time RSSI visualization per frequency.
- Save charts (PNG) and data (TXT) of the scans.
