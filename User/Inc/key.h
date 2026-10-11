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
