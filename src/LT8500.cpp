/**
 * @file LT8500.cpp
 * @author Ruben Neurink-Sluiman (ruben.neurink@gmail.com)
 * @brief Arduino library for driving the LT8500 CPP
 * @version 0.1
 * @date 2021-02-02
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <Arduino.h>
#include "LT8500.h"

/**
 * @brief Construct a new LT8500::LT8500 object
 * 
 * @param LDI_Blank LDI_Blank pin to latch data input and reset the chip
 * @param SDO SDO is the equivalent of MISO, this is used to detect if the chip booted and to get the status from the chip
 * @param N_OpenLED Optional. Triggers a status frame to be sent from the chip (can also be done using the self test command )
 */
LT8500::LT8500(int8_t LDI_Blank, int8_t SDO, int8_t N_OpenLED) {
    LT8500_LDIBLANK = LDI_Blank;
    LT8500_SDO = SDO;
    LT8500_N_OPENLED = N_OpenLED;

    // Spi clock at 5Mhz (max 50MHz) with MSB first and SPI mode 0 Data sampled on the rising edge and shifted out on the falling edge with clock idle on low
    LT8500_SPISettings = SPISettings(5000000, MSBFIRST, SPI_MODE0);
}

/**
 * @brief Destroy the LT8500::LT8500 object
 * 
 */
LT8500::~LT8500() {
    end();
}

/**
 * @brief Initializes the pins and SPI bus and prepares the chip
 * @note also disables the correction as this is enabled by default
 */
void LT8500::begin() {
#if defined(__AVR_ATtinyxy7__)
    LDI_bit_mask = digitalPinToBitMask(LT8500_LDIBLANK);
    LDI_port = digitalPinToPortStruct(LT8500_LDIBLANK);
#endif

    pinMode(LT8500_LDIBLANK, OUTPUT);
    pinMode(LT8500_SDO, INPUT);
    if (LT8500_N_OPENLED != -1)
    {
        pinMode(LT8500_N_OPENLED, INPUT);
    }
    
    memset(PWM_buffer, 0, sizeof(PWM_buffer));

    SPI.begin();

    // Reset the chip
    digitalWrite(LT8500_LDIBLANK, HIGH); // keep the chip in reset state
    delayMicroseconds(500);
    digitalWrite(LT8500_LDIBLANK, LOW); // assert reset low to start up the chip
    while (digitalRead(LT8500_SDO) != LOW) {} // wait for the chip to start up //TODO: add timout
    
    // enable the PWM clock

    // Initialize the correction registers
    sendCorrectionFrame();
    delay(1);
    toggleCorrection(); // Correction enabled by default, so disable it
    Correction_Enabled = false;
    delay(1);

    // update frame
    sendSyncFrame();
    delay(1);
}

/**
 * @brief Closes the SPI bus
 * 
 */
void LT8500::end() {
    SPI.end();
}

/**
 * @brief Resets the chip
 * 
 */
void LT8500::reset() {
    sendResetPulse();
}

/**
 * @brief Sets the PWM value of a channel
 * 
 * @param channel the channel to set the value of (from 1 to 48)
 * @param pwm the PWM value to set in the channel (from 0 to 4095)
 */
void LT8500::setPWM(uint8_t channel, uint16_t pwm) {
    // Using a packet buffer to deal with the 12 bits per pwm channel
    // on an even channel (0, 2, 4 etc) it will take up the complete lower byte (lsB) and half of the upper byte (msB)
    // as 12 bits is 1.5 times 8 bits this is used as a multiplier. (int will be rounded down)
    // on an odd channel (1, 3, 5 etc) it will take up half of the lower byte and the complete upper byte
    pack12to8(PWM_buffer, channel - 1, pwm);
}

/**
 * @brief Retrieves the PWM value of a channel from the buffer
 * 
 * @param channel the channel to get the value from (from 1 to 48)
 * @return uint16_t the PWM value (from 0 to 4095)
 */
uint16_t LT8500::getPWM(uint8_t channel) {
    return unpack12to8(PWM_buffer, channel - 1);
}

/**
 * @brief Sets the correction data of a channel
 * 
 * @param channel the channel to set the correction value of (from 1 to 48)
 * @param correction the correction value to set in the channel (from 0 to 63)
 */
void LT8500::setCorrection(uint8_t channel, uint8_t correction) {
    // Using a packet buffer to deal with the 6 bits of correction per channel
    pack6to8(Correction_buffer, channel - 1, correction);
}

/**
 * @brief Retrieves the correction value of a channel from the buffer
 * 
 * @param channel the channel to get the value from (from 1 to 48)
 * @return uint8_t the correction value (from 0 to 63)
 */
uint8_t LT8500::getCorrection(uint8_t channel) {
    return unpack6to8(Correction_buffer, channel - 1);
}

/**
 * @brief Sends a synchronous update frame which will update the PWM registers on the start of the PWM period
 * 
 */
void LT8500::sendSyncFrame() {
    SPI.beginTransaction(LT8500_SPISettings);

    for (int i = 71; i >= 0; i--) {
        SPI.transfer(PWM_buffer[i]);
    }
    SPI.transfer(CMD_SYNC_UPDATE);

    sendLDIPulse();

    SPI.endTransaction();
}

/**
 * @brief Sends an asynchronous update frame which will update the PWM registers immediatly after the LDI pulse
 * 
 */
void LT8500::sendAsyncFrame() {
    SPI.beginTransaction(LT8500_SPISettings);

    for (int i = 71; i >= 0; i--) {
        SPI.transfer(PWM_buffer[i]);
    }
    SPI.transfer(CMD_ASYNC_UPDATE);

    sendLDIPulse();

    SPI.endTransaction();
}

/**
 * @brief Sends the correction frame
 * 
 */
void LT8500::sendCorrectionFrame() {
    for (int i = 0 ; i < 48; i++) {
        //Repackage the correction buffer into the frame buffer as the correction frame is pretty much only sent 1 time
        pack12to8(Frame_buffer, i, (unpack6to8(Correction_buffer, i) << 6));
    }

    SPI.beginTransaction(LT8500_SPISettings);

    for (int i = 71; i >= 0; i--) {
        SPI.transfer(Frame_buffer[i]);
    }
    SPI.transfer(CMD_CORRECTION);

    sendLDIPulse();

    SPI.endTransaction();
}

/**
 * @brief Enables the PWM outputs of the chip
 * 
 */
void LT8500::enableOutput() {
    // enable outputs
    SPI.beginTransaction(LT8500_SPISettings);

    for (int i = 71; i >= 0; i--) {
        SPI.transfer(0x00);
    }
    SPI.transfer(CMD_OUT_ENABLE);

    sendLDIPulse();

    SPI.endTransaction();
}

/**
 * @brief Disables the PWM outputs of the chip
 * 
 */
void LT8500::disableOutput() {
    // disable outputs
    SPI.beginTransaction(LT8500_SPISettings);

    for (int i = 71; i >= 0; i--) {
        SPI.transfer(0x00);
    }
    SPI.transfer(CMD_OUT_DISABLE);

    sendLDIPulse();

    SPI.endTransaction();
}

/**
 * @brief Toggle the CRD (Correction register disable) bit
 * 
 */
void LT8500::toggleCorrection() {
    SPI.beginTransaction(LT8500_SPISettings);

    for (int i = 71; i >= 0; i--) {
        SPI.transfer(0x00);
    }
    SPI.transfer(CMD_CORRECTION_TOGGLE);

    sendLDIPulse();

    SPI.endTransaction();

    Correction_Enabled = !Correction_Enabled;
}

/**
 * @brief Toggle the PSH (Phase shift) bit
 * 
 */
void LT8500::togglePhaseShift() {
    SPI.beginTransaction(LT8500_SPISettings);

    for (int i = 71; i >= 0; i--) {
        SPI.transfer(0x00);
    }
    SPI.transfer(CMD_PHASE_TOGGLE);

    sendLDIPulse();

    SPI.endTransaction();

    PhaseShift_Enabled = !PhaseShift_Enabled;
}

/**
 * @brief Self test the chip TODO
 * 
 */
void LT8500::selfTest() {
    // NOT IMPLEMENTED YET
}

/**
 * @brief Sends an latch data input pulse on the LDI_BLANK pin. This pulse should be between 8 and 5.000 ns
 * 
 */
void LT8500::sendLDIPulse() {

#if defined (__AVR_ATtinyxy7__)
    LDI_port->OUTSET = LDI_bit_mask; // about 670 ns
    delayMicroseconds(1);
    LDI_port->OUTCLR = LDI_bit_mask;
#else
    digitalWrite(LT8500_LDIBLANK, HIGH); // DigitalWrite is approx 3us on a 20MHz clock
    digitalWrite(LT8500_LDIBLANK, LOW);
#endif

}

/**
 * @brief Sends a reset pulse on the LDI_BLANK pin. This pulse should be above 50.000 ns
 * 
 */
void LT8500::sendResetPulse() {

#if defined (__AVR_ATtinyxy7__)
    LDI_port->OUTSET = LDI_bit_mask;
    delayMicroseconds(70);
    LDI_port->OUTCLR = LDI_bit_mask;
#else
    digitalWrite(LT8500_LDIBLANK, HIGH);
    delayMicroseconds(70);
    digitalWrite(LT8500_LDIBLANK, LOW);
#endif

}

#pragma region Packing_Unpacking_functions
/**
 * Functions to deal with packing and unpacking 12 and 6 bits into 8 bit arrays 
 */

/**
 * @brief Packs a 12 bit value into an 8 bit array
 * 
 * @warning There is no safety and this assumes you dealt correctly with the array sizes!!
 * @param array the array to pack into
 * @param index the packed index of the value 
 * @param value the value to set (be aware that it is 12 bits so 0..4095)
 */
void LT8500::pack12to8(uint8_t array[], uint8_t index, uint16_t value) {
    if (index & 1) {
        // Odd index, half first byte on low
        array[index * 3 / 2] &= ~0xF0; // clear the upper half the lsB
        array[index * 3 / 2] |= (0xF0 & (value << 4)); // push the 4 lower bits of the value into the upper 4 bits of the lsB
        array[index * 3 / 2 + 1] = (value >> 4); // push the upper 8 bits of the value into the msB
    } else {
        array[index * 3 / 2] = (0xFF & value); // push the lower 8 bits into the lsB
        array[index * 3 / 2 + 1] &= ~0x0F; // clear the lower half of the msB 
        array[index * 3 / 2 + 1] |= (value >> 8); // push the 4 upper bits of the value into the lower 4 bits of the msB
    }
}

/**
 * @brief Unpacks a 12 bit value from an 8 bit array
 * 
 * @warning There is no safety and this assumes you dealt correctly with the array sizes!!
 * @param array the array to unpack from
 * @param index the packed index of the value
 * @return uint16_t the 12 bit unpacked value related to the packed \p index
 */
uint16_t LT8500::unpack12to8(uint8_t array[], uint8_t index) {
    uint16_t value =0;
    if (index & 1) {
        value = (array[index * 3 / 2] & 0xF0) >> 4;
        value |= array[index * 3 / 2 + 1] << 4;
    } else {
        value = array[index * 3 / 2];
        value |= (array[index * 3 / 2 + 1] & 0x0F) << 8;
    }
    return value;
}

/**
 * @brief Packs a 6 bit value into an 8 bit array
 * 
 * @warning There is no safety and this assumes you dealt correctly with the array sizes!!
 * @param array the array to pack into
 * @param index the packed index of the value
 * @param value the value to set (be aware that it is 6 bits so 0..63)
 */
void LT8500::pack6to8(uint8_t array[], uint8_t index, uint8_t value) {
    if ((index & 3) == 3) {
        array[index * 3 / 4] &= ~0xFC; // clear the upper 6 bits
        array[index * 3 / 4] |= (0xFC & (value << 2)); // push the 6 bits of the value into the upper 6 bits of the byte
    } else if ((index & 2) == 2) {
        array[index * 3 / 4] &= ~0xF0; // clear the upper 4 bits of the lsB
        array[index * 3 / 4] |= (0xF0 & (value << 4)); // push the lower 4 bits of the value into the lsB
        array[index * 3 / 4 + 1] &= ~0x03; // clear the lower 2 bits of the msB
        array[index * 3 / 4 + 1] |= (0x03 & (value >> 4)); // push the upper 4 bits of the value into the lower 4 bits of the msB
    } else if (index & 1) {
        array[index * 3 / 4] &= ~0xC0; // clear the upper 2 bits of the lsB
        array[index * 3 / 4] |= (0xC0 & (value << 6)); // push the lower 2 bits of the value into the lsB
        array[index * 3 / 4 + 1] &= ~0x0F; // clear the lower 4 bits of the msB
        array[index * 3 / 4 + 1] |= (0x0F & (value >> 2)); // push the upper 4 bits of the value into the lower 4 bits of the msB
    } else {
        array[index * 3 / 4] &= ~0x3F; // clear the lower 6 bits
        array[index * 3 / 4] |= (0x3F & value); // push the lower 6 bits into the byte
    }
}

/**
 * @brief Unpacks a 6 bit value from an 8 bit array
 * 
 * @warning There is no safety and this assumes you dealt correctly with the array sizes!!
 * @param array the array to unpack from
 * @param index the packed index of the value
 * @return uint8_t the 6 bit unpacked value related to the packed \p index
 */
uint8_t LT8500::unpack6to8(uint8_t array[], uint8_t index) {
    // inverse of pack6to8
    uint8_t value = 0;
    if ((index & 3) == 3) {
        value = (array[index * 3/4] & 0xFC) >> 2;
    } else if ((index & 2) == 2) {
        value = (array[index * 3/4] & 0xF0) >> 4;
        value |= (array[index * 3/4 + 1] & 0x03) << 4;
    } else if (index & 1) {
        value = (array[index * 3/4] & 0xC0) >> 6;
        value |= (array[index * 3/4 + 1] & 0x0F) << 2;
    } else {
        value = array[index * 3/4] & 0x3F;
    }
    return value;
}
#pragma endregion Packing_Unpacking_functions
