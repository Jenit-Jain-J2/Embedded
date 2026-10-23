#include "hardware/watchdog.h"
#include "hardware_i2c.h"

void setupI2cHardware(TwoWire& bus, int sdaPin, int sclPin) {
  bus.setSDA(sdaPin);
  bus.setSCL(sclPin);
  bus.begin();
}

void scanI2cDevices(TwoWire& bus) {
  byte error;
  byte address;
  int nDevices = 0;

  Serial.println("Scanning I2C bus...");

  for (address = 1; address < 127; address++) {
    bus.beginTransmission(address);
    error = bus.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");
      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  if (nDevices == 0) {
    Serial.println("No I2C devices found");
  } else {
    Serial.println("Done");
  }
}

void setupAdcModule(Adafruit_ADS1115& positive, Adafruit_ADS1115& negative) {
  positive.setGain(GAIN_TWOTHIRDS);
  negative.setGain(GAIN_TWOTHIRDS);

  if (!positive.begin(ADDR_ADS1115_CH1, &Wire1)) {
    Serial.println("Failed to initialize positive-channel ADS1115. Please check the connection.");
  } else {
    Serial.println("Positive-channel ADS1115 initialized successfully.");
  }

  if (!negative.begin(ADDR_ADS1115_CH2, &Wire1)) {
    Serial.println("Failed to initialize negative-channel ADS1115. Please check the connection.");
  } else {
    Serial.println("Negative-channel ADS1115 initialized successfully.");
  }
}

AdcChannelReadout readAdcChannels(Adafruit_ADS1115& adc) {
  long sum1 = 0;
  long sum2 = 0;
  long sum3 = 0;

  for (uint16_t sample = 0; sample < NUM_SAMPLES; ++sample) {
    watchdog_update();
    sum1 += adc.readADC_SingleEnded(1);
    watchdog_update();
    sum2 += adc.readADC_SingleEnded(2);
    watchdog_update();
    sum3 += adc.readADC_SingleEnded(3);
    if (SAMPLE_DELAY_MS > 0) {
      delay(SAMPLE_DELAY_MS);
    }
  }

  AdcChannelReadout readout;
  const float scale = 1.0f / static_cast<float>(NUM_SAMPLES);
  readout.ch1 = static_cast<float>(sum1) * scale;
  readout.ch2 = static_cast<float>(sum2) * scale;
  readout.ch3 = static_cast<float>(sum3) * scale;
  return readout;
}

namespace {
void applyChannelReadout_pos(Adafruit_ADS1115& adc, const AdcChannelReadout& readout, float hvFactor, float& vset, float& vmon, float& imon, float& hv) {
  vset = adc.computeVolts(readout.ch1);
  vmon = adc.computeVolts(readout.ch2);
  imon = adc.computeVolts(readout.ch3);
  hv = hvFactor * vmon;
}

void applyChannelReadout_neg(Adafruit_ADS1115& adc, const AdcChannelReadout& readout, float hvFactor, float& vset, float& vmon, float& imon, float& hv) {
  vset = adc.computeVolts(readout.ch1);
  vmon = adc.computeVolts(readout.ch2);
  imon = adc.computeVolts(readout.ch3);
  hv = hvFactor * vmon;
}
} // namespace

void updateAdcReadings(Adafruit_ADS1115& positive, Adafruit_ADS1115& negative, SystemState& state) {
  const AdcChannelReadout posReadout = readAdcChannels(positive);
  applyChannelReadout_pos(positive, posReadout, HV_FACTOR_POS, state.hw_vset_pos, state.vmon_pos, state.imon_pos, state.hv_pos);

  const AdcChannelReadout negReadout = readAdcChannels(negative);
  applyChannelReadout_neg(negative, negReadout, HV_FACTOR_NEG, state.hw_vset_neg, state.vmon_neg, state.imon_neg, state.hv_neg);

  state.trend_hv_pos[state.trend_head] = state.hv_pos;
  state.trend_hv_neg[state.trend_head] = state.hv_neg;
  state.trend_imon_pos[state.trend_head] = state.imon_pos;
  state.trend_imon_neg[state.trend_head] = state.imon_neg;

  state.trend_head = static_cast<uint8_t>((state.trend_head + 1) % TREND_SAMPLES);
  if (state.trend_head == 0) {
    state.trend_full = true;
  }
}

void setupDacModule(DFRobot_GP8XXX_IIC& dac, SystemState& state) {
  if (dac.begin() != 0) {
    Serial.println("GP8413 DAC initialization failed. Please check the connection.");
  } else {
    Serial.println("GP8413 DAC initialized successfully.");
  }

  dac.setDACOutRange(dac.eOutputRange5V);

  state.dac_code_0 = 0;
  state.dac_code_1 = 0;
  state.current_vset_pos = 0.0f;
  state.current_vset_neg = 0.0f;
  state.last_ramp_time = millis();

  dac.setDACOutVoltage(0, 0);
  dac.setDACOutVoltage(0, 1);

  delay(SETTLE_DELAY_MS);
}

void updateDacOutputs(DFRobot_GP8XXX_IIC& dac, SystemState& state) {
  uint32_t now = millis();
  if (state.last_ramp_time == 0) state.last_ramp_time = now;

  float elapsedSeconds = static_cast<float>(now - state.last_ramp_time) / 1000.0f;
  state.last_ramp_time = now;

  // SAFETY OVERRIDE: If in HARD FAULT state, force targets to 0
  if (state.control_state == ControlState::FAULT) {
      state.target_vset_pos = 0.0f;
      state.target_vset_neg = 0.0f;
  }

  // --- POSITIVE CHANNEL: CONSTANT CURRENT & RAMPING ---
  float current_ua_pos = state.imon_pos * CH1_IMON_FACTOR;

  if (state.target_iset_pos > 0.0f && current_ua_pos > state.target_iset_pos) {
    // CC Loop active: Current exceeded limit! Fast ramp down to reduce current.
    state.current_vset_pos -= (CH1_RAMP_DOWN_V_PER_S * 5.0f) * elapsedSeconds;
    if (state.current_vset_pos < 0.0f) state.current_vset_pos = 0.0f;
  } else {
    // Normal safe ramping
    if (state.current_vset_pos < state.target_vset_pos) {
      state.current_vset_pos += CH1_RAMP_UP_V_PER_S * elapsedSeconds;
      if (state.current_vset_pos > state.target_vset_pos) state.current_vset_pos = state.target_vset_pos;
    } else if (state.current_vset_pos > state.target_vset_pos) {
      state.current_vset_pos -= CH1_RAMP_DOWN_V_PER_S * elapsedSeconds;
      if (state.current_vset_pos < state.target_vset_pos) state.current_vset_pos = state.target_vset_pos;
    }
  }

  // --- NEGATIVE CHANNEL: CONSTANT CURRENT & RAMPING ---
  float current_ua_neg = state.imon_neg * CH2_IMON_FACTOR;

  if (state.target_iset_neg > 0.0f && current_ua_neg > state.target_iset_neg) {
    // CC Loop active: Current exceeded limit! Fast ramp down.
    state.current_vset_neg -= (CH2_RAMP_DOWN_V_PER_S * 5.0f) * elapsedSeconds;
    if (state.current_vset_neg < 0.0f) state.current_vset_neg = 0.0f;
  } else {
    // Normal safe ramping
    if (state.current_vset_neg < state.target_vset_neg) {
      state.current_vset_neg += CH2_RAMP_UP_V_PER_S * elapsedSeconds;
      if (state.current_vset_neg > state.target_vset_neg) state.current_vset_neg = state.target_vset_neg;
    } else if (state.current_vset_neg > state.target_vset_neg) {
      state.current_vset_neg -= CH2_RAMP_DOWN_V_PER_S * elapsedSeconds;
      if (state.current_vset_neg < state.target_vset_neg) state.current_vset_neg = state.target_vset_neg;
    }
  }

  // --- CONVERT RAMPED VOLTAGE TO DAC CODES ---
  float dac_volts_pos = state.current_vset_pos / HV_FACTOR_POS;
  float dac_volts_neg = state.current_vset_neg / HV_FACTOR_NEG;

  if (dac_volts_pos > 5.0f) dac_volts_pos = 5.0f;
  if (dac_volts_neg > 5.0f) dac_volts_neg = 5.0f;

  uint16_t targetCode0 = static_cast<uint16_t>((dac_volts_pos / 5.0f) * MAX_DAC_BITS); // Positive
  uint16_t targetCode1 = static_cast<uint16_t>((dac_volts_neg / 5.0f) * MAX_DAC_BITS); // Negative

  state.dac_code_0 = targetCode0;
  state.dac_code_1 = targetCode1;

  // --- WRITE TO DAC (Only if changed) ---
  static uint16_t lastOutputCode0 = 0xFFFF;
  static uint16_t lastOutputCode1 = 0xFFFF;

  if (targetCode0 != lastOutputCode0) {
    dac.setDACOutVoltage(targetCode0, 0); // Positive on Channel 0
    lastOutputCode0 = targetCode0;
  }

  if (targetCode1 != lastOutputCode1) {
    dac.setDACOutVoltage(targetCode1, 1); // Negative on Channel 1
    lastOutputCode1 = targetCode1;
  }
}