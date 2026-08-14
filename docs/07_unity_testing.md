# MiniRTOS 核心笔记（七）：嵌入式 C 单元测试框架 Unity 引入

本文档记录 MiniRTOS 引入嵌入式 C 语言通用单元测试框架 **Unity（FreeRTOS 官方同款）** 的架构设计与测试用例说明。

---

## 1. 为什么引入 Unity 单元测试？

随着 MiniRTOS 内核功能的逐渐丰富（链表、TCB、多优先级、延时链表、溢出交换、Round-Robin 调度），手写 `printf` 测试不仅繁琐，而且无法自动化捕捉回归 Bug。

**Unity** 是 ThrowTheSwitch 推出的嵌入式 C 语言轻量级单元测试框架，具有以下优势：
1. **FreeRTOS 官方同款**：FreeRTOS 官方测试库原生使用 Unity 编写。
2. **丰富的断言库**：提供 `TEST_ASSERT_EQUAL_UINT32()`、`TEST_ASSERT_NULL()`、`TEST_ASSERT_EQUAL_STRING()` 等。
3. **测试状态隔离**：提供 `setUp()` / `tearDown()` 生命周期钩子。

---

## 2. 工程目录架构

```text
MiniRTOS/
├── tests/
│   ├── unity/
│   │   ├── unity.h           # Unity 核心头文件
│   │   ├── unity_internals.h # 内部数据结构与 longjmp 异常机制
│   │   └── unity.c           # Unity 断言与测试报告打印实现
│   └── test_runner.c         # 单元测试用例集与主入口
```

---

## 3. 测试用例清单 (Test Suites)

在 `tests/test_runner.c` 中覆盖了目前内核的所有核心模块：

1. **`test_List_Initialization`**：验证双向链表初始化、节点计数为 0、哨兵节点 `portMAX_DELAY` 初始值。
2. **`test_List_Ascending_Insertion_And_Remove`**：验证 `vListInsert` 乱序插入后的自动升序排列与 `uxListRemove` 双向解绑。
3. **`test_Task_StaticCreation_And_StackFrame`**：验证 `xTaskCreateStatic` 静态任务创建、优先权赋值、`pxTopOfStack` 8 字节对齐以及伪造栈帧中的寄存器值（R4-R11, R0, xPSR）。
4. **`test_Task_Delay_And_TickWakeup`**：验证 `vTaskDelay` 将任务从就绪链表剥离加入延时链表，以及 `xTaskIncrementTick` 到期唤醒重新挂载。
5. **`test_RoundRobin_Scheduling`**：验证同优先级任务在 `vTaskSwitchContext` 下 `pxIndex` 指针推进的公平轮转。

---

## 4. 自动化测试命令 (Makefile)

项目 Makefile 中新增了 `make test` 命令：

```bash
# 编译并自动运行所有 Unity 单元测试
make test
```

终端输出报告：

```text
=======================================================
        Unity C Unit Testing Framework (MiniRTOS)       
=======================================================
[TEST 1] test_List_Initialization (line 161) -> [PASS]
[TEST 2] test_List_Ascending_Insertion_And_Remove (line 162) -> [PASS]
[TEST 3] test_Task_StaticCreation_And_StackFrame (line 165) -> [PASS]
[TEST 4] test_Task_Delay_And_TickWakeup (line 168) -> [PASS]
[TEST 5] test_RoundRobin_Scheduling (line 171) -> [PASS]

----------------------- SUMMARY -----------------------
5 Tests, 0 Failures, 0 Ignored
RESULT: SUCCESS (ALL TESTS PASSED)
=======================================================
```
