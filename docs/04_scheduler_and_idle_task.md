# MiniRTOS 核心笔记（四）：调度器启动与空闲任务 (Idle Task)

本文档记录 MiniRTOS 开发阶段一中关于**启动任务调度器 (`vTaskStartScheduler`)** 与 **空闲任务 (Idle Task)** 的内核机制。

---

## 1. 启动任务调度器 (`vTaskStartScheduler`) 流程

当在用户主程序 `main()` 中创建完应用程序任务后，调用 `vTaskStartScheduler()` 开启内核调度。

其内部逻辑分为两步：

1. **自动创建空闲任务 (`prvIdleTask`)**：
   - 优先级固定为 `0`（系统最低优先级）。
   - 为其指定静态 TCB (`xIdleTaskTCB`) 与静态栈数组 (`uxIdleTaskStack`)。
   - 挂载到 0 级就绪链表 `pxReadyTasksLists[0]`。
2. **调用硬件移植层 `xPortStartScheduler()`**：
   - 配置 Cortex-M SysTick 定时器与 PendSV 中断优先级。
   - 启动系统中的第一个任务。

---

## 2. 空闲任务 (Idle Task) 的职责

在 RTOS 系统中，必须保证**任何时刻都至少有一个任务处于运行状态**。

当应用层所有任务都处于阻塞（等待延时、信号量、队列）或挂起状态时：
- 调度器会自动选择 0 级就绪链表中的 **Idle Task** 运行。
- 空闲任务可以在无限循环中执行低功耗休眠指令（如 ARM 的 `WFI` - Wait For Interrupt），降低系统功耗。
- 空闲任务还可用于回收被删除任务的动态内存。

---

## 3. 测试验证输出

执行 `make run` 的模块 3 测试结果：

```text
================ [模块测试 3] MiniRTOS 调度器启动与 IDLE 任务 ================
IDLE 任务所在就绪链表节点数: 1 (期望: 1)
0 级就绪链表首个任务名: IDLE (期望: IDLE)
0 级就绪链表首个任务优先级: 0 (期望: 0)
```
