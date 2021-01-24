
#include <Arduino.h>
#include "LT8500.h"

LT8500::LT8500(int8_t LDI_Blank, int8_t SDO, int8_t N_OpenLED) {
    LT8500_LDIBLANK = LDI_Blank;
    LT8500_SDO = SDO;
    LT8500_N_OPENLED = N_OpenLED;

    // Spi clock at 20Mhz (max 50MHz) with MSB first and SPI mode 0 Data sampled on the rising edge and shifted out on the falling edge with clock idle on low
    LT8500_SPISettings = SPISettings(5000000, MSBFIRST, SPI_MODE0);
}

void LT8500::begin() {
    pinMode(LT8500_LDIBLANK, OUTPUT);
    pinMode(LT8500_SDO, INPUT);
    pinMode(LT8500_N_OPENLED, INPUT);

    memset(PWM_buffer, 0, sizeof(PWM_buffer));

    SPI.begin();
}

void LT8500::end() {
    SPI.end();
}

void LT8500::blank() {

}

bool LT8500::sendFrame(uint8_t *buffer, uint8_t command) {
    uint8_t LDI_bit_mask = digitalPinToBitMask(LT8500_LDIBLANK);
    PORT_t *LDI_port = digitalPinToPortStruct(LT8500_LDIBLANK);
    
    SPI.beginTransaction(LT8500_SPISettings);

    for (int i = 71; i >= 0; i--) {
        SPI.transfer(buffer[i]);
    }
    SPI.transfer(command);

    LDI_port->OUTSET = LDI_bit_mask;
    delayMicroseconds(1);
    LDI_port->OUTCLR = LDI_bit_mask;

    SPI.endTransaction();
}