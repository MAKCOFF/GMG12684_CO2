#ifndef MAIN_ST7565_H_
#define MAIN_ST7565_H_

#include "driver/spi_master.h"

#define BLACK			0xff
#define WHITE			0x00

#define VERTICAL		0
#define HORIZONTAL		1


#define DIRECTION0		0
#define DIRECTION90		1
#define DIRECTION180	2
#define DIRECTION270	3

#define font3x5 0
#define font5x7 1
#define inversion_off 0
#define inversion_on 1

typedef struct {
	uint16_t _width;
	uint16_t _height;
	uint16_t _font_direction;
	uint16_t _font_revert;
	int16_t _dc;
	int16_t _bl;
	int16_t _flip;
	int16_t _invert;
	int16_t _blen;
	uint8_t *_buffer;
	spi_device_handle_t _SPIHandle;
} TFT_t;

void spi_master_init(TFT_t * dev, int16_t GPIO_MOSI, int16_t GPIO_SCLK, int16_t GPIO_CS, int16_t GPIO_DC, int16_t GPIO_RESET, int16_t GPIO_BL);
bool spi_master_write_byte(spi_device_handle_t SPIHandle, const uint8_t* Data, size_t DataLength);
bool spi_master_write_command(TFT_t * dev, uint8_t cmd);
bool spi_master_write_data(TFT_t * dev, uint8_t * data, int16_t len);
bool spi_master_write_data_byte(TFT_t * dev, uint8_t data);

void delayMS(int ms);
void lcdInit(TFT_t * dev, int width, int height);
void lcdWriteBuffer(TFT_t * dev);
void lcdDrawPixel(TFT_t * dev, uint16_t x, uint16_t y, uint8_t color);
void lcdDrawMultiPixels(TFT_t * dev, uint16_t x, uint16_t y, uint16_t size, uint8_t * colors);
void lcdDrawFillRect(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);
void lcdFillScreen(TFT_t * dev, uint8_t color);
void lcdDrawLine(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);
void lcdDrawRect(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);
void lcdDrawRectAngle(TFT_t * dev, uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, uint8_t color);
void lcdDrawTriangle(TFT_t * dev, uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, uint8_t color);
void lcdDrawCircle(TFT_t * dev, uint16_t x0, uint16_t y0, uint16_t r, uint8_t color);
void lcdDrawFillCircle(TFT_t * dev, uint16_t x0, uint16_t y0, uint16_t r, uint8_t color);
void lcdDrawRoundRect(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t r, uint8_t color);
void lcdDrawArrow(TFT_t * dev, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t w, uint8_t color);
void lcdDrawFillArrow(TFT_t * dev, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t w, uint8_t color);
int lcdDrawChar2(TFT_t * dev, uint8_t *font, uint16_t x, uint16_t y, uint8_t ascii, uint8_t color);
int lcdDrawString2(TFT_t * dev, uint8_t * font, uint16_t x, uint16_t y, uint8_t * ascii, uint8_t color);
void lcdSetFontDirection(TFT_t * dev, uint16_t);
void lcdBacklightOff(TFT_t * dev);
void lcdBacklightOn(TFT_t * dev);
void lcdFlipOn(TFT_t * dev);
void lcdInversionOn(TFT_t * dev);
void lcdSetFontRevert(TFT_t * dev);
void lcdUnsetFontRevert(TFT_t * dev);
void GMG12864_Print_symbol_5x7(TFT_t * dev, uint8_t x, uint8_t y, uint16_t symbol, uint8_t inversion);
void GMG12864_Print_symbol_3x5(TFT_t * dev, uint8_t x, uint8_t y, uint16_t symbol, uint8_t inversion);
void GMG12864_Decode_UTF8(TFT_t * dev, uint8_t x, uint8_t y, uint8_t font, bool inversion, char *tx_buffer);
uint8_t GMG12864_Value_for_Plot(TFT_t * dev, int y_min, int y_max, float value);
void GMG12864_Fill_the_array_Plot(uint8_t *counter, uint8_t *array, uint8_t size_array, uint8_t value);
void GMG12864_Draw_line(TFT_t * dev, uint8_t x_start, uint8_t y_start, uint8_t x_end, uint8_t y_end, uint8_t color);
void GMG12864_Generate_a_Graph(TFT_t * dev, uint8_t *counter, uint8_t *array, uint8_t size_array, int y_min, int y_max, uint8_t x_grid_time, uint8_t time_interval,
bool grid, uint8_t color);


#endif /* MAIN_ST7565_H_ */

