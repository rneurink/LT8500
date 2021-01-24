#include <LT8500.h>

LT8500 lt(5,3);

uint8_t testBuffer[72];
void array_set(uint8_t channel, uint16_t value);

void setup() {
#ifdef __AVR_ATtinyx17__
  // Clock output on PB5
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, 1 << 7);
#endif

  Serial.begin(115200);
  // put your setup code here, to run once:
  lt.begin();
  
  testBuffer[0] = 0x00;
  testBuffer[1] = 0x00;
  
  lt.sendFrame(testBuffer, CMD_CORRECTION);
  delay(1);
  lt.sendFrame(testBuffer, CMD_CORRECTION_TOGGLE);
  delay(1);

  testBuffer[0] = 0x00;
  testBuffer[1] = 0x00;

  lt.sendFrame(testBuffer, CMD_SYNC_UPDATE);
  delay(1);
  lt.sendFrame(testBuffer, CMD_OUT_ENABLE);
  delay(1);
}

int16_t i = 0;
bool i_down = false;

void loop() {
  // put your main code here, to run repeatedly:
  //lt.sendFrame(testBuffer, CMD_SELF_TEST);    
  array_set(0,i);
  // Serial.print(i, DEC);
  // Serial.print(" ");
  // Serial.print(testBuffer[1], HEX);
  // Serial.print(" ");
  // Serial.print(testBuffer[0], HEX);
  // Serial.print(" ");
  // Serial.print(i_down, DEC);
  // Serial.print(" ");
  // Serial.println();
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
  delay(5);
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