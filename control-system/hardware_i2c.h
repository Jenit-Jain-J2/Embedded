#pragma once

#include "config.h"
#include "state.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <DFRobot_GP8XXX.h>

void setupI2cHardware(TwoWire& bus, int sdaPin, int sclPin);
void scanI2cDevices(TwoWire& bus);

struct AdcChannelReadout {
  float ch1 = 0.0f;
  float ch2 = 0.0f;
  float ch3 = 0.0f;
};

AdcChannelReadout readAdcChannels(Adafruit_ADS1115& adc);
void setupAdcModule(Adafruit_ADS1115& positive, Adafruit_ADS1115& negative);
void updateAdcReadings(Adafruit_ADS1115& positive, Adafruit_ADS1115& negative, SystemState& state);

void setupDacModule(DFRobot_GP8XXX_IIC& dac, SystemState& state);
// Changed from const to non-const because we update current_vset and timers
void updateDacOutputs(DFRobot_GP8XXX_IIC& dac, SystemState& state); 