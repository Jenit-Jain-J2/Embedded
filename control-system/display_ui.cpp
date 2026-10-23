#include "hardware/watchdog.h"
#include "display_ui.h"
#include "display_images.h"
#include "error_handling.h"
#include "ethernet_communication.h"
#include "tifrh_logo.h"

namespace {

constexpr int16_t kTextLineHeight = lineHeight;
constexpr int16_t kLiveValueX = valueX;
constexpr int16_t kLiveValueWidth = valueWidth;

struct LiveDisplayCache {
  bool initialized = false;
  int16_t live_text_start_x = -1;

  float target_vset_pos = 0.0f;
  float target_vset_neg = 0.0f;
  float target_iset_pos = 0.0f;
  float target_iset_neg = 0.0f;
  float hv_pos = 0.0f;
  float hv_neg = 0.0f;
  float imon_pos_ua = 0.0f;
  float imon_neg_ua = 0.0f;
};

enum class StartupPhase { Welcome, Warning, Live };

void setDisplayInverted(Adafruit_ST7789& tft, bool inverted) {
  static bool isInverted = false;
  if (isInverted == inverted) return;
  tft.invertDisplay(inverted);
  isInverted = inverted;
}

void delayPettingWatchdog(uint32_t durationMs) {
  constexpr uint32_t kChunkMs = 100;
  uint32_t remainingMs = durationMs;
  while (remainingMs > 0) {
    const uint32_t stepMs = (remainingMs < kChunkMs) ? remainingMs : kChunkMs;
    delay(stepMs);
    watchdog_update();
    remainingMs -= stepMs;
  }
}

bool nearlyEqual(float lhs, float rhs, float epsilon = 0.05f) {
  return fabs(lhs - rhs) <= epsilon;
}

void drawLeftAlignedTextLine(Adafruit_ST7789& tft, int16_t x, int16_t y, const char* text) {
  tft.setCursor(x, y);
  tft.print(text);
}

// Custom formatter: shows NO sign for 0, + for positive, - for negative
void formatDisplayValue(char* lineBuffer, size_t size, const char* label, float value, const char* unit) {
  // If value is within 0.005 of zero, treat it as exactly zero
  if (fabs(value) < 0.005f) {
    // 8 characters wide, zero-padded, 2 decimals, NO sign
    snprintf(lineBuffer, size, "%s%08.2f %s", label, 0.0f, unit);
  }
  else if (value > 0.0f) {
    // 8 characters wide, zero-padded, 2 decimals, EXPLICIT + sign
    snprintf(lineBuffer, size, "%s+%07.2f %s", label, value, unit);
  }
  else {
    // 8 characters wide, zero-padded, 2 decimals, EXPLICIT - sign
    snprintf(lineBuffer, size, "%s-%07.2f %s", label, fabs(value), unit);
  }
}

void drawWarningSign(Adafruit_ST7789& tft) {
  const int16_t x = (screenWidth - 170) / 2;
  const int16_t y = (screenHeight - 143) / 2;
  tft.drawRGBBitmap(x, y, dangerHighVoltageData, 170, 143);
}

void drawTifrLogo(Adafruit_ST7789& tft) {
  const int16_t x = 0;
  const int16_t y = (screenHeight - kTifrhJpegLogoHeight) / 2;
  tft.fillRect(0, 0, screenWidth, screenHeight, ST77XX_WHITE);
  tft.drawRGBBitmap(x, y, tifrhJpegLogoData, kTifrhJpegLogoWidth, kTifrhJpegLogoHeight);
}

void showWelcomeLogo(Adafruit_ST7789& tft) {
  tft.fillScreen(ST77XX_WHITE);
  drawTifrLogo(tft);
}

void showWarningScreen(Adafruit_ST7789& tft) {
  tft.fillScreen(ST77XX_WHITE);
  drawWarningSign(tft);
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(2);
  tft.setCursor(120, 194);
  tft.println("WARNING");
  tft.setTextColor(ST77XX_BLACK);
  tft.setCursor(95, 214);
  tft.println("HIGH VOLTAGE");
}

void drawLiveValueLine(Adafruit_ST7789& tft, int16_t x, int16_t y, const char* text, float value, float& lastValue, bool initialized, bool forceRedraw, float epsilon = 0.05f) {
  if (!forceRedraw && initialized && nearlyEqual(lastValue, value, epsilon)) return;
  tft.fillRect(kLiveValueX, y, kLiveValueWidth, kTextLineHeight, ST77XX_BLACK);
  drawLeftAlignedTextLine(tft, x, y, text);
  lastValue = value;
}

void renderLiveView(Adafruit_ST7789& tft, const SystemState& state, LiveDisplayCache& cache) {
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(textSize);

  char vsetPosLine[32], vsetNegLine[32];
  char isetPosLine[32], isetNegLine[32];
  char vmonPosLine[32], vmonNegLine[32];
  char imonPosLine[32], imonNegLine[32];
  char modeLine[32];

  float imon_pos_ua = state.imon_pos * CH1_IMON_FACTOR;
  float imon_neg_ua = state.imon_neg * CH2_IMON_FACTOR;

  // 1. Targets (Negative channels are passed as negative values so they display -1000.00, but 0 shows as 0000.00)
  formatDisplayValue(vsetPosLine, sizeof(vsetPosLine), "+Vset : ", state.target_vset_pos, "V");
  formatDisplayValue(vsetNegLine, sizeof(vsetNegLine), "-Vset : ", -state.target_vset_neg, "V");
  formatDisplayValue(isetPosLine, sizeof(isetPosLine), "+Iset : ", state.target_iset_pos, "uA");
  formatDisplayValue(isetNegLine, sizeof(isetNegLine), "-Iset : ", -state.target_iset_neg, "uA");

  // 2. Measurements
  formatDisplayValue(vmonPosLine, sizeof(vmonPosLine), "+Vmon : ", state.hv_pos, "V");
  formatDisplayValue(vmonNegLine, sizeof(vmonNegLine), "-Vmon : ", -state.hv_neg, "V");
  formatDisplayValue(imonPosLine, sizeof(imonPosLine), "+Imon : ", imon_pos_ua, "uA");
  formatDisplayValue(imonNegLine, sizeof(imonNegLine), "-Imon : ", -imon_neg_ua, "uA");

  // 3. Mode (Exactly 8 characters long: M-o-d-e- - - -: - )
  snprintf(modeLine, sizeof(modeLine), "Mode  : ");

  int16_t textStartX = 10;
  bool forceRedraw = !cache.initialized;

  drawLiveValueLine(tft, textStartX, liveTopY + 0 * lineHeight, vsetPosLine, state.target_vset_pos, cache.target_vset_pos, cache.initialized, forceRedraw);
  drawLiveValueLine(tft, textStartX, liveTopY + 1 * lineHeight, vsetNegLine, state.target_vset_neg, cache.target_vset_neg, cache.initialized, forceRedraw);
  drawLiveValueLine(tft, textStartX, liveTopY + 2 * lineHeight, isetPosLine, state.target_iset_pos, cache.target_iset_pos, cache.initialized, forceRedraw);
  drawLiveValueLine(tft, textStartX, liveTopY + 3 * lineHeight, isetNegLine, state.target_iset_neg, cache.target_iset_neg, cache.initialized, forceRedraw);

  drawLiveValueLine(tft, textStartX, liveTopY + 4 * lineHeight, vmonPosLine, state.hv_pos, cache.hv_pos, cache.initialized, forceRedraw);
  drawLiveValueLine(tft, textStartX, liveTopY + 5 * lineHeight, vmonNegLine, state.hv_neg, cache.hv_neg, cache.initialized, forceRedraw);
  drawLiveValueLine(tft, textStartX, liveTopY + 6 * lineHeight, imonPosLine, imon_pos_ua, cache.imon_pos_ua, cache.initialized, forceRedraw);
  drawLiveValueLine(tft, textStartX, liveTopY + 7 * lineHeight, imonNegLine, imon_neg_ua, cache.imon_neg_ua, cache.initialized, forceRedraw);

  if (forceRedraw) {
    tft.fillRect(kLiveValueX, liveTopY + 8 * lineHeight, kLiveValueWidth, kTextLineHeight, ST77XX_BLACK);
    drawLeftAlignedTextLine(tft, textStartX, liveTopY + 8 * lineHeight, modeLine);
  }

  cache.live_text_start_x = textStartX;
  cache.initialized = true;
}

}  // namespace

void initDisplay(Adafruit_ST7789& tft) {
  tft.init(screenHeight, screenWidth);
  tft.setRotation(3); // Aligned to how you want it
  tft.invertDisplay(false);
  tft.fillScreen(ST77XX_BLACK);
}

void renderDisplay(Adafruit_ST7789& tft, const SystemState& state) {
  static LiveDisplayCache cache;
  static bool initialFrame = true;
  static bool errorWasActive = false;
  static uint32_t welcomeStartMs = 0;
  static uint32_t warningStartMs = 0;
  static StartupPhase startupPhase = StartupPhase::Welcome;

  const bool errorActive = hasActiveError();

  if (errorActive) {
    setDisplayInverted(tft, false);
    renderErrorScreen(tft);
    errorWasActive = true;
    return;
  }

  if (errorWasActive) {
    startupPhase = StartupPhase::Live;
    initialFrame = false;
    cache.initialized = false;
    tft.invertDisplay(false);
    tft.fillScreen(ST77XX_BLACK);
    renderLiveView(tft, state, cache);
    errorWasActive = false;
    return;
  }

  const uint32_t now = millis();

  if (startupPhase == StartupPhase::Welcome) {
    if (welcomeStartMs == 0) {
      welcomeStartMs = now;
      showWelcomeLogo(tft);
      delayPettingWatchdog(welcomeScreenMs);
    }
    startupPhase = StartupPhase::Warning;
    warningStartMs = 0;
    return;
  }

  if (startupPhase == StartupPhase::Warning) {
    if (warningStartMs == 0) {
      warningStartMs = now;
      showWarningScreen(tft);
      delayPettingWatchdog(warningScreenMs);
    }
    startupPhase = StartupPhase::Live;
    initialFrame = true;
  }

  if (initialFrame) {
    tft.invertDisplay(false);
    tft.fillScreen(ST77XX_BLACK);
    initialFrame = false;
    cache.initialized = false;
    renderLiveView(tft, state, cache);
    return;
  }

  renderLiveView(tft, state, cache);
}