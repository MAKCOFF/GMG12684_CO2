#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"

#include "st7565.h"
#include "bmpfile.h"
#include "font6x8.h"
#include "font8x8.h"
#include "font8x8_bold.h"

#include "driver/uart.h"
#include "driver/gpio.h"

#define	INTERVAL 400
#define WAIT vTaskDelay(INTERVAL)

#define delay(x) vTaskDelay((x) / portTICK_PERIOD_MS)


static const char *TAG = "ST7565";

// You have to set these CONFIG value using menuconfig.
#if 0
#define CONFIG_WIDTH 128
#define CONFIG_HEIGHT 64
#define CONFIG_MOSI_GPIO 23
#define CONFIG_SCLK_GPIO 19
#define CONFIG_CS_GPIO 22
#define CONFIG_DC_GPIO 21
#define CONFIG_RESET_GPIO 18
#define CONFIG_BL_GPIO 32
#endif

//CO2 Config
static const int RX_BUF_SIZE = 1024;
uint16_t co2 = 0;
extern uint8_t cnt;
extern const uint8_t size_array;
extern uint8_t array[100];
extern bool array_is_full;
//#define TXD_PIN (GPIO_NUM_4)
//#define RXD_PIN (GPIO_NUM_5)

void init(void) {
    const uart_config_t uart_config = {
        .baud_rate = CONFIG_baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, CONFIG_TXD_PIN, CONFIG_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);	
}

int sendData(const char* data){
    const int len = 9;
    // ESP_LOGI(logName, "len %d bytes", len);
    const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
    // ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}

void sendABC_off(void){
// команда для отключения автокалибровки датчика    
	uint8_t cmd_cal_off[] = {0xFF, 0x01, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86};
    sendData((const char*) cmd_cal_off);    
}

static void tx_task(void *arg){
// Задача для запроса уровня CO2
	sendABC_off();
    static const char *TX_TASK_TAG = "TX_TASK_req_co2";
    uint8_t cmd_req_co[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
        sendData((const char*) cmd_req_co);
        delay(6000);
    }
}

uint8_t get_CRC(uint8_t *data){
	uint8_t CRC = 0;
	for(uint8_t i = 1; i<8; i++){
		CRC += data[i];				
	}
	CRC = 0xFF - CRC;
	CRC += 1;
	return CRC;
}

static void rx_task(void *arg){
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);
    while (1) {
        uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        // ESP_LOGI(RX_TASK_TAG, "%d\n", rxBytes);		
        if (get_CRC(data) == data[8]) {            
            co2 = (data[2] << 8) + data[3]; // распаковываем уровень CO2 из ответа
            ESP_LOGI(RX_TASK_TAG, "%d\n", co2); // выводим уровень CO2 в консоль
            // ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            // ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }
    }
    free(data);
}

// GMG12864 Config
static void SPIFFS_Directory(char * path) {
	DIR* dir = opendir(path);
	assert(dir != NULL);
	while (true) {
		struct dirent*pe = readdir(dir);
		if (!pe) break;
		ESP_LOGI(__FUNCTION__,"d_name=%s d_ino=%d d_type=%x", pe->d_name,pe->d_ino, pe->d_type);
	}
	closedir(dir);
}

TickType_t FillTest(TFT_t * dev, int width, int height) {
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	lcdFillScreen(dev, WHITE);
	lcdWriteBuffer(dev);
	vTaskDelay(500);
	lcdFillScreen(dev, BLACK);
	lcdWriteBuffer(dev);
	vTaskDelay(500);

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%lu",diffTick*portTICK_PERIOD_MS);
	return diffTick;
}

TickType_t BarTest(TFT_t * dev, int direction, int width, int height) {
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	if (direction == HORIZONTAL) {
		uint16_t y1 = height/5;
		lcdDrawFillRect(dev, 0, 0, width-1, y1-1, BLACK);
		vTaskDelay(1);
		lcdDrawFillRect(dev, 0, y1, width-1, y1*2-1, WHITE);
		vTaskDelay(1);
		lcdDrawFillRect(dev, 0, y1*2, width-1, y1*3-1, BLACK);
		vTaskDelay(1);
		lcdDrawFillRect(dev, 0, y1*3, width-1, y1*4-1, WHITE);
		vTaskDelay(1);
		lcdDrawFillRect(dev, 0, y1*4, width-1, height-1, BLACK);
		lcdWriteBuffer(dev);
	} else {
		uint16_t x1 = width/5;
		lcdDrawFillRect(dev, 0, 0, x1-1, height-1, BLACK);
		vTaskDelay(1);
		lcdDrawFillRect(dev, x1, 0, x1*2-1, height-1, WHITE);
		vTaskDelay(1);
		lcdDrawFillRect(dev, x1*2, 0, x1*3-1, height-1, BLACK);
		vTaskDelay(1);
		lcdDrawFillRect(dev, x1*3, 0, x1*4-1, height-1, WHITE);
		vTaskDelay(1);
		lcdDrawFillRect(dev, x1*4, 0, width-1, height-1, BLACK);
		lcdWriteBuffer(dev);
	}

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%lu",diffTick*portTICK_PERIOD_MS);
	return diffTick;
}

TickType_t ArrowTest(TFT_t * dev, uint8_t * font, int width, int height) {
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	// get font width & height
	uint8_t fontWidth;
	uint8_t fontHeight;
	fontWidth = font[0];
	fontHeight = font[1];
	ESP_LOGI(__FUNCTION__,"fontWidth=%d fontHeight=%d",fontWidth,fontHeight);
	
	uint16_t xpos;
	uint16_t ypos;
	int	stlen;
	uint8_t ascii[24];

	lcdFillScreen(dev, WHITE);

	strcpy((char *)ascii, "ST7565");
	ypos = ((height + fontHeight) / 2) - 1;
	//printf("height=%d fontHeight=%d ypos=%d\n", height,fontHeight,ypos);
	xpos = (width - (strlen((char *)ascii) * fontWidth)) / 2;
	lcdSetFontDirection(dev, DIRECTION0);
	lcdDrawString2(dev, font, xpos, ypos, ascii, BLACK);

	lcdDrawTriangle(dev, 4, 4, 10, 10, 225, BLACK);
	//lcdDrawFillArrow(dev, 10, 10, 0, 0, 5, BLACK);
	strcpy((char *)ascii, "0,0");
	lcdDrawString2(dev, font, 0, 20, ascii, BLACK);

	lcdDrawTriangle(dev, width-4, 4, 10, 10, 135, BLACK);
	//lcdDrawFillArrow(dev, width-11, 10, width-1, 0, 5, BLACK);
	sprintf((char *)ascii, "%d,0",width-1);
	stlen = strlen((char *)ascii);
	xpos = (width-1) - (fontWidth*stlen);
	lcdDrawString2(dev, font, xpos, 20, ascii, BLACK);

	lcdDrawTriangle(dev, width-4, height-5, 10, 10, 45, BLACK);
	//lcdDrawFillArrow(dev, 10, height-11, 0, height-1, 5, BLACK);
	sprintf((char *)ascii, "0,%d",height-1);
	ypos = (height-11) - (fontHeight) + 5;
	lcdDrawString2(dev, font, 0, ypos, ascii, BLACK);

	lcdDrawTriangle(dev, 4, height-5, 10, 10, 315, BLACK);
	//lcdDrawFillArrow(dev, width-11, height-11, width-1, height-1, 5, BLACK);
	sprintf((char *)ascii, "%d,%d",width-1, height-1);
	stlen = strlen((char *)ascii);
	xpos = (width-1) - (fontWidth*stlen);
	lcdDrawString2(dev, font, xpos, ypos, ascii, BLACK);
	lcdWriteBuffer(dev);

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%lu",diffTick*portTICK_PERIOD_MS);
	return diffTick;
}

TickType_t DirectionTest(TFT_t * dev, uint8_t * font, int width, int height) {
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	// get font width & height
	uint8_t fontWidth;
	uint8_t fontHeight;
	fontWidth = font[0];
	fontHeight = font[1];
	ESP_LOGI(__FUNCTION__,"fontWidth=%d fontHeight=%d",fontWidth,fontHeight);

	lcdFillScreen(dev, WHITE);
	uint8_t ascii[20];
	strcpy((char *)ascii, "ST7565");
	lcdSetFontDirection(dev, DIRECTION0);
	lcdDrawString2(dev, font, 0, fontHeight-1, ascii, BLACK);

	lcdSetFontDirection(dev, DIRECTION90);
	lcdDrawString2(dev, font, (width-fontHeight-1), 0, ascii, BLACK);

	lcdSetFontDirection(dev, DIRECTION180);
	lcdDrawString2(dev, font, (width-1), (height-fontHeight-1), ascii, BLACK);

	lcdSetFontDirection(dev, DIRECTION270);
	lcdDrawString2(dev, font, fontWidth-1, height-1, ascii, BLACK);
	lcdWriteBuffer(dev);

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%lu",diffTick*portTICK_PERIOD_MS);
	return diffTick;
}


TickType_t LineTest(TFT_t * dev, int width, int height) {
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	lcdFillScreen(dev, WHITE);
	for(int ypos=0;ypos<height;ypos=ypos+10) {
		lcdDrawLine(dev, 0, ypos, width, ypos, BLACK);
	}

	for(int xpos=0;xpos<width;xpos=xpos+10) {
		lcdDrawLine(dev, xpos, 0, xpos, height, BLACK);
	}
	lcdWriteBuffer(dev);

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%lu",diffTick*portTICK_PERIOD_MS);
	return diffTick;
}

TickType_t CircleTest(TFT_t * dev, int width, int height) {
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	lcdFillScreen(dev, WHITE);
	uint16_t xpos = width/2;
	uint16_t ypos = height/2;
	for(int i=5;i<height;i=i+5) {
		lcdDrawCircle(dev, xpos, ypos, i, BLACK);
	}
	lcdWriteBuffer(dev);

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%lu",diffTick*portTICK_PERIOD_MS);
	return diffTick;
}

TickType_t RectAngleTest(TFT_t * dev, int width, int height) {
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	lcdFillScreen(dev, WHITE);
	uint16_t xpos = width/2;
	uint16_t ypos = height/2;

	uint16_t w = height * 0.8;
	uint16_t h = w * 0.5;

#if 0
	for(int angle=0;angle<=(360*3);angle=angle+30) {
		lcdDrawRectAngle(dev, xpos, ypos, w, h, angle, BLACK);
		lcdWriteBuffer(dev);
		usleep(10000);
		lcdDrawRectAngle(dev, xpos, ypos, w, h, angle, WHITE);
		lcdWriteBuffer(dev);
	}
#endif

	lcdDrawRectAngle(dev, xpos, ypos, w, h, 0, BLACK);
	lcdWriteBuffer(dev);
	vTaskDelay(1000 / portTICK_PERIOD_MS);

	for(int angle=0;angle<=180;angle=angle+30) {
		lcdDrawRectAngle(dev, xpos, ypos, w, h, angle, BLACK);
		lcdWriteBuffer(dev);
	}

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%lu",diffTick*portTICK_PERIOD_MS);
	return diffTick;
}

TickType_t TriangleTest(TFT_t * dev, int width, int height) {
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	lcdFillScreen(dev, WHITE);
	uint16_t xpos = width/2;
	uint16_t ypos = height/2;

	uint16_t w = height / 2;
	uint16_t h = w * 1.0;

#if 0
	for(int angle=0;angle<=(360*2);angle=angle+30) {
		lcdDrawTriangle(dev, xpos, ypos, w, h, angle, BLACK);
		lcdWriteBuffer(dev);
		usleep(10000);
		lcdDrawTriangle(dev, xpos, ypos, w, h, angle, WHITE);
		lcdWriteBuffer(dev);
	}
#endif

	lcdDrawTriangle(dev, xpos, ypos, w, h, 0, BLACK);
	lcdWriteBuffer(dev);
	vTaskDelay(1000 / portTICK_PERIOD_MS);

	for(int angle=0;angle<=360;angle=angle+30) {
		lcdDrawTriangle(dev, xpos, ypos, w, h, angle, BLACK);
		lcdWriteBuffer(dev);
	}

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%lu",diffTick*portTICK_PERIOD_MS);
	return diffTick;
}

TickType_t RoundRectTest(TFT_t * dev, int width, int height) {
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	uint16_t limit = width;
	if (width > height) limit = height;
	lcdFillScreen(dev, WHITE);
	for(int i=5;i<limit;i=i+5) {
		if (i > (limit-i-1) ) break;
		ESP_LOGI(__FUNCTION__, "i=%d, width-i-1=%d",i, width-i-1);
		lcdDrawRoundRect(dev, i, i, (width-i-1), (height-i-1), 10, BLACK);
	}
	lcdWriteBuffer(dev);

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%lu",diffTick*portTICK_PERIOD_MS);
	return diffTick;
}

TickType_t BMPTest(TFT_t * dev, char * file, int width, int height) {
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	lcdSetFontDirection(dev, 0);
	lcdFillScreen(dev, BLACK);

	// open BMP file
	esp_err_t ret;
	FILE* fp = fopen(file, "rb");
	if (fp == NULL) {
		ESP_LOGW(__FUNCTION__, "File not found [%s]", file);
		return 0;
	}

	// read bmp header
	bmpfile_t *result = (bmpfile_t*)malloc(sizeof(bmpfile_t));
	ret = fread(result->header.magic, 1, 2, fp);
	assert(ret == 2);
	ESP_LOGD(__FUNCTION__,"result->header.magic=%c %c", result->header.magic[0], result->header.magic[1]);
	if (result->header.magic[0]!='B' || result->header.magic[1] != 'M') {
		ESP_LOGW(__FUNCTION__, "File is not BMP");
		free(result);
		fclose(fp);
		return 0;
	}
	ret = fread(&result->header.filesz, 4, 1 , fp);
	assert(ret == 1);
	ESP_LOGD(__FUNCTION__,"result->header.filesz=%lu", result->header.filesz);
	ret = fread(&result->header.creator1, 2, 1, fp);
	assert(ret == 1);
	ret = fread(&result->header.creator2, 2, 1, fp);
	assert(ret == 1);
	ret = fread(&result->header.offset, 4, 1, fp);
	assert(ret == 1);

	// read dib header
	ret = fread(&result->dib.header_sz, 4, 1, fp);
	assert(ret == 1);
	ret = fread(&result->dib.width, 4, 1, fp);
	assert(ret == 1);
	ret = fread(&result->dib.height, 4, 1, fp);
	assert(ret == 1);
	ret = fread(&result->dib.nplanes, 2, 1, fp);
	assert(ret == 1);
	ret = fread(&result->dib.depth, 2, 1, fp);
	assert(ret == 1);
	ret = fread(&result->dib.compress_type, 4, 1, fp);
	assert(ret == 1);
	ret = fread(&result->dib.bmp_bytesz, 4, 1, fp);
	assert(ret == 1);
	ret = fread(&result->dib.hres, 4, 1, fp);
	assert(ret == 1);
	ret = fread(&result->dib.vres, 4, 1, fp);
	assert(ret == 1);
	ret = fread(&result->dib.ncolors, 4, 1, fp);
	assert(ret == 1);
	ret = fread(&result->dib.nimpcolors, 4, 1, fp);
	assert(ret == 1);

	ESP_LOGD(__FUNCTION__, "result->dib.header_sz=%lu", result->dib.header_sz);
	ESP_LOGD(__FUNCTION__, "result->dib.depth=%hu", result->dib.depth);
	ESP_LOGD(__FUNCTION__, "result->dib.compress_type=%lu", result->dib.compress_type);
	ESP_LOGD(__FUNCTION__, "result->dib.width=%lu", result->dib.width);
	ESP_LOGD(__FUNCTION__, "result->dib.height=%lu", result->dib.height);
	ESP_LOGD(__FUNCTION__, "result->dib.bmp_bytesz=%lu", result->dib.bmp_bytesz);
	ESP_LOGD(__FUNCTION__, "result->dib.ncolors=%lu", result->dib.ncolors);
	if((result->dib.depth == 1) && (result->dib.compress_type == 0)) {
		// BMP rows are padded (if needed) to 4-byte boundary
		//uint32_t rowSize = (result->dib.width * 3 + 3) & ~3;
		//uint32_t rowSize = (result->dib.width * 1 + 3) & ~3;
		uint32_t rowSize = result->dib.bmp_bytesz/result->dib.height;
		ESP_LOGD(__FUNCTION__,"rowSize=%lu", rowSize);
		int w = result->dib.width;
		int h = result->dib.height;
		ESP_LOGD(__FUNCTION__,"w=%d h=%d", w, h);
		int _x;
		int _w;
		int _cols;
		int _cole;
		if (width >= w) {
			_x = (width - w) / 2;
			_w = w;
			_cols = 0;
			_cole = w - 1;
		} else {
			_x = 0;
			_w = width;
			_cols = (w - width) / 2;
			_cole = _cols + width - 1;
		}
		ESP_LOGD(__FUNCTION__,"_x=%d _w=%d _cols=%d _cole=%d",_x, _w, _cols, _cole);

		int _y;
		int _rows;
		int _rowe;
		if (height >= h) {
			_y = (height - h) / 2;
			_rows = 0;
			_rowe = h -1;
		} else {
			_y = 0;
			_rows = (h - height) / 2;
			_rowe = _rows + height - 1;
		}
		ESP_LOGD(__FUNCTION__,"_y=%d _rows=%d _rowe=%d", _y, _rows, _rowe);

#define BUFFPIXEL 10
		uint8_t sdbuffer[BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
		uint8_t *colors = (uint8_t*)malloc(sizeof(uint8_t) * w);

		for (int row=0; row<h; row++) { // For each scanline...
			if (row < _rows || row > _rowe) continue;
			// Seek to start of scan line.	It might seem labor-
			// intensive to be doing this on every line, but this
			// method covers a lot of gritty details like cropping
			// and scanline padding.	Also, the seek only takes
			// place if the file position actually needs to change
			// (avoids a lot of cluster math in SD library).
			// Bitmap is stored bottom-to-top order (normal BMP)
			int pos = result->header.offset + (h - 1 - row) * rowSize;
			ESP_LOGD(__FUNCTION__,"_result->header.offset=%lu pos=%d rowSize=%lu", result->header.offset, pos, rowSize);
			fseek(fp, pos, SEEK_SET);
			int buffidx = sizeof(sdbuffer); // Force buffer reload

			int index = 0;
			uint8_t mask = 0x80;
			for (int col=0; col<w; col++) { // For each pixel...
				if (buffidx >= sizeof(sdbuffer)) { // Indeed
					fread(sdbuffer, sizeof(sdbuffer), 1, fp);
					buffidx = 0; // Set index to beginning
				}
				if (col < _cols || col > _cole) continue;
				// Convert pixel from BMP to TFT format, push to display
				colors[index] = sdbuffer[buffidx] & mask;
				if (colors[index] != 0) colors[index] = BLACK; 
#if 0
				if (row == 0) {
					ESP_LOGI(__FUNCTION__,"dbuffer[%d]=%02x colors[%d]=%d", buffidx, sdbuffer[buffidx], index, colors[index]);
				}
#endif

				index++;
				if (mask == 0x01) {
					buffidx++;
					mask = 0x80;
				} else {
					mask = mask >> 1;
				}
			} // end col
			//lcdDrawMultiPixels(dev, _x, row+_y, _w, colors);
			lcdDrawMultiPixels(dev, _x, _y, _w, colors);
			//lcdWriteBuffer(dev);
			_y++;
		} // end row
		lcdWriteBuffer(dev);
		free(colors);
	} else {
		ESP_LOGW(__FUNCTION__, "Illegal BMP format");
	} // end if 
	free(result);
	fclose(fp);

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%lu",diffTick*portTICK_PERIOD_MS);
	return diffTick;
}


void ST7565(void *pvParameters)
{
	TFT_t dev;
	spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO, CONFIG_BL_GPIO);
	lcdInit(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
	uint8_t arr[100] = { 0, };

	char file[32];
	strcpy(file, "/spiffs/sample1.bmp");
	BMPTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT);
	WAIT;

#if CONFIG_FLIP
	ESP_LOGI(TAG, "Flip upside down");
	lcdFlipOn(&dev);
#endif

#if CONFIG_INVERSION
	ESP_LOGI(TAG, "Display Inversion");
	lcdInversionOn(&dev);
#endif

	//for TEST
#if 0
	while (1) {
		char file[32];
		strcpy(file, "/spiffs/sample1.bmp");
		BMPTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;
	}
#endif


	while(1) {
		/*FillTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		BarTest(&dev, VERTICAL, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		BarTest(&dev, HORIZONTAL, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		ArrowTest(&dev, font8x8, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		LineTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		CircleTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		RectAngleTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		RoundRectTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		TriangleTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		DirectionTest(&dev, font8x8, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;*/

		// char file[32];
		// strcpy(file, "/spiffs/sample1.bmp");
		// BMPTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT);
		// WAIT;

		// Multi Font Test
		uint8_t ascii[40];
		uint8_t txbuf[40];
		lcdFillScreen(&dev, WHITE);
		lcdSetFontDirection(&dev, DIRECTION0);
		// strcpy((char *)ascii, "6x8 Font");
		// int xpos = (CONFIG_WIDTH - (strlen((char *)ascii) * 6)) / 2;
		// lcdDrawString2(&dev, font6x8, xpos, 8, ascii, BLACK);

		// lcdSetFontRevert(&dev);
		// lcdDrawString2(&dev, font6x8, xpos, 16, ascii, BLACK);
		// lcdUnsetFontRevert(&dev);

		//strcpy((char *)ascii, "8x8 NormalFont");
		// sprintf((char *)ascii, "CO2: %.3d ppm", co2);
		// int xpos = (CONFIG_WIDTH - (strlen((char *)ascii) * 8)) / 2;
		// lcdDrawString2(&dev, font8x8, xpos, 26, ascii, BLACK);

		// lcdSetFontRevert(&dev);
		// lcdDrawString2(&dev, font8x8, xpos, 40, ascii, BLACK);
		// lcdUnsetFontRevert(&dev);

		//strcpy((char *)ascii, "8x8 Bold Font");

        /*- Основная вкладка концентрации -*/
		/*sprintf((char *)txbuf, "Концентрация");
		//int xpos = (CONFIG_WIDTH - (strlen((char *)txbuf) * 8)) / 2;
		GMG12864_Decode_UTF8(&dev, 2, 14, font5x7, inversion_off, (char *)txbuf);*/
		sprintf((char *)ascii, "%.3d ppm", co2);
		int xpos = CONFIG_WIDTH - (strlen((char *)ascii) * 8);
		lcdDrawString2(&dev, font8x8_bold, xpos, 8, ascii, BLACK);

        /*- График -*/
		uint16_t co2_d = co2/10;
		uint8_t value = GMG12864_Value_for_Plot(&dev, 0, 255, co2_d);
		GMG12864_Fill_the_array_Plot(&cnt, array, size_array, value);
        GMG12864_Generate_a_Graph(&dev, &cnt, array, size_array, 0, 255, 1, 1, 0, BLACK);


		// lcdSetFontRevert(&dev);
		// lcdDrawString2(&dev, font8x8_bold, xpos, 64, ascii, BLACK);
		// lcdUnsetFontRevert(&dev);

		lcdWriteBuffer(&dev);
        delay(5000);
		//WAIT;

	} // end while

	// never reach
	while (1) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}


void app_main(void)
{
	ESP_LOGI(TAG, "Initializing SPIFFS");
    init();
    xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
    xTaskCreate(tx_task, "uart_tx_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);

	esp_vfs_spiffs_conf_t conf = {
		.base_path = "/spiffs",
		.partition_label = NULL,
		.max_files = 16,
		.format_if_mount_failed =true
	};

	// Use settings defined above toinitialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is anall-in-one convenience function.
	esp_err_t ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)",esp_err_to_name(ret));
		}
		return;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(NULL, &total,&used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG,"Failed to get SPIFFS partition information (%s)",esp_err_to_name(ret));
	} else {
		ESP_LOGI(TAG,"Partition size: total: %d, used: %d", total, used);
	}

	SPIFFS_Directory("/spiffs/");
	xTaskCreate(ST7565, "ST7565", 1024*6, NULL, configMAX_PRIORITIES-2, NULL);
}
