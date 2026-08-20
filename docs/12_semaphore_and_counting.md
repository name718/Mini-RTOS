# MiniRTOS 核心笔记（十二）：二值信号量与计数信号量 (Semaphores)

本文档记录 MiniRTOS 开发阶段四中关于 **二值信号量 (Binary Semaphore)** 与 **计数信号量 (Counting Semaphore)** 的实现与零内存开销设计。

---

## 1. 为什么信号量基于队列实现？

在 FreeRTOS 中，信号量并不是一套独立的底层系统，而是通过 **`uxItemSize = 0` 的消息队列** 宏封装实现的：

```text
 ┌─────────────────────────────────────────────────────────────┐
 │ 消息队列 Queue (uxLength = 1, uxItemSize = 4)                │
 │ 包含真实数据存储区: [ 0x12345678 ]                          │
 └─────────────────────────────────────────────────────────────┘
                               ▲
                               │ 当数据大小变为 0 字节 (uxItemSize = 0)
                               ▼
 ┌─────────────────────────────────────────────────────────────┐
 │ 二值信号量 Binary Semaphore (uxLength = 1, uxItemSize = 0)   │
 │ 无需数据拷贝，仅用 uxMessagesWaiting (0 或 1) 代表资源状态！ │
 └─────────────────────────────────────────────────────────────┘
```

---

## 2. API 宏封装架构 (`semphr.h`)

```c
/* 1. 二值信号量：长度为 1，元素大小为 0 */
#define xSemaphoreCreateBinary() xQueueCreate((UBaseType_t)1U, (UBaseType_t)0U)

/* 2. 计数信号量：长度为 uxMaxCount，初始计数为 uxInitialCount */
#define xSemaphoreCreateCounting(uxMaxCount, uxInitialCount)                   \
  xQueueCreateCounting((uxMaxCount), (uxInitialCount))

/* 3. 释放信号量 (Give) -> 向队列发送 NULL 数据 */
#define xSemaphoreGive(xSemaphore)                                             \
  xQueueSend((QueueHandle_t)(xSemaphore), NULL, (TickType_t)0U)

/* 4. 获取信号量 (Take) -> 从队列接收 NULL 数据 (支持阻塞等待) */
#define xSemaphoreTake(xSemaphore, xBlockTime)                                 \
  xQueueReceive((QueueHandle_t)(xSemaphore), NULL, (TickType_t)(xBlockTime))
```

---

## 3. 零内存与安全保护机制

当 `uxItemSize == 0` 时：
1. `xQueueCreate` 只分配队列头结构体 `sizeof(Queue_t)`，不需要开辟任何环形缓冲区内存；
2. `prvCopyDataToQueue` 和 `prvCopyDataFromQueue` 内部自动跳过 `memcpy` 数据拷贝，只对计数器 `uxMessagesWaiting` 进行自增/自减与阻塞任务唤醒。

---

## 4. Unity 单元测试验证输出

运行 `make test` 输出：

```text
=======================================================
        Unity C Unit Testing Framework (MiniRTOS)       
=======================================================
[TEST 1] test_List_Initialization (line 324) -> [PASS]
[TEST 2] test_List_Ascending_Insertion_And_Remove (line 325) -> [PASS]
[TEST 3] test_Task_StaticCreation_And_StackFrame (line 328) -> [PASS]
[TEST 4] test_Task_Delay_And_TickWakeup (line 331) -> [PASS]
[TEST 5] test_RoundRobin_Scheduling (line 334) -> [PASS]
[TEST 6] test_Heap4_Malloc_Free_And_Coalescing (line 337) -> [PASS]
[TEST 7] test_Task_DynamicCreate_And_Delete (line 340) -> [PASS]
[TEST 8] test_Queue_Create_Send_Receive_RingBuffer (line 343) -> [PASS]
[TEST 9] test_Queue_Blocking_And_Unblocking (line 346) -> [PASS]
[TEST 10] test_Binary_And_Counting_Semaphore (line 349) -> [PASS]

----------------------- SUMMARY -----------------------
10 Tests, 0 Failures, 0 Ignored
RESULT: SUCCESS (ALL TESTS PASSED)
=======================================================
```
