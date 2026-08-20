#ifndef TASK_H
#define TASK_H

#include "./FreeRTOSConfig.h"
#include "./list.h"
#include <stdint.h>

#define pdPASS ((BaseType_t)1)
#define pdFAIL ((BaseType_t)0)
#define errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY ((BaseType_t) - 1)

/* 1. 架构相关的栈数据类型 (32位 ARM Cortex-M 架构，栈为 32 位/4字节对齐) */
typedef uint32_t StackType_t;
/* 2. 任务函数指针类型 (比如 void vTask1(void *pvParameters)) */
typedef void (*TaskFunction_t)(void *);
/* 3. 任务控制块结构体 (TCB - Task Control Block) */
typedef struct tskTCB {
  /* ⚠️ 必须是结构体的第 0 个成员！指向当前任务栈顶 */
  volatile StackType_t *pxTopOfStack;

  /* 状态链表节点（用于挂在就绪链表或延时链表上）*/
  ListItem_t xStateListItem;

  /* 事件链表节点（用于挂在队列或信号量的等待链表上）*/
  ListItem_t xEventListItem;

  /* 任务优先级 (0 为最低优先级) */
  UBaseType_t uxPriority;

  /* 指向任务栈内存块的起始地址 */
  StackType_t *pxStack;

  /* 任务名称字符串 (方便调试) */
  char pcTaskName[16];
} TCB_t;

/* 4. 声明全局变量：指向当前正在运行的任务 TCB */
extern TCB_t *volatile pxCurrentTCB;

/* 5. 任务句柄类型 (实际上就是指向 TCB 的万能指针 void *) */
typedef void *TaskHandle_t;

/* 6. 静态创建任务 API 声明 */
TaskHandle_t xTaskCreateStatic(TaskFunction_t pxTaskCode,
                               const char *const pcName,
                               const uint32_t ulStackDepth,
                               void *const pvParameters, UBaseType_t uxPriority,
                               StackType_t *const puxStackBuffer,
                               TCB_t *const pxTaskBuffer);

/* 12. 动态创建任务 API 声明 */
BaseType_t xTaskCreate(TaskFunction_t pxTaskCode, const char *const pcName,
                       const uint16_t usStackDepth, void *const pvParameters,
                       UBaseType_t uxPriority,
                       TaskHandle_t *const pxCreatedTask);

/* 13. 动态删除任务 API 声明 */
void vTaskDelete(TaskHandle_t xTaskToDelete);

/* 7. 初始化调度器列表 (初始化就绪链表数组) */
void prvInitialiseTaskLists(void);

/* 8. 启动任务调度器 */
void vTaskStartScheduler(void);

/* 9. 任务延时函数 (单位: SysTick 滴答数) */
void vTaskDelay(const TickType_t xTicksToWait);

/* 10. 滴答定时器中断处理：时间自增与到期任务唤醒 */
BaseType_t xTaskIncrementTick(void);

/* 11. 任务上下文切换选择器 (挑选下一个最高优/轮转任务) */
void vTaskSwitchContext(void);

/* 14. 将当前任务挂载到事件阻塞链表（如队列的等待链表） */
void vTaskPlaceOnEventList(List_t *const pxEventList,
                           const TickType_t xTicksToWait);

/* 15. 从事件阻塞链表唤醒最高优先级的任务，返回是否需要触发上下文切换 */
BaseType_t xTaskRemoveFromEventList(const List_t *const pxEventList);

#endif /* TASK_H */
