# MiniRTOS 核心笔记（十三）：互斥量与优先级继承 (Priority Inheritance)

本文档记录 MiniRTOS 开发阶段四中关于 **互斥量 (Mutex)** 与 **优先级反转 (Priority Inversion) 的终极解法 —— 优先级继承机制** 的底层架构原理与实现。

---

## 1. 什么是优先级反转 (Priority Inversion)？

在实时多任务系统中，若高优先级任务 $H$ 和低优先级任务 $L$ 共享同一个互斥资源（如互斥锁 $M$）：

```text
时刻 1：低优先级任务 L 运行，获取了互斥锁 M；
时刻 2：中优先级任务 M（不需要锁 M）就绪，由于优先级高于 L，抢占了 CPU；
时刻 3：高优先级任务 H 就绪，尝试获取锁 M，发现锁在 L 手中，H 只能阻塞等待 L 释放；
后果：中优先级任务 M 抢占了低优先级任务 L，导致 L 无法尽快执行并释放锁，高优先级任务 H 被无限期延迟！
```

这就是著名的**优先级反转**漏洞（曾导致火星探测器“火星探路者号”在火星表面发生系统死锁复位）。

---

## 2. 优先级继承 (Priority Inheritance) 的核心原理

FreeRTOS 解决优先级反转的核心武器是**优先级继承**：

```text
当高优先级任务 H 因申请锁 M 阻塞时：
1. 内核发现锁 M 正被低优先级任务 L 持有；
2. 内核临时将任务 L 的优先级提升到与任务 H 相同（优先级 1 -> 优先级 3）；
3. 此时中优先级任务 M 无法再抢占任务 L！
4. 任务 L 迅速执行完临界区代码，调用 xSemaphoreGive(M) 释放互斥锁；
5. 锁释放时，内核将任务 L 的优先级自动恢复为原来的基准优先级 (uxBasePriority = 1)；
6. 高优先级任务 H 立即被唤醒并抢占 CPU 运行！
```

---

## 3. 内核关键数据结构与实现

### 3.1 TCB 增加基准优先级 (`uxBasePriority`)
```c
typedef struct tskTCB {
  ...
  UBaseType_t uxPriority;     /* 当前优先级 (可能被临时提升) */
  UBaseType_t uxBasePriority; /* 初始基准优先级 (释放锁后恢复) */
  ...
} TCB_t;
```

### 3.2 优先级继承函数 (`vTaskPriorityInherit`)
```c
void vTaskPriorityInherit(TCB_t *const pxMutexHolder) {
  if (pxMutexHolder->uxPriority < pxCurrentTCB->uxPriority) {
    /* 从旧优先级就绪链表移出 */
    uxListRemove(&(pxMutexHolder->xStateListItem));
    /* 临时提升优先级 */
    pxMutexHolder->uxPriority = pxCurrentTCB->uxPriority;
    /* 挂入新的高优先级就绪链表 */
    vListInsertEnd(&(pxReadyTasksLists[pxMutexHolder->uxPriority]), &(pxMutexHolder->xStateListItem));
  }
}
```

### 3.3 优先级恢复函数 (`xTaskPriorityDisinherit`)
```c
BaseType_t xTaskPriorityDisinherit(TCB_t *const pxMutexHolder) {
  if (pxMutexHolder->uxPriority != pxMutexHolder->uxBasePriority) {
    uxListRemove(&(pxMutexHolder->xStateListItem));
    /* 恢复初始基准优先级 */
    pxMutexHolder->uxPriority = pxMutexHolder->uxBasePriority;
    vListInsertEnd(&(pxReadyTasksLists[pxMutexHolder->uxPriority]), &(pxMutexHolder->xStateListItem));
    return pdPASS;
  }
  return pdFAIL;
}
```

---

## 4. Unity 单元测试验证输出

运行 `make test` 输出：

```text
=======================================================
        Unity C Unit Testing Framework (MiniRTOS)       
=======================================================
[TEST 1] test_List_Initialization (line 364) -> [PASS]
[TEST 2] test_List_Ascending_Insertion_And_Remove (line 365) -> [PASS]
[TEST 3] test_Task_StaticCreation_And_StackFrame (line 368) -> [PASS]
[TEST 4] test_Task_Delay_And_TickWakeup (line 371) -> [PASS]
[TEST 5] test_RoundRobin_Scheduling (line 374) -> [PASS]
[TEST 6] test_Heap4_Malloc_Free_And_Coalescing (line 377) -> [PASS]
[TEST 7] test_Task_DynamicCreate_And_Delete (line 380) -> [PASS]
[TEST 8] test_Queue_Create_Send_Receive_RingBuffer (line 383) -> [PASS]
[TEST 9] test_Queue_Blocking_And_Unblocking (line 386) -> [PASS]
[TEST 10] test_Binary_And_Counting_Semaphore (line 389) -> [PASS]
[TEST 11] test_Mutex_And_Priority_Inheritance (line 392) -> [PASS]

----------------------- SUMMARY -----------------------
11 Tests, 0 Failures, 0 Ignored
RESULT: SUCCESS (ALL TESTS PASSED)
=======================================================
```
