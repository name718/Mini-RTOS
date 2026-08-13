# MiniRTOS 核心笔记（三）：就绪链表数组与多优先级调度机制

本文档记录 MiniRTOS 开发阶段一中关于**就绪链表数组 (`pxReadyTasksLists`)** 与 **多优先级抢占指针 (`pxCurrentTCB`)** 的底层架构设计原理。

---

## 1. 就绪链表数组 (`pxReadyTasksLists`) 架构设计

在 FreeRTOS 中，处于“就绪状态（Ready）”的任务通过优先级划分并归类到对应的链表中。

### 1.1 数据结构定义
```c
/* 定义在 src/task.c 中 */
List_t pxReadyTasksLists[ configMAX_PRIORITIES ];
```

- **`configMAX_PRIORITIES`**：在 `FreeRTOSConfig.h` 中配置的最大优先级数量（例如 5，代表优先级 0 ~ 4）。
- **数组下标即优先级**：
  - 下标 `0` 对应最低优先级（通常由系统 Idle 任务占用）。
  - 下标 `configMAX_PRIORITIES - 1` 对应最高优先级。

### 1.2 物理内存模型图示

```text
pxReadyTasksLists[4] ──► [ Task C (优先级 4) ] ──► [ Task D (优先级 4) ]  (最高优先级)
pxReadyTasksLists[3] ──► [ 空链表 (哨兵 xListEnd) ]
pxReadyTasksLists[2] ──► [ Task B (优先级 2) ]
pxReadyTasksLists[1] ──► [ Task A (优先级 1) ]
pxReadyTasksLists[0] ──► [ Idle Task (优先级 0) ]                         (最低优先级)
```

---

## 2. 任务入队与 `pxCurrentTCB` 抢占指针更新

当创建一个新任务 (`xTaskCreateStatic`) 时：

1. **确定优先级**：校准入参 `uxPriority`，确保其在 `[0, configMAX_PRIORITIES - 1]` 范围内。
2. **就绪入队**：
   ```c
   vListInsertEnd( &( pxReadyTasksLists[ uxPriority ] ), &( pxNewTCB->xStateListItem ) );
   ```
   将任务节点的 `xStateListItem` 插入到对应优先级的就绪链表尾部。
3. **`pxCurrentTCB` 抢占指针选择**：
   ```c
   if( ( pxCurrentTCB == NULL ) || ( pxCurrentTCB->uxPriority < uxPriority ) )
   {
       pxCurrentTCB = pxNewTCB;
   }
   ```
   如果新创建的任务优先级高于当前正在运行的任务 `pxCurrentTCB`，`pxCurrentTCB` 指针会**立刻更新指向最高优先级的任务**。这为接下来的软件/硬件抢占式上下文切换奠定了基础！

---

## 3. 测试验证输出

运行 `make run` 验证结果：

```text
[测试 3] 创建 Task_1 后，pxCurrentTCB 指向: Task_1 (优先级: 1)

创建高优先级的 Task_2 (优先级 3)...
[测试 4] 创建 Task_2 后，pxCurrentTCB 自动抢占更新为: Task_2 (优先级: 3)
```
