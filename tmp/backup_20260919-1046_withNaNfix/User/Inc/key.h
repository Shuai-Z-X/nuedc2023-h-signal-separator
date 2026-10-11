/*
 * @Author: wzccccccc
 * @Date: 2023-08-30 16:58:21
 * @LastEditors: wzccccccc wu854065020@gmail.com
 * @FilePath: \signal_separator\User\Inc\key.h
 *
 * 按键输入（按下为低电平，板上 5.1K 上拉到 3.3V，GPIO 内部再上拉）：
 *   KEY1(PC6) 短按 = 启动 / 重新测量，长按 = 相位模式 开 / 关
 *   KEY2(PC8) 短按 = 相位 +15 度，长按 = 相位 -15 度
 * 原理图(2026-09-11)上只有两个按键，原来的 KEY3/KEY4 功能合并成两个长按动作。
 * 引脚在 CubeMX 里改（标签保持 KEY1/KEY2），这里和 key.c 都不用动。
 */
/* 2026-09-11 原理图：只有 KEY1=PC6 / KEY2=PC8 两个按键（SW-PB，5.1K 上拉到 3.3V） */
#ifndef KEY_H_
#define KEY_H_

#include "main.h"

/* key_scan() 的返回值：短按在松手时产生，长按在按住超过 KEY_LONG_MS 时产生 */
#define KEY_START        0x01
#define KEY_PHASE_INC    0x02
#define KEY_PHASE_DEC    0x04
#define KEY_PHASE_TOGGLE 0x08

/* 备用引脚定义：CubeMX 按标签 KEY1/KEY2 生成同名宏时以生成的为准，
   没生成（例如还没重新生成代码）时用下面这份。换引脚改 CubeMX 或这里都行。 */
#ifndef KEY1_Pin
#define KEY1_GPIO_Port  GPIOC
#define KEY1_Pin        GPIO_PIN_6
#endif
#ifndef KEY2_Pin
#define KEY2_GPIO_Port  GPIOC
#define KEY2_Pin        GPIO_PIN_8
#endif

/* 长按判定时间(ms)：按住这么久还没松手就算长按 */
#define KEY_LONG_MS     600

/* 每轮主循环调用一次，返回本次新产生的按键事件位图，没有事件时返回 0 */
uint8_t key_scan(void);

#endif /* KEY_H_ */
