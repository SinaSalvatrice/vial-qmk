/* Copyright 2020 Yiancar
 *
 * GPL v2 or later
 *
 * Small fix: avoid full 0 brightness at breathing trough to remove the visible blink.
 * Strategy: use a small non-zero breathing floor (base_v_min) so base breathing never hits 0.
 * Kept all other logic intact.
 */
#include QMK_KEYBOARD_H
#include "lib/lib8tion/lib8tion.h"   // sin8(), scale8()
#include "timer.h"

/*
  RGB behavior adapted per user request:
  - Default: wandering dot only (no static breathing)
  - Encoder button toggles RGB on/off
  - Encoder turn = mouse wheel (up/down). Encoder dot overlay kept.
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
#define STARTUP_MS            0     // disabled
#define FRAME_MS               20   // a bit slower to reduce flicker

/* Runtime-changeable through Settings-Layer */
static uint8_t  base_v_max      = 25;    // breathing max (low)
static uint8_t  base_v_min      = 1;     // breathing floor (non-zero to avoid blink)
static uint16_t wander_step_ms  = 120;   // wander speed
static uint8_t  current_sat     = 255;   // saturation (0..255)

#define WANDER_V                55   // wander-peak brightness
#define WANDER_TRAIL_V          24   // trail neighbor brightness

#define DOT_V                   80   // encoder-dot brightness
#define DOT_HOLD_MS            250
#define DOT_STEP_PER_TICK        1

#define IND_HOLD_MS            280   // layer indicator duration
#define IND_V                  255   // indicator brightness

/* ---------- State ---------- */
static uint16_t t_frame       = 0;

static uint16_t last_turn     = 0;  // encoder dot timer
static uint8_t  enc_dot_pos   = 0;

static uint8_t  wander_pos    = 0;
static uint16_t wander_tmr    = 0;

static uint8_t  current_hue   = 128;

static uint8_t  last_layer    = 0;
static uint16_t ind_tmr       = 0;
static bool     ind_active    = false;

/* RGB style mode:
   0 = Wander-only (default)
   1 = Breathing + wander overlay
   2 = All-LED breathing
*/
static uint8_t rgb_mode = 0;

/* user-level on/off (keeps from calling library toggle which causes a blink) */
static bool user_rgb_on = true;

/* Button debounce for encoder button */
static bool     btn_released  = true;
static uint16_t btn_tmr       = 0;

/* ---------- Custom keycodes (Settings Layer) ---------- */
enum custom_keycodes {
    RGB_UI_TOG = SAFE_RANGE,
    RGB_UI_WTOG,      // cycle rgb_mode
    RGB_UI_HUI,
    RGB_UI_HUD,
    RGB_UI_SAI,
    RGB_UI_SAD,
    RGB_UI_VAI,
    RGB_UI_VAD,
    RGB_UI_WSPD_UP,
    RGB_UI_WSPD_DN
};

/* ---------- Layer -> Hue (0..255) ----------
   L0: light blue, L1: yellow-green, L2: blue, L3: magenta, L4: red
*/
static uint8_t hue_for_layer(uint8_t layer) {
    switch (layer) {
        case 0:  return 149; // light blue
        case 1:  return 64;  // yellow-green
        case 2:  return 170; // blue
        case 3:  return 213; // magenta
        case 4:  return 0;   // red
        default: return 149; // fallback
    }
}

/* ---------- Layer -> "corner" (LED Index) ----------
   Adjust these to your physical layout.
*/
static const uint8_t indicator_led_for_layer[5] = { 0, 2, 7, 9, 4 };

/* Dithered scaling: makes low-brightness smooth */
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

/* helper: clear all LEDs immediately (per-LED writes) */
static void clear_all_leds(void) {
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        set_led_hsv(i, 0, 0, 0);
    }
}

static void render_frame(void) {
    /* Use user_rgb_on as guard to avoid library-level toggles that cause a blink */
    if (!user_rgb_on) {
        clear_all_leds();
        return;
    }

    uint16_t now = timer_read();

    /* Mode handling:
       - MODE 2 (All-LED breathing): set every LED to the same breathing value
       - MODE 1 (Breathing + wander): base breathing for all LEDs + wander overlay
       - MODE 0 (Wander-only): clear all LEDs and show only the wander dot + trail
    */

    if (rgb_mode == 2) {
        /* All-LED breathing (per-LED write) */
        uint8_t base_v;
        if (base_v_max > base_v_min) {
            /* scale sin into range [base_v_min .. base_v_max] */
            uint8_t span = base_v_max - base_v_min;
            base_v = dither_scale_sin8(now / 14, span);
            base_v = base_v + base_v_min;
        } else {
            base_v = base_v_min;
        }

        for (uint8_t i = 0; i < LED_COUNT; i++) {
            set_led_hsv(i, current_hue, current_sat, base_v);
        }
    } else if (rgb_mode == 1) {
        /* Breathing base (per-LED) */
        uint8_t base_v;
        if (base_v_max > base_v_min) {
            uint8_t span = base_v_max - base_v_min;
            base_v = dither_scale_sin8(now / 14, span);
            base_v = base_v + base_v_min;
        } else {
            base_v = base_v_min;
        }

        for (uint8_t i = 0; i < LED_COUNT; i++) {
            set_led_hsv(i, current_hue, current_sat, base_v);
        }
        /* wander overlay drawn below */
    } else {
        /* Wander-only: clear all LEDs first (per-LED) */
        for (uint8_t i = 0; i < LED_COUNT; i++) {
            set_led_hsv(i, 0, 0, 0);
        }
    }

    /* ---------- Wander dot (present in mode 0 and 1) ---------- */
    if (rgb_mode != 2) {
        uint8_t w_v = dither_scale_sin8(now / 10, WANDER_V);

        uint8_t wp = (wander_pos >= LED_COUNT) ? 0 : wander_pos;
        uint8_t left  = (wp == 0) ? (LED_COUNT - 1) : (wp - 1);
        uint8_t right = (wp + 1) % LED_COUNT;

        uint8_t trail_v = dither_scale_sin8(now / 12, WANDER_TRAIL_V);

        if (last_layer == 0) {
            uint8_t rainbow_base = (now / 8) & 0xFF;
            uint8_t h_left  = rainbow_base + (left  * (uint8_t)(256 / LED_COUNT));
            uint8_t h_wp    = rainbow_base + (wp    * (uint8_t)(256 / LED_COUNT));
            uint8_t h_right = rainbow_base + (right * (uint8_t)(256 / LED_COUNT));

            set_led_hsv(left,  h_left,  255, trail_v);
            set_led_hsv(wp,    h_wp,    255, w_v);
            set_led_hsv(right, h_right, 255, trail_v);
        } else {
            set_led_hsv(left,  current_hue, current_sat, trail_v);
            set_led_hsv(wp,    current_hue, current_sat, w_v);
            set_led_hsv(right, current_hue, current_sat, trail_v);
        }
    }

    /* ---------- Layer change indicator ---------- */
    if (ind_active && timer_elapsed(ind_tmr) < IND_HOLD_MS) {
        uint8_t layer = last_layer;
        uint8_t idx = indicator_led_for_layer[(layer < 5) ? layer : 0];
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
    // NOTE: removed rgblight_enable_noeeprom() to avoid a hardware-level one-frame flash at startup.
    // If your controller requires enabling the driver, uncomment the next line — but it may cause a blink.
    // rgblight_enable_noeeprom();

    t_frame    = timer_read();
    wander_tmr = timer_read();

    uint8_t layer = get_highest_layer(layer_state | default_layer_state);
    last_layer  = layer;
    current_hue = hue_for_layer(layer);

    /* Start with indicator briefly */
    ind_active = true;
    ind_tmr    = timer_read();

    /* default mode = wander-only */
    rgb_mode = 0;

    /* default user-level on */
    user_rgb_on = true;

    render_frame();
}

void matrix_scan_user(void) {
    /* Animation tick */
    if (timer_elapsed(t_frame) >= FRAME_MS) {
        t_frame = timer_read();

        /* Advance wander position */
        if (timer_elapsed(wander_tmr) >= wander_step_ms) {
            wander_tmr = timer_read();
            wander_pos = (wander_pos + 1) % LED_COUNT;
        }

        render_frame();
    }

#ifdef ENCODER_BTN_PIN
    /* Encoder-Button: toggle USER RGB on/off (active low).
       We DO NOT call rgblight_toggle_noeeprom() to avoid the one-frame blink.
    */
    if (timer_elapsed(btn_tmr) >= 10) {
        bool pressed = (readPin(ENCODER_BTN_PIN) == 0);

        if (pressed && btn_released) {
            btn_tmr = timer_read();

            user_rgb_on = !user_rgb_on;

            if (!user_rgb_on) {
                /* Turn "off": clear all LEDs (per-LED) */
                clear_all_leds();
            } else {
                /* Turn "on": show indicator and render next frame (no library global write) */
                ind_active = true;
                ind_tmr    = timer_read();
                render_frame();
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

/* ---------- Encoder: send mouse wheel on turn + encoder dot overlay ---------- */
#ifdef ENCODER_ENABLE
bool encoder_update_user(uint8_t index, bool clockwise) {
    (void)index;

    /* Dot movement/visibility */
    if (clockwise) {
        enc_dot_pos = (enc_dot_pos + DOT_STEP_PER_TICK) % LED_COUNT;
    } else {
        enc_dot_pos = (enc_dot_pos + LED_COUNT - (DOT_STEP_PER_TICK % LED_COUNT)) % LED_COUNT;
    }
    last_turn = timer_read();

    /* Send mouse wheel events */
    if (clockwise) {
        tap_code(MS_WHLU); /* wheel up */
    } else {
        tap_code(MS_WHLD); /* wheel down */
    }

    render_frame();
    return false;
}
#endif

/* ---------- Settings-Layer Custom RGB Controls ---------- */
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) return true;

    switch (keycode) {
        case RGB_UI_TOG:
            /* user-level toggle (avoid library global write) */
            user_rgb_on = !user_rgb_on;
            if (!user_rgb_on) {
                clear_all_leds();
            } else {
                ind_active = true;
                ind_tmr    = timer_read();
                render_frame();
            }
            return false;

        case RGB_UI_WTOG:
            /* Cycle rgb_mode: 0 -> 1 -> 2 -> 0 ... */
            rgb_mode = (rgb_mode + 1) % 3;
            ind_active = true;
            ind_tmr = timer_read();
            render_frame();
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

        case RGB_UI_WSPD_UP:
            if (wander_step_ms > 20) wander_step_ms -= 10;   // faster
            render_frame();
            return false;

        case RGB_UI_WSPD_DN:
            if (wander_step_ms < 1000) wander_step_ms += 10; // slower
            render_frame();
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
        KC_NO,              MO(1),                  MO(4),                  KC_BSPC,
        KC_NUM,             KC_PAST,                KC_PSLS,                KC_PMNS,
        KC_P7,              KC_P8,                  KC_P9,                  KC_PPLS,
        KC_P4,              KC_P5,                  KC_P6,                  KC_NO,
        KC_P1,              KC_P2,                  KC_P3,                  KC_PENT,
        KC_NO,              KC_P0,                  KC_PDOT,                KC_NO
    ),

    /*-----------------------EDIT------------------------------*/
    [1] = LAYOUT_6x4(
        KC_NO,                  TO(0),                  MO(4),                  KC_BSPC,
        KC_NO,                  KC_NO,                  KC_NO,                  LCTL(KC_A),
        LCTL(KC_Z),             S(KC_HOME),             LCTL(KC_R),             LCTL(KC_C),
        S(KC_LEFT),             LCTL(KC_S),             S(KC_RGHT),             KC_NO,
        LCTL(LSFT(KC_LEFT)),    S(KC_END),              LCTL(LSFT(KC_RGHT)),    KC_PENT,
        KC_NO,                  KC_SPACE,               LCTL(KC_X),             KC_NO
    ),

    /*-----------------------NAVIGATION---------------------------*/
    [2] = LAYOUT_6x4(
        KC_NO,                  MO(0),                  MO(4),                  KC_NO,
        KC_NO,                  KC_NO,                  KC_NO,                  KC_NO,
        LALT(LCTL(KC_LEFT)),    KC_NO,                  LALT(LCTL(KC_RGHT)),    KC_NO,
        LCTL(LGUI(KC_LEFT)),    KC_NO,                  LCTL(LGUI(KC_RGHT)),    KC_NO,
        KC_NO,                  KC_NO,                  KC_NO,                  KC_PENT,
        KC_NO,                  KC_NO,                  LCTL(LALT(KC_DEL)),     KC_NO
    ),

    /*-----------------------MAKRO--------------------------*/
    [3] = LAYOUT_6x4(
        KC_NO,                TO(0),                       MO(4),                           KC_NO,
        KC_NO,                KC_NO,                       KC_NO,                           KC_NO,
        KC_F14,               KC_F15,                      KC_F16,                          KC_NO,
        KC_F17,               KC_F18,                      KC_F19,                          KC_NO,
        KC_F20,               KC_F21,                      KC_F22,                          KC_NO,
        KC_NO,                KC_NO,                       KC_NO,                           KC_NO
    ),

    /*-----------------------SETTINGS--------------------------*/
    [4] = LAYOUT_6x4(
        KC_NO,              TO(0),              MO(4),                          KC_NO,
        RGB_UI_WSPD_UP,     RGB_UI_WSPD_DN,     RGB_UI_HUI,                     RGB_UI_HUD,
        RGB_UI_VAI,         RGB_UI_VAD,         RGB_UI_WTOG,                    RGB_UI_TOG,
        RGB_UI_SAI,         RGB_UI_SAD,         KC_NO,                          KC_NO,
        TO(1),              TO(2),              TO(3),                          KC_NO,
        KC_NO,              KC_NO,              KC_NO,                          KC_NO
    ),
};