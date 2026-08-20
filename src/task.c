#include "../include/task.h"
#include "../include/port.h"
#include "../include/portable.h"

/* 定义全局指针：指向当前正在运行的任务 TCB */
TCB_t *volatile pxCurrentTCB = NULL;

/* 核心数据结构：就绪任务链表数组 (优先级从 0 到 configMAX_PRIORITIES - 1) */
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

  /* 自动防护：若就绪链表尚未初始化，自动进行初始化 */
  if (pxReadyTasksLists[0].pxIndex == NULL) {
    prvInitialiseTaskLists();
  }

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

    /* 初始化事件链表节点 (其排序值设为反向优先级，确保高优先级任务排在最前) */
    vListInitialiseItem(&(pxNewTCB->xEventListItem));
    listSET_LIST_ITEM_OWNER(&(pxNewTCB->xEventListItem), pxNewTCB);
    listSET_LIST_ITEM_VALUE(
        &(pxNewTCB->xEventListItem),
        (TickType_t)(configMAX_PRIORITIES - (UBaseType_t)1U - uxPriority));

    /* 5. 调用移植层硬件接口伪造寄存器，并将返回的最新栈顶赋值给 TCB 的
     * pxTopOfStack */
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

/* 动态创建任务 API */
BaseType_t xTaskCreate(TaskFunction_t pxTaskCode, const char *const pcName,
                       const uint16_t usStackDepth, void *const pvParameters,
                       UBaseType_t uxPriority,
                       TaskHandle_t *const pxCreatedTask) {

  TCB_t *pxNewTCB;
  StackType_t *pxStackBuffer;
  BaseType_t xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
  /* 1. 动态分配任务栈空间 */
  pxStackBuffer =
      (StackType_t *)pvPortMalloc(((size_t)usStackDepth) * sizeof(StackType_t));

  if (pxStackBuffer != NULL) {
    /* 2. 动态分配 TCB 结构体 */
    pxNewTCB = (TCB_t *)pvPortMalloc(sizeof(TCB_t));
    if (pxNewTCB != NULL) {
      /* 3. 调用 xTaskCreateStatic 完成 TCB 与栈帧初始化及就绪链表挂载 */
      (void)xTaskCreateStatic(pxTaskCode, pcName, (uint32_t)usStackDepth,
                              pvParameters, uxPriority, pxStackBuffer,
                              pxNewTCB);
      /* 4. 如果用户传入了输出句柄指针，输出 TCB 指针 */
      if (pxCreatedTask != NULL) {
        *pxCreatedTask = (TaskHandle_t)pxNewTCB;
      }
      xReturn = pdPASS;
    } else {
      /* TCB 分配失败，释放已分配的任务栈 */
      vPortFree(pxStackBuffer);
    }
  }
  return xReturn;
}

/* 定义空闲任务的静态 TCB 与栈空间 */
static TCB_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

/* 空闲任务函数：当无其他任务就绪时运行 */
static void prvIdleTask(void *pvParameters) {
  (void)pvParameters;
  for (;;) {
    /* 空闲任务死循环 (以后可以在这里加入低功耗休眠指令 WFI) */
  }
}

/* 启动调度器 */
void vTaskStartScheduler(void) {
  /* 1. 初始化就绪链表数组 (若尚未初始化) */
  prvInitialiseTaskLists();
  /* 1. 静态创建最低优先级 (0) 的空闲任务 */
  xTaskCreateStatic(prvIdleTask, "IDLE", configMINIMAL_STACK_SIZE, NULL,
                    (UBaseType_t)0U, uxIdleTaskStack, &xIdleTaskTCB);
  /* 2. 硬件层启动调度器 (初始化 SysTick 定时器并启动第一个任务) */
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

  /* 3. 将节点的 xItemValue 设为唤醒时刻 (供 vListInsert 升序排序) */
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
    /* 1. 将当前任务移出就绪链表，加入延时链表 */
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

  /* 2. 关键核心：32 位溢出判断与双延时链表指针交换 */
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

      /* 获取 TCB */
      pxTCB = (TCB_t *)listGET_LIST_ITEM_OWNER(pxListItem);

      /* 如果该任务同时还在等待事件 (如队列)，也从事件链表中剥离 */
      if (pxTCB->xEventListItem.pxContainer != NULL) {
        (void)uxListRemove(&(pxTCB->xEventListItem));
      }

      /* 重新挂载回对应的就绪链表尾部 */
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
  /* 1. 从最高优先级向下查找第一个非空的就绪链表 (加边界防保护 uxTopPriority >
   * 0) */
  while ((uxTopPriority > 0) &&
         (listLIST_IS_EMPTY(&(pxReadyTasksLists[uxTopPriority])) != 0)) {
    uxTopPriority--;
  }

  if (listLIST_IS_EMPTY(&(pxReadyTasksLists[uxTopPriority])) == 0) {
    /* 2. 利用 listGET_OWNER_OF_NEXT_ENTRY 宏挪动游标
     * pxIndex，获取下一个就绪任务 */
    listGET_OWNER_OF_NEXT_ENTRY(pxCurrentTCB,
                                &(pxReadyTasksLists[uxTopPriority]));
  } else {
    pxCurrentTCB = NULL;
  }
}

/* 动态删除任务 API */
void vTaskDelete(TaskHandle_t xTaskToDelete) {
  TCB_t *pxTCB;
  BaseType_t xDeletingCurrentTask = 0;

  /* 1. 若入参为 NULL，代表删除当前正在运行的任务 */
  if (xTaskToDelete == NULL) {
    pxTCB = pxCurrentTCB;
  } else {
    pxTCB = (TCB_t *)xTaskToDelete;
  }

  if (pxTCB != NULL) {
    if (pxTCB == pxCurrentTCB) {
      xDeletingCurrentTask = 1;
    }

    /* 2. 将任务节点从其所在链表中解除挂载 */
    (void)uxListRemove(&(pxTCB->xStateListItem));
    if (pxTCB->xEventListItem.pxContainer != NULL) {
      (void)uxListRemove(&(pxTCB->xEventListItem));
    }

    /* 3. 释放动态分配的任务栈和 TCB 内存 */
    if (pxTCB->pxStack != NULL) {
      vPortFree(pxTCB->pxStack);
      pxTCB->pxStack = NULL;
    }

    vPortFree(pxTCB);

    /* 4. 若删除的是当前正在运行的任务，重置 pxCurrentTCB 并重新选出新任务 */
    if (xDeletingCurrentTask != 0) {
      pxCurrentTCB = NULL;
      vTaskSwitchContext();
    }
  }
}

/* 14. 将当前任务挂载到事件阻塞链表（如队列的等待链表） */
void vTaskPlaceOnEventList(List_t *const pxEventList,
                           const TickType_t xTicksToWait) {
  /* 1. 插入到事件等待链表中 (按优先级由高到低有序排列) */
  vListInsert(pxEventList, &(pxCurrentTCB->xEventListItem));

  /* 2. 将当前任务移出就绪链表，若指定了超时时间则加入延时链表 */
  if (xTicksToWait > (TickType_t)0U) {
    prvAddCurrentTaskToDelayedList(xTicksToWait);
  } else {
    (void)uxListRemove(&(pxCurrentTCB->xStateListItem));
  }
}

/* 15. 从事件阻塞链表唤醒最高优先级的任务，返回是否需要触发上下文切换 */
BaseType_t xTaskRemoveFromEventList(const List_t *const pxEventList) {
  TCB_t *pxUnblockedTCB;
  BaseType_t xReturn = pdFAIL;

  if (listLIST_IS_EMPTY(pxEventList) == 0) {
    /* 1. 取出事件等待链表头部的最高优先级任务节点并移出 */
    ListItem_t *pxListItem = listGET_HEAD_ENTRY(pxEventList);
    (void)uxListRemove(pxListItem);

    /* 2. 获取该节点对应的 TCB 指针 */
    pxUnblockedTCB = (TCB_t *)listGET_LIST_ITEM_OWNER(pxListItem);

    /* 3. 若任务此前在延时链表中等待超时，将其从延时链表移出 */
    if (pxUnblockedTCB->xStateListItem.pxContainer != NULL) {
      (void)uxListRemove(&(pxUnblockedTCB->xStateListItem));
    }

    /* 4. 重新挂载回就绪链表 */
    vListInsertEnd(&(pxReadyTasksLists[pxUnblockedTCB->uxPriority]),
                   &(pxUnblockedTCB->xStateListItem));

    /* 5. 若解封的任务优先级高于或等于当前运行任务，需要请求调度切换 */
    if ((pxCurrentTCB == NULL) ||
        (pxUnblockedTCB->uxPriority >= pxCurrentTCB->uxPriority)) {
      xReturn = pdPASS;
    }
  }

  return xReturn;
}
