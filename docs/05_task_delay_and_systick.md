# MiniRTOS 核心笔记（五）：TaskDelay 延时机制与 SysTick 滴答唤醒

本文档记录 MiniRTOS 开发阶段二中关于**任务延时 (`vTaskDelay`)**、**双延时链表 (Dual Delayed Lists)** 与 **SysTick 滴答自增唤醒 (`xTaskIncrementTick`)** 的底层架构原理。

---

## 1. 为什么需要双延时链表 (Dual Delayed Lists)？

当任务调用 `vTaskDelay(xTicksToWait)` 时，唤醒时刻为 `xTimeToWake = xTickCount + xTicksToWait`。

### 3.1 32 位 Tick 计数器溢出风险
设当前系统时间 `xTickCount = 0xFFFFFFF0`，任务延时 `0x20`：
`xTimeToWake = 0xFFFFFFF0 + 0x20 = 0x00000010` （发生了 32 位溢出回绕到小数了！）。

如果直接升序插入普通的延时链表，因为 `0x00000010 < 0xFFFFFFF0`，该任务会被错误地放在链表头部并**被立刻误唤醒**！

### 1.2 双延时链表解法
内核定义了两个延时链表实体与指针：
- **`pxDelayedTaskList`**：保存唤醒时间在**当前时间周期**内的任务。
- **`pxOverflowDelayedTaskList`**：保存唤醒时间**跨越 32 位溢出**的任务。

```c
if( xTimeToWake < xConstTickCount )
{
    /* 溢出：插入到 pxOverflowDelayedTaskList */
    vListInsert( pxOverflowDelayedTaskList, &( pxCurrentTCB->xStateListItem ) );
}
else
{
    /* 未溢出：插入到当前 pxDelayedTaskList */
    vListInsert( pxDelayedTaskList, &( pxCurrentTCB->xStateListItem ) );
}
```

当 `xTickCount` 增加到 `0`（发生溢出）时，在 `xTaskIncrementTick()` 中只需**一步交换指针**：
```c
List_t * pxTemp = pxDelayedTaskList;
pxDelayedTaskList = pxOverflowDelayedTaskList;
pxOverflowDelayedTaskList = pxTemp;
```

---

## 2. 滴答自增与任务到期解封流程 (`xTaskIncrementTick`)

每次硬件产生 SysTick 中断时：

1. **`xTickCount++` 自增**。若溢出归零，交换双延时链表指针。
2. **检查当前延时链表**：取出 `pxDelayedTaskList` 的头部节点（最先唤醒的任务）。
3. **到期判断**：
   - 若 `xTickCount < xItemValue`，说明尚未有任务到期，直接跳出循环（链表按时间升序排列，后续任务必定未到期）。
   - 若 `xTickCount >= xItemValue`，从延时链表剥离任务，重新挂载回对应的**就绪链表 `pxReadyTasksLists[uxPriority]`**。
   - 若解封的任务优先级 $\ge$ 当前任务优先级，标记 `xSwitchRequired = 1` 请求上下文切换！

---

## 3. 测试验证输出

运行 `make run` 的模块 4 延时与唤醒测试：

```text
================ [模块测试 4] vTaskDelay 延时与 SysTick 中断唤醒 ================
延时前：Task_Delay 所在 2 级就绪链表节点数: 1 (期望: 1)
Task_Delay 调用 vTaskDelay(5)...
延时后：Task_Delay 所在 2 级就绪链表节点数: 0 (期望: 0, 已成功移出就绪链表)
 -> SysTick 1 次推移... 就绪节点数: 0, 是否申请调度切换: 0
 -> SysTick 2 次推移... 就绪节点数: 0, 是否申请调度切换: 0
 -> SysTick 3 次推移... 就绪节点数: 0, 是否申请调度切换: 0
 -> SysTick 4 次推移... 就绪节点数: 0, 是否申请调度切换: 0
 -> SysTick 5 次推移... 就绪节点数: 1, 是否申请调度切换: 1
Tick = 5 时：Task_Delay 成功被唤醒并重新挂载回 2 级就绪链表！
======================================================================
```
