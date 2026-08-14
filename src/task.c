#include "../include/task.h"
#include "../include/port.h"

/* 定义全局指针：指向当前正在运行的任务 TCB */
TCB_t *volatile pxCurrentTCB = NULL;

/* 核心数据结构：就绪任务链表数组 (优先级从 0 到configMAX_PRIORITIES - 1) */
List_t pxReadyTasksLists[configMAX_PRIORITIES];

/* 全局滴答时间计数器 */
static volatile TickType_t xTickCount = (TickType_t)0U;

/* 双延时链表实体与指针 */
static List_t xDelayedTaskList1;
static List_t xDelayedTaskList2;
static List_t *volatile pxDelayedTaskList = NULL;
static List_t *volatile pxOverflowDelayedTaskList = NULL;

/* 初始化所有优先级的就绪链表 */
void prvInitialiseTaskLists(void) {
  UBaseType_t uxPriority;
  for (uxPriority = (UBaseType_t)0U;
       uxPriority < (UBaseType_t)configMAX_PRIORITIES; uxPriority++) {
    vListInitialise(&(pxReadyTasksLists[uxPriority]));
  }

  /* 初始化双延时链表 */
  vListInitialise(&xDelayedTaskList1);
  vListInitialise(&xDelayedTaskList2);
  pxDelayedTaskList = &xDelayedTaskList1;
  pxOverflowDelayedTaskList = &xDelayedTaskList2;
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

/* 将当前任务加入延时链表的内部辅助函数 */
static void prvAddCurrentTaskToDelayedList(TickType_t xTicksToWait) {
  TickType_t xTimeToWake;
  const TickType_t xConstTickCount = xTickCount;

  /* 1. 将当前任务从就绪列表中剥离移除 */
  (void)uxListRemove(&(pxCurrentTCB->xStateListItem));

  /* 2. 计算唤醒时刻 */
  xTimeToWake = xConstTickCount + xTicksToWait;

  /* 3. 将节点的 xItemValue 设为唤醒时刻 (供vListInsert 升序排序) */
  listSET_LIST_ITEM_VALUE(&(pxCurrentTCB->xStateListItem), xTimeToWake);

  /* 4. 判断是否发生了 32 位溢出回绕 */
  if (xTimeToWake < xConstTickCount) {
    /* 溢出：插入到溢出延时链表 */
    vListInsert(pxOverflowDelayedTaskList, &(pxCurrentTCB->xStateListItem));
  } else {
    /* 未溢出：插入到当前周期的延时链表 */
    vListInsert(pxDelayedTaskList, &(pxCurrentTCB->xStateListItem));
  }
}

/* 任务延时 API */
void vTaskDelay(const TickType_t xTicksToWait) {
  if (xTicksToWait > (TickType_t)0U) {
    /* 1.将当前任务移出就绪链表，加入延时链表 */
    prvAddCurrentTaskToDelayedList(xTicksToWait);
  }
}

/* 滴答定时器自增与延时唤醒处理 */
BaseType_t xTaskIncrementTick(void) {
  TCB_t *pxTCB;
  TickType_t xItemValue;
  BaseType_t xSwitchRequired = 0;

  /* 1. 自增 32 位 Tick 计数器 */
  const TickType_t xConstTickCount = xTickCount + (TickType_t)1U;
  xTickCount = xConstTickCount;

  /* 2. 关键核心：32位溢出判断与双延时链表指针交换 */
  if (xConstTickCount == (TickType_t)0U) {
    List_t *pxTemp = pxDelayedTaskList;
    pxDelayedTaskList = pxOverflowDelayedTaskList;
    pxOverflowDelayedTaskList = pxTemp;
  }

  /* 3. 检查当前延时链表是否有到期任务 */
  if (listLIST_IS_EMPTY(pxDelayedTaskList) == 0) {
    ListItem_t *pxListItem = listGET_HEAD_ENTRY(pxDelayedTaskList);
    while (pxListItem != listGET_END_MARKER(pxDelayedTaskList)) {
      xItemValue = listGET_LIST_ITEM_VALUE(pxListItem);
      /* 如果当前时间还没到解封时刻，直接退出（链表已升序排列） */
      if (xConstTickCount < xItemValue) {
        break;
      }
      /* 到期！从延时链表剥离 */
      (void)uxListRemove(pxListItem);
      /* 获取 TCB并重新挂载回对应的就绪链表尾部 */
      pxTCB = (TCB_t *)listGET_LIST_ITEM_OWNER(pxListItem);
      vListInsertEnd(&(pxReadyTasksLists[pxTCB->uxPriority]),
                     &(pxTCB->xStateListItem));

      /* 若被唤醒的任务优先级高于或等于当前任务，请求上下文切换 */
      if (pxTCB->uxPriority >= pxCurrentTCB->uxPriority) {
        xSwitchRequired = 1;
      }
      /* 重新获取链表头节点，继续检查下一个 */
      pxListItem = listGET_HEAD_ENTRY(pxDelayedTaskList);
    }
  }
  return xSwitchRequired;
}

/* 上下文切换选择器：选择下一个要运行的任务 */
void vTaskSwitchContext(void) {
  UBaseType_t uxTopPriority =
      (UBaseType_t)configMAX_PRIORITIES - (UBaseType_t)1U;
  /* 1.从最高优先级向下查找第一个非空的就绪链表 */
  while (listLIST_IS_EMPTY(&(pxReadyTasksLists[uxTopPriority])) != 0) {
    uxTopPriority--;
  }
  /* 2. 利用 listGET_OWNER_OF_NEXT_ENTRY宏挪动游标 pxIndex，获取下一个就绪任务
   */
  listGET_OWNER_OF_NEXT_ENTRY(pxCurrentTCB,
                              &(pxReadyTasksLists[uxTopPriority]));
}
