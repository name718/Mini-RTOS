#ifndef SEMPHR_H
#define SEMPHR_H

#include "queue.h"

/* 信号量句柄定义 (底层直接复用队列句柄) */
typedef QueueHandle_t SemaphoreHandle_t;

/* 1. 创建二值信号量宏 (容量为 1, 元素大小为 0 字节, 初始无信号) */
#define xSemaphoreCreateBinary() xQueueCreate((UBaseType_t)1U, (UBaseType_t)0U)

/* 2. 创建计数信号量宏 */
#define xSemaphoreCreateCounting(uxMaxCount, uxInitialCount)                   \
  xQueueCreateCounting((uxMaxCount), (uxInitialCount))

/* 3. 创建互斥量宏 (带优先级继承机制，初始拥有 1 把可用锁) */
#define xSemaphoreCreateMutex() xQueueCreateMutex()

/* 4. 释放/给出信号量或互斥量 (Give) */
#define xSemaphoreGive(xSemaphore)                                             \
  xQueueSend((QueueHandle_t)(xSemaphore), NULL, (TickType_t)0U)

/* 5. 获取/等待信号量或互斥量 (Take) */
#define xSemaphoreTake(xSemaphore, xBlockTime)                                 \
  xQueueReceive((QueueHandle_t)(xSemaphore), NULL, (TickType_t)(xBlockTime))

#endif /* SEMPHR_H */
