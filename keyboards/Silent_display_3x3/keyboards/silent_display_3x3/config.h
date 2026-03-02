#pragma once

// Encoder button – active-low against GND, internal pull-up enabled
#define ENCODER_BTN_PIN B1

// WS2812 RGB LEDs (10 LEDs, data on Arduino pin 1 = D3)
#define WS2812_DI_PIN D3
#define RGBLIGHT_LED_COUNT 10
#define RGBLIGHT_LIMIT_VAL 150
#define RGBLIGHT_DEFAULT_MODE  RGBLIGHT_MODE_STATIC_LIGHT
#define RGBLIGHT_DEFAULT_HUE   149
#define RGBLIGHT_DEFAULT_SAT   255
#define RGBLIGHT_DEFAULT_VAL   80

// OLED (SSD1306 via I2C on D1/D0)
#define I2C1_SCL_PIN D2
#define I2C1_SDA_PIN D3
#define OLED_TIMEOUT 60000

// VIA – 6 layers (0-5)
#define DYNAMIC_KEYMAP_LAYER_COUNT 6
