/* HAL — Raspberry Pi Pico + ST7789V SPI display. Pin assignments in pico_config.h. */

#include "hal.h"
#include "config.h"
#include "pico_config.h"

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <string.h>

#include "pico_assets/pico_atlases.h"

static void st7789_cmd(uint8_t cmd) {
    gpio_put(CONFIG_PIN_DC, 0);
    gpio_put(CONFIG_PIN_CS, 0);
    spi_write_blocking(spi0, &cmd, 1);
    gpio_put(CONFIG_PIN_CS, 1);
}

static void st7789_data(const uint8_t *data, size_t len) {
    gpio_put(CONFIG_PIN_DC, 1);
    gpio_put(CONFIG_PIN_CS, 0);
    spi_write_blocking(spi0, data, len);
    gpio_put(CONFIG_PIN_CS, 1);
}

static inline void st7789_u8(uint8_t v) { st7789_data(&v, 1); }

static uint16_t fb[DISPLAY_W * DISPLAY_H];
static bool     prev_a, prev_b, prev_start, prev_sel;

void hal_display_init(void) {
    spi_init(spi0, CONFIG_SPI_BAUD);
    gpio_set_function(CONFIG_PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(CONFIG_PIN_MOSI, GPIO_FUNC_SPI);

    gpio_init(CONFIG_PIN_DC);  gpio_set_dir(CONFIG_PIN_DC,  GPIO_OUT);
    gpio_init(CONFIG_PIN_CS);  gpio_set_dir(CONFIG_PIN_CS,  GPIO_OUT);
    gpio_init(CONFIG_PIN_RST); gpio_set_dir(CONFIG_PIN_RST, GPIO_OUT);
    gpio_init(CONFIG_PIN_BL);  gpio_set_dir(CONFIG_PIN_BL,  GPIO_OUT);

    gpio_put(CONFIG_PIN_CS, 1);
    gpio_put(CONFIG_PIN_DC, 1);
    gpio_put(CONFIG_PIN_BL, 0);

    gpio_put(CONFIG_PIN_RST, 1); sleep_ms(10);
    gpio_put(CONFIG_PIN_RST, 0); sleep_ms(10);
    gpio_put(CONFIG_PIN_RST, 1); sleep_ms(150);

    st7789_cmd(0x01); sleep_ms(150);  /* SWRESET */
    st7789_cmd(0x11); sleep_ms(120);  /* SLPOUT  */
    st7789_cmd(0x3A); st7789_u8(0x55); /* COLMD = RGB565 */
    st7789_cmd(0x36); st7789_u8(0x00); /* MADCTL normal portrait */
    st7789_cmd(0x2A); { uint8_t d[4]={0,0,0,0xEF}; st7789_data(d,4); }
    st7789_cmd(0x2B); { uint8_t d[4]={0,0,0,0xEF}; st7789_data(d,4); }
    st7789_cmd(0x21); sleep_ms(10);   /* INVON — required on most ST7789V modules */
    st7789_cmd(0x13); sleep_ms(10);   /* NORON */
    st7789_cmd(0x29); sleep_ms(10);   /* DISPON */

    gpio_put(CONFIG_PIN_BL, 1);
}

void hal_fill(uint16_t color) {
    for (int i = 0; i < DISPLAY_W * DISPLAY_H; i++)
        fb[i] = color;
}

void hal_fill_rect(int x, int y, int w, int h, uint16_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > DISPLAY_W) w = DISPLAY_W - x;
    if (y + h > DISPLAY_H) h = DISPLAY_H - y;
    if (w <= 0 || h <= 0) return;
    for (int row = y; row < y + h; row++) {
        uint16_t *dst = fb + row * DISPLAY_W + x;
        for (int col = 0; col < w; col++) dst[col] = color;
    }
}

void hal_pixel(int x, int y, uint16_t color) {
    if ((unsigned)x < DISPLAY_W && (unsigned)y < DISPLAY_H)
        fb[y * DISPLAY_W + x] = color;
}

void hal_show(void) {
    /*
     * Byte-swap each row into a stack line buffer and blast it out. ST7789
     * wants big-endian RGB565; using one spi_write_blocking per row is ~240×
     * fewer API calls than a pixel loop.
     */
    st7789_cmd(0x2A);
    { uint8_t d[4] = {0, 0, 0, 0xEF}; st7789_data(d, 4); }
    st7789_cmd(0x2B);
    { uint8_t d[4] = {0, 0, 0, 0xEF}; st7789_data(d, 4); }
    st7789_cmd(0x2C);

    gpio_put(CONFIG_PIN_DC, 1);
    gpio_put(CONFIG_PIN_CS, 0);

    uint8_t line[DISPLAY_W * 2];
    for (int y = 0; y < DISPLAY_H; y++) {
        const uint16_t *src = fb + y * DISPLAY_W;
        for (int x = 0; x < DISPLAY_W; x++) {
            line[x * 2]     = (uint8_t)(src[x] >> 8);
            line[x * 2 + 1] = (uint8_t)(src[x] & 0xFF);
        }
        spi_write_blocking(spi0, line, DISPLAY_W * 2);
    }

    gpio_put(CONFIG_PIN_CS, 1);
}

/*
 * hal_image_load_rgba returns pointers into flash — hal_image_free is a no-op.
 */
bool hal_image_load_rgba(const char *path, HalImageRGBA *out) {
    static const struct {
        const char    *name;
        int            w, h;
        const uint8_t *data;
    } atlases[] = {
        { "assets_iso_tiles", ATLAS_ISO_TILES_W, ATLAS_ISO_TILES_H, atlas_iso_tiles_rgba },
        { "assets_chars",     ATLAS_CHARS_W,     ATLAS_CHARS_H,     atlas_chars_rgba     },
    };

    for (int i = 0; i < (int)(sizeof atlases / sizeof atlases[0]); i++) {
        if (strstr(path, atlases[i].name)) {
            out->w    = atlases[i].w;
            out->h    = atlases[i].h;
            out->rgba = (uint8_t *)atlases[i].data;
            return true;
        }
    }
    out->w = out->h = 0;
    out->rgba = NULL;
    return false;
}

void hal_image_free(HalImageRGBA *img) {
    if (!img) return;
    img->w = img->h = 0;
    img->rgba = NULL;
}

void hal_input_init(void) {
    const int btns[] = {
        CONFIG_PIN_UP, CONFIG_PIN_DOWN, CONFIG_PIN_LEFT, CONFIG_PIN_RIGHT,
        CONFIG_PIN_A,  CONFIG_PIN_B,    CONFIG_PIN_START, CONFIG_PIN_SEL
    };
    for (int i = 0; i < 8; i++) {
        gpio_init(btns[i]);
        gpio_set_dir(btns[i], GPIO_IN);
        gpio_pull_up(btns[i]);
    }
    prev_a = prev_b = prev_start = prev_sel = false;
}

void hal_input_poll(Input *inp) {
    inp->up    = !gpio_get(CONFIG_PIN_UP);
    inp->down  = !gpio_get(CONFIG_PIN_DOWN);
    inp->left  = !gpio_get(CONFIG_PIN_LEFT);
    inp->right = !gpio_get(CONFIG_PIN_RIGHT);
    inp->a     = !gpio_get(CONFIG_PIN_A);
    inp->b     = !gpio_get(CONFIG_PIN_B);
    inp->start = !gpio_get(CONFIG_PIN_START);
    inp->sel   = !gpio_get(CONFIG_PIN_SEL);
    inp->cam_l = false;
    inp->cam_r = false;

    inp->a_press     = inp->a     && !prev_a;
    inp->b_press     = inp->b     && !prev_b;
    inp->start_press = inp->start && !prev_start;
    inp->sel_press   = inp->sel   && !prev_sel;
    inp->cam_l_press = false;
    inp->cam_r_press = false;

    prev_a     = inp->a;
    prev_b     = inp->b;
    prev_start = inp->start;
    prev_sel   = inp->sel;
}

uint32_t hal_ticks_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

#define FLASH_SAVE_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define FLASH_SAVE_ADDR   (XIP_BASE + FLASH_SAVE_OFFSET)

/*
 * Flash layout: page 0 holds a uint32_t length header (rest 0xFF-padded);
 * data starts at page 1. Length mismatch on load is treated as missing
 * save, which cleanly handles both erased sectors (len reads as 0xFFFFFFFF)
 * and stale saves from a different GameState layout.
 */
bool hal_save(const void *data, size_t len) {
    if (FLASH_PAGE_SIZE + len > FLASH_SECTOR_SIZE) return false;

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_SAVE_OFFSET, FLASH_SECTOR_SIZE);

    uint8_t page[FLASH_PAGE_SIZE];
    uint32_t len32 = (uint32_t)len;
    memset(page, 0xFF, FLASH_PAGE_SIZE);
    memcpy(page, &len32, sizeof(len32));
    flash_range_program(FLASH_SAVE_OFFSET, page, FLASH_PAGE_SIZE);

    size_t offset = 0;
    while (offset < len) {
        size_t chunk = len - offset;
        if (chunk > FLASH_PAGE_SIZE) chunk = FLASH_PAGE_SIZE;
        memcpy(page, (const uint8_t *)data + offset, chunk);
        if (chunk < FLASH_PAGE_SIZE)
            memset(page + chunk, 0xFF, FLASH_PAGE_SIZE - chunk);
        flash_range_program(FLASH_SAVE_OFFSET + FLASH_PAGE_SIZE + offset,
                            page, FLASH_PAGE_SIZE);
        offset += FLASH_PAGE_SIZE;
    }

    restore_interrupts(ints);
    return true;
}

bool hal_load(void *data, size_t len) {
    const uint8_t *src = (const uint8_t *)FLASH_SAVE_ADDR;
    uint32_t saved_len;
    memcpy(&saved_len, src, sizeof(saved_len));
    if (saved_len != (uint32_t)len) return false;
    memcpy(data, src + FLASH_PAGE_SIZE, len);
    return true;
}

void hal_init(void) {
    stdio_init_all();
    hal_display_init();
    hal_input_init();
}

void hal_deinit(void) { }
