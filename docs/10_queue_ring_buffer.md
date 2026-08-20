# MiniRTOS 核心笔记（十）：消息队列与环形缓冲区 (Queue Ring Buffer)

本文档记录 MiniRTOS 开发阶段四中关于**消息队列 (`Queue_t`)** 的底层架构与环形缓冲区数据拷贝机制。

---

## 1. 消息队列核心架构

在 FreeRTOS 中，队列不仅用于传递数据，更是二值信号量、计数信号量、互斥锁的通用基石。

### 1.1 队列结构体 (`QueueDefinition`)

```c
typedef struct QueueDefinition {
  int8_t *pcHead;     /* 指向环形缓冲区存储区的起始物理地址 */
  int8_t *pcWriteTo;  /* 指向下一个写入位置 */
  int8_t *pcReadFrom; /* 指向上一个读取位置 */

  List_t xTasksWaitingToSend;    /* 等待发送的任务阻塞链表 (队列满时使用) */
  List_t xTasksWaitingToReceive; /* 等待接收的任务阻塞链表 (队列空时使用) */

  volatile UBaseType_t uxMessagesWaiting; /* 当前队列中已有的消息数量 */
  UBaseType_t uxLength;                   /* 队列的最大容量 (最大消息数) */
  UBaseType_t uxItemSize;                 /* 单个消息的字节大小 */
} Queue_t;
```

---

## 2. 环形缓冲区指针回绕机制 (Wrap Around)

### 2.1 写入机制 (`prvCopyDataToQueue`)
1. 使用 `memcpy` 将数据拷贝到 `pcWriteTo` 指向的地址。
2. 将 `pcWriteTo` 指针向后移动 `uxItemSize` 字节。
3. **回绕检查**：若 `pcWriteTo` 越过缓冲区边界 `pcHead + (uxLength * uxItemSize)`，则将 `pcWriteTo` 重置回 `pcHead`。
4. `uxMessagesWaiting++`。

### 2.2 读取机制 (`prvCopyDataFromQueue`)
1. 先将 `pcReadFrom` 指针向后移动 `uxItemSize` 字节。
2. **回绕检查**：若 `pcReadFrom` 越过缓冲区边界，则将其重置回 `pcHead`。
3. 从 `pcReadFrom` 处使用 `memcpy` 将数据拷贝给调用者。
4. `uxMessagesWaiting--`。

---

## 3. Unity 单元测试验证输出

运行 `make test` 输出：

```text
=======================================================
        Unity C Unit Testing Framework (MiniRTOS)       
=======================================================
[TEST 1] test_List_Initialization (line 248) -> [PASS]
[TEST 2] test_List_Ascending_Insertion_And_Remove (line 249) -> [PASS]
[TEST 3] test_Task_StaticCreation_And_StackFrame (line 252) -> [PASS]
[TEST 4] test_Task_Delay_And_TickWakeup (line 255) -> [PASS]
[TEST 5] test_RoundRobin_Scheduling (line 258) -> [PASS]
[TEST 6] test_Heap4_Malloc_Free_And_Coalescing (line 261) -> [PASS]
[TEST 7] test_Task_DynamicCreate_And_Delete (line 264) -> [PASS]
[TEST 8] test_Queue_Create_Send_Receive_RingBuffer (line 267) -> [PASS]

----------------------- SUMMARY -----------------------
8 Tests, 0 Failures, 0 Ignored
RESULT: SUCCESS (ALL TESTS PASSED)
=======================================================
```
