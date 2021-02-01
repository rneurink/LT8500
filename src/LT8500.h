/**
 * @file LT8500.h
 * @author Ruben Neurink-Sluiman (ruben.neurink@gmail.com)
 * @brief Arduino library for driving the LT8500
 * @version 0.1
 * @date 2021-01-12
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef __LT8500_H
#define __LT8500_H

/* 
 * LT8500 Description
 * 48 independent channel PWM generator
 * 12-bit resolution
 * 50MHz cascadable serial interface
 * 
 * 3-5.5V input voltage
 * 6-bit PWM correction
 * Up to 6.1kHz PWM frequency with PWMCK at 25MHz
 * Phase shift option to reduce input currents
*/

#include <Arduino.h>
#include <SPI.h>

// CMD make sure not to send 0x8X
#define CMD_SYNC_UPDATE 0x00
#define CMD_ASYNC_UPDATE 0x10
#define CMD_CORRECTION 0x20
#define CMD_OUT_ENABLE 0x30
#define CMD_OUT_DISABLE 0x40
#define CMD_SELF_TEST 0x50
#define CMD_PHASE_TOGGLE 0x60
#define CMD_CORRECTION_TOGGLE 0x70

// pins needed: SDI SCKI PWMCLK LDI

class LT8500 {
// Variables
//public:
//protected:
private:
    int8_t LT8500_LDIBLANK = -1;
    int8_t LT8500_N_OPENLED = -1;
    int8_t LT8500_SDO = -1;

    SPISettings LT8500_SPISettings;

    uint8_t PWM_buffer[72]; // 48 channels of 12 bits
    uint8_t Correction_buffer[72];
    
    uint8_t Status_buffer[73];

#if defined(__AVR_ATtinyxy7__)
    uint8_t LDI_bit_mask;
    PORT_t *LDI_port;
#endif

//Functions
public:
    LT8500(int8_t LDI_Blank, int8_t SDO, int8_t N_OpenLED = -1);

    void begin();
    void end();

    void reset();

    void enableOutput();
    void disableOutput();

//protected:
//private:

    bool sendFrame(uint8_t *buffer, uint8_t command);
    void sendLDIPulse();
    void sendResetPulse();
};


#endif // __LT8500_H