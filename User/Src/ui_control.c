/*
 * 240x240 SPI 液晶（ST7789）显示部分，硬件连线（旧版接口）：
 *   PB3  -> SPI1_SCK      PB5   -> SPI1_MOSI
 *   PA15 -> LCD_CS        PD13  -> LCD_DC        PD12 -> LCD_BL
 * SPI1 配置：主机、8 位、MSB、CPOL=0/CPHA=0、APB2(64M)/8 = 8MBits/s
 * （SPI3 已经让给双路 AD9833：PC10=SCLK、PC12=SDATA、CS1=PB7、CS2=PB9）
 * 原 Nextion 串口屏相关的收/发代码已全部移除，输入由按键完成（见 key.c）。
 */
#include <string.h>
#include <stdio.h>
#include "ui_control.h"
#include "spi.h"
#include "sample.h"
#include "calculate.h"

/* ============================ 1. 硬件引脚 ============================ */
#define LCD_CS_PORT         GPIOA
#define LCD_CS_PIN          GPIO_PIN_15
#define LCD_DC_PORT         GPIOD
#define LCD_DC_PIN          GPIO_PIN_13
#define LCD_BL_PORT         GPIOD
#define LCD_BL_PIN          GPIO_PIN_12

#define LCD_CS_LOW()        HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET)
#define LCD_CS_HIGH()       HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET)
#define LCD_DC_CMD()        HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_RESET)
#define LCD_DC_DATA()       HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET)
#define LCD_BL_ON()         HAL_GPIO_WritePin(LCD_BL_PORT, LCD_BL_PIN, GPIO_PIN_SET)
#define LCD_BL_OFF()        HAL_GPIO_WritePin(LCD_BL_PORT, LCD_BL_PIN, GPIO_PIN_RESET)

#define LCD_W               240
#define LCD_H               240

/* ================= 液晶驱动（原理图新板没有屏，下面整段默认不编译） ================= */
#if UI_LCD_ENABLE

/* 如果屏上有复位脚（FPC 第 3 脚）并接到了某个 IO，把它打开，
 * 三个值改成实际引脚即可；默认关闭。 */
#define LCD_USE_RST         0
#if LCD_USE_RST
#define LCD_RST_PORT        GPIOE
#define LCD_RST_PIN         GPIO_PIN_0
#define LCD_RST_LOW()       HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_RESET)
#define LCD_RST_HIGH()      HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET)
#endif

/* ============================ 2. 颜色（RGB565） ============================ */
#define UI_COL_BG           0x0000  /* 黑 */
#define UI_COL_HEADER       0x001F  /* 蓝 */
#define UI_COL_FOOTER       0x18E3  /* 深灰 */
#define UI_COL_LABEL        0xC618  /* 浅灰 */
#define UI_COL_FREQ         0x07FF  /* 青 */
#define UI_COL_WAVE         0x07E0  /* 绿 */
#define UI_COL_PHASE        0xFFE0  /* 黄 */
#define UI_COL_MODE         0xFFFF  /* 白 */
#define UI_COL_STATE        0xFC00  /* 橙 */
#define UI_COL_HINT         0x8410  /* 灰 */

/* ============================ 3. 5x7 点阵字库 ============================ */
/* 每个字 7 行、每行 5 列，'1' 为亮点；只做了大写字母/数字/常用符号，
 * 小写字母在查找时自动转成大写。想加字直接在表里加一行即可。 */
typedef struct
{
    char        ch;
    const char *rows;
} LcdGlyph;

static const LcdGlyph LCD_FONT[] =
{
    {' ', "00000""00000""00000""00000""00000""00000""00000"},
    {'0', "01110""10001""10011""10101""11001""10001""01110"},
    {'1', "00100""01100""00100""00100""00100""00100""01110"},
    {'2', "01110""10001""00001""00010""00100""01000""11111"},
    {'3', "11111""00010""00100""00010""00001""10001""01110"},
    {'4', "00010""00110""01010""10010""11111""00010""00010"},
    {'5', "11111""10000""11110""00001""00001""10001""01110"},
    {'6', "00110""01000""10000""11110""10001""10001""01110"},
    {'7', "11111""00001""00010""00100""01000""01000""01000"},
    {'8', "01110""10001""10001""01110""10001""10001""01110"},
    {'9', "01110""10001""10001""01111""00001""00010""01100"},
    {'A', "01110""10001""10001""11111""10001""10001""10001"},
    {'B', "11110""10001""10001""11110""10001""10001""11110"},
    {'C', "01110""10001""10000""10000""10000""10001""01110"},
    {'D', "11100""10010""10001""10001""10001""10010""11100"},
    {'E', "11111""10000""10000""11110""10000""10000""11111"},
    {'F', "11111""10000""10000""11110""10000""10000""10000"},
    {'G', "01110""10001""10000""10111""10001""10001""01111"},
    {'H', "10001""10001""10001""11111""10001""10001""10001"},
    {'I', "01110""00100""00100""00100""00100""00100""01110"},
    {'J', "00111""00010""00010""00010""00010""10010""01100"},
    {'K', "10001""10010""10100""11000""10100""10010""10001"},
    {'L', "10000""10000""10000""10000""10000""10000""11111"},
    {'M', "10001""11011""10101""10101""10001""10001""10001"},
    {'N', "10001""11001""10101""10011""10001""10001""10001"},
    {'O', "01110""10001""10001""10001""10001""10001""01110"},
    {'P', "11110""10001""10001""11110""10000""10000""10000"},
    {'Q', "01110""10001""10001""10001""10101""10010""01101"},
    {'R', "11110""10001""10001""11110""10100""10010""10001"},
    {'S', "01111""10000""10000""01110""00001""00001""11110"},
    {'T', "11111""00100""00100""00100""00100""00100""00100"},
    {'U', "10001""10001""10001""10001""10001""10001""01110"},
    {'V', "10001""10001""10001""10001""10001""01010""00100"},
    {'W', "10001""10001""10001""10101""10101""11011""10001"},
    {'X', "10001""10001""01010""00100""01010""10001""10001"},
    {'Y', "10001""10001""01010""00100""00100""00100""00100"},
    {'Z', "11111""00001""00010""00100""01000""10000""11111"},
    {'.', "00000""00000""00000""00000""00000""01100""01100"},
    {':', "00000""01100""01100""00000""01100""01100""00000"},
    {'-', "00000""00000""00000""11111""00000""00000""00000"},
    {'+', "00000""00100""00100""11111""00100""00100""00000"},
    {'/', "00001""00010""00010""00100""01000""01000""10000"},
    {'%', "11001""11010""00010""00100""01000""01011""10011"},
    {'=', "00000""00000""11111""00000""11111""00000""00000"},
    {'(', "00010""00100""01000""01000""01000""00100""00010"},
    {')', "01000""00100""00010""00010""00010""00100""01000"},
    {'?', "01110""10001""00001""00110""00100""00000""00100"},
};

#define LCD_FONT_NUM        (sizeof(LCD_FONT) / sizeof(LCD_FONT[0]))
#define LCD_SCALE_MAX       2       /* 本界面只用 1 倍和 2 倍字号 */
#define LCD_CHAR_W          5
#define LCD_CHAR_H          7
#define LCD_CHAR_PITCH(s)   (6 * (s))   /* 字宽 + 1 格间距 */
#define LCD_FILL_CHUNK      120         /* 填色时每次发送的像素个数 */

static uint8_t s_fillBuf[LCD_FILL_CHUNK * 2];
static uint8_t s_glyphBuf[LCD_CHAR_W * LCD_SCALE_MAX * LCD_CHAR_H * LCD_SCALE_MAX * 2];

/* ============================ 4. ST7789 底层驱动 ============================ */
/* 下面几个函数都要求已经在 lcdOpen() 之后调用，中途不放开 CS，
 * 这样“设窗口 + 写显存”是一整段连续传输。 */
static void lcdSendCmd(uint8_t cmd)
{
    LCD_DC_CMD();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
}

static void lcdSendData(const uint8_t *data, uint16_t len)
{
    if (len == 0) return;
    LCD_DC_DATA();
    HAL_SPI_Transmit(&hspi1, (uint8_t *)data, len, HAL_MAX_DELAY);
}

static void lcdSendCmdData(uint8_t cmd, const uint8_t *data, uint16_t len)
{
    lcdSendCmd(cmd);
    lcdSendData(data, len);
}

/* 设置写显存窗口，并以 0x2C 进入写显存状态 */
static void lcdSetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t d[4];
    d[0] = (uint8_t)(x0 >> 8); d[1] = (uint8_t)x0;
    d[2] = (uint8_t)(x1 >> 8); d[3] = (uint8_t)x1;
    lcdSendCmdData(0x2A, d, 4);
    d[0] = (uint8_t)(y0 >> 8); d[1] = (uint8_t)y0;
    d[2] = (uint8_t)(y1 >> 8); d[3] = (uint8_t)y1;
    lcdSendCmdData(0x2B, d, 4);
    lcdSendCmd(0x2C);
}

static void lcdOpen(void)
{
    LCD_CS_LOW();
}

static void lcdClose(void)
{
    LCD_CS_HIGH();
}

/* CubeMX 默认把 GPIO 输出配成低速，DC 线要跟着 SPI 翻转，这里统一改成最高速，
 * 免得出现数据被误当命令、画面错位的问题。 */
static void lcdGpioSpeedUp(void)
{
    GPIO_InitTypeDef gi = {0};

    gi.Mode  = GPIO_MODE_OUTPUT_PP;
    gi.Pull  = GPIO_NOPULL;
    gi.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    gi.Pin = LCD_CS_PIN; HAL_GPIO_Init(LCD_CS_PORT, &gi);
    gi.Pin = LCD_DC_PIN; HAL_GPIO_Init(LCD_DC_PORT, &gi);
    gi.Pin = LCD_BL_PIN; HAL_GPIO_Init(LCD_BL_PORT, &gi);
}

/* 填色块 */
static void lcdFillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint32_t total;
    uint16_t i;

    if (w == 0 || h == 0) return;
    if ((x >= LCD_W) || (y >= LCD_H)) return;
    if ((uint32_t)x + w > LCD_W) w = (uint16_t)(LCD_W - x);
    if ((uint32_t)y + h > LCD_H) h = (uint16_t)(LCD_H - y);

    for (i = 0; i < LCD_FILL_CHUNK; i++)
    {
        s_fillBuf[2 * i]     = (uint8_t)(color >> 8);
        s_fillBuf[2 * i + 1] = (uint8_t)color;
    }

    total = (uint32_t)w * h;
    lcdOpen();
    lcdSetWindow(x, y, (uint16_t)(x + w - 1), (uint16_t)(y + h - 1));
    while (total)
    {
        uint16_t n = (total > LCD_FILL_CHUNK) ? LCD_FILL_CHUNK : (uint16_t)total;
        lcdSendData(s_fillBuf, (uint16_t)(n * 2));
        total -= n;
    }
    lcdClose();
}

static const char *lcdFontLookup(char c)
{
    uint8_t i;

    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    for (i = 0; i < LCD_FONT_NUM; i++)
    {
        if (LCD_FONT[i].ch == c) return LCD_FONT[i].rows;
    }
    return LCD_FONT[LCD_FONT_NUM - 1].rows;    /* 未知字符显示 '?' */
}

/* 画一个字符：先在缓冲区拼好位图，再一次性写进显存 */
static void lcdDrawChar(uint16_t x, uint16_t y, char c, uint8_t scale, uint16_t fg, uint16_t bg)
{
    const char *rows;
    uint16_t w, h, px, py;
    uint8_t r, col, dx, dy;

    if (scale == 0) return;
    if (scale > LCD_SCALE_MAX) scale = LCD_SCALE_MAX;
    if ((uint32_t)x + LCD_CHAR_W * scale > LCD_W) return;
    if ((uint32_t)y + LCD_CHAR_H * scale > LCD_H) return;

    rows = lcdFontLookup(c);
    w = (uint16_t)(LCD_CHAR_W * scale);
    h = (uint16_t)(LCD_CHAR_H * scale);

    for (r = 0; r < LCD_CHAR_H; r++)
    {
        for (col = 0; col < LCD_CHAR_W; col++)
        {
            uint16_t color = (rows[r * LCD_CHAR_W + col] == '1') ? fg : bg;
            for (dy = 0; dy < scale; dy++)
            {
                py = (uint16_t)(r * scale + dy);
                for (dx = 0; dx < scale; dx++)
                {
                    px = (uint16_t)(col * scale + dx);
                    s_glyphBuf[(py * w + px) * 2]     = (uint8_t)(color >> 8);
                    s_glyphBuf[(py * w + px) * 2 + 1] = (uint8_t)color;
                }
            }
        }
    }

    lcdOpen();
    lcdSetWindow(x, y, (uint16_t)(x + w - 1), (uint16_t)(y + h - 1));
    lcdSendData(s_glyphBuf, (uint16_t)(w * h * 2));
    lcdClose();
}

static void lcdDrawText(uint16_t x, uint16_t y, const char *str, uint8_t scale, uint16_t fg, uint16_t bg)
{
    while (*str)
    {
        lcdDrawChar(x, y, *str, scale, fg, bg);
        x += LCD_CHAR_PITCH(scale);
        str++;
    }
}

/* ST7789 初始化序列（240x240，RGB565） */
typedef struct
{
    uint8_t  cmd;
    uint8_t  len;
    uint8_t  data[16];
    uint16_t delayMs;
} LcdInitItem;

static const LcdInitItem LCD_INIT_SEQ[] =
{
    {0x01,  0, {0}, 150},                                                        /* 软复位 */
    {0x11,  0, {0}, 120},                                                        /* 退出睡眠 */
    {0x3A,  1, {0x55}, 0},                                                       /* 像素格式 16bit */
    {0x36,  1, {0x00}, 0},                                                       /* 扫描方向，画面反了改这里 */
    {0xB2,  5, {0x0C, 0x0C, 0x00, 0x33, 0x33}, 0},                               /* 消隐区 */
    {0xB7,  1, {0x35}, 0},                                                       /* 门控 */
    {0xBB,  1, {0x19}, 0},                                                       /* VCOM */
    {0xC0,  1, {0x2C}, 0},                                                       /* 电源控制 */
    {0xC2,  1, {0x01}, 0},
    {0xC3,  1, {0x12}, 0},
    {0xC4,  1, {0x20}, 0},
    {0xC6,  1, {0x0F}, 0},                                                       /* 帧率 */
    {0xD0,  2, {0xA4, 0xA1}, 0},                                                 /* 电源控制 */
    {0xE0, 14, {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54,
                0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23}, 0},                        /* 正极性 gamma */
    {0xE1, 14, {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44,
                0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23}, 0},                        /* 负极性 gamma */
    {0x21,  0, {0}, 0},                                                          /* IPS 屏需要反显 */
    {0x13,  0, {0}, 10},                                                         /* 正常显示 */
    {0x29,  0, {0}, 100},                                                        /* 开显示 */
};

static void lcdInit(void)
{
    uint8_t i;

    for (i = 0; i < sizeof(LCD_INIT_SEQ) / sizeof(LCD_INIT_SEQ[0]); i++)
    {
        const LcdInitItem *item = &LCD_INIT_SEQ[i];
        lcdOpen();
        lcdSendCmdData(item->cmd, item->data, item->len);
        lcdClose();
        if (item->delayMs) HAL_Delay(item->delayMs);
    }
}

/* ============================ 5. 界面字段 ============================ */
/* 每个字段只重画“变化的那几个字符”，所以刷新开销很小 */
#define UI_HEADER_H         24
#define UI_FOOTER_Y         208
#define UI_REFRESH_MS       100

enum
{
    UI_F_CHA = 0,   /* 通道 A 频率 */
    UI_F_CHB,       /* 通道 B 频率 */
    UI_F_WAVA,      /* 通道 A 波形 */
    UI_F_WAVB,      /* 通道 B 波形 */
    UI_F_PHASE,     /* 相位设定 */
    UI_F_MODE,      /* 工作模式 */
    UI_F_STATE,     /* 运行状态 */
    UI_F_OFS,       /* 频偏（底部小字） */
    UI_F_NUM
};

typedef struct
{
    uint16_t x;
    uint16_t y;
    uint8_t  scale;
    uint8_t  maxLen;
    uint16_t color;
    uint16_t bg;
    char     next[20];      /* 待显示内容 */
    char     last[20];      /* 已经画上去的内容 */
    uint8_t  dirty;
} UiField;

static UiField g_uiField[UI_F_NUM] =
{
    { 92,  32, 2, 12, UI_COL_FREQ,  UI_COL_BG,     {0}, {0}, 1},   /* UI_F_CHA  */
    { 92,  84, 2, 12, UI_COL_FREQ,  UI_COL_BG,     {0}, {0}, 1},   /* UI_F_CHB  */
    { 92,  58, 2,  9, UI_COL_WAVE,  UI_COL_BG,     {0}, {0}, 1},   /* UI_F_WAVA */
    { 92, 110, 2,  9, UI_COL_WAVE,  UI_COL_BG,     {0}, {0}, 1},   /* UI_F_WAVB */
    { 80, 136, 2, 12, UI_COL_PHASE, UI_COL_BG,     {0}, {0}, 1},   /* UI_F_PHASE*/
    { 80, 162, 2,  8, UI_COL_MODE,  UI_COL_BG,     {0}, {0}, 1},   /* UI_F_MODE */
    { 80, 188, 2,  8, UI_COL_STATE, UI_COL_BG,     {0}, {0}, 1},   /* UI_F_STATE*/
    {  4, 226, 1, 19, UI_COL_HINT,  UI_COL_FOOTER, {0}, {0}, 1},   /* UI_F_OFS  */
};

static const struct
{
    uint16_t    x;
    uint16_t    y;
    const char *txt;
} UI_LABEL[] =
{
    {4,  32, "A FREQ:"},
    {4,  58, "  WAVE:"},
    {4,  84, "B FREQ:"},
    {4, 110, "  WAVE:"},
    {4, 136, "PHASE:"},
    {4, 162, "MODE:"},
    {4, 188, "STATE:"},
};

/* 显示缓存：由 uiSendWaveInf / uiSendOffset 写入 */
static uint32_t g_uiFreq[2]     = {0, 0};
static uint8_t  g_uiWave[2]     = {0, 0};
static int8_t   g_uiOffset[2]   = {0, 0};
static uint8_t  g_uiHasData     = 0;

/* 采样状态机里的全局变量，用于显示模式/状态/相位 */
extern SampleState  g_sampleState;
extern WorkMode     g_workMode;
extern PhaseLockState g_phaseLockState;
extern float        g_firstPhase;

static void uiFieldSet(uint8_t idx, const char *text)
{
    UiField *f = &g_uiField[idx];
    uint8_t  len = (uint8_t)strlen(text);

    if (len > f->maxLen) len = f->maxLen;
    memcpy(f->next, text, len);
    f->next[len] = '\0';
    if (strcmp(f->next, f->last) != 0) f->dirty = 1;
}

/* 只重画 old -> new 之间发生变化的字符 */
static void uiFieldFlush(uint8_t idx)
{
    UiField *f = &g_uiField[idx];
    uint8_t  oldLen = (uint8_t)strlen(f->last);
    uint8_t  newLen = (uint8_t)strlen(f->next);
    uint8_t  first = 0;
    uint8_t  lastLen;
    char     patch[24];
    uint8_t  i;

    while ((first < oldLen) && (first < newLen) && (f->last[first] == f->next[first])) first++;
    lastLen = (oldLen > newLen) ? oldLen : newLen;

    if (lastLen > first)
    {
        for (i = first; i < lastLen; i++)
        {
            patch[i - first] = (i < newLen) ? f->next[i] : ' ';   /* 多出来的补空格 */
        }
        patch[lastLen - first] = '\0';
        lcdDrawText((uint16_t)(f->x + first * LCD_CHAR_PITCH(f->scale)), f->y,
                    patch, f->scale, f->color, f->bg);
    }

    memcpy(f->last, f->next, sizeof(f->last));
    f->dirty = 0;
}

static void uiFmtSigned(char *dst, int8_t v)
{
    int t = (int)v;
    if (t < 0) { *dst++ = '-'; t = -t; }
    else       { *dst++ = '+'; }
    sprintf(dst, "%d", t);
}

static const char *uiStateText(void)
{
    switch (g_sampleState)
    {
        case SAMPLE_IDLE:                                   return "IDLE";
        case SAMPLE_INIT:
        case SAMPLEING:                                     return "SAMPLING";
        case SAMPLE_FINISH:
        case FFT_FINISH:                                    return "FFT";
        case GET_WARE_FINISH:                               return "SETUP";
        case PHASE_LOCK:
            switch (g_phaseLockState)
            {
                case PHASE_LOCKING:                         return "LOCKING";
                case PHASE_LOCKED:                          return "LOCKED";
                case GOT_PHASE_DIFF:                        return "TRACK";
                default:                                    return "LOCK OK";
            }
        case PHASE_OVER:                                    return "DONE";
        case TEST_STATUS:                                   return "TEST";
        case AUTO_SETOFFSET:                                return "CALIB";
        default:                                            return "---";
    }
}

static void uiBuildFields(void)
{
    char    buf[24];
    char    sa[8], sb[8];
    uint8_t i;
    uint32_t whole, frac;
    uint16_t phaseDeg;

    for (i = 0; i < 2; i++)
    {
        if (g_uiHasData)
        {
            whole = g_uiFreq[i] / 1000;
            frac  = (g_uiFreq[i] % 1000 + 5) / 10;      /* 两位小数，四舍五入 */
            if (frac >= 100) { frac = 0; whole++; }
            sprintf(buf, "%lu.%02luKHZ", (unsigned long)whole, (unsigned long)frac);
        }
        else
        {
            strcpy(buf, "--.--KHZ");
        }
        uiFieldSet((i == 0) ? UI_F_CHA : UI_F_CHB, buf);
        uiFieldSet((i == 0) ? UI_F_WAVA : UI_F_WAVB,
                   (g_uiWave[i] == TRIANGLE) ? "TRIANGLE" : "SINE");
    }

    phaseDeg = (uint16_t)(g_firstPhase * (180.0f / 3.14159265f) + 0.5f) % 360;
    sprintf(buf, "%u DEG %s", (unsigned int)phaseDeg,
            (g_workMode == PHASE_MODE) ? "ON" : "OFF");
    uiFieldSet(UI_F_PHASE, buf);

    switch (g_workMode)
    {
        case PHASE_MODE: uiFieldSet(UI_F_MODE, "PHASE");  break;
        case Test_MODE:  uiFieldSet(UI_F_MODE, "TEST");   break;
        default:         uiFieldSet(UI_F_MODE, "NORMAL"); break;
    }

    uiFieldSet(UI_F_STATE, uiStateText());

    uiFmtSigned(sa, g_uiOffset[0]);
    uiFmtSigned(sb, g_uiOffset[1]);
    sprintf(buf, "OFS A%s B%s", sa, sb);
    uiFieldSet(UI_F_OFS, buf);
}

/* ============================ 6. 对外接口 ============================ */
void uiControlInit(void)
{
    uint8_t i;

    LCD_CS_HIGH();
    LCD_DC_DATA();
    LCD_BL_OFF();

    lcdGpioSpeedUp();
    lcdInit();

    lcdFillRect(0, 0, LCD_W, LCD_H, UI_COL_BG);
    lcdFillRect(0, 0, LCD_W, UI_HEADER_H, UI_COL_HEADER);
    lcdDrawText(24, 5, "SIGNAL SEPARATOR", 2, 0xFFFF, UI_COL_HEADER);
    lcdFillRect(0, UI_FOOTER_Y, LCD_W, LCD_H - UI_FOOTER_Y, UI_COL_FOOTER);
    lcdDrawText(4, UI_FOOTER_Y + 4, "K1:RUN K2:PH+ HOLD:K1=MODE K2=PH-", 1, UI_COL_HINT, UI_COL_FOOTER);

    for (i = 0; i < sizeof(UI_LABEL) / sizeof(UI_LABEL[0]); i++)
    {
        lcdDrawText(UI_LABEL[i].x, UI_LABEL[i].y, UI_LABEL[i].txt, 2, UI_COL_LABEL, UI_COL_BG);
    }

    LCD_BL_ON();
}

void uiControlLoop(void)
{
    static uint32_t lastTick = 0;
    uint32_t now = HAL_GetTick();
    uint8_t  i;

    if (now - lastTick < UI_REFRESH_MS) return;
    lastTick = now;

    uiBuildFields();

    for (i = 0; i < UI_F_NUM; i++)
    {
        if (!g_uiField[i].dirty) continue;
        /* 锁相环部分对时序敏感，测量通道的频率/波形等锁相结束再刷 */
        if ((g_sampleState == PHASE_LOCK) &&
            ((i == UI_F_CHA) || (i == UI_F_CHB) || (i == UI_F_WAVA) || (i == UI_F_WAVB)))
        {
            continue;
        }
        uiFieldFlush(i);
    }

    /* 状态灯：LED1 采样/FFT 常亮、锁相时闪烁；LED2 相位模式常亮 */
    {
        GPIO_PinState led1;

        switch (g_sampleState)
        {
            case SAMPLE_IDLE:
            case PHASE_OVER:
                led1 = GPIO_PIN_RESET;
                break;
            case PHASE_LOCK:
                led1 = ((now / 100) & 1) ? GPIO_PIN_SET : GPIO_PIN_RESET;
                break;
            default:
                led1 = GPIO_PIN_SET;
                break;
        }
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, led1);
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin,
                          (g_workMode == PHASE_MODE) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

void uiSendWaveInf(uint32_t wave1Freq, uint32_t wave2Freq, uint8_t wave1Type, uint8_t wave2Type)
{
    g_uiFreq[0] = wave1Freq;
    g_uiFreq[1] = wave2Freq;
    g_uiWave[0] = wave1Type;
    g_uiWave[1] = wave2Type;
    /* 全 0 表示清空（开始新一轮测量时调用） */
    g_uiHasData = ((wave1Freq == 0) && (wave2Freq == 0)) ? 0 : 1;
}

void uiSendOffset(int8_t offset1, int8_t offset2)
{
    g_uiOffset[0] = offset1;
    g_uiOffset[1] = offset2;
}

/* ============================ 7. 液晶自检 ============================ */
/*
 * 把 ui_control.h 里的 LCD_SELFTEST 置 1，上电就会跑这个自检（不会返回）。
 * 自检通过的样子：
 *   1) 红/绿/蓝/白/黑五个纯色画面干净、没有杂色条纹；
 *   2) 红色边框四条边都贴着屏幕边缘，四个角有绿色方块（少一条边说明有坐标偏移）；
 *   3) 16 级灰阶从黑到白平滑过渡；
 *   4) 棋盘格边缘锐利，出现摩尔纹说明像素寻址错位；
 *   5) 字库表和 1 倍/2 倍字号都完整，没有缺字或乱码；
 *   6) 刷屏速度打印出来接近 8Mbit/s（满屏 240*240*2 字节约 115~130ms）。
 * 最后停在持续刷新页：秒计数在动、蓝色方块在弹，就说明 SPI 一直在正常通信。
 */
void uiLcdTest(void)
{
    char     buf[24];
    uint16_t i, x, y;
    uint32_t t0, ms, mbps_x10, cnt = 0;
    int16_t  bx = 8, by = 8, dx = 4, dy = 3;

    /* ---------- 1. 纯色满屏：检查颜色通道、坏点 ---------- */
    lcdFillRect(0, 0, LCD_W, LCD_H, 0xF800); HAL_Delay(400);   /* 红 */
    lcdFillRect(0, 0, LCD_W, LCD_H, 0x07E0); HAL_Delay(400);   /* 绿 */
    lcdFillRect(0, 0, LCD_W, LCD_H, 0x001F); HAL_Delay(400);   /* 蓝 */
    lcdFillRect(0, 0, LCD_W, LCD_H, 0xFFFF); HAL_Delay(400);   /* 白 */
    lcdFillRect(0, 0, LCD_W, LCD_H, 0x0000); HAL_Delay(400);   /* 黑 */

    /* ---------- 2. 边框 + 四角：检查坐标原点和偏移 ---------- */
    lcdFillRect(0, 0, LCD_W, 3, 0xF800);
    lcdFillRect(0, LCD_H - 3, LCD_W, 3, 0xF800);
    lcdFillRect(0, 0, 3, LCD_H, 0xF800);
    lcdFillRect(LCD_W - 3, 0, 3, LCD_H, 0xF800);
    lcdFillRect(0, 0, 20, 20, 0x07E0);
    lcdFillRect(LCD_W - 20, 0, 20, 20, 0x07E0);
    lcdFillRect(0, LCD_H - 20, 20, 20, 0x07E0);
    lcdFillRect(LCD_W - 20, LCD_H - 20, 20, 20, 0x07E0);
    lcdDrawText(54, 100, "BORDER", 2, 0xFFFF, 0x0000);
    HAL_Delay(1500);

    /* ---------- 3. 16 级灰阶条 ---------- */
    lcdFillRect(0, 0, LCD_W, LCD_H, 0x0000);
    lcdDrawText(4, 10, "GRAY 16", 2, 0xFFFF, 0x0000);
    for (i = 0; i < 16; i++)
    {
        uint8_t  g = (uint8_t)(i * 255 / 15);
        uint16_t c = (uint16_t)(((g >> 3) << 11) | ((g >> 2) << 5) | (g >> 3));   /* RGB565 */
        lcdFillRect((uint16_t)(i * 15), 40, 15, 80, c);
    }
    HAL_Delay(1500);

    /* ---------- 4. 三色竖条 ---------- */
    lcdFillRect(0, 0, LCD_W, LCD_H, 0x0000);
    for (i = 0; i < 6; i++)
    {
        uint16_t c = ((i % 3) == 0) ? 0xF800 : (((i % 3) == 1) ? 0x07E0 : 0x001F);
        lcdFillRect((uint16_t)(i * 40), 160, 40, 80, c);
    }
    lcdDrawText(4, 10, "RGB BAR", 2, 0xFFFF, 0x0000);
    HAL_Delay(1200);

    /* ---------- 5. 棋盘格：检查像素寻址 ---------- */
    lcdFillRect(0, 0, LCD_W, LCD_H, 0x0000);
    for (y = 0; y < LCD_H; y += 16)
    {
        for (x = 0; x < LCD_W; x += 16)
        {
            lcdFillRect((uint16_t)(x + 8), (uint16_t)(y + 8), 8, 8, 0xFFFF);
        }
    }
    HAL_Delay(1200);

    /* ---------- 6. 对角线：逐点寻址检查 ---------- */
    lcdFillRect(0, 0, LCD_W, LCD_H, 0x0000);
    for (i = 0; i < LCD_W; i++)
    {
        lcdFillRect(i, i, 1, 1, 0x07FF);
    }
    lcdDrawText(4, 10, "DIAGONAL", 2, 0xFFFF, 0x0000);
    HAL_Delay(1200);

    /* ---------- 7. 字库：全字符表 + 两种字号 ---------- */
    lcdFillRect(0, 0, LCD_W, LCD_H, 0x0000);
    for (i = 0; i < LCD_FONT_NUM; i++)
    {
        uint16_t cx = (uint16_t)((i % 20) * 12);
        uint16_t cy = (uint16_t)(4 + (i / 20) * 12);
        lcdDrawChar(cx, cy, LCD_FONT[i].ch, 1, 0xFFFF, 0x0000);
    }
    lcdDrawText(4, 46, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 1, 0x07FF, 0x0000);
    lcdDrawText(4, 60, "0123456789 +-*/.:%=", 2, 0xFFE0, 0x0000);
    lcdDrawText(4, 86, "20.50KHZ SINE", 2, 0x07E0, 0x0000);
    HAL_Delay(2000);

    /* ---------- 8. 实测刷屏速度 ---------- */
    lcdFillRect(0, 0, LCD_W, LCD_H, 0x0000);
    t0 = HAL_GetTick();
    lcdFillRect(0, 0, LCD_W, LCD_H, 0x001F);
    ms = HAL_GetTick() - t0;
    if (ms == 0) ms = 1;
    mbps_x10 = 9216UL / ms;                 /* 240*240*2*8 位，换算成 0.1Mbit/s */
    sprintf(buf, "%luMS", (unsigned long)ms);
    lcdDrawText(4, 60, "FILL", 2, 0xFFFF, 0x001F);
    lcdDrawText(4, 84, buf, 2, 0xFFFF, 0x001F);
    sprintf(buf, "%lu.%luMBPS", (unsigned long)(mbps_x10 / 10), (unsigned long)(mbps_x10 % 10));
    lcdDrawText(4, 108, buf, 2, 0xFFFF, 0x001F);
    HAL_Delay(2000);

    /* ---------- 9. 持续刷新页 ---------- */
    lcdFillRect(0, 0, LCD_W, LCD_H, 0x0000);
    lcdDrawText(40, 4, "LCD TEST OK", 2, 0x07E0, 0x0000);
    t0 = HAL_GetTick();
    for (;;)
    {
        if ((HAL_GetTick() - t0) >= 1000)
        {
            t0 = HAL_GetTick();
            cnt++;
            sprintf(buf, "RUN %lu S", (unsigned long)cnt);
            lcdDrawText(40, 30, buf, 2, 0xFFFF, 0x0000);
        }

        /* 弹跳方块：全程都在动，说明 SPI 通信正常 */
        lcdFillRect((uint16_t)bx, (uint16_t)by, 20, 20, 0x0000);
        bx = (int16_t)(bx + dx);
        by = (int16_t)(by + dy);
        if (bx < 0)              { bx = 0;              dx = (int16_t)(-dx); }
        if (bx > LCD_W - 20)     { bx = LCD_W - 20;     dx = (int16_t)(-dx); }
        if (by < 0)              { by = 0;              dy = (int16_t)(-dy); }
        if (by > LCD_H - 20)     { by = LCD_H - 20;     dy = (int16_t)(-dy); }
        lcdFillRect((uint16_t)bx, (uint16_t)by, 20, 20, 0x07FF);
        HAL_Delay(10);
    }
}

#else  /* !UI_LCD_ENABLE：新板没有液晶，改用 LED1(PE3) / LED2(PE5) 做状态指示 */

#include "main.h"

/*
 * LED1(PE3) 状态灯：
 *   空闲/结束           灭
 *   采样、FFT、参数设置 常亮
 *   锁相环工作中        5Hz 闪烁
 * LED2(PE5) 相位模式灯：WorkMode = PHASE_MODE 时常亮
 */
extern SampleState g_sampleState;
extern WorkMode    g_workMode;

/* 初始化（没有液晶，只要把两个灯先关掉） */
void uiControlInit(void)
{
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
}

/* 主循环里调用，50ms 刷新一次灯 */
void uiControlLoop(void)
{
    static uint32_t lastTick = 0;
    uint32_t now = HAL_GetTick();
    GPIO_PinState led1;

    if ((now - lastTick) < 50) return;
    lastTick = now;

    switch (g_sampleState)
    {
        case SAMPLE_IDLE:
        case PHASE_OVER:
            led1 = GPIO_PIN_RESET;
            break;
        case PHASE_LOCK:
            led1 = ((now / 100) & 1) ? GPIO_PIN_SET : GPIO_PIN_RESET;
            break;
        default:
            led1 = GPIO_PIN_SET;
            break;
    }

    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, led1);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin,
                      (g_workMode == PHASE_MODE) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* 接口与液晶版本保持一致：没有屏就不显示，sample.c 不用改 */
void uiSendWaveInf(uint32_t wave1Freq, uint32_t wave2Freq, uint8_t wave1Type, uint8_t wave2Type)
{
    (void)wave1Freq;
    (void)wave2Freq;
    (void)wave1Type;
    (void)wave2Type;
}

void uiSendOffset(int8_t offset1, int8_t offset2)
{
    (void)offset1;
    (void)offset2;
}

/* 没有液晶，自检函数留空 */
void uiLcdTest(void)
{
}

#endif /* UI_LCD_ENABLE */
