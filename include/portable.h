#ifndef PORTABLE_H
#define PORTABLE_H

#include "FreeRTOSConfig.h"
#include <stddef.h>
#include <stdint.h>

/* 动态内存分配 API (FreeRTOS 版 malloc) */
void *pvPortMalloc(size_t xWantedSize);

/* 动态内存释放 API (FreeRTOS 版 free) */
void vPortFree(void *pv);

/* 获取当前剩余可用空闲堆内存大小 */
size_t xPortGetFreeHeapSize(void);

#endif /* PORTABLE_H */
