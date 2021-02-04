#include <LT8500.h>

#define LT8500_LDIBLANK_PIN 5
#define LT8500_SDO_PIN 19

LT8500 lt(LT8500_LDIBLANK_PIN, LT8500_SDO_PIN);

void setup() {
#ifdef __AVR_ATtinyx17__
  // Using the Xplained pro Attiny3217
  // Clock output on PB5 to use as the PWM clock
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, 1 << 7);

  // Software reset as hardware reset is not available on this IC (without disabling UPDI)
  pinMode(17, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(17), soft_reset, FALLING);
#endif

  Serial.begin(115200);

  Serial.println("PWM example");
  Serial.println("PWM Cycle on PWM channel 1");

  // Initialize
  lt.begin();

  // Enable the PWM outputs
  lt.enableOutput();
}

#ifdef __AVR_ATtinyx17__
void soft_reset() {
  _PROTECTED_WRITE(RSTCTRL.SWRR,1);
}
#endif

void loop() {
  // Looping from duty cycle 0% to 100% and back to 0%
  for (int i = 0; i < 4096; i+=8) {
    lt.setPWM(0, i); // Sets the PWM value in the buffer
    lt.sendSyncFrame(); // Sends the PWM buffer to the chip
    delay(1);
  }
  for (int i = 4095; i >= 0; i-=8) {
    lt.setPWM(0, i); // Sets the PWM value in the buffer
    lt.sendSyncFrame(); // Sends the PWM buffer to the chip
    delay(1);
  }
}
