#pragma once
/*
 * Hardware pin assignments and SPI baud rate for the Pico + ST7789V build.
 *
 * Wiring:
 *   SPI0 SCK  → GP18    SPI0 TX (MOSI) → GP19
 *   DC        → GP20    CS             → GP17
 *   RST       → GP21    Backlight       → GP22
 *
 * Buttons (active-low, internal pull-up):
 *   UP  → GP2   DOWN  → GP3   LEFT → GP4   RIGHT → GP5
 *   A   → GP6   B     → GP7   START→ GP8   SEL   → GP9
 *
 * Adjust the GP numbers here to match your PCB layout.
 */

/* SPI display */
#define CONFIG_SPI_BAUD  40000000   /* 40 MHz  */
#define CONFIG_PIN_SCK   18
#define CONFIG_PIN_MOSI  19
#define CONFIG_PIN_DC    20
#define CONFIG_PIN_CS    17
#define CONFIG_PIN_RST   21
#define CONFIG_PIN_BL    22

/* Buttons */
#define CONFIG_PIN_UP     2
#define CONFIG_PIN_DOWN   3
#define CONFIG_PIN_LEFT   4
#define CONFIG_PIN_RIGHT  5
#define CONFIG_PIN_A      6
#define CONFIG_PIN_B      7
#define CONFIG_PIN_START  8
#define CONFIG_PIN_SEL    9
