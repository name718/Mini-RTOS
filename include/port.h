
#ifndef PORT_H
#define PORT_H

#include "task.h"

/* 硬件移植层接口：初始化任务栈帧 */
StackType_t *pxPortInitialiseStack(StackType_t *pxTopOfStack,
                                   TaskFunction_t pxCode, void *pvParameters);

/* 启动调度器移植层接口 */
BaseType_t xPortStartScheduler(void);

#endif
