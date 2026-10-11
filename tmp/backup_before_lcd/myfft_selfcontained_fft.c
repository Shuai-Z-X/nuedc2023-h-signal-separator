/*
 * @Author: wzccccccc
 * @Description: FFT 函数。原来用的是 CMSIS-DSP 的 arm_cfft_radix2_*，
 *               现在换成自带的 radix-2 实现：同样原地运算、自然顺序输入输出，
 *               接口（MyFFTInit / MyFFT / ALterFFT / GetFFTMag）完全不变，
 *               这样工程不再依赖 Middlewares 里的 DSP 库和 arm_math.h。
 * @Date: 2023-08-02 09:15:38
 * @LastEditors: wzccccccc wu854065020@gmail.com
 * @FilePath: \signal_separator\User\Src\myfft.c
 */
#include <math.h>
#include "myfft.h"
#include "sample_config.h"

#define FFT_PI        3.14159265358979f

/* 旋转因子表按最大点数生成，256 点 FFT 复用同一张表（1024 能被 256 整除），
   只在 MyFFTInit() 里算一次，占 1024/2*2*4 = 4KB RAM。 */
#define FFT_TABLE_N   FFT_NUM

static float   s_twRe[FFT_TABLE_N / 2];   /* cos(-2*PI*k/N) */
static float   s_twIm[FFT_TABLE_N / 2];   /* sin(-2*PI*k/N) */
static uint8_t s_twReady = 0;

static void fftTwiddleInit(void)
{
    uint16_t k;

    if (s_twReady) return;
    for (k = 0; k < FFT_TABLE_N / 2; k++)
    {
        float ang = -2.0f * FFT_PI * (float)k / (float)FFT_TABLE_N;
        s_twRe[k] = cosf(ang);
        s_twIm[k] = sinf(ang);
    }
    s_twReady = 1;
}

/**
 * @description: 原地 radix-2 复数 FFT（DIT），data 里实虚部交错存放，
 *               输入自然顺序、输出自然顺序，与原 CMSIS 用法一致
 * @param {float} *data 数据缓冲，长度 2*n
 * @param {uint16_t} n  点数，2 的整数次幂且不超过 FFT_TABLE_N
 * @return {*}
 */
static void fftRadix2(float *data, uint16_t n)
{
    uint16_t i, j, k, len, half, step;
    float tr, ti, wr, wi, ur, ui, vr, vi;

    /* 位反转置换 */
    for (i = 1, j = 0; i < n; i++)
    {
        k = (uint16_t)(n >> 1);
        while (j & k)
        {
            j ^= k;
            k >>= 1;
        }
        j |= k;
        if (i < j)
        {
            tr = data[2 * i];     data[2 * i]     = data[2 * j];     data[2 * j]     = tr;
            ti = data[2 * i + 1]; data[2 * i + 1] = data[2 * j + 1]; data[2 * j + 1] = ti;
        }
    }

    /* 蝶形运算：len 为当前级每组长度，逐级加倍 */
    for (len = 2; len <= n; len = (uint16_t)(len << 1))
    {
        half = (uint16_t)(len >> 1);
        step = (uint16_t)(FFT_TABLE_N / len);
        for (i = 0; i < n; i = (uint16_t)(i + len))
        {
            for (j = 0; j < half; j++)
            {
                uint16_t a = (uint16_t)(i + j);
                uint16_t b = (uint16_t)(i + j + half);

                k  = (uint16_t)(j * step);
                wr = s_twRe[k];
                wi = s_twIm[k];

                ur = data[2 * a];
                ui = data[2 * a + 1];
                vr = data[2 * b] * wr - data[2 * b + 1] * wi;
                vi = data[2 * b] * wi + data[2 * b + 1] * wr;

                data[2 * a]     = ur + vr;
                data[2 * a + 1] = ui + vi;
                data[2 * b]     = ur - vr;
                data[2 * b + 1] = ui - vi;
            }
        }
    }
}

/**
 * @description: FFT 初始化，生成旋转因子表
 * @return {*}
 */
void MyFFTInit(void)
{
    fftTwiddleInit();
}

/**
 * @description: 1024点FFT
 * @param {float} *signvolt 传入信号数组，实虚部交错，长度 2*FFT_NUM
 * @return {*}
 */
void MyFFT(float *signvolt)
{
    fftRadix2(signvolt, FFT_NUM);
}

/**
 * @description: 256点FFT（锁相环用）
 * @param {float} *signvolt 传入信号数组，实虚部交错
 * @return {*}
 */
void ALterFFT(float *signvolt)
{
    fftRadix2(signvolt, PHASE_LOCKED_FFT_NUM);
}

/**
 * @description: 幅度谱，取前 FFT_NUM 个点的模，并做归一化
 * @param {float} *input   FFT 结果（实虚部交错）
 * @param {float} *output  幅度谱
 * @return {*}
 */
void GetFFTMag(float *input, float *output)
{
    uint16_t i;

    for (i = 0; i < FFT_NUM; i++)
    {
        float re = input[2 * i];
        float im = input[2 * i + 1];
        output[i] = sqrtf(re * re + im * im);
    }
    output[0] /= FFT_NUM;
    for (i = 1; i < FFT_NUM; i++)
    {
        output[i] /= (FFT_NUM / 2);
    }
}
