#include "../include/task.h"
#include "../include/port.h"

/* 定义全局指针：指向当前正在运行的任务 TCB */
TCB_t *volatile pxCurrentTCB = NULL;

/* 静态创建任务函数 */
TaskHandle_t xTaskCreateStatic(TaskFunction_t pxTaskCode,
                               const char *const pcName,
                               const uint32_t ulStackDepth,
                               void *const pvParameters,
                               StackType_t *const puxStackBuffer,
                               TCB_t *const pxTaskBuffer) {
  TCB_t *pxNewTCB;
  StackType_t *pxTopOfStack;
  UBaseType_t x;

  /* 1. 安全检查：确保用户传入的 TCB 和 栈数组指针不为空 */
  if ((pxTaskBuffer != NULL) && (puxStackBuffer != NULL)) {
    pxNewTCB = pxTaskBuffer;
    pxNewTCB->pxStack = puxStackBuffer;

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
  } else {
    pxNewTCB = NULL;
  }

  /* 6. 返回任务句柄 (TCB 指针) */
  return (TaskHandle_t)pxNewTCB;
}
