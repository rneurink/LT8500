#include <LT8500.h>

LT8500 lt(5,19);

void setup() {
#ifdef __AVR_ATtinyx17__
  // Clock output on PB5
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, 1 << 7);

  pinMode(17, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(17), soft_reset, FALLING);
#endif

  Serial.begin(115200);
  
  // put your setup code here, to run once:
  lt.begin();
  lt.enableOutput();
}

#ifdef __AVR_ATtinyx17__
void soft_reset() {
  _PROTECTED_WRITE(RSTCTRL.SWRR,1);
}
#endif

void loop() {
  // put your main code here, to run repeatedly:
  for (int i = 0; i < 4096; i+=8) {
    lt.setPWM(0, i);
    lt.sendSyncFrame();
    delay(1);
  }
  for (int i = 4095; i >= 0; i-=8) {
    lt.setPWM(0,i);
    lt.sendSyncFrame();
    delay(1);
  }
}
