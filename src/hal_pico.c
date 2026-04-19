/*
 * HAL implementation — Raspberry Pi Pico + ST7789V SPI display.
 * Compiled when building the grumblequest (Pico) target.
 *
 * Pinout (configurable in config.h):
 *   SPI0 SCK  → GP18    SPI0 TX  → GP19
 *   DC        → GP20    CS       → GP17
 *   RST       → GP21    Backlight→ GP22
 *
 * Buttons (active-low, internal pull-up):
 *   UP→GP2  DOWN→GP3  LEFT→GP4  RIGHT→GP5
 *   A→GP6   B→GP7     START→GP8 SEL→GP9
 *
 * TODO: implement when hardware arrives.
 * All function signatures match hal.h exactly — no game code changes needed.
 */

#include "hal.h"
#include "config.h"

/* Pico SDK headers — only compiled when PICO_PLATFORM is defined */
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

/* ------------------------------------------------------------------ */
/* ST7789 driver — minimal inline implementation                       */
/* Full driver with DMA: see pico/st7789_dma.h (TODO)                 */
/* ------------------------------------------------------------------ */

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

static void st7789_set_window(int x, int y, int w, int h) {
    uint8_t buf[4];
    /* Column address */
    st7789_cmd(0x2A);
    buf[0] = x >> 8; buf[1] = x & 0xFF;
    buf[2] = (x+w-1) >> 8; buf[3] = (x+w-1) & 0xFF;
    st7789_data(buf, 4);
    /* Row address */
    st7789_cmd(0x2B);
    buf[0] = y >> 8; buf[1] = y & 0xFF;
    buf[2] = (y+h-1) >> 8; buf[3] = (y+h-1) & 0xFF;
    st7789_data(buf, 4);
    /* Write RAM */
    st7789_cmd(0x2C);
}

/* ------------------------------------------------------------------ */

static uint16_t fb[DISPLAY_W * DISPLAY_H];
static bool     prev_a, prev_b, prev_start, prev_sel, prev_cam_l, prev_cam_r;

/* ================================================================== */
/* Display                                                             */
/* ================================================================== */

void hal_display_init(void) {
    /* TODO: full ST7789 init sequence */
    spi_init(spi0, CONFIG_SPI_BAUD);
    gpio_set_function(CONFIG_PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(CONFIG_PIN_MOSI, GPIO_FUNC_SPI);

    gpio_init(CONFIG_PIN_DC);  gpio_set_dir(CONFIG_PIN_DC, GPIO_OUT);
    gpio_init(CONFIG_PIN_CS);  gpio_set_dir(CONFIG_PIN_CS, GPIO_OUT);
    gpio_init(CONFIG_PIN_RST); gpio_set_dir(CONFIG_PIN_RST, GPIO_OUT);
    gpio_init(CONFIG_PIN_BL);  gpio_set_dir(CONFIG_PIN_BL, GPIO_OUT);

    /* Reset pulse */
    gpio_put(CONFIG_PIN_RST, 0); sleep_ms(10);
    gpio_put(CONFIG_PIN_RST, 1); sleep_ms(120);

    /* ST7789 init commands (sleep out, colour mode, display on) */
    st7789_cmd(0x11); sleep_ms(120); /* SLPOUT */
    uint8_t colmod = 0x55; /* 16-bit RGB565 */
    st7789_cmd(0x3A); st7789_data(&colmod, 1);
    st7789_cmd(0x29); /* Display on */

    gpio_put(CONFIG_PIN_BL, 1); /* Backlight on */
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
    if (x >= 0 && x < DISPLAY_W && y >= 0 && y < DISPLAY_H)
        fb[y * DISPLAY_W + x] = color;
}

void hal_blit(int x, int y, int w, int h, const uint16_t *buf) {
    for (int row = 0; row < h; row++) {
        int dy = y + row;
        if (dy < 0 || dy >= DISPLAY_H) continue;
        for (int col = 0; col < w; col++) {
            int dx = x + col;
            if (dx < 0 || dx >= DISPLAY_W) continue;
            fb[dy * DISPLAY_W + dx] = buf[row * w + col];
        }
    }
}

void hal_show(void) {
    /* Push entire framebuffer over SPI.
     * ST7789 expects big-endian RGB565; our fb is native-endian.
     * Swap bytes for each pixel before sending.
     * TODO: use DMA for async transfer while CPU runs next frame. */
    st7789_set_window(0, 0, DISPLAY_W, DISPLAY_H);
    gpio_put(CONFIG_PIN_DC, 1);
    gpio_put(CONFIG_PIN_CS, 0);
    for (int i = 0; i < DISPLAY_W * DISPLAY_H; i++) {
        uint8_t bytes[2] = { fb[i] >> 8, fb[i] & 0xFF };
        spi_write_blocking(spi0, bytes, 2);
    }
    gpio_put(CONFIG_PIN_CS, 1);
}

bool hal_image_load_rgba(const char *path, HalImageRGBA *out) {
    (void)path;
    if (out) {
        out->w = out->h = 0;
        out->rgba = NULL;
    }
    return false;
}

void hal_image_free(HalImageRGBA *img) {
    if (!img) return;
    img->w = img->h = 0;
    img->rgba = NULL;
}

/* ================================================================== */
/* Input                                                               */
/* ================================================================== */

void hal_input_init(void) {
    const int btns[] = {
        CONFIG_PIN_UP, CONFIG_PIN_DOWN, CONFIG_PIN_LEFT, CONFIG_PIN_RIGHT,
        CONFIG_PIN_A, CONFIG_PIN_B, CONFIG_PIN_START, CONFIG_PIN_SEL
    };
    for (int i = 0; i < 8; i++) {
        gpio_init(btns[i]);
        gpio_set_dir(btns[i], GPIO_IN);
        gpio_pull_up(btns[i]);
    }
    prev_a = prev_b = prev_start = prev_sel = false;
    prev_cam_l = prev_cam_r = false;
}

void hal_input_poll(Input *inp) {
    /* Active-low: pin reads 0 when pressed */
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
    inp->cam_l_press = inp->cam_l && !prev_cam_l;
    inp->cam_r_press = inp->cam_r && !prev_cam_r;

    prev_a     = inp->a;
    prev_b     = inp->b;
    prev_start = inp->start;
    prev_sel   = inp->sel;
    prev_cam_l = inp->cam_l;
    prev_cam_r = inp->cam_r;
}

/* ================================================================== */
/* Time                                                                */
/* ================================================================== */

uint32_t hal_ticks_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

void hal_sleep_ms(uint32_t ms) {
    sleep_ms(ms);
}

/* ================================================================== */
/* Storage — flash (last sector)                                       */
/* ================================================================== */

#include "hardware/flash.h"
#include "hardware/sync.h"

#define FLASH_SAVE_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define FLASH_SAVE_ADDR   (XIP_BASE + FLASH_SAVE_OFFSET)

bool hal_save(const void *data, size_t len) {
    if (len > FLASH_SECTOR_SIZE) return false;
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_SAVE_OFFSET, FLASH_SECTOR_SIZE);
    /* Flash writes must be in 256-byte pages */
    uint8_t page[FLASH_PAGE_SIZE];
    size_t offset = 0;
    while (offset < len) {
        size_t chunk = len - offset;
        if (chunk > FLASH_PAGE_SIZE) chunk = FLASH_PAGE_SIZE;
        memcpy(page, (const uint8_t *)data + offset, chunk);
        if (chunk < FLASH_PAGE_SIZE) memset(page + chunk, 0xFF, FLASH_PAGE_SIZE - chunk);
        flash_range_program(FLASH_SAVE_OFFSET + offset, page, FLASH_PAGE_SIZE);
        offset += FLASH_PAGE_SIZE;
    }
    restore_interrupts(ints);
    return true;
}

bool hal_load(void *data, size_t len) {
    const uint8_t *src = (const uint8_t *)FLASH_SAVE_ADDR;
    /* Simple validity check: first byte != 0xFF means something was written */
    if (src[0] == 0xFF) return false;
    memcpy(data, src, len);
    return true;
}

/* ================================================================== */
/* Lifecycle                                                           */
/* ================================================================== */

void hal_init(void) {
    stdio_init_all();
    hal_display_init();
    hal_input_init();
}

void hal_deinit(void) {
    /* Nothing to clean up on Pico */
}
