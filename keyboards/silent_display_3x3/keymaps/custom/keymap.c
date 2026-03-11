#include QMK_KEYBOARD_H
#include "gpio.h"

static bool rgb_on = true;
static uint8_t rgb_val = RGBLIGHT_DEFAULT_VAL;

void keyboard_post_init_user(void) {
    gpio_set_pin_input_high(ENCODER_BTN_PIN);
    rgblight_enable_noeeprom();
    rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);
    rgblight_sethsv_noeeprom(RGBLIGHT_DEFAULT_HUE, RGBLIGHT_DEFAULT_SAT, rgb_val);
}

void matrix_scan_user(void) {
    bool is_pressed = (gpio_read_pin(ENCODER_BTN_PIN) == 0);

    if (is_pressed && !btn_pressed) {
        // Rising edge: button just pressed
        btn_pressed        = true;
        btn_held_with_turn = false;
    } else if (!is_pressed && btn_pressed) {
        // Falling edge: button just released
        btn_pressed = false;

        if (!btn_held_with_turn) {
            // Short-click action per current layer
            uint8_t layer = get_highest_layer(layer_state | default_layer_state);
            switch (layer) {
                case _BASE:
                    pending_layer = 0;
                    layer_move(_SELECT);
                    break;
                case _EDIT:
                    tap_code16(LCTL(KC_S));
                    break;
                case _MEDIA:
                    tap_code(KC_MUTE);
                    break;
                case _FN:
                    tap_code(KC_F23);
                    break;
                case _RGB:
                    user_rgb_on = !user_rgb_on;
                    apply_rgb_for_layer(get_highest_layer(layer_state | default_layer_state));
                    break;
                case _SELECT:
                    layer_move(pending_layer);
                    break;
            }
    static bool last_pressed = false;
    bool pressed = (gpio_read_pin(ENCODER_BTN_PIN) == 0);

    // Toggle RGB on encoder button release.
    if (last_pressed && !pressed) {
        rgb_on = !rgb_on;
        if (rgb_on) {
            rgblight_enable_noeeprom();
            rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);
            rgblight_sethsv_noeeprom(RGBLIGHT_DEFAULT_HUE, RGBLIGHT_DEFAULT_SAT, rgb_val);
        } else {
            rgblight_disable_noeeprom();
        }
    }

    last_pressed = pressed;
}

bool encoder_update_user(uint8_t index, bool clockwise) {
    (void)index;

    if (rgb_on) {
        if (clockwise) {
            rgb_val = (rgb_val <= 247) ? (rgb_val + 8) : 255;
        } else {
            rgb_val = (rgb_val >= 8) ? (rgb_val - 8) : 0;
        }
        rgblight_sethsv_noeeprom(RGBLIGHT_DEFAULT_HUE, RGBLIGHT_DEFAULT_SAT, rgb_val);
    }

    clockwise ? tap_code(KC_UP) : tap_code(KC_DOWN);
    return false;
}

#ifdef OLED_ENABLE
oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    return OLED_ROTATION_0;
}

bool oled_task_user(void) {
    oled_clear();
    oled_write_ln_P(PSTR("silent 3x3"), false);
    oled_write_P(PSTR("RGB: "), false);
    oled_write_ln_P(rgb_on ? PSTR("on") : PSTR("off"), false);
    oled_write_P(PSTR("Val: "), false);
    oled_write(get_u8_str(rgb_val, ' '), false);
    oled_write_ln_P(PSTR(""), false);
    return false;
}
#endif

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        KC_1, KC_2, KC_3,
        KC_4, KC_5, KC_6,
        KC_7, KC_8, KC_9
    ),
};
