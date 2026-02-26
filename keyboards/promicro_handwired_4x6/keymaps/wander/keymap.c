/* Copyright 2020 Yiancar
 *
 * GPL v2 or later
 */
#include QMK_KEYBOARD_H
#include "lib/lib8tion/lib8tion.h"   // sin8(), scale8()
#include "timer.h"

/*
  RGB (flash-schonend + smooth) — RGBLIGHT (WS2812 strip, 10 LEDs):
  - Startup: bunt + smooth pulsierend (global)
  - Danach: ruhige Layerfarbe als low-brightness "breathing"
  - Layerwechsel: kurzer Indikator auf einer anderen "Ecke" (unterschiedliche LED pro Layer)
  - Encoder drehen: EIN extra Dot (Layerfarbe) liegt kurz "oben drauf"
  - Encoder-Button: Layer 1 = Ctrl+V, sonst RGB an/aus
*/

/* ---------- LED count compatibility ---------- */
#if defined(RGBLIGHT_LED_COUNT)
#    define LED_COUNT RGBLIGHT_LED_COUNT
#elif defined(RGBLED_NUM)
#    define LED_COUNT RGBLED_NUM
#else
#    define LED_COUNT 10
#endif

/* ---------- Tuning ---------- */
#define STARTUP_MS            1800
#define FRAME_MS                15   // 15 = sehr smooth

/* Laufzeit-veränderbar über Settings-Layer */
static uint8_t  base_v_max      = 18;    // Grund-"breathing" max (schwach)
static uint8_t  current_sat     = 255;   // Sättigung (0..255)


#define DOT_V                   80   // Encoder-Dot Helligkeit
#define DOT_HOLD_MS            250
#define DOT_STEP_PER_TICK        1

#define IND_HOLD_MS            280   // Layer-Indikator Dauer
#define IND_V                  255   // Layer-Indikator Helligkeit

/* ---------- State ---------- */
static uint16_t t_start       = 0;
static uint16_t t_frame       = 0;

static uint16_t last_turn     = 0;  // encoder dot timer
static uint8_t  enc_dot_pos   = 0;


static uint8_t  current_hue   = 128;

static uint8_t  last_layer    = 0;
static uint16_t ind_tmr       = 0;
static bool     ind_active    = false;

/* Button debounce */
static bool     btn_released  = true;
static uint16_t btn_tmr       = 0;

/* ---------- Custom keycodes (Settings Layer) ---------- */
enum custom_keycodes {
    RGB_UI_TOG = SAFE_RANGE,
    RGB_UI_HUI,
    RGB_UI_HUD,
    RGB_UI_SAI,
    RGB_UI_SAD,
    RGB_UI_VAI,
    RGB_UI_VAD,
    RGB_UI_EFF_NEXT,
    RGB_UI_EFF_PREV
};

/* ---------- Layer -> Hue (0..255) ----------
   L0: türkis, L1: grün, L2: gelb, L3: lila
*/
static uint8_t hue_for_layer(uint8_t layer) {
    switch (layer) {
        case 0:  return 149; // hellblau
        case 1:  return 64;  // gelbgrün
        case 2:  return 170;  // blau
        case 3:  return 213; // magenta
        case 4:  return 0; //   rot
        default: return 149; // hellblau
    }
}

/* ---------- Layer -> "Ecke" (LED Index) ----------
   Passe diese Zuordnung an DEIN physisches Layout an.
*/
static const uint8_t indicator_led_for_layer[4] = { 0, 2, 7, 9 };

/* Dithered scaling: macht low-brightness smooth */
static uint8_t dither_scale_sin8(uint16_t now_div, uint8_t vmax) {
    uint8_t s = sin8(now_div);               // 0..255
    uint16_t v16 = (uint16_t)s * vmax;       // 0..(255*vmax)
    uint8_t v = v16 >> 8;                    // floor
    uint8_t frac = v16 & 0xFF;               // remainder

    uint16_t n = timer_read();
    uint8_t r = (uint8_t)(n ^ (n >> 8));     // cheap pseudo-random

    if (r < frac && v < vmax) v++;
    return v;
}

static inline void set_led_hsv(uint8_t idx, uint8_t h, uint8_t s, uint8_t v) {
    if (idx >= LED_COUNT) return;
    rgblight_sethsv_at(h, s, v, idx);
}

static void render_frame(void) {
    if (!rgblight_is_enabled()) return;

    uint16_t now = timer_read();
    bool startup = timer_elapsed(t_start) < STARTUP_MS;

    /* Settings-Layer (4): QMK RGBLIGHT-Effekte nicht übermalen (Startup darf laufen) */
    uint8_t layer = get_highest_layer(layer_state | default_layer_state);
    if (layer == 4 && !startup) {
        return;
    }

    /* Kein erzwungenes STATIC mehr */
    /* rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT); */

    if (startup) {
        /* bunt + smooth pulse (global) */
        uint8_t hue = (now / 8) & 0xFF;
#ifdef RGBLIGHT_LIMIT_VAL
        uint8_t v   = scale8(sin8(now / 6), RGBLIGHT_LIMIT_VAL);
#else
        uint8_t v   = scale8(sin8(now / 6), 120);
#endif
        rgblight_sethsv_noeeprom(hue, 255, v);
        return;
    }

    /* ---------- Base breathing (global, low brightness) ---------- */
    uint8_t base_v = dither_scale_sin8(now / 14, base_v_max);
    rgblight_sethsv_noeeprom(current_hue, current_sat, base_v);

    /* ---------- Layerwechsel-Indikator ---------- */
    if (ind_active && timer_elapsed(ind_tmr) < IND_HOLD_MS) {
        uint8_t layer = last_layer;
        uint8_t idx = indicator_led_for_layer[layer & 3];
        set_led_hsv(idx, current_hue, current_sat, IND_V);
    } else {
        ind_active = false;
    }

    /* ---------- Encoder-Dot overlay ---------- */
    if (timer_elapsed(last_turn) < DOT_HOLD_MS) {
        uint8_t dot_v = dither_scale_sin8(now / 6, DOT_V);
        uint8_t dp = (enc_dot_pos >= LED_COUNT) ? 0 : enc_dot_pos;
        set_led_hsv(dp, current_hue, current_sat, dot_v);
    }
}

void keyboard_post_init_user(void) {
#ifdef ENCODER_BTN_PIN
    setPinInputHigh(ENCODER_BTN_PIN);
#endif
    rgblight_enable_noeeprom();
    /* rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT); */

    t_start    = timer_read();
    t_frame    = timer_read();

    uint8_t layer = get_highest_layer(layer_state | default_layer_state);
    last_layer  = layer;
    current_hue = hue_for_layer(layer);

    /* Startzustand: Indicator kurz zeigen */
    ind_active = true;
    ind_tmr    = timer_read();

    render_frame();
}

void matrix_scan_user(void) {
    /* Animation tick */
    if (rgblight_is_enabled() && timer_elapsed(t_frame) >= FRAME_MS) {
        t_frame = timer_read();
        render_frame();
    }

#ifdef ENCODER_BTN_PIN
    /* Encoder-Button: Layer 1 = Ctrl+V, sonst RGB Toggle (active low) */
    if (timer_elapsed(btn_tmr) >= 10) {
        bool pressed = (readPin(ENCODER_BTN_PIN) == 0);

        if (pressed && btn_released) {
            btn_tmr = timer_read();

            uint8_t layer = get_highest_layer(layer_state | default_layer_state);

            if (layer == 1) {
                tap_code16(LCTL(KC_V));   // Ctrl+V
            } else {
                rgblight_toggle_noeeprom();
                if (rgblight_is_enabled()) {
                    ind_active = true;
                    ind_tmr    = timer_read();
                    render_frame();
                }
            }
        }
        btn_released = !pressed;
    }
#endif
}

layer_state_t layer_state_set_user(layer_state_t state) {
    uint8_t layer = get_highest_layer(state | default_layer_state);

    current_hue = hue_for_layer(layer);

    if (layer != last_layer) {
        last_layer = layer;
        ind_active = true;
        ind_tmr    = timer_read();
    }

    render_frame();
    return state;
}


/* ---------- Settings-Layer Custom RGB Controls ---------- */
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) return true;

    switch (keycode) {
        case RGB_UI_TOG:
            rgblight_toggle_noeeprom();
            if (rgblight_is_enabled()) {
                ind_active = true;
                ind_tmr    = timer_read();
                render_frame();
            }
            return false;

        case RGB_UI_HUI:
            current_hue += 8;
            render_frame();
            return false;

        case RGB_UI_HUD:
            current_hue -= 8;
            render_frame();
            return false;

        case RGB_UI_SAI:
            if (current_sat <= 247) current_sat += 8;
            else current_sat = 255;
            render_frame();
            return false;

        case RGB_UI_SAD:
            if (current_sat >= 8) current_sat -= 8;
            else current_sat = 0;
            render_frame();
            return false;

        case RGB_UI_VAI:
            if (base_v_max < 100) base_v_max += 2;
            render_frame();
            return false;

        case RGB_UI_VAD:
            if (base_v_max > 2) base_v_max -= 2;
            render_frame();
            return false;

        case RGB_UI_EFF_NEXT:
#ifdef RGBLIGHT_ENABLE
            rgblight_step_noeeprom();
#endif
            return false;

        case RGB_UI_EFF_PREV:
#ifdef RGBLIGHT_ENABLE
            rgblight_step_reverse_noeeprom();
#endif
            return false;
    }

    return true;
}

/* --------------------------
   Keymaps
   -------------------------- */
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    /*-----------------------NUMPAD----------------------------*/
    [0] = LAYOUT_6x4(
        KC_NO,              MO(1),                  MO(3),                  KC_BSPC,
        KC_NUM,             KC_PAST,                KC_PSLS,                KC_PMNS,
        KC_P7,              KC_P8,                  KC_P9,                  KC_PPLS,
        KC_P4,              KC_P5,                  KC_P6,                  KC_NO,
        KC_P1,              KC_P2,                  KC_P3,                  KC_PENT,
        KC_NO,              KC_P0,                  KC_PDOT,                KC_NO
    ),

    /*-----------------------EDIT------------------------------*/

    [1] = LAYOUT_6x4(
        KC_NO,                  MO(1),                  MO(3),                  KC_BSPC,
        KC_NO,                  KC_NO,                  KC_NO,                  LCTL(KC_A),
        LCTL(KC_Z),             S(KC_HOME),             LCTL(KC_R),             LCTL(KC_C),
        S(KC_LEFT),             LCTL(KC_S),             S(KC_RGHT),             KC_NO,
        LCTL(LSFT(KC_LEFT)),    S(KC_END),              LCTL(LSFT(KC_RGHT)),    KC_PENT,
        KC_NO,                  KC_SPACE,               LCTL(KC_X),             KC_NO
    ),

    /*-----------------------NAVIGATION---------------------------*/
    [2] = LAYOUT_6x4(
        KC_NO,                  TO(0),                  MO(3),                  KC_NO,
        KC_NO,                  KC_NO,                  KC_NO,                  KC_NO,
        LALT(LCTL(KC_LEFT)),    KC_NO,                  LALT(LCTL(KC_RGHT)),    KC_NO,
        LCTL(LGUI(KC_LEFT)),    KC_NO,                  LCTL(LGUI(KC_RGHT)),    KC_NO,
        KC_NO,                  KC_NO,                  KC_NO,                  KC_PENT,
        KC_NO,                  KC_NO,                  LCTL(LALT(KC_DEL)),     KC_NO
    ),

    /*-----------------------MAKRO--------------------------*/
    [3] = LAYOUT_6x4(
        KC_NO,         TO(0),             MO(3),                   KC_NO,
        KC_NO,         KC_NO,             KC_NO,                   KC_NO,             
        KC_F14,        KC_F15,            KC_F16,                  KC_NO,
        KC_F17,        KC_F18,            KC_F19,                  KC_PENT,
        KC_F20,        KC_F21,            KC_F22,                  KC_NO,
        KC_NO,         KC_NO,             KC_NO,                   KC_NO
    ),
    
 /*-----------------------SETTINGS--------------------------*/
    [4] = LAYOUT_6x4(
        KC_NO,              TO(0),              MO(3),                  KC_NO,
        KC_NO,              KC_NO,              RGB_UI_HUI,             RGB_UI_HUD,
        RGB_UI_VAI,         RGB_UI_VAD,         RGB_UI_EFF_NEXT,        KC_NO,
        RGB_UI_SAI,         RGB_UI_SAD,         RGB_UI_EFF_PREV,        KC_PENT,
        TO(1),              TO(2),              TO(3),                  KC_NO,
        KC_NO,              KC_NO,              KC_NO,                  KC_NO
    ),

   

    
};