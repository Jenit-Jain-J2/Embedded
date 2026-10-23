#include "serial_communication.h"

#include <Ethernet.h>

#include "config.h"
#include "error_handling.h"

namespace {

bool canPrintToStream(Stream& serial, const SystemState& state) {
  (void)state;
  if (&serial == &Serial) {
    return isSerialHostConnected();
  }
  return true;
}

bool isSelectablePin(int pin) {
  return pin >= 0;
}

bool parseChannelAndValue(const String& payload,
                          uint8_t& channel,
                          float& value) {
  const int commaIndex = payload.indexOf(',');
  if (commaIndex < 0) return false;

  const String channelText = payload.substring(0, commaIndex);
  const String valueText = payload.substring(commaIndex + 1);

  channel = static_cast<uint8_t>(channelText.toInt());
  value = valueText.toFloat();

  return (channel == 1 || channel == 2);
}

uint16_t clampDacCode(long value) {
  if (value < 0) return 0;
  if (value > MAX_DAC_BITS) return MAX_DAC_BITS;
  return static_cast<uint16_t>(value);
}

bool parseDacAssignment(const String& payload, uint16_t& code) {
  char* endPtr = nullptr;
  const long parsedValue = strtol(payload.c_str(), &endPtr, 10);
  if (endPtr == payload.c_str() || *endPtr != '\0') return false;
  code = clampDacCode(parsedValue);
  return true;
}

void printDacCodes(Stream& serial, const SystemState& state) {
  if (!canPrintToStream(serial, state)) return;
  serial.print("DAC_CODE_0=");
  serial.print(state.dac_code_0);
  serial.print(", DAC_CODE_1=");
  serial.println(state.dac_code_1);
}

const char* transportText(TransportMode mode) {
  return (mode == TransportMode::Ethernet) ? "ETHERNET" : "USB";
}

const char* pinStateText(bool isHigh) {
  return isHigh ? "HIGH" : "LOW";
}

void printLine(Stream& serial, const SystemState& state, const String& message) {
  if (canPrintToStream(serial, state)) serial.println(message);
}

void printLine(Stream& serial, const SystemState& state, const __FlashStringHelper* message) {
  if (canPrintToStream(serial, state)) serial.println(message);
}

}  // namespace


void initializeSerialPort() {
  Serial.begin(SERIAL_BAUD_RATE);
}

bool isSerialHostConnected() {
  return static_cast<bool>(Serial);
}

bool isDebugOutputEnabled(const SystemState& state) {
  return state.debug_mode;
}

void setupModeSelectPins() {
  if (isSelectablePin(TRANSPORT_SEL_PIN)) {
    pinMode(TRANSPORT_SEL_PIN, TRANSPORT_SEL_USE_PULLUP ? INPUT_PULLUP : INPUT);
    if (TRANSPORT_SEL_USE_PULLUP) digitalWrite(TRANSPORT_SEL_PIN, HIGH);
  }
}

void updateTransportMode(SystemState& state) {
  if (!isSelectablePin(TRANSPORT_SEL_PIN)) {
    state.transport_mode = TransportMode::Usb;
    state.display_mode = DisplayMode::Live;
    return;
  }

  const bool ethernetSelected = (digitalRead(TRANSPORT_SEL_PIN) == HIGH);
  state.transport_mode = ethernetSelected ? TransportMode::Ethernet : TransportMode::Usb;

  if (state.transport_mode == TransportMode::Usb) {
    state.display_mode = DisplayMode::Live;
  }
}


void handleSerialCommands(Stream& serial, SystemState& state, CommandSource source) {
  (void)source;
  if (serial.available() <= 0) return;

  String command = serial.readStringUntil('\n');
  command.trim();
  command.toUpperCase();

  if (canPrintToStream(serial, state)) serial.println(command);

  if (command == "DEBUG" || command == "DEBUG ON") {
    state.debug_mode = true;
    printLine(serial, state, "Entering debug mode...");
  }

  else if (command == "STOP" || command == "DEBUG OFF") {
    state.debug_mode = false;
    printLine(serial, state, "Exiting debug mode...");
  }

  else if (command == "ERROR:CLEAR") {
    clearErrorState(state); // Pass state to reset FAULT
    printLine(serial, state, "Error state cleared");
  }

  else if (command == "CONF:TRAN?") {
    if (canPrintToStream(serial, state)) {
      serial.print("TRANSPORT=");
      serial.println(transportText(state.transport_mode));
    }
  }

  else if (command == "CONF:SEL?") {
    if (canPrintToStream(serial, state)) {
      const bool selectorHigh = isSelectablePin(TRANSPORT_SEL_PIN) && (digitalRead(TRANSPORT_SEL_PIN) == HIGH);
      serial.print("TRANSPORT_SEL_PIN=");
      serial.println(pinStateText(selectorHigh));
    }
  }

  else if (command.startsWith("SOUR:VOLT")) {
    if (state.control_state == ControlState::FAULT) {
       raiseError(serial, state, ErrorCode::Command, "REJECTED", "Cannot set voltage during FAULT");
       return;
    }

    const String payload = command.substring(9);
    uint8_t channel = 0;
    float volts = 0.0f;

    if (!parseChannelAndValue(payload, channel, volts)) {
      raiseError(serial, state, ErrorCode::Command, "COMMAND", "Expected SOUR:VOLT <ch>,<volts>");
      return;
    }

    volts = constrain(volts, 0.0f, VSET_MAX_VOLTS);

    if (channel == 1) {
      state.target_vset_pos = volts; // Only set target, DAC ramps later
      if (canPrintToStream(serial, state)) {
        serial.print("TARGET_VOLT1=");
        serial.println(state.target_vset_pos, 3);
      }
    } else {
      state.target_vset_neg = volts;
      if (canPrintToStream(serial, state)) {
        serial.print("TARGET_VOLT2=");
        serial.println(state.target_vset_neg, 3);
      }
    }
  }

  else if (command.startsWith("SOUR:CURR")) {
    const String payload = command.substring(9);
    uint8_t channel = 0;
    float ua = 0.0f;

    if (!parseChannelAndValue(payload, channel, ua)) {
      raiseError(serial, state, ErrorCode::Command, "COMMAND", "Expected SOUR:CURR <ch>,<ua>");
      return;
    }

    // HARD SAFETY LIMIT CHECK
    if (ua > HARD_IMAX_UA) {
       raiseError(serial, state, ErrorCode::Command, "REJECTED", "Current exceeds HARD_IMAX_UA");
       return;
    }

    if (channel == 1) {
      state.target_iset_pos = ua;
      if (canPrintToStream(serial, state)) { serial.print("TARGET_CURR1="); serial.println(state.target_iset_pos, 1); }
    } else {
      state.target_iset_neg = ua;
      if (canPrintToStream(serial, state)) { serial.print("TARGET_CURR2="); serial.println(state.target_iset_neg, 1); }
    }
  }

  else if (command == "MEAS:VOLT?" || command == "MEAS:VOLT") {
    if (canPrintToStream(serial, state)) {
      serial.printf("+Vset=%.1f, -Vset=%.1f, +Vmon=%.1f, -Vmon=%.1f\n",
                    state.current_vset_pos, state.current_vset_neg, state.hv_pos, state.hv_neg);
    }
  }

  else if (command == "MEAS:CURR?") {
    if (canPrintToStream(serial, state)) {
      float imon_uA_pos = state.imon_pos * CH1_IMON_FACTOR;
      float imon_uA_neg = state.imon_neg * CH2_IMON_FACTOR;
      serial.printf("+Iset=%.1f, -Iset=%.1f, +Imon=%.1f, -Imon=%.1f\n",
                    state.target_iset_pos, state.target_iset_neg, imon_uA_pos, imon_uA_neg);
    }
  }

  else if (command == "MEAS:DAC?") {
    if (canPrintToStream(serial, state)) {
      float vdac_pos = (state.dac_code_0 / (float)MAX_DAC_BITS) * 5.0f;
      float vdac_neg = (state.dac_code_1 / (float)MAX_DAC_BITS) * 5.0f;
      serial.printf("+Vdac=%.2f, -Vdac=%.2f\n", vdac_pos, vdac_neg);
    }
  }

  else if (command.startsWith("SOUR:DAC")) {
    if (!state.debug_mode) {
      raiseError(serial, state, ErrorCode::Command, "COMMAND", "Raw DAC commands require debug mode");
      return;
    }
    const String payload = command.substring(8);
    uint8_t channel = 0;
    float codeValue = 0.0f;
    if (!parseChannelAndValue(payload, channel, codeValue)) {
      raiseError(serial, state, ErrorCode::Command, "COMMAND", "Expected SOUR:DAC <ch>,<code>");
      return;
    }
    const uint16_t code = static_cast<uint16_t>(constrain(codeValue, 0.0f, static_cast<float>(MAX_DAC_BITS)));
    if (channel == 1) { state.dac_code_0 = code; } else { state.dac_code_1 = code; }
    if (canPrintToStream(serial, state)) { serial.print("DAC"); serial.print(channel); serial.print("="); serial.println(code); }
  }

  else if (command == "DAC?") { printDacCodes(serial, state); }

  else if (command.startsWith("DAC0=")) {
    if (!state.debug_mode) { raiseError(serial, state, ErrorCode::Command, "COMMAND", "Raw DAC commands require debug mode"); return; }
    if (parseDacAssignment(command.substring(5), state.dac_code_0)) {
      if (canPrintToStream(serial, state)) { serial.print("Updated DAC_CODE_0 to "); serial.println(state.dac_code_0); }
    } else { raiseError(serial, state, ErrorCode::Command, "COMMAND", "Expected DAC0=<0-32767>"); }
  }

  else if (command.startsWith("DAC1=")) {
    if (!state.debug_mode) { raiseError(serial, state, ErrorCode::Command, "COMMAND", "Raw DAC commands require debug mode"); return; }
    if (parseDacAssignment(command.substring(5), state.dac_code_1)) {
      if (canPrintToStream(serial, state)) { serial.print("Updated DAC_CODE_1 to "); serial.println(state.dac_code_1); }
    } else { raiseError(serial, state, ErrorCode::Command, "COMMAND", "Expected DAC1=<0-32767>"); }
  }

  else if (command.startsWith("DAC=")) {
    if (!state.debug_mode) { raiseError(serial, state, ErrorCode::Command, "COMMAND", "Raw DAC commands require debug mode"); return; }
    const String payload = command.substring(4);
    const int separatorIndex = payload.indexOf(',');
    if (separatorIndex < 0) { raiseError(serial, state, ErrorCode::Command, "COMMAND", "Expected DAC=<code0>,<code1>"); return; }
    const String code0Text = payload.substring(0, separatorIndex);
    const String code1Text = payload.substring(separatorIndex + 1);
    uint16_t newCode0 = state.dac_code_0;
    uint16_t newCode1 = state.dac_code_1;
    if (!parseDacAssignment(code0Text, newCode0) || !parseDacAssignment(code1Text, newCode1)) {
      printLine(serial, state, "ERROR: expected DAC=<code0>,<code1>");
      return;
    }
    state.dac_code_0 = newCode0;
    state.dac_code_1 = newCode1;
    if (canPrintToStream(serial, state)) {
      serial.print("Updated DAC_CODE_0 to "); serial.print(state.dac_code_0);
      serial.print(", DAC_CODE_1 to "); serial.println(state.dac_code_1);
    }
  }

  else {
    raiseError(serial, state, ErrorCode::Command, "COMMAND", "Unsupported command");
  }
}

// --- UPDATED: Matching the 2-decimal display format ---
void flushToSerial(const SystemState& state) {
  if (!isDebugOutputEnabled(state) || !isSerialHostConnected()) return;

  float imon_uA_pos = state.imon_pos * CH1_IMON_FACTOR;
  float imon_uA_neg = state.imon_neg * CH2_IMON_FACTOR;
  float vdac_pos = (state.dac_code_0 / (float)MAX_DAC_BITS) * 5.0f;
  float vdac_neg = (state.dac_code_1 / (float)MAX_DAC_BITS) * 5.0f;

  Serial.printf("MODE: %s | STATE: %d\n", transportText(state.transport_mode), static_cast<int>(state.control_state));
  Serial.printf("+Vset : %+08.2f V, -Vset : %+08.2f V\n", state.target_vset_pos, -state.target_vset_neg);
  Serial.printf("+Iset : %+08.2f uA, -Iset : %+08.2f uA\n", state.target_iset_pos, -state.target_iset_neg);
  Serial.printf("+Vmon : %+08.2f V, -Vmon : %+08.2f V\n", state.hv_pos, -state.hv_neg);
  Serial.printf("+Imon : %+08.2f uA, -Imon : %+08.2f uA\n", imon_uA_pos, -imon_uA_neg);
  Serial.printf("+Vdac : %+08.2f V, -Vdac : %+08.2f V\n", vdac_pos, -vdac_neg);
}