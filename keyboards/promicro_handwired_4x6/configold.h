#pragma once

// Encoder Button (active-low gegen GND, mit Pullup)
#define ENCODER_BTN_PIN F7
#define DYNAMIC_KEYMAP_LAYER_COUNT 5

// WS2812 / RGBLIGHT (QMK aktuell)
#define WS2812_DI_PIN F4          // Arduino A3 = F4 (Pro Micro)
#define RGBLIGHT_LED_COUNT 10     // Anzahl NeoPixel
#define RGBLIGHT_LIMIT_VAL 80
#define RGBLIGHT_LAYERS
#ifdef RGBLIGHT_ENABLE
#  define RGBLIGHT_EFFECT_BREATHING
#endif

// optional, aber oft sinnvoll:
#define RGBLIGHT_SLEEP
