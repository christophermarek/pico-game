#pragma once
#include <stdint.h>

void font_draw_char(char c, int x, int y, uint16_t color, int scale);
void font_draw_str(const char *s, int x, int y, uint16_t color, int scale);
int  font_str_width(const char *s, int scale);
