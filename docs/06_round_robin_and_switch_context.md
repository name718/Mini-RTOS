# MiniRTOS 核心笔记（六）：时间片轮转 (Round-Robin) 与上下文切换选择器

本文档记录 MiniRTOS 开发阶段二中关于**最高优先级任务查找**与**同优先级时间片轮转 (Round-Robin)** 的选择逻辑 (`vTaskSwitchContext`)。

---

## 1. 任务上下文切换选择器 (`vTaskSwitchContext`)

在发生抢占、中断到期唤醒或调用 `vTaskDelay` 时，内核需要确定下一个获得 CPU 执行权的任务。

`vTaskSwitchContext()` 承担了这一核心职责：

```c
void vTaskSwitchContext( void )
{
    UBaseType_t uxTopPriority = ( UBaseType_t ) configMAX_PRIORITIES - ( UBaseType_t ) 1U;

    /* 1. 从最高优先级 (configMAX_PRIORITIES - 1) 向下寻找第一个非空就绪链表 */
    while( listLIST_IS_EMPTY( &( pxReadyTasksLists[ uxTopPriority ] ) ) != 0 )
    {
        uxTopPriority--;
    }

    /* 2. 核心：从最高优先级的就绪链表中轮转获取下一个任务 TCB */
    listGET_OWNER_OF_NEXT_ENTRY( pxCurrentTCB, &( pxReadyTasksLists[ uxTopPriority ] ) );
}
```

---

## 2. 同优先级时间片轮转 (Round-Robin) 原理

当链表中挂载了多个相同优先级的任务（如 `Task_A` 和 `Task_B`）时：

### 2.1 `listGET_OWNER_OF_NEXT_ENTRY` 宏原理解剖
```c
#define listGET_OWNER_OF_NEXT_ENTRY( pxTCB, pxList )                                       \
{                                                                                           \
    List_t * const pxConstList = ( pxList );                                                \
    /* 1. 将游标 pxIndex 向后推移一个节点 */                                                 \
    ( pxConstList )->pxIndex = ( pxConstList )->pxIndex->pxNext;                            \
    /* 2. 如果推移到了尾部哨兵 xListEnd，跳过哨兵，指向链表头部的第一个有效节点 */             \
    if( ( void * ) ( pxConstList )->pxIndex == ( void * ) &( ( pxConstList )->xListEnd ) )  \
    {                                                                                       \
        ( pxConstList )->pxIndex = ( pxConstList )->pxIndex->pxNext;                        \
    }                                                                                       \
    /* 3. 获取该节点对应的 TCB 指针 */                                                       \
    ( pxTCB ) = ( pxConstList )->pxIndex->pvOwner;                                          \
}
```

- **初始状态**：`pxIndex` 指向哨兵 `xListEnd`；
- **第 1 次切换**：`pxIndex` 移向 `Task_A`，`pxCurrentTCB = Task_A`；
- **第 2 次切换**：`pxIndex` 移向 `Task_B`，`pxCurrentTCB = Task_B`；
- **第 3 次切换**：`pxIndex` 移向哨兵 `xListEnd`，触发跳过哨兵条件，自动回到 `Task_A`！

通过游标指针在闭环双向链表中的推移，实现了完美的 **$O(1)$ 时间复杂度时间片轮转调度**。

---

## 3. 测试验证输出

运行 `make run` 验证结果：

```text
================ [模块测试 5] 同优先级时间片轮转 (Round-Robin) ================
初始就绪列表 (优先级 2): Task_A 和 Task_B, pxCurrentTCB 当前指向: Task_A
 -> 第 1 次触发 vTaskSwitchContext(), 游标移动切换到: Task_A
 -> 第 2 次触发 vTaskSwitchContext(), 游标移动切换到: Task_B
 -> 第 3 次触发 vTaskSwitchContext(), 游标移动切换到: Task_A
======================================================================
```
