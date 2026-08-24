#ifndef PORT_H
#define PORT_H

#include "task.h"
#include <stdint.h>

/* 1. 声明硬件栈初始化接口 (伪造 ARM Cortex-M 架构 16 个寄存器) */
StackType_t *pxPortInitialiseStack(StackType_t *pxTopOfStack,
                                   TaskFunction_t pxCode, void *pvParameters);

/* 2. 硬件层启动调度器接口 */
BaseType_t xPortStartScheduler(void);

/* 3. 临界区硬件层进出接口与嵌套计数器 */
void vPortEnterCritical(void);
void vPortExitCritical(void);
UBaseType_t uxPortGetCriticalNesting(void);

#define portENTER_CRITICAL() vPortEnterCritical()
#define portEXIT_CRITICAL()  vPortExitCritical()

#endif
