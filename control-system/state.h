#pragma once

#include <Arduino.h>

#include "config.h"

enum class TransportMode : uint8_t {
  Usb,
  Ethernet,
};

enum class DisplayMode : uint8_t {
  Live,
  Trend,
};

// New: Safety State Machine
enum class ControlState : uint8_t {
  IDLE,          // Output is 0V, ready to ramp
  RAMPING_UP,    // Moving towards target
  RAMPING_DOWN,  // Moving towards target (or 0V)
  STEADY,        // Reached target and holding
  FAULT          // Error detected, ramping down safely (or hardware shutoff)
};

// New: Fault Codes for debugging and display
enum class FaultCode : uint8_t {
  NONE,
  OVER_CURRENT,
  OVER_VOLTAGE,
  SENSOR_FAILURE,
  COMMS_LOSS,
  WATCHDOG_RESET
};

struct SystemState {
  // --- MEASUREMENTS (From ADC) ---
  float vmon_pos = 0.0f;
  float imon_pos = 0.0f;
  float vmon_neg = 0.0f;
  float imon_neg = 0.0f;

  // NEW: Hardware Vset feedback from the iseg module
  float hw_vset_pos = 0.0f;
  float hw_vset_neg = 0.0f;

  // --- TARGETS (Set by User/Remote) ---
  float target_vset_pos = 0.0f; // Where we WANT the voltage to be
  float target_vset_neg = 0.0f;
  float target_iset_pos = 0.0f; // User-set current limit (must be <= HARD_IMAX)
  float target_iset_neg = 0.0f;

  // --- ACTIVE SETPOINTS (Ramped values, sent to DAC) ---
  float current_vset_pos = 0.0f; // The actual value being sent to the DAC right now
  float current_vset_neg = 0.0f;
  float hv_pos = 0.0f;           // Calculated actual HV based on Vmon
  float hv_neg = 0.0f;

  // --- DAC CODES ---
  uint16_t dac_code_1 = 0; // Positive Channel
  uint16_t dac_code_0 = 0; // Negative Channel

  // --- STATE MACHINE & SAFETY ---
  ControlState control_state = ControlState::IDLE;
  FaultCode active_fault = FaultCode::NONE;
  uint32_t last_ramp_time = 0; // Timer for the 10V/s ramp calculation

  // --- DISPLAY & COMMS ---
  TransportMode transport_mode = TransportMode::Usb;
  DisplayMode display_mode = DisplayMode::Live;
  bool debug_mode = false;

  // --- TREND BUFFERS (For Graph) ---
  float trend_hv_pos[TREND_SAMPLES] = {0.0f};
  float trend_hv_neg[TREND_SAMPLES] = {0.0f};
  float trend_imon_pos[TREND_SAMPLES] = {0.0f};
  float trend_imon_neg[TREND_SAMPLES] = {0.0f};
  uint8_t trend_head = 0;
  bool trend_full = false;
};