#include "../include/queue.h"
#include "../include/portable.h"
#include <string.h>

/* 队列核心结构体定义 (在 .c 文件中定义以实现数据隐藏) */
typedef struct QueueDefinition {
  int8_t *pcHead;     /* 指向环形缓冲区存储区的起始物理地址 */
  int8_t *pcWriteTo;  /* 指向下一个写入位置 */
  int8_t *pcReadFrom; /* 指向上一个读取位置 */

  List_t xTasksWaitingToSend;    /* 等待发送的任务阻塞链表 (队列满时使用) */
  List_t xTasksWaitingToReceive; /* 等待接收的任务阻塞链表 (队列空时使用) */

  volatile UBaseType_t uxMessagesWaiting; /* 当前队列中已有的消息数量 */
  UBaseType_t uxLength;                   /* 队列的最大容量 (最大消息数) */
  UBaseType_t uxItemSize;                 /* 单个消息的字节大小 */

  TCB_t *pxMutexHolder; /* 持有当前互斥量的任务 TCB (仅互斥量使用) */

} xQUEUE;

typedef xQUEUE Queue_t;

/* 动态创建队列 API */
QueueHandle_t xQueueCreate(const UBaseType_t uxQueueLength,
                           const UBaseType_t uxItemSize) {
  Queue_t *pxNewQueue;
  size_t xQueueSizeInBytes;

  /* 1. 计算总内存需求：结构体大小 + (队列长度 * 单个消息大小) */
  xQueueSizeInBytes = (size_t)(uxQueueLength * uxItemSize);

  /* 2. 调用上一阶段写的动态内存管理分配堆内存 */
  pxNewQueue = (Queue_t *)pvPortMalloc(sizeof(Queue_t) + xQueueSizeInBytes);

  if (pxNewQueue != NULL) {
    /* 3. 内存首部是结构体，结构体尾部紧跟着就是环形缓冲区数据区 */
    pxNewQueue->pcHead = ((int8_t *)pxNewQueue) + sizeof(Queue_t);

    /* 4. 初始化环形缓冲区的读写指针和基本信息 */
    pxNewQueue->uxLength = uxQueueLength;
    pxNewQueue->uxItemSize = uxItemSize;
    pxNewQueue->uxMessagesWaiting = (UBaseType_t)0U;
    pxNewQueue->pcWriteTo = pxNewQueue->pcHead;
    if (uxItemSize > 0) {
      pxNewQueue->pcReadFrom =
          pxNewQueue->pcHead + ((uxQueueLength - 1) * uxItemSize);
    } else {
      pxNewQueue->pcReadFrom = pxNewQueue->pcHead;
    }

    pxNewQueue->pxMutexHolder = NULL;

    /* 5. 初始化两条等待链表 */
    vListInitialise(&(pxNewQueue->xTasksWaitingToSend));
    vListInitialise(&(pxNewQueue->xTasksWaitingToReceive));
  }

  return (QueueHandle_t)pxNewQueue;
}

/* 创建计数信号量队列 API */
QueueHandle_t xQueueCreateCounting(const UBaseType_t uxMaxCount,
                                   const UBaseType_t uxInitialCount) {
  Queue_t *pxNewQueue = (Queue_t *)xQueueCreate(uxMaxCount, 0);
  if (pxNewQueue != NULL) {
    pxNewQueue->uxMessagesWaiting = uxInitialCount;
  }
  return (QueueHandle_t)pxNewQueue;
}

/* 创建互斥量队列 API */
QueueHandle_t xQueueCreateMutex(void) {
  /* 互斥量本质是长度为 1，元素大小为 0 的队列，初始拥有 1 把锁可用 */
  Queue_t *pxNewQueue = (Queue_t *)xQueueCreate(1, 0);
  if (pxNewQueue != NULL) {
    pxNewQueue->uxMessagesWaiting = (UBaseType_t)1U;
    pxNewQueue->pxMutexHolder = NULL;
  }
  return (QueueHandle_t)pxNewQueue;
}

/* 内部辅助函数：将数据拷贝入环形缓冲区 */
static void prvCopyDataToQueue(Queue_t *const pxQueue,
                               const void *pvItemToQueue) {
  if (pxQueue->uxItemSize > 0) {
    /* 1. 将数据拷贝到写指针 pcWriteTo 的位置 */
    memcpy((void *)pxQueue->pcWriteTo, pvItemToQueue,
           (size_t)pxQueue->uxItemSize);

    /* 2. 移动写指针。如果越界，则回绕到缓冲区头部 (Ring Buffer 回绕) */
    pxQueue->pcWriteTo += pxQueue->uxItemSize;
    if (pxQueue->pcWriteTo >=
        pxQueue->pcHead + (pxQueue->uxLength * pxQueue->uxItemSize)) {
      pxQueue->pcWriteTo = pxQueue->pcHead;
    }
  }

  /* 3. 队列里的消息/信号量总数 + 1 */
  pxQueue->uxMessagesWaiting++;
}

/* 内部辅助函数：从环形缓冲区拷贝出数据 */
static void prvCopyDataFromQueue(Queue_t *const pxQueue, void *const pvBuffer) {
  if (pxQueue->uxItemSize > 0) {
    /* 1. 移动读指针。读取位置总是由 pcReadFrom 推进得到，同样需要处理回绕 */
    pxQueue->pcReadFrom += pxQueue->uxItemSize;
    if (pxQueue->pcReadFrom >=
        pxQueue->pcHead + (pxQueue->uxLength * pxQueue->uxItemSize)) {
      pxQueue->pcReadFrom = pxQueue->pcHead;
    }

    /* 2. 将数据拷贝到用户的 buffer 中 */
    if (pvBuffer != NULL) {
      memcpy(pvBuffer, (void *)pxQueue->pcReadFrom, (size_t)pxQueue->uxItemSize);
    }
  }

  /* 3. 队列里的消息/信号量总数 - 1 */
  pxQueue->uxMessagesWaiting--;
}

/* 2. 发送消息 / 释放信号量/互斥量 API */
BaseType_t xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue,
                      TickType_t xTicksToWait) {
  Queue_t *const pxQueue = (Queue_t *)xQueue;

  for (;;) {
    /* 检查队列是否未满 */
    if (pxQueue->uxMessagesWaiting < pxQueue->uxLength) {
      /* 队列未满，直接拷贝数据进去 */
      prvCopyDataToQueue(pxQueue, pvItemToQueue);

      /* 如果释放的是互斥量，恢复持有者的基准优先级 */
      if (pxQueue->pxMutexHolder != NULL) {
        (void)xTaskPriorityDisinherit(pxQueue->pxMutexHolder);
        pxQueue->pxMutexHolder = NULL;
      }

      /* 检查是否有任务正在阻塞等待接收消息，若有则唤醒最高优等待任务 */
      if (listLIST_IS_EMPTY(&(pxQueue->xTasksWaitingToReceive)) == 0) {
        if (xTaskRemoveFromEventList(&(pxQueue->xTasksWaitingToReceive)) ==
            pdPASS) {
          /* 唤醒的任务优先级高于或等于当前任务，请求切换 */
          vTaskSwitchContext();
        }
      }
      return pdPASS;
    } else {
      /* 队列已满 */
      if (xTicksToWait == (TickType_t)0U) {
        return errQUEUE_FULL;
      } else {
        /* 将当前任务挂入发送等待链表，并根据超时时间阻塞 */
        vTaskPlaceOnEventList(&(pxQueue->xTasksWaitingToSend), xTicksToWait);
        vTaskSwitchContext();
        xTicksToWait = 0;
      }
    }
  }
}

/* 3. 接收消息 / 获取信号量/互斥量 API */
BaseType_t xQueueReceive(QueueHandle_t xQueue, void *const pvBuffer,
                         TickType_t xTicksToWait) {
  Queue_t *const pxQueue = (Queue_t *)xQueue;

  for (;;) {
    /* 检查队列是否非空 */
    if (pxQueue->uxMessagesWaiting > (UBaseType_t)0) {
      /* 队列有数据，拷贝出来 */
      prvCopyDataFromQueue(pxQueue, pvBuffer);

      /* 若此队列用作互斥量（uxLength == 1 且 uxItemSize == 0），记录持有者 */
      if ((pxQueue->uxLength == (UBaseType_t)1U) &&
          (pxQueue->uxItemSize == (UBaseType_t)0U)) {
        pxQueue->pxMutexHolder = pxCurrentTCB;
      }

      /* 检查是否有任务正在阻塞等待发送消息，若有则唤醒 */
      if (listLIST_IS_EMPTY(&(pxQueue->xTasksWaitingToSend)) == 0) {
        if (xTaskRemoveFromEventList(&(pxQueue->xTasksWaitingToSend)) ==
            pdPASS) {
          vTaskSwitchContext();
        }
      }
      return pdPASS;
    } else {
      /* 队列为空 (锁已被他人获取) */
      if (xTicksToWait == (TickType_t)0U) {
        return errQUEUE_EMPTY;
      } else {
        /* 如果是互斥量且当前被低优先级任务持有，触发优先级继承提升持有者 */
        if (pxQueue->pxMutexHolder != NULL) {
          vTaskPriorityInherit(pxQueue->pxMutexHolder);
        }

        /* 将当前任务挂入接收等待链表，并根据超时时间阻塞 */
        vTaskPlaceOnEventList(&(pxQueue->xTasksWaitingToReceive), xTicksToWait);
        vTaskSwitchContext();
        xTicksToWait = 0;
      }
    }
  }
}
