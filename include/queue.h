#ifndef QUEUE_H
#define QUEUE_H

#include "task.h"

/* 队列句柄类型 (对外隐藏内部结构体细节) */
typedef void *QueueHandle_t;

#define errQUEUE_FULL ((BaseType_t)0)
#define errQUEUE_EMPTY ((BaseType_t)0)

/*
 * 1. 动态创建队列 API
 * uxQueueLength: 队列中最多能存放多少个消息
 * uxItemSize: 每个消息的字节大小
 */
QueueHandle_t xQueueCreate(const UBaseType_t uxQueueLength,
                           const UBaseType_t uxItemSize);

/* 创建计数信号量队列 API */
QueueHandle_t xQueueCreateCounting(const UBaseType_t uxMaxCount,
                                   const UBaseType_t uxInitialCount);

/* 创建互斥信号量队列 API */
QueueHandle_t xQueueCreateMutex(void);

/* 向队列尾部发送消息 (支持超时阻塞与唤醒接收任务) */
BaseType_t xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue,
                      TickType_t xTicksToWait);

/* 从队列头部接收消息 (支持超时阻塞与唤醒发送任务) */
BaseType_t xQueueReceive(QueueHandle_t xQueue, void *const pvBuffer,
                          TickType_t xTicksToWait);

#endif /* QUEUE_H */
