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
    pxNewQueue->pcReadFrom =
        pxNewQueue->pcHead + ((uxQueueLength - 1) * uxItemSize);

    /* 5. 初始化两条等待链表 */
    vListInitialise(&(pxNewQueue->xTasksWaitingToSend));
    vListInitialise(&(pxNewQueue->xTasksWaitingToReceive));
  }

  return (QueueHandle_t)pxNewQueue;
}

/* 内部辅助函数：将数据拷贝入环形缓冲区 */
static void prvCopyDataToQueue(Queue_t *const pxQueue,
                               const void *pvItemToQueue) {
  /* 1. 将数据拷贝到写指针 pcWriteTo 的位置 */
  memcpy((void *)pxQueue->pcWriteTo, pvItemToQueue,
         (size_t)pxQueue->uxItemSize);

  /* 2. 移动写指针。如果越界，则回绕到缓冲区头部 (Ring Buffer 回绕) */
  pxQueue->pcWriteTo += pxQueue->uxItemSize;
  if (pxQueue->pcWriteTo >=
      pxQueue->pcHead + (pxQueue->uxLength * pxQueue->uxItemSize)) {
    pxQueue->pcWriteTo = pxQueue->pcHead;
  }

  /* 3. 队列里的消息总数 + 1 */
  pxQueue->uxMessagesWaiting++;
}

/* 内部辅助函数：从环形缓冲区拷贝出数据 */
static void prvCopyDataFromQueue(Queue_t *const pxQueue, void *const pvBuffer) {
  /* 1. 移动读指针。读取位置总是由 pcReadFrom 推进得到，同样需要处理回绕 */
  pxQueue->pcReadFrom += pxQueue->uxItemSize;
  if (pxQueue->pcReadFrom >=
      pxQueue->pcHead + (pxQueue->uxLength * pxQueue->uxItemSize)) {
    pxQueue->pcReadFrom = pxQueue->pcHead;
  }

  /* 2. 将数据拷贝到用户的 buffer 中 */
  memcpy(pvBuffer, (void *)pxQueue->pcReadFrom, (size_t)pxQueue->uxItemSize);

  /* 3. 队列里的消息总数 - 1 */
  pxQueue->uxMessagesWaiting--;
}

/* 2. 发送消息到队列 API */
BaseType_t xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue,
                      TickType_t xTicksToWait) {
  Queue_t *const pxQueue = (Queue_t *)xQueue;
  (void)xTicksToWait; /* 本节暂不处理阻塞等待 */

  /* 检查队列是否已满 */
  if (pxQueue->uxMessagesWaiting < pxQueue->uxLength) {
    /* 队列未满，直接拷贝数据进去 */
    prvCopyDataToQueue(pxQueue, pvItemToQueue);
    return pdPASS;
  } else {
    /* 队列满了，直接返回错误 */
    return errQUEUE_FULL;
  }
}

/* 3. 从队列接收消息 API */
BaseType_t xQueueReceive(QueueHandle_t xQueue, void *const pvBuffer,
                         TickType_t xTicksToWait) {
  Queue_t *const pxQueue = (Queue_t *)xQueue;
  (void)xTicksToWait; /* 本节暂不处理阻塞等待 */

  /* 检查队列是否为空 */
  if (pxQueue->uxMessagesWaiting > (UBaseType_t)0) {
    /* 队列有数据，拷贝出来 */
    prvCopyDataFromQueue(pxQueue, pvBuffer);
    return pdPASS;
  } else {
    /* 队列是空的，读不到数据 */
    return errQUEUE_EMPTY;
  }
}
