/* Copyright 2020 Yiancar
 *
 * GPL v2 or later
 */
#include QMK_KEYBOARD_H
#include "print.h"

/* --------------------------
   LED / Wanderlicht State
   -------------------------- */
static bool     enc_btn_released = true;   // Pullup => released = 1
static uint16_t enc_btn_tmr      = 0;

static uint8_t  dot_pos          = 0;
static uint16_t current_hue      = 128;

/* Layer -> Farbe (HSV Hue 0..255) */
static uint16_t hue_for_layer(uint8_t layer) {
    switch (layer) {
        case 0:  return 128; // HOME: türkis
        case 1:  return 43;  // L1:   gelb
        case 2:  return 170; // L2:   blau/purple
        case 3:  return 0;   // L3:   rot
        default: return 128;
    }
}

/* Render: alle LEDs aus, nur dot_pos an */
static void render_wander(uint16_t hue) {
    if (!rgblight_is_enabled()) return;

    // Effekt aus, sonst übermalt QMK unseren Render
    rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);

    for (uint8_t i = 0; i < RGBLIGHT_LED_COUNT; i++) {
        rgblight_sethsv_at(hue, 255, 0, i);
    }
    rgblight_sethsv_at(hue, 255, RGBLIGHT_LIMIT_VAL, dot_pos);
}

/* Proof-of-life + Button Pullup */
void keyboard_post_init_user(void) {
    uprintf("SINA_FIRMWARE_ALIVE\n");

#ifdef ENCODER_BTN_PIN
    setPinInputHigh(ENCODER_BTN_PIN); // Pullup für Button
#endif

    // Wenn du willst, dass RGB beim Boot garantiert an ist:
    // rgblight_enable_noeeprom();
    // render_wander(current_hue);
}

/* Button Polling (GPIO) */
void matrix_scan_user(void) {
#ifndef ENCODER_BTN_PIN
    return;
#else
    // 10ms debounce window
    if (timer_elapsed(enc_btn_tmr) < 10) return;

    bool pressed = (readPin(ENCODER_BTN_PIN) == 0); // active low gegen GND

    // Flanke: released -> pressed
    if (pressed && enc_btn_released) {
        enc_btn_tmr = timer_read();

        rgblight_toggle();

        // Wenn wir eingeschaltet haben: sofort rendern
        if (rgblight_is_enabled()) {
            render_wander(current_hue);
        }
    }

    enc_btn_released = !pressed;
#endif
}

/* Layerwechsel -> Farbe neu setzen */
layer_state_t layer_state_set_user(layer_state_t state) {
    uint8_t layer = get_highest_layer(state | default_layer_state);
    current_hue = hue_for_layer(layer);
    render_wander(current_hue);
    return state;
}

/* Encoder drehen -> Position bewegen */
bool encoder_update_user(uint8_t index, bool clockwise) {
    if (!rgblight_is_enabled()) return true;

    // Wenn deine Richtung falsch ist, NUR diese Zeile aktivieren:
    // clockwise = !clockwise;

    if (clockwise) {
        dot_pos = (dot_pos + 1) % RGBLIGHT_LED_COUNT;
    } else {
        dot_pos = (dot_pos == 0) ? (RGBLIGHT_LED_COUNT - 1) : (dot_pos - 1);
    }

    render_wander(current_hue);
    return true;
}

/* --------------------------
   Keymaps
   -------------------------- */
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT_6x4(
        KC_NO,   MO(2),        MO(1),        KC_BSPC,
        KC_NUM,  KC_KP_SLASH,  KC_PAST,      KC_PMNS,
        KC_KP_7, KC_KP_8,      KC_KP_9,      KC_KP_PLUS,
        KC_KP_4, KC_KP_5,      KC_KP_6,      KC_KP_PLUS,
        KC_KP_1, KC_KP_2,      KC_KP_3,      KC_PENT,
        KC_KP_0, KC_KP_0,      KC_KP_DOT,    KC_PENT
    ),

    [1] = LAYOUT_6x4(
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        BL_TOGG, BL_UP,   BL_DOWN, BL_BRTG,
        KC_TRNS, KC_UP,   KC_TRNS, KC_TRNS,
        KC_LEFT, KC_DOWN, KC_RGHT, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),

    [2] = LAYOUT_6x4(
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),

    [3] = LAYOUT_6x4(
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),
};
