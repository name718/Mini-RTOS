# MiniRTOS 核心笔记（九）：动态任务创建 (xTaskCreate) 与动态任务删除 (vTaskDelete)

本文档记录 MiniRTOS 开发阶段三中关于**动态任务分配 (`xTaskCreate`)** 与 **动态任务摧毁与回收 (`vTaskDelete`)** 的实现细节。

---

## 1. 动态任务创建机制 (`xTaskCreate`)

对比阶段一的静态创建 `xTaskCreateStatic()`，动态创建不需要用户手动声明 `TCB_t` 和 `StackType_t stack[]` 数组。

### 1.1 内部分配流程
```c
BaseType_t xTaskCreate( TaskFunction_t pxTaskCode,
                        const char * const pcName,
                        const uint16_t usStackDepth,
                        void * const pvParameters,
                        UBaseType_t uxPriority,
                        TaskHandle_t * const pxCreatedTask )
{
    /* 1. 动态分配任务栈空间 (Stack) */
    pxStackBuffer = ( StackType_t * ) pvPortMalloc( usStackDepth * sizeof( StackType_t ) );

    /* 2. 动态分配任务控制块 (TCB) */
    pxNewTCB = ( TCB_t * ) pvPortMalloc( sizeof( TCB_t ) );

    /* 3. 复用静态创建逻辑完成栈帧伪造与就绪链表挂载 */
    xTaskCreateStatic( pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxStackBuffer, pxNewTCB );

    /* 4. 返回状态 pdPASS */
    return pdPASS;
}
```

---

## 2. 动态任务删除与内存回收机制 (`vTaskDelete`)

当任务调用 `vTaskDelete(xTaskToDelete)` 时，内核必须安全剥离链表并释放其占用的堆内存：

### 2.1 释放顺序与野指针防护
1. **就绪/延时链表解绑**：调用 `uxListRemove(&(pxTCB->xStateListItem))`。
2. **记录标志位**：先记录 `xDeletingCurrentTask = (pxTCB == pxCurrentTCB)`，防止释放后访问已被销毁的 TCB 指针（避免 Use-After-Free 漏洞）。
3. **动态堆内存回收**：
   - `vPortFree( pxTCB->pxStack )` 释放任务栈空间；
   - `vPortFree( pxTCB )` 释放 TCB 结构体本身。
4. **重新选择任务**：若删除的是当前任务，重置 `pxCurrentTCB = NULL` 并触发 `vTaskSwitchContext()`。

---

## 3. Unity 单元测试验证输出

运行 `make test` 输出：

```text
=======================================================
        Unity C Unit Testing Framework (MiniRTOS)       
=======================================================
[TEST 1] test_List_Initialization (line 211) -> [PASS]
[TEST 2] test_List_Ascending_Insertion_And_Remove (line 212) -> [PASS]
[TEST 3] test_Task_StaticCreation_And_StackFrame (line 215) -> [PASS]
[TEST 4] test_Task_Delay_And_TickWakeup (line 218) -> [PASS]
[TEST 5] test_RoundRobin_Scheduling (line 221) -> [PASS]
[TEST 6] test_Heap4_Malloc_Free_And_Coalescing (line 224) -> [PASS]
[TEST 7] test_Task_DynamicCreate_And_Delete (line 227) -> [PASS]

----------------------- SUMMARY -----------------------
7 Tests, 0 Failures, 0 Ignored
RESULT: SUCCESS (ALL TESTS PASSED)
=======================================================
```
