/**
  ******************************************************************************
  * @file           : ssd1306.h
  * @brief          : Header for SSD1306 128x64 I2C OLED display driver
  ******************************************************************************
  */

#ifndef __SSD1306_H__
#define __SSD1306_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* SSD1306 Configuration */
#define SSD1306_I2C_ADDR            (0x3C << 1)
#define SSD1306_WIDTH               128
#define SSD1306_HEIGHT              64
#define SSD1306_BUFFER_SIZE         (SSD1306_WIDTH * SSD1306_HEIGHT / 8)

typedef enum {
    SSD1306_COLOR_BLACK = 0x00,
    SSD1306_COLOR_WHITE = 0x01
} SSD1306_COLOR;

typedef enum {
    SSD1306_OK = 0,
    SSD1306_ERR = 1
} SSD1306_Status;

typedef struct {
    const uint8_t width;
    const uint8_t height;
    const void *data;
    const uint8_t is16bit;
} SSD1306_Font_t;

extern const SSD1306_Font_t Font_6x8;
extern const SSD1306_Font_t Font_7x10;

SSD1306_Status SSD1306_Init(I2C_HandleTypeDef *hi2c);
void SSD1306_Fill(SSD1306_COLOR color);
void SSD1306_Clear(void);
SSD1306_Status SSD1306_UpdateScreen(void);
void SSD1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color);
void SSD1306_SetCursor(uint8_t x, uint8_t y);
char SSD1306_WriteChar(char ch, const SSD1306_Font_t *font, SSD1306_COLOR color);
char SSD1306_WriteString(const char *str, const SSD1306_Font_t *font, SSD1306_COLOR color);
void SSD1306_PrintCentered(uint8_t y, const char *str, const SSD1306_Font_t *font, SSD1306_COLOR color);
void SSD1306_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color);

#ifdef __cplusplus
}
#endif

#endif /* __SSD1306_H__ */
