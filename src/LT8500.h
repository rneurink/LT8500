/**
 * @file LT8500.h
 * @author Ruben Neurink-Sluiman (ruben.neurink@gmail.com)
 * @brief Arduino library for driving the LT8500
 * @version 0.1
 * @date 2021-02-02
 * 
 * @copyright Copyright (c) 2021
 * 
 * https://www.analog.com/media/en/technical-documentation/data-sheets/LT8500.pdf 
 * 
 * LT8500 Description: \n
 * 48 independent channel PWM generator \n
 * 12-bit resolution \n
 * 50MHz cascadable serial interface \n
 * \n
 * 3-5.5V input voltage \n
 * 6-bit PWM correction \n
 * Up to 6.1kHz PWM frequency with PWMCK at 25MHz \n
 * Phase shift option to reduce input currents \n
 * \n
 * \n
 * \b TODO:
 * - add self test
 * - add port manipulation for SAMD, ESP32 and Atmega328 to speed up the commands
 * - add examples
 */

#ifndef __LT8500_H
#define __LT8500_H

#include <Arduino.h>
#include <SPI.h>

// CMD make sure not to send 0x8X as it is a reserved command
/** Update PWM’s Synchronously to PWM Period */
#define CMD_SYNC_UPDATE 0x00 
/** Update PWM’s Asynchronously to PWM Period */
#define CMD_ASYNC_UPDATE 0x10 
/** Set PWM Correction Factor */
#define CMD_CORRECTION 0x20
/** Enable PWM Outputs */
#define CMD_OUT_ENABLE 0x30
/** Disable (Drive Low) PWM Outputs */
#define CMD_OUT_DISABLE 0x40
/** Initiates Self Test */
#define CMD_SELF_TEST 0x50
/** Toggle 16-Channel Bank 120° Phase-Shift (PHS) */
#define CMD_PHASE_TOGGLE 0x60
/** Toggle Correction Disable Bit in Multiplier (CRD) */
#define CMD_CORRECTION_TOGGLE 0x70 

/**
 * LT8500 power up:
 *  1. Apply power and drive LDIBLANK low. SDO will go low when the POR de-asserts
 *  2. Send a correction frame 
 *  3. Send an update frame
 *  4. Send an output enable frame
 * 
 * The correction is \b enabled by default \n
 * The phase shifting is \b disabled by default \n
 * \n
 * The correction value is applied as following: \n
 * PWM(corrected) = PWM * (2/3) * ((Correction + 32)/64) \n
 * This correction sets a multiplier of about 0.5 times to 1.5 times the PWM value \n
 * 
 */

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
    uint8_t Correction_buffer[36]; // 48 channels of 6 bits
    
    uint8_t Frame_buffer[72];

    bool Correction_Enabled = true;
    bool PhaseShift_Enabled = false;

#if defined(__AVR_ATtinyxy7__)
    uint8_t LDI_bit_mask;
    PORT_t *LDI_port;
#endif

//Functions
public:
    LT8500(int8_t LDI_Blank, int8_t SDO, int8_t N_OpenLED = -1);
    ~LT8500();

    void begin();
    void end();

    void reset();

    void setPWM(uint8_t channel, uint16_t pwm);
    uint16_t getPWM(uint8_t channel);
    void setCorrection(uint8_t channel, uint8_t correction);
    uint8_t getCorrection(uint8_t channel);

    void sendSyncFrame();
    void sendAsyncFrame();
    void sendCorrectionFrame();

    void enableOutput();
    void disableOutput();

    void toggleCorrection();
    void togglePhaseShift();

    void selfTest();


//protected:
private:
    void sendLDIPulse();
    void sendResetPulse();

    void pack12to8(uint8_t array[], uint8_t index, uint16_t value);
    uint16_t unpack12to8(uint8_t array[], uint8_t index);

    void pack6to8(uint8_t array[], uint8_t index, uint8_t value);
    uint8_t unpack6to8(uint8_t array[], uint8_t index);

};


#endif // __LT8500_H