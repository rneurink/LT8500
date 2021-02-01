#include <LT8500.h>

LT8500 lt(5,19);

uint8_t testBuffer[72];
void array_set(uint8_t channel, uint16_t value);

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

int16_t i = 0;
bool i_down = false;

void loop() {
  // put your main code here, to run repeatedly:
  array_set(0,i);
  lt.sendFrame(testBuffer, CMD_SYNC_UPDATE);
  if (i_down) {
    i -= 8;
    if (i <= 0) {
      i = 0;
      i_down = false;
    }
  } else {
    i += 8;
    if (i >= 4095) {
      i = 4095;
      i_down = true;
    }
  }
  delay(1);
}

void array_set(uint8_t channel, uint16_t value)
{
    if (channel & 1)
    {
        // Odd channel, half first byte on low
        testBuffer[channel*3/2] &= ~0xF0;
        testBuffer[channel*3/2] |= (0xF0 & (value << 4));
        testBuffer[channel*3/2 + 1] = (value >> 4);
    } else {
        testBuffer[channel*3/2] = (0xFF & value);
        testBuffer[channel*3/2 + 1] &= ~0x0F;
        testBuffer[channel*3/2 + 1] |= (value >> 8);
    }
}
