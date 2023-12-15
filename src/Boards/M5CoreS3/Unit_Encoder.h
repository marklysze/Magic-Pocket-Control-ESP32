/*!
 * @brief A rotary encoder expansion From M5Stack
 * @copyright Copyright (c) 2022 by M5Stack[https://m5stack.com]
 *
 * @Links [Unit Encoder](https://docs.m5stack.com/en/unit/encoder)
 * @version  V0.0.2
 * @date  2022-07-11
 */
#ifndef _UNIT_ENCODER_H_
#define _UNIT_ENCODER_H_

#include "Arduino.h"
#include "Wire.h"
#include "pins_arduino.h"

#define ENCODER_ADDR 0x40
#define ENCODER_REG  0x10
#define BUTTON_REG   0x20
#define RGB_LED_REG  0x30

// M5Stack CoreS3 Port A
#define G1 1
#define G2 2

class Unit_Encoder {
   private:
    uint8_t _addr;
    TwoWire* _wire;
    uint8_t _scl;
    uint8_t _sda;
    uint32_t _speed;
    void writeBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, uint8_t length);
    void readBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, uint8_t length);

   public:
    void begin(TwoWire* wire = &Wire, uint8_t addr = ENCODER_ADDR, uint8_t sda = G2, uint8_t scl = G1, uint32_t speed = 200000L);
    signed short int getEncoderValue();
    bool getButtonStatus();
    void setLEDColor(uint8_t index, uint32_t color);
};

#endif
