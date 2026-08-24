# MiniRTOS 核心笔记（十五）：轻量级任务通知 (Task Notifications)

本文档记录 MiniRTOS 开发阶段五中关于 **任务通知 (Task Notifications)** 的底层原理与零内存开销通信优势。

---

## 1. 什么是任务通知？为什么它比队列/信号量快 45%？

在 FreeRTOS 中，传统的信号量、事件标志组、消息邮箱都需要在堆内存中独立开辟一个 `Queue_t` 或 `EventGroup_t` 结构体。

而 **任务通知 (Task Notification)** 直接内嵌在每一个任务的控制块 `TCB_t` 中：

```c
typedef struct tskTCB {
  ...
  /* 任务通知内部变量 (零堆内存开销，直接集成在 TCB 中) */
  volatile uint32_t ulNotifiedValue; /* 32 位通知值 (可充当计数器、位掩码或数据) */
  volatile uint8_t  ucNotifyState;   /* 通知状态 (NOT_WAITING, WAITING, RECEIVED) */
  ...
} TCB_t;
```

### 1.1 核心优势
1. **零额外堆内存开销**：每个任务天然自带，不需要动态分配任何 `Queue_t`；
2. **速度提升 45%**：无需经过通用的队列环形缓冲区处理，直接操作目标任务 TCB 寄存器级变量，唤醒速度极快！

---

## 2. 灵活的多重工作模式 (`eNotifyAction`)

通过传入不同的动作枚举 `eNotifyAction`，单个任务通知可以替代多种传统通信机制：

| 动作枚举 (`eNotifyAction`) | 对应替代的传统机制 | 行为说明 |
| :--- | :--- | :--- |
| **`eIncrement`** | 二值信号量 / 计数信号量 | `ulNotifiedValue++`，搭配 `ulTaskNotifyTake()` 消费 |
| **`eSetBits`** | 事件标志组 (Event Group) | `ulNotifiedValue \|= ulValue`，支持多事件位组合等待 |
| **`eSetValueWithOverwrite`** | 单值消息邮箱 (Mailbox) | 强制更新为最新数据 |
| **`eSetValueWithoutOverwrite`**| 单值消息队列 | 若旧通知未被消费则拒绝写入并返回 `pdFAIL` |

---

## 3. 核心 API 与实现

### 3.1 替代二值/计数信号量：`xTaskNotifyGive` 与 `ulTaskNotifyTake`
```c
/* 发送端：通知值自增 */
xTaskNotifyGive( xTaskToNotify );

/* 接收端：等待通知并自动递减或清空 */
uint32_t count = ulTaskNotifyTake( pdTRUE, xTicksToWait );
```

### 3.2 替代事件标志组/邮箱：`xTaskNotify` 与 `xTaskNotifyWait`
```c
/* 发送端：按位或设置事件 Bit 0 与 Bit 3 */
xTaskNotify( xTaskToNotify, ( 0x01 | 0x08 ), eSetBits );

/* 接收端：等待事件标志并提取通知值 */
uint32_t ulReceivedEvents = 0;
xTaskNotifyWait( 0x00, 0xFFFFFFFF, &ulReceivedEvents, xTicksToWait );
```

---

## 4. Unity 单元测试验证输出

运行 `make test` 输出：

```text
[TEST 13] test_Task_Notifications_NotifyTake -> [PASS]
[TEST 14] test_Task_Notifications_SetBits_And_NotifyWait -> [PASS]
```
