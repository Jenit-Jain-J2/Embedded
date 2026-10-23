#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#include "state.h"

constexpr uint16_t screenWidth = 320;
constexpr uint16_t screenHeight = 240;
constexpr uint16_t textSize = 2; // Size 2 fits 9 lines perfectly

constexpr uint16_t lineHeight = 24;
constexpr uint16_t valueX = 0;
constexpr uint16_t valueWidth = screenWidth - valueX;
constexpr uint16_t liveTopY = 0;

constexpr uint16_t trendHeaderY = 0;
constexpr uint16_t trendHeaderHeight = 24;
constexpr uint16_t trendGraphTop1 = 24;
constexpr uint16_t trendGraphTop2 = 130;
constexpr uint16_t trendGraphHeight = 100;
constexpr uint16_t trendGraphWidth = screenWidth;
constexpr uint16_t trendPlotPadding = 6;
constexpr uint16_t trendPlotOffset = 3;
constexpr uint16_t trendLabelOffset = 12;
constexpr uint8_t trendSamples = TREND_SAMPLES;
constexpr uint32_t welcomeScreenMs = 2000;
constexpr uint32_t warningScreenMs = 3000;

void initDisplay(Adafruit_ST7789& tft);
void renderDisplay(Adafruit_ST7789& tft, const SystemState& state);