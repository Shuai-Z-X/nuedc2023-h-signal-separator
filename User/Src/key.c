/*
 * @Author: wzccccccc
 * @Date: 2023-08-30 16:58:21
 * @LastEditors: wzccccccc wu854065020@gmail.com
 * @FilePath: \signal_separator\User\Src\key.c
 *
 * 两个按键的扫描（原理图 2026-09-11：KEY1=PC6、KEY2=PC8，按下为低电平）。
 * KEY1_GPIO_Port / KEY1_Pin 这些宏由 CubeMX 按标签生成，所以换引脚只需在 CubeMX 里改。
 * 只有两个按键，短按/长按分别对应原来的四个功能：
 *   KEY1 短按 -> KEY_START（启动/重新测量）   长按 -> KEY_PHASE_TOGGLE（相位模式 开/关）
 *   KEY2 短按 -> KEY_PHASE_INC（相位 +15°）   长按 -> KEY_PHASE_DEC（相位 -15°）
 */
#include "key.h"

/* 原始电平位：bit0 = KEY1(PC6)，bit1 = KEY2(PC8)，按下为低电平 */
#define KEY1_RAW 0x01
#define KEY2_RAW 0x02

/* 读当前按下的按键（一端接 GND，按下为低电平） */
static uint8_t keyRawRead(void)
{
    uint8_t raw = 0;

    if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET) raw |= KEY1_RAW;
    if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET) raw |= KEY2_RAW;

    return raw;
}

/* 松手（短按）产生的事件 */
static uint8_t keyShortEvent(uint8_t raw)
{
    uint8_t event = 0;

    if (raw & KEY1_RAW) event |= KEY_START;
    if (raw & KEY2_RAW) event |= KEY_PHASE_INC;

    return event;
}

/* 按住超时（长按）产生的事件 */
static uint8_t keyLongEvent(uint8_t raw)
{
    uint8_t event = 0;

    if (raw & KEY1_RAW) event |= KEY_PHASE_TOGGLE;
    if (raw & KEY2_RAW) event |= KEY_PHASE_DEC;

    return event;
}

/**
 * @description: 按键扫描（10ms 消抖 + 短按/长按区分）
 *               没有按键变化时立即返回，不占用主循环时间
 * @return {*} 本次新产生的按键事件位图，没有事件返回 0
 */
uint8_t key_scan(void)
{
    static uint8_t  stable = 0;     /* 已消抖的按下状态 */
    static uint32_t pressTick = 0;  /* 按下的时刻 */
    static uint8_t  longSent = 0;   /* 长按事件只报一次 */
    uint8_t raw;
    uint8_t event = 0;

    raw = keyRawRead();
    if (raw != stable)
    {
        HAL_Delay(10);                      /* 消抖 */
        raw = keyRawRead();
        if (raw == stable) return 0;        /* 是抖动，丢掉 */

        if (raw != 0)                       /* 有新按下：重新计时 */
        {
            pressTick = HAL_GetTick();
            longSent  = 0;
        }
        else                                /* 全部松手：补发短按事件 */
        {
            if (!longSent) event = keyShortEvent(stable);
        }
        stable = raw;
        return event;
    }

    /* 电平没变化：看是不是按住超过长按时间了 */
    if ((raw != 0) && (!longSent) && ((HAL_GetTick() - pressTick) >= KEY_LONG_MS))
    {
        longSent = 1;
        event = keyLongEvent(raw);
    }

    return event;
}
