#ifndef TASK_H
#define TASK_H

#include "./FreeRTOSConfig.h"
#include "./list.h"
#include <stdint.h>

#define pdPASS  ((BaseType_t)1)
#define pdFAIL  ((BaseType_t)0)
#define pdTRUE  ((BaseType_t)1)
#define pdFALSE ((BaseType_t)0)
#define errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY ((BaseType_t) - 1)

/* 任务通知动作枚举 (Notify Actions) */
typedef enum {
  eNoAction = 0,             /* 不改变通知值，仅标记已收到通知 */
  eSetBits,                  /* 按位或更新通知值 (相当于轻量级事件标志组) */
  eIncrement,                /* 通知值自增 (相当于轻量级计数信号量) */
  eSetValueWithOverwrite,    /* 强制覆盖通知值 (相当于单值邮箱) */
  eSetValueWithoutOverwrite  /* 不覆盖写入，若已有未读通知则返回失败 */
} eNotifyAction;

/* 任务通知状态 */
#define taskNOT_WAITING_NOTIFICATION ((uint8_t)0)
#define taskWAITING_NOTIFICATION     ((uint8_t)1)
#define taskNOTIFICATION_RECEIVED    ((uint8_t)2)

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

  /* 任务当前优先级 (可能因优先级继承而临时提升) */
  UBaseType_t uxPriority;

  /* 任务基准初始优先级 (用于互斥量释放后恢复) */
  UBaseType_t uxBasePriority;

  /* 指向任务栈内存块的起始地址 */
  StackType_t *pxStack;

  /* 任务名称字符串 (方便调试) */
  char pcTaskName[16];

  /* 任务通知内部变量 (Task Notifications: 零堆内存开销轻量级通信) */
  volatile uint32_t ulNotifiedValue;
  volatile uint8_t ucNotifyState;
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

/* 16. 优先级继承：临时提升持有互斥锁任务的优先级 */
void vTaskPriorityInherit(TCB_t *const pxMutexHolder);

/* 17. 优先级恢复：互斥锁释放后恢复持有者的基准优先级 */
BaseType_t xTaskPriorityDisinherit(TCB_t *const pxMutexHolder);

/* 18. 临界区管理 API */
void vTaskEnterCritical(void);
void vTaskExitCritical(void);
#define taskENTER_CRITICAL() vTaskEnterCritical()
#define taskEXIT_CRITICAL()  vTaskExitCritical()

/* 19. 任务通知 API */
BaseType_t xTaskGenericNotify(TaskHandle_t xTaskToNotify, uint32_t ulValue,
                              eNotifyAction eAction,
                              uint32_t *pulPreviousNotificationValue);

#define xTaskNotify(xTaskToNotify, ulValue, eAction)                           \
  xTaskGenericNotify((xTaskToNotify), (ulValue), (eAction), NULL)

#define xTaskNotifyGive(xTaskToNotify)                                         \
  xTaskGenericNotify((xTaskToNotify), (0), eIncrement, NULL)

uint32_t ulTaskNotifyTake(BaseType_t xClearCountOnExit,
                          TickType_t xTicksToWait);

BaseType_t xTaskNotifyWait(uint32_t ulBitsToClearOnEntry,
                           uint32_t ulBitsToClearOnExit,
                           uint32_t *pulNotificationValue,
                           TickType_t xTicksToWait);

#endif /* TASK_H */
