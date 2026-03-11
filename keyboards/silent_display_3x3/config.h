#pragma once

// Matrix diode direction
#define DIODE_DIRECTION COL2ROW

// Encoder button – active-low against GND, internal pull-up enabled
#define ENCODER_BTN_PIN GP10

// WS2812 RGB LEDs (10 LEDs, data on GP13)
// WS2812_DI_PIN and RGBLIGHT_LED_COUNT are defined by keyboard.json;
// redefining them here causes macro-redefinition errors.
#define RGBLIGHT_LIMIT_VAL 150
#define RGBLIGHT_DEFAULT_MODE  RGBLIGHT_MODE_STATIC_LIGHT
#define RGBLIGHT_DEFAULT_HUE   149
#define RGBLIGHT_DEFAULT_SAT   255
#define RGBLIGHT_DEFAULT_VAL   80

// OLED (SSD1306 via I2C0 on GP0/GP1)
// I2CD1 = RP2040 I2C0 hardware; QMK uses "I2C1_" prefix for its first I2C interface.
#define I2C_DRIVER I2CD1
#define I2C1_SCL_PIN GP1
#define I2C1_SDA_PIN GP0
#define OLED_TIMEOUT 60000

