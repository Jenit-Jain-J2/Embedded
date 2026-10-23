#pragma once

#include <Arduino.h>
#include <Adafruit_ST7789.h>
#include "state.h" // Added to access SystemState

enum class ErrorCode : uint8_t {
  None = 0,
  Startup,
  Hardware,
  Command,
  Transport,
  SafetyFault // NEW: Critical HV fault (Over-current, Over-voltage)
};

void clearErrorState(SystemState& state); // Changed to take state
bool hasActiveError();
ErrorCode activeErrorCode();
const char* activeErrorTitle();
const char* activeErrorMessage();
void raiseError(Stream& channel, SystemState& state, ErrorCode code, const char* title, const char* message); // Changed to take state
void updateErrorState(SystemState& state);
void renderErrorScreen(Adafruit_ST7789& tft);