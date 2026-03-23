#include QMK_KEYBOARD_H
#include "timer.h"
#include "gpio.h"

bool encoder_update_user(uint8_t index, bool clockwise) {
    (void)index;

    if (clockwise) {
        tap_code(KC_VOLU);
    } else {
        tap_code(KC_VOLD);
    }

    return false;
}

#ifdef RGBLIGHT_ENABLE
void keyboard_post_init_user(void) {
    // Initialize OLED power pin
    gpio_set_pin_output(GP25);
    gpio_write_pin_high(GP25);

    rgblight_enable_noeeprom();
    rgblight_mode_noeeprom(RGBLIGHT_MODE_BREATHING + 3);
    rgblight_set_speed_noeeprom(128);
    rgblight_sethsv_noeeprom(170, 255, 80);
}
#endif

#ifdef OLED_ENABLE
oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    return rotation;
}

static void render_welcome_animation(void) {
    uint8_t frame = (timer_read32() / 500) % 4;

    oled_clear();
    switch (frame) {
        case 0:
            oled_set_cursor(3, 1);
            oled_write_ln_P(PSTR("9KEY"), false);
            oled_set_cursor(1, 3);
            oled_write_ln_P(PSTR("MAKRO"), false);
            oled_set_cursor(0, 5);
            oled_write_ln_P(PSTR("MASTER"), false);
            break;
        case 1:
            oled_set_cursor(1, 0);
            oled_write_ln_P(PSTR("9key_makro"), false);
            oled_set_cursor(2, 2);
            oled_write_ln_P(PSTR("_master"), false);
            oled_set_cursor(4, 5);
            oled_write_ln_P(PSTR("hello"), false);
            break;
        case 2:
            oled_set_cursor(0, 1);
            oled_write_ln_P(PSTR("> 9key <"), false);
            oled_set_cursor(1, 3);
            oled_write_ln_P(PSTR("makro"), false);
            oled_set_cursor(1, 5);
            oled_write_ln_P(PSTR("master"), false);
            break;
        default:
            oled_set_cursor(0, 1);
            oled_write_ln_P(PSTR("WELCOME TO"), false);
            oled_set_cursor(0, 3);
            oled_write_ln_P(PSTR("9key_makro"), false);
            oled_set_cursor(1, 5);
            oled_write_ln_P(PSTR("master"), false);
            break;
    }
}

bool oled_task_user(void) {
    render_welcome_animation();
    return false;
}
#endif

enum layers {
    _BASE,
    _NAV,
    _EDIT,
    _MEDIA,
    _FN,
    _RGB,
    _SELECT
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT(
        LGUI(KC_TAB),  KC_UP,    LALT(KC_TAB),
        KC_LEFT,       KC_ENT,   KC_RGHT,
        LCTL(KC_Z),    KC_DOWN,  LCTL(KC_R)
    ),
    [_NAV] = LAYOUT(
        LGUI(KC_TAB),  KC_UP,    LALT(KC_TAB),
        KC_LEFT,       KC_ENT,   KC_RGHT,
        LCTL(KC_Z),    KC_DOWN,  LCTL(KC_R)
    ),
    [_EDIT] = LAYOUT(
        LCTL(KC_A),         LCTL(KC_C),   LCTL(KC_V),
        LCTL(KC_X),         LCTL(KC_ENT), KC_NO,
        LCTL(LSFT(KC_Z)),   KC_SPC,       KC_BSPC
    ),
    [_MEDIA] = LAYOUT(
        KC_MPRV,  KC_MSEL,  KC_MNXT,
        KC_MRWD,  KC_MPLY,  KC_MFFD,
        KC_DOWN,  KC_MSTP,  KC_UP
    ),
    [_FN] = LAYOUT(
        KC_F13,  KC_F14,  KC_F15,
        KC_F16,  KC_F17,  KC_F18,
        KC_F19,  KC_F20,  KC_F21
    ),
    [_RGB] = LAYOUT(
        UG_SPDU,  UG_SPDD,  UG_TOGG,
        UG_HUEU,  UG_HUED,  UG_VALU,
        UG_SATU,  UG_SATD,  UG_VALD
    ),
    [_SELECT] = LAYOUT(
        TO(_NAV),  TO(_EDIT),  TO(_MEDIA),
        TO(_FN),   TO(_BASE),  KC_NO,
        KC_NO,     KC_NO,      KC_NO
    ),
};

// Encoder, RGB, OLED logic should be implemented here for full compatibility.
