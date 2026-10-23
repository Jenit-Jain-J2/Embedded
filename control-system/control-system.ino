#include <Arduino.h>
#include "hardware/watchdog.h"

#include "config.h"
#include "state.h"
#include "hardware_i2c.h"
#include "display_ui.h"
#include "serial_communication.h"
#include "ethernet_communication.h"
#include "error_handling.h"

Adafruit_ADS1115 adsPositive;
Adafruit_ADS1115 adsNegative;
Adafruit_ST7789 tft = Adafruit_ST7789(&SPI1, TFT_CS, TFT_DC, TFT_RST);
DFRobot_GP8XXX_IIC gp8413(RESOLUTION_15_BIT, 0x58, &Wire1);

SystemState systemState;
bool startupScreensCompleted = false;

// --- Function to check safety limits ---
void evaluateSafety(SystemState& state) {
  if (state.control_state == ControlState::FAULT) return;

  float current_ua_pos = state.imon_pos * CH1_IMON_FACTOR;
  float current_ua_neg = state.imon_neg * CH2_IMON_FACTOR;

  // 1. Only trigger a hard FAULT if we hit the absolute physical limit of the iseg (470uA)
  if (current_ua_pos > HARD_IMAX_UA) {
    raiseError(Serial, state, ErrorCode::SafetyFault, "HARD_OVER_CURRENT", "Positive ch exceeded 470uA");
  }
  else if (current_ua_neg > HARD_IMAX_UA) {
    raiseError(Serial, state, ErrorCode::SafetyFault, "HARD_OVER_CURRENT", "Negative ch exceeded 470uA");
  }

  // 2. Check Over-Voltage
  if (state.hv_pos > (state.current_vset_pos + VOLTAGE_ERROR_MARGIN) && state.current_vset_pos > 10.0f) {
    raiseError(Serial, state, ErrorCode::SafetyFault, "OVER_VOLTAGE", "Positive ch over-voltage");
  }
  else if (state.hv_neg > (state.current_vset_neg + VOLTAGE_ERROR_MARGIN) && state.current_vset_neg > 10.0f) {
    raiseError(Serial, state, ErrorCode::SafetyFault, "OVER_VOLTAGE", "Negative ch over-voltage");
  }
}

void setup() {
  pinMode(LED_GPIO, OUTPUT);
  digitalWrite(LED_GPIO, LOW);

  systemState.transport_mode = TransportMode::Usb;
  systemState.display_mode = DisplayMode::Live;
  systemState.debug_mode = false;
  systemState.control_state = ControlState::IDLE;

  initializeSerialPort();
  delay(100);
  Serial.println("\n\n=== SYSTEM BOOTING ===");

  SPI1.setSCK(10);
  SPI1.setTX(11);
  SPI1.setRX(12);
  SPI1.setCS(TFT_CS);
  initDisplay(tft);

  renderDisplay(tft, systemState);  // Welcome screen
  renderDisplay(tft, systemState);  // Warning screen -> first live frame
  startupScreensCompleted = true;

  setupModeSelectPins();
  setupI2cHardware(Wire1, I2C_SDA_PIN, I2C_SCL_PIN);
  scanI2cDevices(Wire1);
  setupAdcModule(adsPositive, adsNegative);
  setupDacModule(gp8413, systemState);

  // Initialize Ethernet
  initializeEthernetCommunication(systemState);

  // Start in IDLE state, ready to accept targets
  systemState.control_state = ControlState::IDLE;

  watchdog_enable(I2C_WATCHDOG_TIMEOUT_MS, true);
}

void loop() {
  watchdog_update();

  if (!startupScreensCompleted) {
    renderDisplay(tft, systemState);
    renderDisplay(tft, systemState);
    startupScreensCompleted = true;
  }

  updateTransportMode(systemState);
  updateErrorState(systemState);

  // Handle incoming commands
  if (systemState.transport_mode == TransportMode::Ethernet) {
    pollEthernetCommunication(systemState);
  } else {
    handleSerialCommands(Serial, systemState, CommandSource::Usb);
  }

  // --- SAFETY ORDER OF OPERATIONS ---
  // 1. Read sensors FIRST
  updateAdcReadings(adsPositive, adsNegative, systemState);
  // 2. Evaluate safety
  evaluateSafety(systemState);
  // 3. Update DAC (ramps down if FAULT)
  updateDacOutputs(gp8413, systemState);

  // --- NEW TIMERS TO STOP FLOODING ---
  static uint32_t lastSerialPrint = 0;
  static uint32_t lastDisplayUpdate = 0;
  uint32_t now = millis();

  // Update display 10 times a second (100ms)
  if (now - lastDisplayUpdate >= 100) {
    renderDisplay(tft, systemState);
    lastDisplayUpdate = now;
  }

  // Print to serial exactly once per second (1000ms)
  if (now - lastSerialPrint >= 1000) {
    flushToSerial(systemState);
    lastSerialPrint = now;
  }

  digitalWrite(LED_GPIO, !digitalRead(LED_GPIO));
}