#pragma once

// Encoder Button (active-low gegen GND, mit Pullup)
#define ENCODER_BTN_PIN GP14
#define DYNAMIC_KEYMAP_LAYER_COUNT 5

// WS2812 / RGBLIGHT (QMK aktuell)
#define WS2812_DI_PIN GP15
#define RGBLIGHT_LED_COUNT 10     // Anzahl NeoPixel
#define RGBLIGHT_LIMIT_VAL 80
#define RGBLIGHT_LAYERS
#ifdef RGBLIGHT_ENABLE
#  define RGBLIGHT_EFFECT_BREATHING
#endif

// RP2040: double-tap reset into UF2 bootloader
#define RP2040_BOOTLOADER_DOUBLE_TAP_RESET
#define RP2040_BOOTLOADER_DOUBLE_TAP_RESET_TIMEOUT 200U

// optional, aber oft sinnvoll:
#define RGBLIGHT_SLEEP
#define RGBLIGHT_TIMEOUT 600000  // 10 Minuten (ms)


#define VIAL_KEYBOARD_UID {0x44, 0x33, 0x22, 0x11, 0x88, 0x77, 0x66, 0x55}
#define VIAL_UNLOCK_COMBO_ROWS {0, 5}
#define VIAL_UNLOCK_COMBO_COLS {0, 3}