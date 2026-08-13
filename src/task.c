#include "../include/task.h"
#include "../include/port.h"

/* 定义全局指针：指向当前正在运行的任务 TCB */
TCB_t *volatile pxCurrentTCB = NULL;

/* 核心数据结构：就绪任务链表数组 (优先级从 0 到configMAX_PRIORITIES - 1) */
List_t pxReadyTasksLists[configMAX_PRIORITIES];

/* 初始化所有优先级的就绪链表 */
void prvInitialiseTaskLists(void) {
  UBaseType_t uxPriority;
  for (uxPriority = (UBaseType_t)0U;
       uxPriority < (UBaseType_t)configMAX_PRIORITIES; uxPriority++) {
    vListInitialise(&(pxReadyTasksLists[uxPriority]));
  }
}

/* 静态创建任务函数 */
TaskHandle_t xTaskCreateStatic(TaskFunction_t pxTaskCode,
                               const char *const pcName,
                               const uint32_t ulStackDepth,
                               void *const pvParameters, UBaseType_t uxPriority,
                               StackType_t *const puxStackBuffer,
                               TCB_t *const pxTaskBuffer) {
  TCB_t *pxNewTCB;
  StackType_t *pxTopOfStack;
  UBaseType_t x;

  /* 1. 安全检查：确保用户传入的 TCB 和 栈数组指针不为空 */
  if ((pxTaskBuffer != NULL) && (puxStackBuffer != NULL)) {
    pxNewTCB = pxTaskBuffer;
    pxNewTCB->pxStack = puxStackBuffer;

    if (uxPriority >= configMAX_PRIORITIES) {
      uxPriority = configMAX_PRIORITIES - 1;
    }

    pxNewTCB->uxPriority = uxPriority;

    /* 2. 计算初始栈顶物理地址（满递减栈：栈顶为数组最后一个元素） */
    pxTopOfStack = &(puxStackBuffer[ulStackDepth - (uint32_t)1]);

    /* ⚠️ 核心点：ARM Cortex-M 架构要求栈顶必须 8 字节对齐，清除低 3 位 */
    pxTopOfStack =
        (StackType_t *)(((uintptr_t)pxTopOfStack) & (~((uintptr_t)0x0007)));

    /* 3. 安全复制任务名称 */
    if (pcName != NULL) {
      for (x = (UBaseType_t)0; x < (UBaseType_t)15; x++) {
        pxNewTCB->pcTaskName[x] = pcName[x];
        if (pcName[x] == (char)0) {
          break;
        }
      }
      pxNewTCB->pcTaskName[15] = '\0';
    }

    /* 4. 初始化 TCB 内部的链表节点，并将节点的 Owner 绑定为当前 TCB */
    vListInitialiseItem(&(pxNewTCB->xStateListItem));
    listSET_LIST_ITEM_OWNER(&(pxNewTCB->xStateListItem), pxNewTCB);

    /* 5. 调用移植层硬件接口伪造寄存器，并将返回的最新栈顶赋值给 TCB
     * 的pxTopOfStack */
    pxNewTCB->pxTopOfStack =
        pxPortInitialiseStack(pxTopOfStack, pxTaskCode, pvParameters);

    /* 挂载到就绪链表 */
    vListInsertEnd(&(pxReadyTasksLists[uxPriority]),
                   &(pxNewTCB->xStateListItem));

    /* 更新最高优先级当前任务指针 */
    if ((pxCurrentTCB == NULL) || (pxCurrentTCB->uxPriority < uxPriority)) {
      pxCurrentTCB = pxNewTCB;
    }
  } else {
    pxNewTCB = NULL;
  }

  /* 6. 返回任务句柄 (TCB 指针) */
  return (TaskHandle_t)pxNewTCB;
}

/* 定义空闲任务的静态 TCB 与栈空间 */
static TCB_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

/* 空闲任务函数：当无其他任务就绪时运行 */
static void prvIdleTask(void *pvParameters) {
  (void)pvParameters;
  for (;;) {
    /* 空闲任务死循环(以后可以在这里加入低功耗休眠指令 WFI) */
  }
}

/* 启动调度器 */
void vTaskStartScheduler(void) {
  /* 1. 初始化就绪链表数组 (若尚未初始化) */
  prvInitialiseTaskLists();
  /* 1. 静态创建最低优先级 (0) 的空闲任务 */
  xTaskCreateStatic(prvIdleTask, "IDLE", configMINIMAL_STACK_SIZE, NULL,
                    (UBaseType_t)0U, uxIdleTaskStack, &xIdleTaskTCB);
  /* 2. 硬件层启动调度器 (初始化 SysTick定时器并启动第一个任务) */
  /* 稍后我们将在 port.c 中实现xPortStartScheduler() */
  if (xPortStartScheduler() != 0) {
    /* 启动成功，此后 CPU 将接管运行任务 */
  }
}
