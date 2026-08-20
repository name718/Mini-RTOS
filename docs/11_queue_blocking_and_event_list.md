# MiniRTOS 核心笔记（十一）：双链表节点与队列阻塞唤醒机制 (Event Lists)

本文档记录 MiniRTOS 开发阶段四中关于 **TCB 双链表节点设计 (`xStateListItem` / `xEventListItem`)** 与 **队列阻塞与唤醒机制 (`vTaskPlaceOnEventList` / `xTaskRemoveFromEventList`)**。

---

## 1. 为什么 TCB 需要双链表节点？

在 FreeRTOS 中，每个任务控制块 `TCB_t` 都内嵌了两个链表节点：

```c
typedef struct tskTCB {
  volatile StackType_t *pxTopOfStack;
  ListItem_t xStateListItem; /* 状态节点：挂载在 就绪链表 (pxReadyTasksLists) 或 延时链表 (pxDelayedTaskList) */
  ListItem_t xEventListItem; /* 事件节点：挂载在 队列/信号量 等待链表 (xTasksWaitingToReceive / Send) */
  UBaseType_t uxPriority;
  ...
} TCB_t;
```

### 1.1 双链表节点解决的核心难题
当任务调用 `xQueueReceive(xQueue, &buf, 100)` 时：
1. 任务需要等待**消息到达**；
2. 任务同时需要等待**100 个 Tick 超时**。

因此任务必须**同时处于两个链表**中：
- 通过 `xEventListItem` 挂在队列的 `xTasksWaitingToReceive` 链表上；
- 通过 `xStateListItem` 挂在系统的 `pxDelayedTaskList` 延时链表上。

---

## 2. 优先级逆序排序的巧妙设计

在 `xTaskCreateStatic()` 中，事件节点的初始排序值计算公式为：
```c
listSET_LIST_ITEM_VALUE(&(pxNewTCB->xEventListItem), (TickType_t)(configMAX_PRIORITIES - 1 - uxPriority));
```

- FreeRTOS 链表 `vListInsert` 默认按升序插入。
- 假设 `configMAX_PRIORITIES = 5`：
  - 优先级 4（最高）：排序值 = $5 - 1 - 4 = 0$
  - 优先级 1（较低）：排序值 = $5 - 1 - 1 = 3$
- 升序插入后，**优先级最高的任务总是排在等待链表的最头部**！
- 唤醒时只需取链表头节点 `listGET_HEAD_ENTRY()`，时间复杂度为 $O(1)$。

---

## 3. 阻塞与唤醒流程

### 3.1 阻塞过程 (`vTaskPlaceOnEventList`)
1. 插入到队列的等待链表中 (`xEventListItem`)；
2. 将当前任务移出就绪链表；
3. 若指定了超时时间，挂入系统的 `pxDelayedTaskList`；
4. 调用 `vTaskSwitchContext()` 让出 CPU 执行权。

### 3.2 唤醒过程 (`xTaskRemoveFromEventList`)
1. 取出事件等待链表头部的最高优先级任务；
2. 将其从事件等待链表移出；
3. 将其从延时链表移出；
4. 重新挂回其优先级对应的 `pxReadyTasksLists[uxPriority]`；
5. 若被唤醒的任务优先级 $\ge$ 当前运行任务，触发上下文切换抢占。

---

## 4. Unity 单元测试验证输出

运行 `make test` 输出：

```text
=======================================================
        Unity C Unit Testing Framework (MiniRTOS)       
=======================================================
[TEST 1] test_List_Initialization (line 282) -> [PASS]
[TEST 2] test_List_Ascending_Insertion_And_Remove (line 283) -> [PASS]
[TEST 3] test_Task_StaticCreation_And_StackFrame (line 286) -> [PASS]
[TEST 4] test_Task_Delay_And_TickWakeup (line 289) -> [PASS]
[TEST 5] test_RoundRobin_Scheduling (line 292) -> [PASS]
[TEST 6] test_Heap4_Malloc_Free_And_Coalescing (line 295) -> [PASS]
[TEST 7] test_Task_DynamicCreate_And_Delete (line 298) -> [PASS]
[TEST 8] test_Queue_Create_Send_Receive_RingBuffer (line 301) -> [PASS]
[TEST 9] test_Queue_Blocking_And_Unblocking (line 304) -> [PASS]

----------------------- SUMMARY -----------------------
9 Tests, 0 Failures, 0 Ignored
RESULT: SUCCESS (ALL TESTS PASSED)
=======================================================
```
