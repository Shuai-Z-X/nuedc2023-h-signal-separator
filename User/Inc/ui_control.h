/*
 
 * 人机界面：240x240 SPI 液晶（ST7789），旧版接口
 *   PB3  -> SPI1_SCK      PB5  -> SPI1_MOSI
 *   PA15 -> LCD_CS        PD13 -> LCD_DC       PD12 -> LCD_BL
 * 液晶走 SPI1（模式0/8MHz），新板的双路 AD9833 走 SPI3（PC10/PC12，模式2/4MHz），
 * 两个 SPI 互相独立，不再抢总线。
 * 状态灯 LED1(PE3)/LED2(PE5) 与按键 KEY1(PC6)/KEY2(PC8) 见 key.c。
 */
#ifndef UI_CONTROL_H_
#define UI_CONTROL_H_

#include "stm32f4xx_hal.h"

/* 液晶开关：1 = 启用 ST7789 液晶界面；0 = 关掉液晶驱动，只用 LED 指示 */
#define UI_LCD_ENABLE       1

/* 液晶自检开关：
 *   1 = 上电后跑自检画面（纯色 / 边框 / 灰阶 / 棋盘 / 字库 / 刷屏速度），
 *       跑完停在持续刷新页，不会进入正常测量流程；测完改回 0 即可。
 *   0 = 正常工作模式。 */
#define LCD_SELFTEST        0

/* 初始化液晶并绘制静态界面，在 main 里外设初始化之后调用 */
void uiControlInit(void);

/* 主循环里调用，只刷新发生变化的内容（内部限速，可以每轮都调） */
void uiControlLoop(void);

/* 下面两个函数与原串口屏版本完全一致，sample.c 无需改动 */
/* 传 (0,0,0,0) 表示清空显示 */
void uiSendWaveInf(uint32_t wave1Freq, uint32_t wave2Freq, uint8_t wave1Type, uint8_t wave2Type);
void uiSendOffset(int8_t offset1, int8_t offset2);

/* 液晶自检程序，需先调用 uiControlInit()；调用后不会返回 */
void uiLcdTest(void);

#endif /* UI_CONTROL_H_ */
