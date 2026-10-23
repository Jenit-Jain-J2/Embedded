#pragma once

#include <Arduino.h>

#include "state.h"

enum class CommandSource : uint8_t {
  Usb,
  Ethernet,
};

void initializeSerialPort();
bool isSerialHostConnected();
bool isDebugOutputEnabled(const SystemState& state);
void setupModeSelectPins();
void updateTransportMode(SystemState& state);
void handleSerialCommands(Stream& serial, SystemState& state, CommandSource source);
void flushToSerial(const SystemState& state);