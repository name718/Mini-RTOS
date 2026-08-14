#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* 支持的最大优先级数量 (0 ~ 4，共 5 个优先级)*/
#define configMAX_PRIORITIES (5)

/* 最小任务栈空间大小 (字数/Word，即 128 * 4 =512 字节) */
#define configMINIMAL_STACK_SIZE ((uint16_t)128)

/* 系统总动态堆内存大小 (4 * 1024 = 4096字节) */
#define configTOTAL_HEAP_SIZE ((size_t)(4 * 1024))

#endif /* FREERTOS_CONFIG_H */
