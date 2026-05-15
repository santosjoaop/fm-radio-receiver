#include "btn.h"

void JoyStickInit() {
  pinMode(UP, INPUT_PULLUP);
  pinMode(DWN, INPUT_PULLUP);
  pinMode(LEF, INPUT_PULLUP);
  pinMode(RHT, INPUT_PULLUP);
  pinMode(MID, INPUT_PULLUP);
  pinMode(SET, INPUT_PULLUP);
  pinMode(RST, INPUT_PULLUP);
}

int ReadButton() {
  if(digitalRead(UP) == LOW) return BTN_UP;
  if(digitalRead(DWN) == LOW) return BTN_DWN;
  if(digitalRead(LEF) == LOW) return BTN_LEF;
  if(digitalRead(RHT) == LOW) return BTN_RHT;
  if(digitalRead(MID) == LOW) return BTN_MID;
  if(digitalRead(SET) == LOW) return BTN_SET;
  if(digitalRead(RST) == LOW) return BTN_RST;
  
  return BTN_NONE; // Nenhum "botão" pressionado
}
