#include QMK_KEYBOARD_H
#include "timer.h"

// ---------------------------------------------------------------------------
// Layer definitions
// ---------------------------------------------------------------------------
enum layer_names {
    _BASE   = 0,  // Navigation
    _EDIT   = 1,  // Edit / clipboard
    _MEDIA  = 2,  // Media controls
    _FN     = 3,  // Function keys
    _RGB    = 4,  // RGB controls
    _SELECT = 5   // Layer selector
};

// ---------------------------------------------------------------------------
// Encoder button state
// ---------------------------------------------------------------------------
static bool     btn_pressed          = false;
static bool     btn_held_with_turn   = false;
static uint16_t btn_timer            = 0;

// ---------------------------------------------------------------------------
// Layer-selector state
// ---------------------------------------------------------------------------
static uint8_t pending_layer = 0;

// ---------------------------------------------------------------------------
// RGB state
// ---------------------------------------------------------------------------
static bool    user_rgb_on  = true;
static uint8_t current_val  = 80;

static uint8_t hue_for_layer(uint8_t layer) {
    switch (layer) {
        case _BASE:   return 149;  // Teal
        case _EDIT:   return 64;   // Yellow
        case _MEDIA:  return 170;  // Blue
        case _FN:     return 213;  // Purple
        case _RGB:    return 0;    // Red
        case _SELECT: return 85;   // Green
        default:      return 149;
    }
}

static void apply_rgb_for_layer(uint8_t layer) {
#ifdef RGBLIGHT_ENABLE
    if (!user_rgb_on) {
        rgblight_disable_noeeprom();
        return;
    }
    rgblight_enable_noeeprom();
    rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);
    rgblight_sethsv_noeeprom(hue_for_layer(layer), 255, current_val);
#endif
}

// ---------------------------------------------------------------------------
// Keyboard init
// ---------------------------------------------------------------------------
void keyboard_post_init_user(void) {
    debug_enable   = false;
    debug_matrix   = false;
    debug_keyboard = false;

    setPinInputHigh(ENCODER_BTN_PIN);
    apply_rgb_for_layer(_BASE);
}

// ---------------------------------------------------------------------------
// Layer change hook – update RGB colour
// ---------------------------------------------------------------------------
layer_state_t layer_state_set_user(layer_state_t state) {
    uint8_t layer = get_highest_layer(state | default_layer_state);
    apply_rgb_for_layer(layer);
    return state;
}

// ---------------------------------------------------------------------------
// Encoder button handling (pin not in matrix → polled in matrix_scan_user)
// ---------------------------------------------------------------------------
void matrix_scan_user(void) {
    bool is_pressed = (readPin(ENCODER_BTN_PIN) == 0);

    if (is_pressed && !btn_pressed) {
        // Rising edge: button just pressed
        btn_pressed        = true;
        btn_held_with_turn = false;
        btn_timer          = timer_read();
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
        }
    }
}

// ---------------------------------------------------------------------------
// Encoder rotation
// ---------------------------------------------------------------------------
bool encoder_update_user(uint8_t index, bool clockwise) {
    (void)index;

    uint8_t layer = get_highest_layer(layer_state | default_layer_state);

    if (btn_pressed) {
        // Hold button + rotate → cycle active layer (0-4, skip _SELECT)
        btn_held_with_turn = true;
        if (clockwise) {
            uint8_t next = (layer >= _RGB) ? _BASE : (layer + 1);
            layer_move(next);
        } else {
            uint8_t prev = (layer <= _BASE) ? _RGB : (layer - 1);
            layer_move(prev);
        }
        return false;
    }

    switch (layer) {
        case _BASE:
        case _EDIT:
        case _FN:
            clockwise ? tap_code16(MS_WHLU) : tap_code16(MS_WHLD);
            break;

        case _MEDIA:
            clockwise ? tap_code(KC_VOLU) : tap_code(KC_VOLD);
            break;

        case _RGB:
            if (clockwise) {
                current_val = (current_val <= 247) ? (current_val + 8) : 255;
            } else {
                current_val = (current_val >= 8) ? (current_val - 8) : 0;
            }
            if (user_rgb_on) {
                rgblight_sethsv_noeeprom(hue_for_layer(_RGB), 255, current_val);
            }
            break;

        case _SELECT:
            if (clockwise) {
                pending_layer = (pending_layer + 1) % 5;
            } else {
                pending_layer = (pending_layer == 0) ? 4 : (pending_layer - 1);
            }
            break;
    }
    return false;
}

// ---------------------------------------------------------------------------
// OLED display (SSD1306 via I2C)
// ---------------------------------------------------------------------------
#ifdef OLED_ENABLE
oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    return OLED_ROTATION_0;
}

static void oled_write_layer_name(uint8_t layer) {
    switch (layer) {
        case _BASE:   oled_write_P(PSTR("Base   "), false); break;
        case _EDIT:   oled_write_P(PSTR("Edit   "), false); break;
        case _MEDIA:  oled_write_P(PSTR("Media  "), false); break;
        case _FN:     oled_write_P(PSTR("Fn Keys"), false); break;
        case _RGB:    oled_write_P(PSTR("RGB    "), false); break;
        case _SELECT: oled_write_P(PSTR("Select "), false); break;
        default:      oled_write_P(PSTR("???    "), false); break;
    }
}

bool oled_task_user(void) {
    uint8_t layer = get_highest_layer(layer_state | default_layer_state);

    oled_write_P(PSTR("Silent 3x3\n"), false);
    oled_write_P(PSTR("Layer: "), false);
    oled_write_layer_name(layer);
    oled_write_P(PSTR("\n"), false);

    if (layer == _SELECT) {
        oled_write_P(PSTR("Goto: "), false);
        oled_write_layer_name(pending_layer);
        oled_write_P(PSTR("\n"), false);
    } else {
        oled_write_P(PSTR("\n"), false);
    }

    return false;
}
#endif  // OLED_ENABLE

// ---------------------------------------------------------------------------
// Key maps
// ---------------------------------------------------------------------------
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    // Layer 0 – Navigation
    [_BASE] = LAYOUT(
        LGUI(KC_TAB),  KC_UP,    LALT(KC_TAB),
        KC_LEFT,       KC_ENT,   KC_RGHT,
        LCTL(KC_Z),    KC_DOWN,  LCTL(KC_R)
    ),

    // Layer 1 – Edit / clipboard
    [_EDIT] = LAYOUT(
        LCTL(KC_A),         LCTL(KC_C),   LCTL(KC_V),
        LCTL(KC_X),         LCTL(KC_ENT), KC_NO,
        LCTL(LSFT(KC_Z)),   KC_SPC,       KC_BSPC
    ),

    // Layer 2 – Media controls
    [_MEDIA] = LAYOUT(
        KC_MPRV,  KC_MSEL,  KC_MNXT,
        KC_MRWD,  KC_MPLY,  KC_MFFD,
        KC_DOWN,  KC_MSTP,  KC_UP
    ),

    // Layer 3 – Function keys (F13-F21)
    [_FN] = LAYOUT(
        KC_F13,  KC_F14,  KC_F15,
        KC_F16,  KC_F17,  KC_F18,
        KC_F19,  KC_F20,  KC_F21
    ),

    // Layer 4 – RGB controls
    [_RGB] = LAYOUT(
        UG_SPDU,  UG_SPDD,  UG_TOGG,
        UG_HUEU,  UG_HUED,  UG_VALU,
        UG_SATU,  UG_SATD,  UG_VALD
    ),

    // Layer 5 – Layer selector (each key directly jumps to a layer)
    [_SELECT] = LAYOUT(
        TO(1),  TO(2),  TO(3),
        TO(4),  TO(0),  KC_NO,
        KC_NO,  KC_NO,  KC_NO
    ),
};
