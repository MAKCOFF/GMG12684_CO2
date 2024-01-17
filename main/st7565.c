#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "esp_log.h"

#include "st7565.h"

#define TAG "ST7565"
#define	_DEBUG_ 0

#define _BV(bit) (1 << (bit))

#ifdef CONFIG_IDF_TARGET_ESP32
#define LCD_HOST    HSPI_HOST
#else
#define LCD_HOST    SPI2_HOST
#endif

static const int SPI_Command_Mode = 0;
static const int SPI_Data_Mode = 1;
//bool array_is_full = 0;
//static const int SPI_Frequency = SPI_MASTER_FREQ_8M;
static const int SPI_Frequency = SPI_MASTER_FREQ_20M;
//static const int SPI_Frequency = SPI_MASTER_FREQ_26M;
//static const int SPI_Frequency = SPI_MASTER_FREQ_40M;

//Для работы отрисовки графика:
uint8_t cnt = 0; //счетчик накопления значений в окне графика
const uint8_t size_array = 100; //размер массива. В нашем случае ширина 100 точек(График 100*50 пикселей)
uint8_t array[100] = { 0, }; //значения на графике. Заполняются в определенный момент времени(каждый шаг сдвига графика влево)
bool array_is_full = 0; //значения заполнили массив, можно сдвигать график влево

/*-----------------------------------Шрифт 3*5----------------------------------*/
const uint8_t Font_3x5[] = { 0x00, 0x00, 0x00, //0/ --> space     20
		0x17, 0x00, 0x00, //1/ --> !         21
		0x03, 0x00, 0x03, //2/ --> "         22 и т.д.
		0xFF, 0x0A, 0xFF, //3/ --> #
		0x17, 0xFF, 0x1D, //4/ --> $
		0x09, 0x04, 0x12, //5/ --> %
		0x0E, 0x1B, 0x0A, //6/ --> &
		0x03, 0x00, 0x00, //7/ --> '
		0x0E, 0x11, 0x00, //8/ --> (
		0x11, 0x0E, 0x00, //9/ --> )
		0x04, 0x0A, 0x04, //10/ --> *
		0x04, 0x0E, 0x04, //11/ --> +
		0x10, 0x18, 0x00, //12/ --> ,
		0x04, 0x04, 0x04, //13/ --> -
		0x10, 0x00, 0x00, //14/ --> .
		0x08, 0x04, 0x02, //15/ --> /
		0xFF, 0x11, 0xFF, //16/ --> 0
		0x11, 0xFF, 0x10, //17/ --> 1
		0x1D, 0x15, 0x17, //18/ --> 2
		0x15, 0x15, 0xFF, //19/ --> 3
		0x07, 0x04, 0xFF, //20/ --> 4
		0x17, 0x15, 0x1D, //21/ --> 5
		0xFF, 0x15, 0x1D, //22/ --> 6
		0x01, 0x01, 0xFF, //23/ --> 7
		0xFF, 0x15, 0xFF, //24/ --> 8
		0x17, 0x15, 0x1F, //25/ --> 9
		0x0A, 0x00, 0x00, //26/ --> :
		0x10, 0x0A, 0x00, //27/ --> ;
		0x04, 0x0A, 0x00, //28/ --> <
		0x0A, 0x0A, 0x0A, //29/ --> =
		0x0A, 0x04, 0x00, //30/ --> >
		0x01, 0x15, 0x07, //31/ --> ?
		0x1F, 0x11, 0x17, //32/ --> @
		0x1F, 0x05, 0x1F, //33/ --> A
		0x1F, 0x15, 0x1B, //34/ --> B
		0x1F, 0x11, 0x11, //35/ --> C
		0x1F, 0x11, 0x0E, //36/ --> D
		0x1F, 0x15, 0x15, //37/ --> E
		0x1F, 0x05, 0x05, //38/ --> F
		0x1F, 0x11, 0x1D, //39/ --> G
		0x1F, 0x04, 0x1F, //40/ --> H
		0x11, 0x1F, 0x11, //41/ --> I
		0x18, 0x10, 0x1F, //42/ --> J
		0x1F, 0x04, 0x1B, //43/ --> K
		0x1F, 0x10, 0x10, //44/ --> L
		0x1F, 0x02, 0x1F, //45/ --> M
		0x1F, 0x01, 0x1F, //46/ --> N
		0x1F, 0x11, 0x1F, //47/ --> O
		0x1F, 0x05, 0x07, //48/ --> P
		0x0F, 0x19, 0x0F, //49/ --> Q
		0x1F, 0x05, 0x1B, //50/ --> R
		0x17, 0x15, 0x1D, //51/ --> S
		0x01, 0x1F, 0x01, //52/ --> T
		0x1F, 0x10, 0x1F, //53/ --> U
		0x0F, 0x10, 0x0F, //54/ --> V
		0x1F, 0x08, 0x1F, //55/ --> W
		0x1B, 0x04, 0x1B, //56/ --> X
		0x03, 0x1C, 0x03, //57/ --> Y
		0x19, 0x15, 0x13, //58/ --> Z
		0x1F, 0x11, 0x00, //59/ --> [
		0x02, 0x04, 0x08, //60/ --> '\'
		0x11, 0x1F, 0x00, //61/ --> ]
		0x02, 0x01, 0x02, //62/ --> ^
		0x10, 0x10, 0x10, //63/ --> _
		0x01, 0x02, 0x00, //64/ --> `
		0x0C, 0x12, 0x1E, //65/ --> a
		0x1E, 0x14, 0x08, //66/ --> b
		0x1C, 0x14, 0x14, //67/ --> c
		0x0C, 0x12, 0x1F, //68/ --> d
		0x0C, 0x1A, 0x14, //69/ --> e
		0x04, 0x1F, 0x05, //70/ --> f
		0x17, 0x15, 0x0F, //71/ --> g
		0x1F, 0x02, 0x1C, //72/ --> h
		0x00, 0x1D, 0x00, //73/ --> i
		0x10, 0x0D, 0x00, //74/ --> j
		0x1F, 0x04, 0x1A, //75/ --> k
		0x01, 0x1F, 0x00, //76/ --> l
		0x1E, 0x04, 0x1E, //77/ --> m
		0x1E, 0x02, 0x1E, //78/ --> n
		0x1E, 0x12, 0x1E, //79/ --> o
		0x1E, 0x0A, 0x04, //80/ --> p
		0x04, 0x0A, 0x1E, //81/ --> q
		0x1E, 0x02, 0x06, //82/ --> r
		0x14, 0x12, 0x0A, //83/ --> s
		0x02, 0x1F, 0x12, //84/ --> t
		0x1E, 0x10, 0x1E, //85/ --> u
		0x0E, 0x10, 0x0E, //86/ --> v
		0x1E, 0x08, 0x1E, //87/ --> w
		0x1A, 0x04, 0x1A, //88/ --> x
		0x13, 0x14, 0x0F, //89/ --> y
		0x1A, 0x12, 0x16, //90/ --> z
		0x04, 0x1F, 0x11, //91/ --> {
		0x00, 0x1F, 0x00, //92/ --> |
		0x11, 0x1F, 0x04, //93/ --> }
		0x0C, 0x04, 0x06, //94/ --> ~
		0x1F, 0x05, 0x1F, //95/ --> А
		0x1F, 0x15, 0x1D, //96/ --> Б
		0x1F, 0x15, 0x1B, //97/ --> В
		0x1F, 0x01, 0x01, //98/ --> Г
		0x1E, 0x11, 0x1E, //99/ --> Д
		0x1F, 0x15, 0x15, //100/ --> Е
		0x1B, 0x1F, 0x1B, //101/ --> Ж
		0x15, 0x15, 0x1B, //102/ --> З
		0x1F, 0x10, 0x1F, //103/ --> И
		0x1D, 0x11, 0x1D, //104/ --> Й
		0x1F, 0x04, 0x1B, //105/ --> К
		0x1E, 0x01, 0x1F, //106/ --> Л
		0x1F, 0x02, 0x1F, //107/ --> М
		0x1F, 0x04, 0x1F, //108/ --> Н
		0x1F, 0x11, 0x1F, //109/ --> О
		0x1F, 0x01, 0x1F, //110/ --> П
		0x1F, 0x05, 0x07, //111/ --> Р
		0x1F, 0x11, 0x11, //112/ --> С
		0x01, 0x1F, 0x01, //113/ --> Т
		0x13, 0x14, 0x1F, //114/ --> У
		0x0E, 0x1B, 0x0E, //115/ --> Ф
		0x1B, 0x04, 0x1B, //116/ --> Х
		0x0F, 0x08, 0x1F, //117/ --> Ц
		0x07, 0x04, 0x1F, //118/ --> Ч
		0x1F, 0x18, 0x1F, //119/ --> Ш
		0x0F, 0x0C, 0x1F, //120/ --> Щ
		0x01, 0x1F, 0x1C, //121/ --> Ъ
		0x1F, 0x14, 0x1F, //122/ --> Ы
		0x15, 0x15, 0x0E, //123/ --> Э
		0x1F, 0x14, 0x08, //124/ --> Ь
		0x1F, 0x0E, 0x1F, //125/ --> Ю
		0x1B, 0x05, 0x1F, //126/ --> Я
		0x0C, 0x12, 0x1E, //127/ --> a
		0x1E, 0x15, 0x1D, //128/ --> б
		0x1E, 0x16, 0x0C, //129/ --> в
		0x1E, 0x02, 0x02, //130/ --> г
		0x1C, 0x12, 0x1C, //131/ --> д
		0x0C, 0x1A, 0x14, //132/ --> е
		0x1A, 0x1E, 0x1A, //133/ --> ж
		0x12, 0x16, 0x1E, //134/ --> з
		0x1E, 0x10, 0x1E, //135/ --> и
		0x1A, 0x12, 0x1A, //136/ --> й
		0x1E, 0x04, 0x1A, //137/ --> к
		0x1C, 0x02, 0x1E, //138/ --> л
		0x1E, 0x04, 0x1E, //139/ --> м
		0x1E, 0x08, 0x1E, //140/ --> н
		0x1E, 0x12, 0x1E, //141/ --> о
		0x1E, 0x02, 0x1E, //142/ --> п
		0x1E, 0x0A, 0x04, //143/ --> р
		0x1E, 0x12, 0x12, //144/ --> с
		0x02, 0x1E, 0x02, //145/ --> т
		0x16, 0x14, 0x0E, //146/ --> у
		0x0C, 0x1E, 0x0C, //147/ --> ф
		0x1A, 0x0C, 0x1A, //148/ --> х
		0x0E, 0x08, 0x1E, //149/ --> ц
		0x06, 0x04, 0x1E, //150/ --> ч
		0x1E, 0x18, 0x1E, //151/ --> ш
		0x0E, 0x0C, 0x1E, //152/ --> щ
		0x02, 0x1E, 0x18, //153/ --> ъ
		0x1E, 0x14, 0x1E, //154/ --> ы
		0x1E, 0x18, 0x00, //155/ --> ь
		0x12, 0x16, 0x0C, //156/ --> э
		0x1E, 0x0C, 0x1E, //157/ --> ю
		0x14, 0x0A, 0x1E, //158/ --> я
		0x1F, 0x15, 0x15, //159/ --> Ё
		0x0C, 0x1A, 0x14, //160/ --> ё
		0x03, 0x03, 0x00, //161/ --> °
		};
/*-----------------------------------Шрифт 3*5----------------------------------*/

/*-----------------------------------Шрифт 5*7----------------------------------*/
const uint8_t Font_5x7[] = { 0x00, 0x00, 0x00, 0x00, 0x00, //0/ --> space     20
		0x00, 0x4F, 0x00, 0x00, 0x00, //1/ --> !         21
		0x07, 0x00, 0x07, 0x00, 0x00, //2/ --> "         22 и т.д.
		0x14, 0x7F, 0x14, 0x7F, 0x14, //3/ --> #
		0x24, 0x2A, 0x7F, 0x2A, 0x12, //4/ --> $
		0x23, 0x13, 0x08, 0x64, 0x62, //5/ --> %
		0x36, 0x49, 0x55, 0x22, 0x40, //6/ --> &
		0x00, 0x05, 0x03, 0x00, 0x00, //7/ --> '
		0x1C, 0x22, 0x41, 0x00, 0x00, //8/ --> (
		0x41, 0x22, 0x1C, 0x00, 0x00, //9/ --> )
		0x14, 0x08, 0x3E, 0x08, 0x14, //10/ --> *
		0x08, 0x08, 0x3E, 0x08, 0x08, //11/ --> +
		0xA0, 0x60, 0x00, 0x00, 0x00, //12/ --> ,
		0x08, 0x08, 0x08, 0x08, 0x08, //13/ --> -
		0x60, 0x60, 0x00, 0x00, 0x00, //14/ --> .
		0x20, 0x10, 0x08, 0x04, 0x02, //15/ --> /
		0x3E, 0x51, 0x49, 0x45, 0x3E, //16/ --> 0
		0x00, 0x42, 0x7F, 0x40, 0x00, //17/ --> 1
		0x42, 0x61, 0x51, 0x49, 0x46, //18/ --> 2
		0x21, 0x41, 0x45, 0x4B, 0x31, //19/ --> 3
		0x18, 0x14, 0x12, 0x7F, 0x10, //20/ --> 4
		0x27, 0x45, 0x45, 0x45, 0x39, //21/ --> 5
		0x3C, 0x4A, 0x49, 0x49, 0x30, //22/ --> 6
		0x01, 0x71, 0x09, 0x05, 0x03, //23/ --> 7
		0x36, 0x49, 0x49, 0x49, 0x36, //24/ --> 8
		0x06, 0x49, 0x49, 0x29, 0x1E, //25/ --> 9
		0x6C, 0x6C, 0x00, 0x00, 0x00, //26/ --> :
		0x00, 0x56, 0x36, 0x00, 0x00, //27/ --> ;
		0x08, 0x14, 0x22, 0x41, 0x00, //28/ --> <
		0x24, 0x24, 0x24, 0x24, 0x24, //29/ --> =
		0x00, 0x41, 0x22, 0x14, 0x08, //30/ --> >
		0x02, 0x01, 0x51, 0x09, 0x06, //31/ --> ?
		0x32, 0x49, 0x79, 0x41, 0x3E, //32/ --> @
		0x7E, 0x11, 0x11, 0x11, 0x7E, //33/ --> A
		0x7F, 0x49, 0x49, 0x49, 0x36, //34/ --> B
		0x3E, 0x41, 0x41, 0x41, 0x22, //35/ --> C
		0x7F, 0x41, 0x41, 0x22, 0x1C, //36/ --> D
		0x7F, 0x49, 0x49, 0x49, 0x41, //37/ --> E
		0x7F, 0x09, 0x09, 0x09, 0x01, //38/ --> F
		0x3E, 0x41, 0x49, 0x49, 0x3A, //39/ --> G
		0x7F, 0x08, 0x08, 0x08, 0x7F, //40/ --> H
		0x00, 0x41, 0x7F, 0x41, 0x00, //41/ --> I
		0x20, 0x40, 0x41, 0x3F, 0x01, //42/ --> J
		0x7F, 0x08, 0x14, 0x22, 0x41, //43/ --> K
		0x7F, 0x40, 0x40, 0x40, 0x40, //44/ --> L
		0x7F, 0x02, 0x0C, 0x02, 0x7F, //45/ --> M
		0x7F, 0x04, 0x08, 0x10, 0x7F, //46/ --> N
		0x3E, 0x41, 0x41, 0x41, 0x3E, //47/ --> O
		0x7F, 0x09, 0x09, 0x09, 0x06, //48/ --> P
		0x3E, 0x41, 0x51, 0x21, 0x5E, //49/ --> Q
		0x7F, 0x09, 0x19, 0x29, 0x46, //50/ --> R
		0x46, 0x49, 0x49, 0x49, 0x31, //51/ --> S
		0x01, 0x01, 0x7F, 0x01, 0x01, //52/ --> T
		0x3F, 0x40, 0x40, 0x40, 0x3F, //53/ --> U
		0x1F, 0x20, 0x40, 0x20, 0x1F, //54/ --> V
		0x3F, 0x40, 0x60, 0x40, 0x3F, //55/ --> W
		0x63, 0x14, 0x08, 0x14, 0x63, //56/ --> X
		0x07, 0x08, 0x70, 0x08, 0x07, //57/ --> Y
		0x61, 0x51, 0x49, 0x45, 0x43, //58/ --> Z
		0x7F, 0x41, 0x41, 0x00, 0x00, //59/ --> [
		0x15, 0x16, 0x7C, 0x16, 0x15, //60/ --> '\'
		0x41, 0x41, 0x7F, 0x00, 0x00, //61/ --> ]
		0x04, 0x02, 0x01, 0x02, 0x04, //62/ --> ^
		0x40, 0x40, 0x40, 0x40, 0x40, //63/ --> _
		0x01, 0x02, 0x04, 0x00, 0x00, //64/ --> `
		0x20, 0x54, 0x54, 0x54, 0x78, //65/ --> a
		0x7F, 0x44, 0x44, 0x44, 0x38, //66/ --> b
		0x38, 0x44, 0x44, 0x44, 0x00, //67/ --> c
		0x38, 0x44, 0x44, 0x48, 0x7F, //68/ --> d
		0x38, 0x54, 0x54, 0x54, 0x18, //69/ --> e
		0x10, 0x7E, 0x11, 0x01, 0x02, //70/ --> f
		0x0C, 0x52, 0x52, 0x52, 0x3E, //71/ --> g
		0x7F, 0x08, 0x04, 0x04, 0x78, //72/ --> h
		0x00, 0x44, 0x7D, 0x40, 0x00, //73/ --> i
		0x20, 0x40, 0x40, 0x3D, 0x00, //74/ --> j
		0x7F, 0x10, 0x28, 0x44, 0x00, //75/ --> k
		0x00, 0x41, 0x7F, 0x40, 0x00, //76/ --> l
		0x7C, 0x04, 0x18, 0x04, 0x78, //77/ --> m
		0x7C, 0x08, 0x04, 0x04, 0x78, //78/ --> n
		0x38, 0x44, 0x44, 0x44, 0x38, //79/ --> o
		0x7C, 0x14, 0x14, 0x14, 0x08, //80/ --> p
		0x08, 0x14, 0x14, 0x18, 0x7C, //81/ --> q
		0x7C, 0x08, 0x04, 0x04, 0x08, //82/ --> r
		0x48, 0x54, 0x54, 0x54, 0x20, //83/ --> s
		0x04, 0x3F, 0x44, 0x40, 0x20, //84/ --> t
		0x3C, 0x40, 0x40, 0x20, 0x7C, //85/ --> u
		0x1C, 0x20, 0x40, 0x20, 0x1C, //86/ --> v
		0x1E, 0x20, 0x10, 0x20, 0x1E, //87/ --> w
		0x22, 0x14, 0x08, 0x14, 0x22, //88/ --> x
		0x06, 0x48, 0x48, 0x48, 0x3E, //89/ --> y
		0x44, 0x64, 0x54, 0x4C, 0x44, //90/ --> z
		0x08, 0x36, 0x41, 0x00, 0x00, //91/ --> {
		0x00, 0x00, 0x7F, 0x00, 0x00, //92/ --> |
		0x41, 0x36, 0x08, 0x00, 0x00, //93/ --> }
		0x08, 0x08, 0x2A, 0x1C, 0x08, //94/ --> ~
		0x7E, 0x11, 0x11, 0x11, 0x7E, //95/ --> А
		0x7F, 0x49, 0x49, 0x49, 0x33, //96/ --> Б
		0x7F, 0x49, 0x49, 0x49, 0x36, //97/ --> В
		0x7F, 0x01, 0x01, 0x01, 0x03, //98/ --> Г
		0xE0, 0x51, 0x4F, 0x41, 0xFF, //99/ --> Д
		0x7F, 0x49, 0x49, 0x49, 0x41, //100/ --> Е
		0x77, 0x08, 0x7F, 0x08, 0x77, //101/ --> Ж
		0x41, 0x49, 0x49, 0x49, 0x36, //102/ --> З
		0x7F, 0x10, 0x08, 0x04, 0x7F, //103/ --> И
		0x7C, 0x21, 0x12, 0x09, 0x7C, //104/ --> Й
		0x7F, 0x08, 0x14, 0x22, 0x41, //105/ --> К
		0x20, 0x41, 0x3F, 0x01, 0x7F, //106/ --> Л
		0x7F, 0x02, 0x0C, 0x02, 0x7F, //107/ --> М
		0x7F, 0x08, 0x08, 0x08, 0x7F, //108/ --> Н
		0x3E, 0x41, 0x41, 0x41, 0x3E, //109/ --> О
		0x7F, 0x01, 0x01, 0x01, 0x7F, //110/ --> П
		0x7F, 0x09, 0x09, 0x09, 0x06, //111/ --> Р
		0x3E, 0x41, 0x41, 0x41, 0x22, //112/ --> С
		0x01, 0x01, 0x7F, 0x01, 0x01, //113/ --> Т
		0x47, 0x28, 0x10, 0x08, 0x07, //114/ --> У
		0x1C, 0x22, 0x7F, 0x22, 0x1C, //115/ --> Ф
		0x63, 0x14, 0x08, 0x14, 0x63, //116/ --> Х
		0x7F, 0x40, 0x40, 0x40, 0xFF, //117/ --> Ц
		0x07, 0x08, 0x08, 0x08, 0x7F, //118/ --> Ч
		0x7F, 0x40, 0x7F, 0x40, 0x7F, //119/ --> Ш
		0x7F, 0x40, 0x7F, 0x40, 0xFF, //120/ --> Щ
		0x01, 0x7F, 0x48, 0x48, 0x30, //121/ --> Ъ
		0x7F, 0x48, 0x30, 0x00, 0x7F, //122/ --> Ы
		0x00, 0x7F, 0x48, 0x48, 0x30, //123/ --> Э
		0x22, 0x41, 0x49, 0x49, 0x3E, //124/ --> Ь
		0x7F, 0x08, 0x3E, 0x41, 0x3E, //125/ --> Ю
		0x46, 0x29, 0x19, 0x09, 0x7f, //126/ --> Я
		0x20, 0x54, 0x54, 0x54, 0x78, //127/ --> a
		0x3c, 0x4a, 0x4a, 0x49, 0x31, //128/ --> б
		0x7c, 0x54, 0x54, 0x28, 0x00, //129/ --> в
		0x7c, 0x04, 0x04, 0x04, 0x0c, //130/ --> г
		0xe0, 0x54, 0x4c, 0x44, 0xfc, //131/ --> д
		0x38, 0x54, 0x54, 0x54, 0x18, //132/ --> е
		0x6c, 0x10, 0x7c, 0x10, 0x6c, //133/ --> ж
		0x44, 0x44, 0x54, 0x54, 0x28, //134/ --> з
		0x7c, 0x20, 0x10, 0x08, 0x7c, //135/ --> и
		0x7c, 0x41, 0x22, 0x11, 0x7c, //136/ --> й
		0x7c, 0x10, 0x28, 0x44, 0x00, //137/ --> к
		0x20, 0x44, 0x3c, 0x04, 0x7c, //138/ --> л
		0x7c, 0x08, 0x10, 0x08, 0x7c, //139/ --> м
		0x7c, 0x10, 0x10, 0x10, 0x7c, //140/ --> н
		0x38, 0x44, 0x44, 0x44, 0x38, //141/ --> о
		0x7c, 0x04, 0x04, 0x04, 0x7c, //142/ --> п
		0x7C, 0x14, 0x14, 0x14, 0x08, //143/ --> р
		0x38, 0x44, 0x44, 0x44, 0x28, //144/ --> с
		0x04, 0x04, 0x7c, 0x04, 0x04, //145/ --> т
		0x0C, 0x50, 0x50, 0x50, 0x3C, //146/ --> у
		0x30, 0x48, 0xfc, 0x48, 0x30, //147/ --> ф
		0x44, 0x28, 0x10, 0x28, 0x44, //148/ --> х
		0x7c, 0x40, 0x40, 0x40, 0xfc, //149/ --> ц
		0x0c, 0x10, 0x10, 0x10, 0x7c, //150/ --> ч
		0x7c, 0x40, 0x7c, 0x40, 0x7c, //151/ --> ш
		0x7c, 0x40, 0x7c, 0x40, 0xfc, //152/ --> щ
		0x04, 0x7c, 0x50, 0x50, 0x20, //153/ --> ъ
		0x7c, 0x50, 0x50, 0x20, 0x7c, //154/ --> ы
		0x7c, 0x50, 0x50, 0x20, 0x00, //155/ --> э
		0x28, 0x44, 0x54, 0x54, 0x38, //156/ --> ь
		0x7c, 0x10, 0x38, 0x44, 0x38, //157/ --> ю
		0x08, 0x54, 0x34, 0x14, 0x7c, //158/ --> я
		0x7E, 0x4B, 0x4A, 0x4B, 0x42, //159/ --> Ё
		0x38, 0x55, 0x54, 0x55, 0x18, //160/ --> ё
		0x00, 0x06, 0x09, 0x09, 0x06, //161/ --> °
		};

/*-----------------------------------Шрифт 5*7----------------------------------*/

void spi_master_init(TFT_t * dev, int16_t GPIO_MOSI, int16_t GPIO_SCLK, int16_t GPIO_CS, int16_t GPIO_DC, int16_t GPIO_RESET, int16_t GPIO_BL)
{
	esp_err_t ret;

	ESP_LOGI(TAG, "GPIO_CS=%d",GPIO_CS);
	if ( GPIO_CS >= 0 ) {
		//gpio_pad_select_gpio( GPIO_CS );
		gpio_reset_pin( GPIO_CS );
		gpio_set_direction( GPIO_CS, GPIO_MODE_OUTPUT );
		gpio_set_level( GPIO_CS, 0 );
	}

	ESP_LOGI(TAG, "GPIO_DC=%d",GPIO_DC);
	//gpio_pad_select_gpio( GPIO_DC );
	gpio_reset_pin( GPIO_DC );
	gpio_set_direction( GPIO_DC, GPIO_MODE_OUTPUT );
	gpio_set_level( GPIO_DC, 0 );

	ESP_LOGI(TAG, "GPIO_RESET=%d",GPIO_RESET);
	if ( GPIO_RESET >= 0 ) {
		//gpio_pad_select_gpio( GPIO_RESET );
		gpio_reset_pin( GPIO_RESET );
		gpio_set_direction( GPIO_RESET, GPIO_MODE_OUTPUT );
		gpio_set_level( GPIO_RESET, 1 );
		delayMS(50);
		gpio_set_level( GPIO_RESET, 0 );
		delayMS(50);
		gpio_set_level( GPIO_RESET, 1 );
		delayMS(50);
	}

	ESP_LOGI(TAG, "GPIO_BL=%d",GPIO_BL);
	if ( GPIO_BL >= 0 ) {
		gpio_reset_pin( GPIO_BL );
		gpio_set_direction( GPIO_BL, GPIO_MODE_OUTPUT );
		gpio_set_level( GPIO_BL, 0 );
	}

	ESP_LOGI(TAG, "GPIO_MOSI=%d",GPIO_MOSI);
	ESP_LOGI(TAG, "GPIO_SCLK=%d",GPIO_SCLK);
	spi_bus_config_t buscfg = {
		.sclk_io_num = GPIO_SCLK,
		.mosi_io_num = GPIO_MOSI,
		.miso_io_num = -1,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1
	};

	//ret = spi_bus_initialize( HSPI_HOST, &buscfg, 1 );
	ret = spi_bus_initialize( LCD_HOST, &buscfg, SPI_DMA_CH_AUTO );
	ESP_LOGD(TAG, "spi_bus_initialize=%d",ret);
	assert(ret==ESP_OK);

	spi_device_interface_config_t devcfg={
		.clock_speed_hz = SPI_Frequency,
		.queue_size = 7,
		.mode = 0,
		.flags = SPI_DEVICE_NO_DUMMY,
	};

	if ( GPIO_CS >= 0 ) {
		devcfg.spics_io_num = GPIO_CS;
	} else {
		devcfg.spics_io_num = -1;
	}
	
	spi_device_handle_t handle;
	ret = spi_bus_add_device( LCD_HOST, &devcfg, &handle);
	ESP_LOGD(TAG, "spi_bus_add_device=%d",ret);
	assert(ret==ESP_OK);
	dev->_dc = GPIO_DC;
	dev->_bl = GPIO_BL;
	//dev->_blen = 1024;
	dev->_SPIHandle = handle;
}


bool spi_master_write_byte(spi_device_handle_t SPIHandle, const uint8_t* Data, size_t DataLength)
{
	spi_transaction_t SPITransaction;
	esp_err_t ret;

	if ( DataLength > 0 ) {
		memset( &SPITransaction, 0, sizeof( spi_transaction_t ) );
		SPITransaction.length = DataLength * 8;
		SPITransaction.tx_buffer = Data;
#if 1
		ret = spi_device_transmit( SPIHandle, &SPITransaction );
#endif
#if 0
		ret = spi_device_polling_transmit( SPIHandle, &SPITransaction );
#endif
		assert(ret==ESP_OK); 
	}

	return true;
}

bool spi_master_write_command(TFT_t * dev, uint8_t cmd)
{
	static uint8_t Byte = 0;
	Byte = cmd;
	gpio_set_level( dev->_dc, SPI_Command_Mode );
	return spi_master_write_byte( dev->_SPIHandle, &Byte, 1 );
}

bool spi_master_write_data(TFT_t * dev, uint8_t * data, int16_t len)
{
	gpio_set_level( dev->_dc, SPI_Data_Mode );
	return spi_master_write_byte( dev->_SPIHandle, data, len );
}

bool spi_master_write_data_byte(TFT_t * dev, uint8_t data)
{
	static uint8_t Byte = 0;
	Byte = data;
	gpio_set_level( dev->_dc, SPI_Data_Mode );
	return spi_master_write_byte( dev->_SPIHandle, &Byte, 1 );
}


void delayMS(int ms) {
	int _ms = ms + (portTICK_PERIOD_MS - 1);
	TickType_t xTicksToDelay = _ms / portTICK_PERIOD_MS;
	//ESP_LOGD(TAG, "ms=%d _ms=%d portTICK_PERIOD_MS=%d xTicksToDelay=%d",ms,_ms,portTICK_PERIOD_MS,xTicksToDelay);
	vTaskDelay(xTicksToDelay);
}


void lcdInit(TFT_t * dev, int width, int height)
{
	dev->_width = width;
	dev->_height = height;
	dev->_font_direction = DIRECTION0;
	dev->_font_revert = 0;
	dev->_flip = 0;
	dev->_invert = 0;

	dev->_blen = width * height / 8;
	ESP_LOGD(TAG, "dev->_blen=%d", dev->_blen);
	uint8_t *buffer = (uint8_t*)malloc(dev->_blen);
	dev->_buffer = buffer;

	/*spi_master_write_command(dev, 0xe2); // system reset
	spi_master_write_command(dev, 0x40); // set LCD start line to 0
	spi_master_write_command(dev, 0xa0); // set SEG direction (A1 to flip horizontal)
	spi_master_write_command(dev, 0xc8); // set COM direction (C0 to flip vert)
	spi_master_write_command(dev, 0xa2); // set LCD bias mode 1/9
	spi_master_write_command(dev, 0x2c); // set boost on
	spi_master_write_command(dev, 0x2e); // set voltage regulator on
	spi_master_write_command(dev, 0x2f); // Voltage follower on
	spi_master_write_command(dev, 0xf8); // set booster ratio to
	spi_master_write_command(dev, 0x00); // 4x
	spi_master_write_command(dev, 0x23); // set resistor ratio = 4
	spi_master_write_command(dev, 0x81);
	spi_master_write_command(dev, 0x28); // set contrast = 40
	spi_master_write_command(dev, 0xac); // set static indicator off
	spi_master_write_command(dev, 0x00);
	spi_master_write_command(dev, 0xa6); // disable inverse
	spi_master_write_command(dev, 0xaf); // enable display
	delayMS(100);
	spi_master_write_command(dev, 0xa5); // display all points
	delayMS(200);
	spi_master_write_command(dev, 0xa4); // normal display*/

	spi_master_write_command(dev, 0xA2);
	// Установите горизонтальную и вертикальную ориентацию в известное состояние
	spi_master_write_command(dev, 0xA0); //ADC selection(SEG0->SEG128)
	spi_master_write_command(dev, 0xC8); //SHL selection(COM0->COM64)
	// делитель внутреннего резистора установлен на 7 (от 0..7)
	spi_master_write_command(dev, 0x20 | 0x7);    //Regulator Resistor Selection
	// управление питанием, все внутренние блоки включены	(от 0..7)
	spi_master_write_command(dev, 0x28 | 0x7);
	// войти в режим динамического контраста
	spi_master_write_command(dev, 0x81);    //Electronic Volume
	spi_master_write_command(dev, 18);	// Настройка контраста. Отрегулируйте на своем дисплее. У меня на 15-19 норм. Максимум 63.
	spi_master_write_command(dev, 0x40);
	// CMD_DISPLAY_ON  CMD_DISPLAY_OFF
	spi_master_write_command(dev, 0xAF);    		//Display on
	// Инвертирование экрана
	spi_master_write_command(dev, 0xA6); //0xA6 - nomal, 0xA7 - revers


	if(dev->_bl >= 0) {
		gpio_set_level( dev->_bl, 1 );
	}
}

// Write buffer
void lcdWriteBuffer(TFT_t * dev) {
	for (int y=0; y<8; y++) {
		for (int x=0; x<128; x++) {
			int _x = x;
			if (dev->_flip) {
				_x += 4; // when drawing from the right edge, need a 4 pixel offset
			}

			spi_master_write_command(dev, (0xb0 | y)); // set Y
			spi_master_write_command(dev, (0x10 | (_x >> 4))); // set X MSB
			spi_master_write_command(dev, (0x00 | (_x & 0xf))); // set X LSB
			spi_master_write_data_byte(dev, dev->_buffer[x+y*128]);
		} // for x
	} // for y;
} 


// Draw pixel
// x:X coordinate
// y:Y coordinate
// color:color
void lcdDrawPixel(TFT_t * dev, uint16_t x, uint16_t y, uint8_t color){
	if (x >= dev->_width) return;
	if (y >= dev->_height) return;

	if (color == BLACK) 
		dev->_buffer[x+ (y/8)*128] |= _BV((y%8));  
	else
		dev->_buffer[x+ (y/8)*128] &= ~_BV((y%8)); 
}


// Draw multi pixel
// x:X coordinate
// y:Y coordinate
// size:Number of colors
// colors:colors
void lcdDrawMultiPixels(TFT_t * dev, uint16_t x, uint16_t y, uint16_t size, uint8_t * colors) {
	if (x+size > dev->_width) return;
	if (y >= dev->_height) return;

	for(int i=0; i<size; i++) {
		if (colors[i] == BLACK)
			dev->_buffer[(x+i)+ (y/8)*128] |= _BV((y%8));  
		else
			dev->_buffer[(x+i)+ (y/8)*128] &= ~_BV((y%8)); 
	}
}

// Draw rectangle of filling
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End X coordinate
// y2:End Y coordinate
// color:color
void lcdDrawFillRect(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color) {
	if (x1 >= dev->_width) return;
	if (x2 >= dev->_width) x2=dev->_width-1;
	if (y1 >= dev->_height) return;
	if (y2 >= dev->_height) y2=dev->_height-1;

	for(int _x=x1;_x<=x2;_x++){
		for(int _y=y1;_y<=y2;_y++){
			if (color == BLACK)
				dev->_buffer[_x+ (_y/8)*128] |= _BV((_y%8));  
			else
				dev->_buffer[_x+ (_y/8)*128] &= ~_BV((_y%8)); 
		}
	}
}

// Fill screen
// color:color
void lcdFillScreen(TFT_t * dev, uint8_t color) {
	for(int i=0; i<dev->_blen; i++) dev->_buffer[i] = color;
	//lcdDrawFillRect(dev, 0, 0, dev->_width-1, dev->_height-1, color);
}

// Draw line
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End X coordinate
// y2:End Y coordinate
// color:color 
void lcdDrawLine(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color) {
	int i;
	int dx,dy;
	int sx,sy;
	int E;

	/* distance between two points */
	dx = ( x2 > x1 ) ? x2 - x1 : x1 - x2;
	dy = ( y2 > y1 ) ? y2 - y1 : y1 - y2;

	/* direction of two point */
	sx = ( x2 > x1 ) ? 1 : -1;
	sy = ( y2 > y1 ) ? 1 : -1;

	/* inclination < 1 */
	if ( dx > dy ) {
		E = -dx;
		for ( i = 0 ; i <= dx ; i++ ) {
			lcdDrawPixel(dev, x1, y1, color);
			x1 += sx;
			E += 2 * dy;
			if ( E >= 0 ) {
			y1 += sy;
			E -= 2 * dx;
		}
	}

	/* inclination >= 1 */
	} else {
		E = -dy;
		for ( i = 0 ; i <= dy ; i++ ) {
			lcdDrawPixel(dev, x1, y1, color);
			y1 += sy;
			E += 2 * dx;
			if ( E >= 0 ) {
				x1 += sx;
				E -= 2 * dy;
			}
		}
	}
}

// Draw rectangle
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End	X coordinate
// y2:End	Y coordinate
// color:color
void lcdDrawRect(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color) {
	lcdDrawLine(dev, x1, y1, x2, y1, color);
	lcdDrawLine(dev, x2, y1, x2, y2, color);
	lcdDrawLine(dev, x2, y2, x1, y2, color);
	lcdDrawLine(dev, x1, y2, x1, y1, color);
}

// Draw rectangle with angle
// xc:Center X coordinate
// yc:Center Y coordinate
// w:Width of rectangle
// h:Height of rectangle
// angle :Angle of rectangle
// color :color

//When the origin is (0, 0), the point (x1, y1) after rotating the point (x, y) by the angle is obtained by the following calculation.
// x1 = x * cos(angle) - y * sin(angle)
// y1 = x * sin(angle) + y * cos(angle)
void lcdDrawRectAngle(TFT_t * dev, uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, uint8_t color) {
	double xd,yd,rd;
	int x1,y1;
	int x2,y2;
	int x3,y3;
	int x4,y4;
	rd = -angle * M_PI / 180.0;
	xd = 0.0 - w/2;
	yd = h/2;
	x1 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
	y1 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

	yd = 0.0 - yd;
	x2 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
	y2 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

	xd = w/2;
	yd = h/2;
	x3 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
	y3 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

	yd = 0.0 - yd;
	x4 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
	y4 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

	lcdDrawLine(dev, x1, y1, x2, y2, color);
	lcdDrawLine(dev, x1, y1, x3, y3, color);
	lcdDrawLine(dev, x2, y2, x4, y4, color);
	lcdDrawLine(dev, x3, y3, x4, y4, color);
}

// Draw triangle
// xc:Center X coordinate
// yc:Center Y coordinate
// w:Width of triangle
// h:Height of triangle
// angle :Angle of triangle
// color :color

//When the origin is (0, 0), the point (x1, y1) after rotating the point (x, y) by the angle is obtained by the following calculation.
// x1 = x * cos(angle) - y * sin(angle)
// y1 = x * sin(angle) + y * cos(angle)
void lcdDrawTriangle(TFT_t * dev, uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, uint8_t color) {
	double xd,yd,rd;
	int x1,y1;
	int x2,y2;
	int x3,y3;
	rd = -angle * M_PI / 180.0;
	xd = 0.0;
	yd = h/2;
	x1 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
	y1 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

	xd = w/2;
	yd = 0.0 - yd;
	x2 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
	y2 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

	xd = 0.0 - w/2;
	x3 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
	y3 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

	lcdDrawLine(dev, x1, y1, x2, y2, color);
	lcdDrawLine(dev, x1, y1, x3, y3, color);
	lcdDrawLine(dev, x2, y2, x3, y3, color);
}

// Draw circle
// x0:Central X coordinate
// y0:Central Y coordinate
// r:radius
// color:color
void lcdDrawCircle(TFT_t * dev, uint16_t x0, uint16_t y0, uint16_t r, uint8_t color) {
	int x;
	int y;
	int err;
	int old_err;

	x=0;
	y=-r;
	err=2-2*r;
	do{
		lcdDrawPixel(dev, x0-x, y0+y, color); 
		lcdDrawPixel(dev, x0-y, y0-x, color); 
		lcdDrawPixel(dev, x0+x, y0-y, color); 
		lcdDrawPixel(dev, x0+y, y0+x, color); 
		if ((old_err=err)<=x)	err+=++x*2+1;
		if (old_err>y || err>x) err+=++y*2+1;	 
	} while(y<0);
}

// Draw circle of filling
// x0:Central X coordinate
// y0:Central Y coordinate
// r:radius
// color:color
void lcdDrawFillCircle(TFT_t * dev, uint16_t x0, uint16_t y0, uint16_t r, uint8_t color) {
	int x;
	int y;
	int err;
	int old_err;
	int ChangeX;

	x=0;
	y=-r;
	err=2-2*r;
	ChangeX=1;
	do{
		if(ChangeX) {
			lcdDrawLine(dev, x0-x, y0-y, x0-x, y0+y, color);
			lcdDrawLine(dev, x0+x, y0-y, x0+x, y0+y, color);
		} // endif
		ChangeX=(old_err=err)<=x;
		if (ChangeX)			err+=++x*2+1;
		if (old_err>y || err>x) err+=++y*2+1;
	} while(y<=0);
} 

// Draw rectangle with round corner
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End	X coordinate
// y2:End	Y coordinate
// r:radius
// color:color
void lcdDrawRoundRect(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t r, uint8_t color) {
	int x;
	int y;
	int err;
	int old_err;
	unsigned char temp;

	if(x1>x2) {
		temp=x1; x1=x2; x2=temp;
	} // endif
	  
	if(y1>y2) {
		temp=y1; y1=y2; y2=temp;
	} // endif

	ESP_LOGD(TAG, "x1=%d x2=%d delta=%d r=%d",x1, x2, x2-x1, r);
	ESP_LOGD(TAG, "y1=%d y2=%d delta=%d r=%d",y1, y2, y2-y1, r);
	if (x2-x1 < r) return; // Add 20190517
	if (y2-y1 < r) return; // Add 20190517

	x=0;
	y=-r;
	err=2-2*r;

	do{
		if(x) {
			lcdDrawPixel(dev, x1+r-x, y1+r+y, color); 
			lcdDrawPixel(dev, x2-r+x, y1+r+y, color); 
			lcdDrawPixel(dev, x1+r-x, y2-r-y, color); 
			lcdDrawPixel(dev, x2-r+x, y2-r-y, color);
		} // endif 
		if ((old_err=err)<=x)	err+=++x*2+1;
		if (old_err>y || err>x) err+=++y*2+1;	 
	} while(y<0);

	ESP_LOGD(TAG, "x1+r=%d x2-r=%d",x1+r, x2-r);
	lcdDrawLine(dev, x1+r,y1  ,x2-r,y1	,color);
	lcdDrawLine(dev, x1+r,y2  ,x2-r,y2	,color);
	ESP_LOGD(TAG, "y1+r=%d y2-r=%d",y1+r, y2-r);
	lcdDrawLine(dev, x1  ,y1+r,x1  ,y2-r,color);
	lcdDrawLine(dev, x2  ,y1+r,x2  ,y2-r,color);  
} 

// Draw arrow
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End	X coordinate
// y2:End	Y coordinate
// w:Width of the botom
// color:color
// Thanks http://k-hiura.cocolog-nifty.com/blog/2010/11/post-2a62.html
void lcdDrawArrow(TFT_t * dev, uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t w,uint8_t color) {
	double Vx= x1 - x0;
	double Vy= y1 - y0;
	double v = sqrt(Vx*Vx+Vy*Vy);
	//	 printf("v=%f\n",v);
	double Ux= Vx/v;
	double Uy= Vy/v;

	uint16_t L[2],R[2];
	L[0]= x1 - Uy*w - Ux*v;
	L[1]= y1 + Ux*w - Uy*v;
	R[0]= x1 + Uy*w - Ux*v;
	R[1]= y1 - Ux*w - Uy*v;
	//printf("L=%d-%d R=%d-%d\n",L[0],L[1],R[0],R[1]);

	//lcdDrawLine(x0,y0,x1,y1,color);
	lcdDrawLine(dev, x1, y1, L[0], L[1], color);
	lcdDrawLine(dev, x1, y1, R[0], R[1], color);
	lcdDrawLine(dev, L[0], L[1], R[0], R[1], color);
}


// Draw arrow of filling
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End	X coordinate
// y2:End	Y coordinate
// w:Width of the botom
// color:color
void lcdDrawFillArrow(TFT_t * dev, uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t w,uint8_t color) {
	double Vx= x1 - x0;
	double Vy= y1 - y0;
	double v = sqrt(Vx*Vx+Vy*Vy);
	//printf("v=%f\n",v);
	double Ux= Vx/v;
	double Uy= Vy/v;

	uint16_t L[2],R[2];
	L[0]= x1 - Uy*w - Ux*v;
	L[1]= y1 + Ux*w - Uy*v;
	R[0]= x1 + Uy*w - Ux*v;
	R[1]= y1 - Ux*w - Uy*v;
	//printf("L=%d-%d R=%d-%d\n",L[0],L[1],R[0],R[1]);

	lcdDrawLine(dev, x0, y0, x1, y1, color);
	lcdDrawLine(dev, x1, y1, L[0], L[1], color);
	lcdDrawLine(dev, x1, y1, R[0], R[1], color);
	lcdDrawLine(dev, L[0], L[1], R[0], R[1], color);

	int ww;
	for(ww=w-1;ww>0;ww--) {
		L[0]= x1 - Uy*ww - Ux*v;
		L[1]= y1 + Ux*ww - Uy*v;
		R[0]= x1 + Uy*ww - Ux*v;
		R[1]= y1 - Ux*ww - Uy*v;
		//printf("Fill>L=%d-%d R=%d-%d\n",L[0],L[1],R[0],R[1]);
		lcdDrawLine(dev, x1, y1, L[0], L[1], color);
		lcdDrawLine(dev, x1, y1, R[0], R[1], color);
	}
}



// Draw ASCII character
// x:X coordinate
// y:Y coordinate
// ascii: ascii code
// color:color
int lcdDrawChar2(TFT_t * dev, uint8_t *font, uint16_t x, uint16_t y, uint8_t ascii, uint8_t color) {
	uint16_t xx,yy;
	unsigned char fonts[128]; // font pattern
	unsigned char pw, ph;
	uint16_t mask;

	if(_DEBUG_)printf("_font_direction=%d\n",dev->_font_direction);
	pw = font[0];
	ph = font[1];
	int fsize = pw * ph / 8;
	int index = ascii * fsize;
	if(_DEBUG_)printf("ascii=%d pw=%d ph=%d fsize=%d index=%d\n",ascii,pw,ph,fsize,index);
	//for(int i=0; i<pw; i++) {
	for(int i=0; i<fsize; i++) {
		fonts[i] = font[index+i];
		if (dev->_font_revert) fonts[i] = ~fonts[i];
		if(_DEBUG_)printf("fonts[%d]=0x%x\n",i, fonts[i]);
	}

	int16_t xd1 = 0;
	int16_t yd1 = 0;
	int16_t xd2 = 0;
	int16_t yd2 = 0;
	uint16_t xss = 0;
	uint16_t yss = 0;
	uint16_t xsd = 0;
	uint16_t ysd = 0;
	int16_t next = 0;
	if (dev->_font_direction == DIRECTION0) {
		xd1 =  0;
		yd1 =  0;
		xd2 = +1;
		yd2 = -1;
		xss =  x;
		yss =  y;
		xsd =  0;
		ysd =  1;
		next = x + pw;
	} else if (dev->_font_direction == DIRECTION90) {
		xd1 = +1;
		yd1 = +1;
		xd2 =  0;
		yd2 =  0;
		xss =  x;
		yss =  y;
		xsd =  1;
		ysd =  0;
		next = y + pw;
	} else if (dev->_font_direction == DIRECTION180) {
		xd1 =  0;
		yd1 =  0;
		xd2 = -1;
		yd2 = +1;
		xss =  x;
		yss =  y;
		xsd =  0;
		ysd =  1;
		next = x - pw;
	} else if (dev->_font_direction == DIRECTION270) {
		xd1 = -1;
		yd1 = -1; //+1;
		xd2 =  0;
		yd2 =  0;
		xss =  x;
		yss =  y;
		xsd =  1;
		ysd =  0;
		next = y - ph;
	}

	if(_DEBUG_)printf("xss=%d yss=%d\n",xss,yss);
	index = 0;
	yy = yss;
	xx = xss;

	//for(int h=0;h<ph;h++) {
	for(int w=0;w<pw;w++) {
		if(xsd) xx = xss;
		if(ysd) yy = yss;
		mask = 0x80;
		for(int bit=0;bit<8;bit++) {
			if(_DEBUG_)printf("xx=%d yy=%d xd1=%x yd2=%d mask=%02x fonts[%d]=%02x\n",
			xx, yy, xd1, yd2, mask, index, fonts[index]);
			if (fonts[index] & mask) {
				lcdDrawPixel(dev, xx, yy, BLACK);
			} else {
				lcdDrawPixel(dev, xx, yy, WHITE);
			}
			xx = xx + xd1;
			yy = yy + yd2;
			mask = mask >> 1;
		}
		index++;
		yy = yy + yd1;
		xx = xx + xd2;
	}

	if (next < 0) next = 0;
	return next;
}


int lcdDrawString2(TFT_t * dev, uint8_t * font, uint16_t x, uint16_t y, uint8_t * ascii, uint8_t color) {
		int length = strlen((char *)ascii);
		if(_DEBUG_)printf("lcdDrawString length=%d\n",length);
		for(int i=0;i<length;i++) {
				if(_DEBUG_)printf("ascii[%d]=%x x=%d y=%d\n",i,ascii[i],x,y);
				if (dev->_font_direction == 0)
						x = lcdDrawChar2(dev, font, x, y, ascii[i], color);
				if (dev->_font_direction == 1)
						y = lcdDrawChar2(dev, font, x, y, ascii[i], color);
				if (dev->_font_direction == 2)
						x = lcdDrawChar2(dev, font, x, y, ascii[i], color);
				if (dev->_font_direction == 3)
						y = lcdDrawChar2(dev, font, x, y, ascii[i], color);
		}
		if (dev->_font_direction == 0) return x;
		if (dev->_font_direction == 2) return x;
		if (dev->_font_direction == 1) return y;
		if (dev->_font_direction == 3) return y;
		return 0;
}


// Set font direction
// dir:Direction
void lcdSetFontDirection(TFT_t * dev, uint16_t dir) {
	dev->_font_direction = dir;
}

// Backlight OFF
void lcdBacklightOff(TFT_t * dev) {
	if(dev->_bl >= 0) {
		gpio_set_level( dev->_bl, 0 );
	}
}

// Backlight ON
void lcdBacklightOn(TFT_t * dev) {
	if(dev->_bl >= 0) {
		gpio_set_level( dev->_bl, 1 );
	}
}

// Display Flip upside down
void lcdFlipOn(TFT_t * dev) {
	spi_master_write_command(dev, 0xa1); // set SEG direction (A1 to flip horizontal)
	spi_master_write_command(dev, 0xc0); // set COM direction (C0 to flip vert)
	dev->_flip = 1;
}

// Display Inversion ON
void lcdInversionOn(TFT_t * dev) {
	spi_master_write_command(dev, 0xa7); // Sets the LCD display reverse
	dev->_invert = 1;
}

// Set font revert
void lcdSetFontRevert(TFT_t * dev) {
    dev->_font_revert = 1;
}

// UnSet font revert
void lcdUnsetFontRevert(TFT_t * dev) {
    dev->_font_revert = 0;
}

/*---------------------Функция вывода символа на дисплей-----------------------------*/
void GMG12864_Print_symbol_5x7(TFT_t * dev, uint8_t x, uint8_t y, uint16_t symbol, uint8_t inversion) {
/// Функция вывода символа на дисплей
	uint8_t x_start = x; //начальное положение по x
	uint8_t x_stop = x + 5; //конечное положение по x с учетом межсимвольного интервала
	for (uint8_t x = x_start; x <= x_stop; x++) {
		if (x == x_stop) { //Заполняем межсимвольные интервалы
			for (uint8_t i = 0; i <= 6; i++) { // от 0 до 6, т.к. шрифт высотой 7 пикселей
				if (0x00 & (1 << i)) {
					if (inversion) {
						//GMG12864_Draw_pixel(x, y + i, 0); //Закрасить пиксель
						lcdDrawPixel(dev, x, y + i, WHITE);
					} else {
						//GMG12864_Draw_pixel(x, y + i, 1); //Закрасить пиксель
						lcdDrawPixel(dev, x, y + i, BLACK);
					}
				} else {
					if (inversion) {
						//GMG12864_Draw_pixel(x, y + i, 1); //Очистить пиксель
						lcdDrawPixel(dev, x, y + i, BLACK);
					} else {
						//GMG12864_Draw_pixel(x, y + i, 0); //Очистить пиксель
						lcdDrawPixel(dev, x, y + i, WHITE);
					}
				}
			}
		} else { //Заполняем полезной информацией
			for (uint8_t i = 0; i <= 6; i++) { // от 0 до 6, т.к. шрифт высотой 7 пикселей
				if (Font_5x7[(symbol * 5) + x - x_start] & (1 << i)) {
					if (inversion) {
						//GMG12864_Draw_pixel(x, y + i, 0); //Закрасить пиксель
						lcdDrawPixel(dev, x, y + i, WHITE);
					} else {
						//GMG12864_Draw_pixel(x, y + i, 1); //Закрасить пиксель
						lcdDrawPixel(dev, x, y + i, BLACK);
					}
				} else {
					if (inversion) {
						//GMG12864_Draw_pixel(x, y + i, 1); //Очистить пиксель
						lcdDrawPixel(dev, x, y + i, BLACK);
					} else {
						//GMG12864_Draw_pixel(x, y + i, 0); //Очистить пиксель
						lcdDrawPixel(dev, x, y + i, WHITE);
					}
				}
			}
		}
	}
}
/*---------------------Функция вывода символа на дисплей-----------------------------*/

/*---------------------Функция вывода символа на дисплей-----------------------------*/
void GMG12864_Print_symbol_3x5(TFT_t * dev, uint8_t x, uint8_t y, uint16_t symbol, uint8_t inversion) {
/// Функция вывода символа на дисплей
	uint8_t x_start = x; //начальное положение по x
	uint8_t x_stop = x + 3; //конечное положение по x с учетом межсимвольного интервала
	for (uint8_t x = x_start; x <= x_stop; x++) {
		if (x == x_stop) { //Заполняем межсимвольные интервалы
			for (uint8_t i = 0; i <= 4; i++) { // от 0 до 6, т.к. шрифт высотой 7 пикселей
				if (0x00 & (1 << i)) {
					if (inversion) {
						//GMG12864_Draw_pixel(x, y + i, 0); //Закрасить пиксель
						lcdDrawPixel(dev, x, y + i, WHITE);
					} else {
						//GMG12864_Draw_pixel(x, y + i, 1); //Закрасить пиксель
						lcdDrawPixel(dev, x, y + i, BLACK);
					}
				} else {
					if (inversion) {
						//GMG12864_Draw_pixel(x, y + i, 1); //Очистить пиксель
						lcdDrawPixel(dev, x, y + i, BLACK);
					} else {
						//GMG12864_Draw_pixel(x, y + i, 0); //Очистить пиксель
						lcdDrawPixel(dev, x, y + i, WHITE);
					}
				}
			}
		} else { //Заполняем полезной информацией
			for (uint8_t i = 0; i <= 4; i++) { // от 0 до 6, т.к. шрифт высотой 7 пикселей
				if (Font_3x5[(symbol * 3) + x - x_start] & (1 << i)) {
					if (inversion) {
						//GMG12864_Draw_pixel(x, y + i, 0); //Закрасить пиксель
						lcdDrawPixel(dev, x, y + i, WHITE);
					} else {
						//GMG12864_Draw_pixel(x, y + i, 1); //Закрасить пиксель
						lcdDrawPixel(dev, x, y + i, BLACK);
					}
				} else {
					if (inversion) {
						//GMG12864_Draw_pixel(x, y + i, 1); //Очистить пиксель
						lcdDrawPixel(dev, x, y + i, BLACK);
					} else {
						//GMG12864_Draw_pixel(x, y + i, 0); //Очистить пиксель
						lcdDrawPixel(dev, x, y + i, WHITE);
					}
				}
			}
		}
	}
}
/*---------------------Функция вывода символа на дисплей-----------------------------*/

/*----------------Функция декодирования UTF-8 в набор символов-----------------*/
void GMG12864_Decode_UTF8(TFT_t * dev, uint8_t x, uint8_t y, uint8_t font, bool inversion, char *tx_buffer) {
/// Функция декодирования UTF-8 в набор символов и последующее занесение в буфер кадра
/// \param x - координата по х. От 0 до 127
/// \param y - координата по y. от 0 до 7
/// \param font - шрифт. 0 - 3x5, 1 - 5x7
	uint16_t symbol = 0;
	bool flag_block = 0;
	for (int i = 0; i < strlen(tx_buffer); i++) {
		if (tx_buffer[i] < 0xC0) { //Английский текст и символы. Если до русского текста, то [i] <0xD0. Но в font добавлен знак "°"
			if (flag_block) {
				flag_block = 0;
			} else {
				symbol = tx_buffer[i];
				if (font == font3x5) { //Если выбран шрифт размера 3x5
					if (inversion) {
						GMG12864_Print_symbol_3x5(dev, x, y, symbol - 32, 1); //Таблица UTF-8. Basic Latin. С "пробел" до "z". Инверсия вкл.
					} else {
						GMG12864_Print_symbol_3x5(dev, x, y, symbol - 32, 0); //Таблица UTF-8. Basic Latin. С "пробел" до "z". Инверсия выкл.
					}
					x = x + 4;
				} else if (font == font5x7) { //Если выбран шрифт размера 5x7
					if (inversion) {
						GMG12864_Print_symbol_5x7(dev, x, y, symbol - 32, 1); //Таблица UTF-8. Basic Latin. С "пробел" до "z". Инверсия вкл.
					} else {
						GMG12864_Print_symbol_5x7(dev, x, y, symbol - 32, 0); //Таблица UTF-8. Basic Latin. С "пробел" до "z". Инверсия выкл.
					}
					x = x + 6;
				}
			}
		}

		else { //Русский текст
			symbol = tx_buffer[i] << 8 | tx_buffer[i + 1];
			if (symbol < 0xD180 && symbol > 0xD081) {
				if (font == font3x5) { //Если выбран шрифт размера 3x5
					if (inversion) {
						GMG12864_Print_symbol_3x5(dev, x, y, symbol - 53297, 1); //Таблица UTF-8. Кириллица. С буквы "А" до "п". Инверсия вкл.
					} else {
						GMG12864_Print_symbol_3x5(dev, x, y, symbol - 53297, 0); //Таблица UTF-8. Кириллица. С буквы "А" до "п". Инверсия выкл.
					}
					x = x + 4;
				} else if (font == font5x7) { //Если выбран шрифт размера 5x7
					if (inversion) {
						GMG12864_Print_symbol_5x7(dev, x, y, symbol - 53297, 1); //Таблица UTF-8. Кириллица. С буквы "А" до "п". Инверсия вкл.
					} else {
						GMG12864_Print_symbol_5x7(dev, x, y, symbol - 53297, 0); //Таблица UTF-8. Кириллица. С буквы "А" до "п". Инверсия выкл.
					}
					x = x + 6;
				}
			} else if (symbol == 0xD081) {
				if (font == font3x5) { //Если выбран шрифт размера 3x5
					if (inversion) {
						GMG12864_Print_symbol_3x5(dev, x, y, 159, 1); ////Таблица UTF-8. Кириллица. Буква "Ё". Инверсия вкл.
					} else {
						GMG12864_Print_symbol_3x5(dev, x, y, 159, 0); ////Таблица UTF-8. Кириллица. Буква "Ё". Инверсия выкл.
					}
					x = x + 4;
				} else if (font == font5x7) { //Если выбран шрифт размера 5x7
					if (inversion) {
						GMG12864_Print_symbol_5x7(dev, x, y, 159, 1); ////Таблица UTF-8. Кириллица. Буква "Ё". Инверсия вкл.
					} else {
						GMG12864_Print_symbol_5x7(dev, x, y, 159, 0); ////Таблица UTF-8. Кириллица. Буква "Ё". Инверсия выкл.
					}
					x = x + 6;
				}
			} else if (symbol == 0xD191) {
				if (font == font3x5) { //Если выбран шрифт размера 3x5
					if (inversion) {
						GMG12864_Print_symbol_3x5(dev, x, y, 160, 1); ////Таблица UTF-8. Кириллица. Буква "ё". Инверсия вкл.
					} else {
						GMG12864_Print_symbol_3x5(dev, x, y, 160, 0); ////Таблица UTF-8. Кириллица. Буква "ё". Инверсия выкл.
					}
					x = x + 4;
				} else if (font == font5x7) { //Если выбран шрифт размера 5x7
					if (inversion) {
						GMG12864_Print_symbol_5x7(dev, x, y, 160, 1); ////Таблица UTF-8. Кириллица. Буква "ё". Инверсия вкл.
					} else {
						GMG12864_Print_symbol_5x7(dev, x, y, 160, 0); ////Таблица UTF-8. Кириллица. Буква "ё". Инверсия выкл.
					}
					x = x + 6;
				}
			} else if (symbol == 0xC2B0) {
				if (font == font3x5) { //Если выбран шрифт размера 3x5
					if (inversion) {
						GMG12864_Print_symbol_3x5( dev, x, y, 161, 1); ////Таблица UTF-8. Basic Latin. Символ "°". Инверсия вкл.
					} else {
						GMG12864_Print_symbol_3x5( dev, x, y, 161, 0); ////Таблица UTF-8. Basic Latin. Символ "°". Инверсия выкл.
					}
					x = x + 4;
				} else if (font == font5x7) { //Если выбран шрифт размера 5x7
					if (inversion) {
						GMG12864_Print_symbol_5x7(dev, x, y, 161, 1); ////Таблица UTF-8. Basic Latin. Символ "°". Инверсия вкл.
					} else {
						GMG12864_Print_symbol_5x7(dev, x, y, 161, 0); ////Таблица UTF-8. Basic Latin. Символ "°". Инверсия выкл.
					}
					x = x + 6;
				}
			}

			else {
				if (font == font3x5) { //Если выбран шрифт размера 3x5
					if (inversion) {
						GMG12864_Print_symbol_3x5(dev, x, y, symbol - 53489, 1); //Таблица UTF-8. Кириллица. С буквы "р", начинается сдвиг. Инверсия вкл.
					} else {
						GMG12864_Print_symbol_3x5(dev, x, y, symbol - 53489, 0); //Таблица UTF-8. Кириллица. С буквы "р", начинается сдвиг. Инверсия выкл.
					}
					x = x + 4;
				} else if (font == font5x7) { //Если выбран шрифт размера 5x7
					if (inversion) {
						GMG12864_Print_symbol_5x7(dev, x, y, symbol - 53489, 1); //Таблица UTF-8. Кириллица. С буквы "р", начинается сдвиг. Инверсия вкл.
					} else {
						GMG12864_Print_symbol_5x7(dev, x, y, symbol - 53489, 0); //Таблица UTF-8. Кириллица. С буквы "р", начинается сдвиг. Инверсия выкл.
					}
					x = x + 6;
				}
			}
			flag_block = 1;
		}
	}
}
/*----------------Функция декодирования UTF-8 в набор символов-----------------*/

/*----------------Функция, формирующая точку значения на графике---------------*/
uint8_t GMG12864_Value_for_Plot(TFT_t * dev, int y_min, int y_max, float value) {
/// Функция, формирующая точку значения на графике
/// \param int y_min - минимальное значение по оси y
/// \param int y_max - максимальное значение по оси y
/// \param float value - значение, которое будем отображать на графике
	char text[20] = { 0, };
	uint8_t Graph_value = 0;
	if (y_max > y_min && value >= y_min && value <= y_max) {
		float y = 50.0f - (y_max + value * (-1)) * (50.0f / (y_max + y_min * (-1)));
		Graph_value = (uint8_t) y;
	} else if (value > y_max) {
		Graph_value = 50;
		sprintf(text, "   Clipping >va");
		GMG12864_Decode_UTF8(dev, 37, 26, 1, 0, text);
		lcdWriteBuffer(dev);
	} else if (value < y_min) {
		Graph_value = 0;
		sprintf(text, "   Clipping <va");
		GMG12864_Decode_UTF8(dev, 37, 26, 1, 0, text);
		lcdWriteBuffer(dev);
	}
	return Graph_value;
}
/*----------------Функция, формирующая точку значения на графике---------------*/

/*-------------------Работа с массивом данных---------------------*/

void GMG12864_Fill_the_array_Plot(uint8_t *counter, uint8_t *array, uint8_t size_array, uint8_t value) {
/// Функция, заполняющая массив значениями, чтоб отрисовывать график
/// \param uint8_t *counter - счетчик значений
/// \param uint8_t *array - массив, куда будем закидывать значения для отображения графика
/// \param uint8_t size_array - размер массива, куда будем закидывать значения для отображения графика
/// \param uint8_t value - подготовленное значение для графика(подготавливается при помощи ф-ии uint8_t GMG12864_Value_for_Plot(int y_min, int y_max, float value))
	if (*counter == 0) {
		array[0] = value;
	}

	if (*counter) {
		if (*counter <= size_array - 1) {
			for (int i = 0; i <= *counter; i++) {
				if (i == *counter) {
					array[0] = value;
				} else {
					array[*counter - i] = array[*counter - i - 1];
				}
			}
		} else if (*counter > size_array - 1) {
			for (int i = size_array - 1; i >= 0; i--) {
				array[i] = array[i - 1];
				if (i == 0) {
					array[i] = value;
				}
			}
		}

	}
	(*counter)++;
	if (*counter == 250) {
		*counter = 128; //Защита от переполнения, иначе график будет сбрасываться(главное чтоб разница была равна или больше кол-ву выводимых точек)
	}

}
/*-------------------Работа с массивом данных---------------------*/

/********************************РАБОТА С ГЕОМЕТРИЧЕСКИМИ ФИГУРАМИ**********************************/

void GMG12864_Draw_line(TFT_t * dev, uint8_t x_start, uint8_t y_start, uint8_t x_end, uint8_t y_end, uint8_t color) {
	int dx = (x_end >= x_start) ? x_end - x_start : x_start - x_end;
	int dy = (y_end >= y_start) ? y_end - y_start : y_start - y_end;
	int sx = (x_start < x_end) ? 1 : -1;
	int sy = (y_start < y_end) ? 1 : -1;
	int err = dx - dy;

	for (;;) {
		lcdDrawPixel(dev, x_start, y_start, color); 
		//GMG12864_Draw_pixel(x_start, y_start, color);
		if (x_start == x_end && y_start == y_end)
			break;
		int e2 = err + err;
		if (e2 > -dy) {
			err -= dy;
			x_start += sx;
		}
		if (e2 < dx) {
			err += dx;
			y_start += sy;
		}
	}
}

/*----------------------Функция, формирующая весь график, который будет отображаться на дисплее-----------------------------*/
void GMG12864_Generate_a_Graph(TFT_t * dev, uint8_t *counter, uint8_t *array, uint8_t size_array, int y_min, int y_max, uint8_t x_grid_time, uint8_t time_interval,
bool grid, uint8_t color) {
/// Функция, формирующая весь график, который будет отображаться на дисплее
/// \param uint8_t *counter - счетчик значений
/// \param uint8_t *array - массив, куда мы закидывали значения для отображения графика
/// \param uint8_t size_array - размер массива, куда мы закидывали значения для отображения графика
/// \param int y_min - минимальное значение по оси y
/// \param int y_max - максимальное значение по оси y
/// \param uint8_t x_grid_time - размер клетки по x(Например, график обновляется раз в 100 мс, значит одна клетка по X будет 1 сек.)
/// \param uint8_t time_interval - интервал времени. 0 - сек., 1 - мин., 2 = час.
/// \param bool grid - Сетка граффика. вкл = 1. выкл = 0.

	int y_minimum = y_min;
	int y_maximum = y_max;
	if (y_min < 0) {
		y_minimum = y_minimum * (-1);
	}
	if (y_max < 0) {
		y_maximum = y_maximum * (-1);
	}
	int val_del = (y_minimum + y_maximum) / 5;
	//printf("%d\r\n", val_del);
	char text[20] = { 0, };
//	lcdFillScreen(dev, WHITE);
	sprintf(text, "%4.1d", y_min);
	GMG12864_Decode_UTF8(dev, 8, 48, 0, 0, text);
	sprintf(text, "%4.1d", (y_min + val_del));
	GMG12864_Decode_UTF8(dev, 8, 38, 0, 0, text);
	sprintf(text, "%4.1d", (y_min + val_del * 2));
	GMG12864_Decode_UTF8(dev, 8, 28, 0, 0, text);
	sprintf(text, "%4.1d", (y_min + val_del * 3));
	GMG12864_Decode_UTF8(dev, 8, 18, 0, 0, text);
	sprintf(text, "%4.1d", (y_min + val_del * 4));
	GMG12864_Decode_UTF8(dev, 8, 8, 0, 0, text);
	sprintf(text, "%4.1d", y_max);
	GMG12864_Decode_UTF8(dev, 8, 0, 0, 0, text);

	if (time_interval == 0) {
		sprintf(text, "%2.1dcек.", x_grid_time);
	} else if (time_interval == 1) {
		sprintf(text, "%2.1dмин.", x_grid_time);
	} else if (time_interval == 2) {
		sprintf(text, "%2.1dчас.", x_grid_time);
	}
	GMG12864_Decode_UTF8(dev, 53, 57, 1, 0, text);

	/*----------------Ось абсцисс, ось ординат, разметка-----------------*/

	GMG12864_Draw_line(dev, 27, 50, 127, 50, BLACK); //ось абсцисс
	for (uint8_t i = 27; i <= 77; i = i + 10) {
		GMG12864_Draw_line(dev, i, 51, i, 52, BLACK);
	}
	for (uint8_t i = 87; i <= 127; i = i + 10) {
		//GMG12864_Draw_pixel(dev, i, 51, 1);
		lcdDrawPixel(dev, i, 51, BLACK);
	}

	GMG12864_Draw_line(dev, 27, 0, 27, 50, BLACK); //ось ординат
	for (uint8_t i = 0; i <= 50; i = i + 10) {
		GMG12864_Draw_line(dev, 25, i, 27, i, BLACK);
	}

	GMG12864_Draw_line(dev, 57, 53, 57, 55, BLACK); //Полоса по х левая
	GMG12864_Draw_line(dev, 67, 53, 67, 55, BLACK); //Полоса по х правая
	GMG12864_Draw_line(dev, 52, 54, 72, 54, BLACK); //Полоса между делениями по х
	GMG12864_Draw_line(dev, 54, 52, 54, 56, BLACK); //Левая стрелка на деление
	GMG12864_Draw_line(dev, 55, 53, 55, 55, BLACK); //Левая стрелка на деление
	GMG12864_Draw_line(dev, 69, 53, 69, 55, BLACK); //Правая стрелка на деление
	GMG12864_Draw_line(dev, 70, 52, 70, 56, BLACK); //Правая стрелка на деление
	GMG12864_Draw_line(dev, 127, 53, 127, 57, BLACK); //Отметка "сейчас"
	GMG12864_Draw_line(dev, 122, 55, 127, 55, BLACK); //Стрелка на "сейчас"
	GMG12864_Draw_line(dev, 124, 53, 124, 57, BLACK); //Стрелка на "сейчас"
	GMG12864_Draw_line(dev, 125, 54, 125, 56, BLACK); //Стрелка на "сейчас"

	/*----------------Ось асцисс, ось ординат, разметка-----------------*/

	/*--------------------------Разметка сетки-------------------------*/
	if (grid) {
		for (uint8_t y = 0; y <= 40; y = y + 10) {
			for (uint8_t x = 27; x <= 127; x = x + 2) {
				//GMG12864_Draw_pixel(x, y, 1);
				lcdDrawPixel(dev, x, y, BLACK);
				//lcdDrawPixel(TFT_t * dev, uint16_t x, uint16_t y, uint8_t color);
			}
		}

		for (uint8_t x = 27; x <= 127; x = x + 10) {
			for (uint8_t y = 0; y <= 50; y = y + 2) {
				//GMG12864_Draw_pixel(x, y, 1);
				lcdDrawPixel(dev, x, y, BLACK);
			}
		}
	}
	/*--------------------------Разметка сетки-------------------------*/

	if (*counter <= size_array) {
		if (*counter == size_array) {
			array_is_full = true;
		}
	}

	if (array_is_full) {
		for (int i = 0; i < size_array - 1; i++) {
			//GMG12864_Draw_pixel(127 - i, 50 - array[i], 1); //Рисуем точками
			GMG12864_Draw_line(dev, 127 - (i + 1), 50 - array[i + 1], 127 - i, 50 - array[i], BLACK); //Рисуем линиями
			//GMG12864_Draw_line(127-i, 50, 127-i, 50 - array[i], 1);//Закрашиваем область

		}
	} else {
		for (int i = 0; i < *counter; i++) {
			//GMG12864_Draw_pixel(127 - i, 50 - array[i], 1); //Рисуем точками
			GMG12864_Draw_line(dev, 127 - (i + 1), 50 - array[i + 1], 127 - i, 50 - array[i], BLACK); //Рисуем линиями
			//GMG12864_Draw_line(127-i, 50, 127-i, 50 - array[i], 1);//Закрашиваем область

		}
	}
}
/*----------------------Функция, формирующая весь график, который будет отображаться на дисплее-----------------------------*/
