# MiniRTOS 核心笔记（十四）：临界区管理与中断嵌套 (Critical Sections)

本文档记录 MiniRTOS 开发阶段五中关于**临界区保护 (`taskENTER_CRITICAL` / `taskEXIT_CRITICAL`)** 与 **可嵌套计数器 (`uxCriticalNesting`)** 的设计与实现。

---

## 1. 为什么需要临界区保护？

在实时操作系统中，当任务或内核在操作全局共享数据（如修改就绪链表、更新 Tick 计数器、调整 `pxCurrentTCB`）时，若被**外部硬件中断**或**SysTick 调度中断**打断，将导致链表指针错乱或数据竞争（Race Condition）。

### 1.1 临界区的核心作用
- **关闭中断**：禁止当前 CPU 响应可屏蔽中断，确保当前执行流绝对原子化。
- **支持嵌套**：函数 A 调用临界区，内部调用的子函数 B 也调用临界区，退出子函数 B 时不能过早打开全局中断，只有在最外层函数 A 退出时才真正重新开启中断！

---

## 2. 嵌套深度计数器原理 (`uxCriticalNesting`)

```c
/* 进入临界区 */
void vPortEnterCritical(void) {
  /* 1. 递增嵌套层级 */
  uxCriticalNesting++;

  /* 2. 首次进入临界区时，关闭 CPU 中断 */
  if (uxCriticalNesting == 1) {
    /* ARM Cortex-M 硬件指令：__disable_irq() 或设置 BASEPRI 寄存器 */
  }
}

/* 退出临界区 */
void vPortExitCritical(void) {
  if (uxCriticalNesting > 0) {
    /* 1. 递减嵌套层级 */
    uxCriticalNesting--;

    /* 2. 只有当嵌套层级回到 0 (最外层) 时，才真正恢复开启 CPU 中断 */
    if (uxCriticalNesting == 0) {
      /* ARM Cortex-M 硬件指令：__enable_irq() 或清除 BASEPRI 寄存器 */
    }
  }
}
```

---

## 3. Unity 单元测试验证输出

运行 `make test` 输出：

```text
[TEST 12] test_Critical_Sections -> [PASS]
```
- 测试了 1 层进入 $\rightarrow$ 2 层嵌套 $\rightarrow$ 逐层安全退出 $\rightarrow$ 最终恢复为 0。
