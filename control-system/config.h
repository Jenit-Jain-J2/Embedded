#pragma once

#include <Arduino.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <DFRobot_GP8XXX.h>
#include <SPI.h>
#include <Wire.h>

// Steady-state watchdog timeout: tight enough to catch a hung I2C bus
constexpr uint32_t I2C_WATCHDOG_TIMEOUT_MS = 2000;
constexpr uint32_t WATCHDOG_STARTUP_TIMEOUT_MS = 8000;

constexpr uint32_t SERIAL_BAUD_RATE = 9600;

constexpr int WNET_RX_PIN = 16;
constexpr int WNET_CS_PIN = 17;
constexpr int WNET_SCK_PIN = 18;
constexpr int WNET_TX_PIN = 19;
constexpr int WNET_RST_PIN = 20;
constexpr int WNET_INT_PIN = 21;

constexpr int TFT_SCK = 10;
constexpr int TFT_TX = 11;
constexpr int TFT_RX = 12;
constexpr int TFT_CS = 13;
constexpr int TFT_DC = 14;
constexpr int TFT_RST = 15;

constexpr int LED_GPIO = 25;
constexpr int I2C_SDA_PIN = 2;
constexpr int I2C_SCL_PIN = 3;

constexpr int TRANSPORT_SEL_PIN = 1;
constexpr bool TRANSPORT_SEL_USE_PULLUP = false;

constexpr uint8_t ADDR_GP8413 = 0x58;
constexpr uint8_t ADDR_ADS1115_CH1 = 0x4A;
constexpr uint8_t ADDR_ADS1115_CH2 = 0x48;

constexpr float CH1_HV_FACTOR = 1233.7708f; // done
constexpr float CH2_HV_FACTOR = 1226.2254f;
constexpr float CH1_MAX_VOLTS = 6000.0f;
constexpr float HV_FACTOR_POS = CH1_HV_FACTOR;
constexpr float HV_FACTOR_NEG = CH2_HV_FACTOR;
constexpr float VSET_MAX_VOLTS = CH1_MAX_VOLTS;

constexpr uint16_t MAX_DAC_BITS = 32767;
constexpr uint16_t DAC_BOOT_CODE = 655; // TODO: Verify if this offset is safe or if it should be 0
constexpr uint16_t NUM_SAMPLES = 50;
constexpr uint16_t SAMPLE_DELAY_MS = 0;
constexpr uint16_t SETTLE_DELAY_MS = 2;
constexpr uint8_t TREND_SAMPLES = 64;

constexpr uint8_t ETHERNET_MAC_ADDRESS[6] = {0x02, 0x60, 0x51, 0x10, 0x50, 0x01};
constexpr uint16_t TCP_PORT = 5025;
constexpr unsigned long ETHERNET_DHCP_TIMEOUT_MS = 4000;
constexpr unsigned long ETHERNET_DHCP_RESPONSE_TIMEOUT_MS = 1000;
constexpr unsigned long ETHERNET_DHCP_RETRY_MS = 10000;

// ==========================================
// --- SAFETY & RAMPING PARAMETERS ---
// ==========================================

// Ramp rates (Volts per second). Defaulted to 10V/s as discussed.
constexpr float CH1_RAMP_UP_V_PER_S   = 2.0f;  // +ve Ramp Up
constexpr float CH1_RAMP_DOWN_V_PER_S = 10.0f;  // +ve Ramp Down
constexpr float CH2_RAMP_UP_V_PER_S   = 2.0f;  // -ve Ramp Up
constexpr float CH2_RAMP_DOWN_V_PER_S = 10.0f;  // -ve Ramp Down

// How often the State Machine updates the DAC to achieve the ramp (in milliseconds)
constexpr uint32_t RAMP_UPDATE_INTERVAL_MS = 100;

// Hardcoded Hardware Current Limits (TODO: Adjust these once we know the exact Imon scaling)
constexpr float HARD_IMAX_UA = 470.0f; // Absolute max current the iseg can output

// TODO: Need the exact scaling factor. (e.g., if 5V output = 470uA, factor is 94.0f)
constexpr float CH1_IMON_FACTOR = 74.70f;
constexpr float CH2_IMON_FACTOR = 74.97f;

// TODO: Over-voltage safety threshold (e.g., abort if Vmon exceeds Vset by 50V)
constexpr float VOLTAGE_ERROR_MARGIN = 50.0f;