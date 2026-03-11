#include QMK_KEYBOARD_H
#include "timer.h"

enum custom_keycodes {
    RGB_UI_TOG = SAFE_RANGE,
    RGB_UI_WTOG,
    RGB_UI_HUI,
    RGB_UI_HUD,
    RGB_UI_SAI,
    RGB_UI_SAD,
    RGB_UI_VAI,
    RGB_UI_VAD,
    RGB_UI_WSPD_UP,
    RGB_UI_WSPD_DN
};

static bool btn_released = true;
static uint16_t btn_tmr = 0;

static bool user_rgb_on = true;
static uint8_t current_hue = 149;
static uint8_t current_sat = 255;
static uint8_t current_val = 80;
static uint8_t rgb_mode_idx = 0;

static const uint8_t rgb_modes[] = {
    RGBLIGHT_MODE_STATIC_LIGHT,
    RGBLIGHT_MODE_BREATHING,
};

static uint8_t hue_for_layer(uint8_t layer) {
    switch (layer) {
        case 0: return 149;
        case 1: return 64;
        case 2: return 170;
        case 3: return 213;
        case 4: return 0;
        default: return 149;
    }
}

static void apply_rgb_state(void) {
#ifdef RGBLIGHT_ENABLE
    if (!user_rgb_on) {
        rgblight_disable_noeeprom();
        return;
    }

    rgblight_enable_noeeprom();
    rgblight_mode_noeeprom(rgb_modes[rgb_mode_idx]);
    rgblight_sethsv_noeeprom(current_hue, current_sat, current_val);
#endif
}

void keyboard_post_init_user(void) {
    debug_enable = false;
    debug_matrix = false;
    debug_keyboard = false;

#ifdef ENCODER_BTN_PIN
    setPinInputHigh(ENCODER_BTN_PIN);
#endif

    uint8_t layer = get_highest_layer(layer_state | default_layer_state);
    current_hue = hue_for_layer(layer);
    apply_rgb_state();
}

void matrix_scan_user(void) {
#ifdef ENCODER_BTN_PIN
    if (timer_elapsed(btn_tmr) >= 10) {
        bool pressed = (readPin(ENCODER_BTN_PIN) == 0);

        if (pressed && btn_released) {
            btn_tmr = timer_read();
            user_rgb_on = !user_rgb_on;
            apply_rgb_state();
        }

        btn_released = !pressed;
    }
#endif
}

layer_state_t layer_state_set_user(layer_state_t state) {
    uint8_t layer = get_highest_layer(state | default_layer_state);
    current_hue = hue_for_layer(layer);
    apply_rgb_state();
    return state;
}

#ifdef ENCODER_ENABLE
bool encoder_update_user(uint8_t index, bool clockwise) {
    (void)index;

    if (clockwise) {
        tap_code16(MS_WHLU);
    } else {
        tap_code16(MS_WHLD);
    }

    return false;
}
#endif

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) {
        return true;
    }

    switch (keycode) {
        case RGB_UI_TOG:
            user_rgb_on = !user_rgb_on;
            apply_rgb_state();
            return false;

        case RGB_UI_WTOG:
            rgb_mode_idx = (rgb_mode_idx + 1) % ARRAY_SIZE(rgb_modes);
            apply_rgb_state();
            return false;

        case RGB_UI_HUI:
            current_hue += 8;
            apply_rgb_state();
            return false;

        case RGB_UI_HUD:
            current_hue -= 8;
            apply_rgb_state();
            return false;

        case RGB_UI_SAI:
            current_sat = (current_sat <= 247) ? (current_sat + 8) : 255;
            apply_rgb_state();
            return false;

        case RGB_UI_SAD:
            current_sat = (current_sat >= 8) ? (current_sat - 8) : 0;
            apply_rgb_state();
            return false;

        case RGB_UI_VAI:
            current_val = (current_val <= 247) ? (current_val + 8) : 255;
            apply_rgb_state();
            return false;

        case RGB_UI_VAD:
            current_val = (current_val >= 8) ? (current_val - 8) : 0;
            apply_rgb_state();
            return false;

        case RGB_UI_WSPD_UP:
            #ifdef RGB_SPI
            tap_code16(RGB_SPI);
            #else
            rgb_mode_idx = (rgb_mode_idx + 1) % ARRAY_SIZE(rgb_modes);
            apply_rgb_state();
            #endif
            return false;

        case RGB_UI_WSPD_DN:
            #ifdef RGB_SPD
            tap_code16(RGB_SPD);
            #else
            rgb_mode_idx = (rgb_mode_idx == 0) ? (ARRAY_SIZE(rgb_modes) - 1) : (rgb_mode_idx - 1);
            apply_rgb_state();
            #endif
            return false;
    }

    return true;
}

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT_6x4(
        KC_NO,              MO(1),                  MO(4),                  KC_BSPC,
        KC_NUM,             KC_PAST,                KC_PSLS,                KC_PMNS,
        KC_P7,              KC_P8,                  KC_P9,                  KC_PPLS,
        KC_P4,              KC_P5,                  KC_P6,                  KC_NO,
        KC_P1,              KC_P2,                  KC_P3,                  KC_PENT,
        KC_NO,              KC_P0,                  KC_PDOT,                KC_NO
    ),

    [1] = LAYOUT_6x4(
        KC_NO,                  TO(0),                  MO(4),                  KC_BSPC,
        KC_NO,                  KC_NO,                  KC_NO,                  LCTL(KC_A),
        LCTL(KC_Z),             S(KC_HOME),             LCTL(KC_R),             LCTL(KC_C),
        S(KC_LEFT),             LCTL(KC_S),             S(KC_RGHT),             KC_NO,
        LCTL(LSFT(KC_LEFT)),    S(KC_END),              LCTL(LSFT(KC_RGHT)),    KC_PENT,
        KC_NO,                  KC_SPACE,               LCTL(KC_X),             KC_NO
    ),

    [2] = LAYOUT_6x4(
        KC_NO,                  MO(0),                  MO(4),                  KC_NO,
        KC_NO,                  KC_NO,                  KC_NO,                  KC_NO,
        LALT(LCTL(KC_LEFT)),    KC_NO,                  LALT(LCTL(KC_RGHT)),    KC_NO,
        LCTL(LGUI(KC_LEFT)),    KC_NO,                  LCTL(LGUI(KC_RGHT)),    KC_NO,
        KC_NO,                  KC_NO,                  KC_NO,                  KC_PENT,
        KC_NO,                  KC_NO,                  LCTL(LALT(KC_DEL)),     KC_NO
    ),

    [3] = LAYOUT_6x4(
        KC_NO,                TO(0),                       MO(4),                           KC_NO,
        KC_NO,                KC_NO,                       KC_NO,                           KC_NO,
        KC_F14,               KC_F15,                      KC_F16,                          KC_NO,
        KC_F17,               KC_F18,                      KC_F19,                          KC_NO,
        KC_F20,               KC_F21,                      KC_F22,                          KC_NO,
        KC_NO,                KC_NO,                       KC_NO,                           KC_NO
    ),

    [4] = LAYOUT_6x4(
        KC_NO,              TO(0),              MO(4),                  KC_NO,
        RGB_UI_WSPD_UP,     RGB_UI_WSPD_DN,     RGB_UI_HUI,             RGB_UI_HUD,
        RGB_UI_VAI,         RGB_UI_VAD,         RGB_UI_WTOG,            RGB_UI_TOG,
        RGB_UI_SAI,         RGB_UI_SAD,         KC_NO,                  KC_NO,
        TO(1),              TO(2),              TO(3),                  KC_NO,
        KC_NO,              KC_NO,              KC_NO,                  KC_NO
    ),
};
