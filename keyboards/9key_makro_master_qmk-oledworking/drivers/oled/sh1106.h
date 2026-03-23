#pragma once

#include <stdint.h>
#include <stdbool.h>

void sh1106_init(uint8_t rotation);
void sh1106_clear(void);
void sh1106_write(const char *data, bool invert);
void sh1106_set_cursor(uint8_t col, uint8_t row);
void sh1106_on(void);
void sh1106_off(void);
void sh1106_set_brightness(uint8_t val);
void sh1106_task(void);
