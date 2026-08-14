# MiniRTOS 核心笔记（八）：动态内存管理 (heap_4) 与碎片自动合并

本文档记录 MiniRTOS 开发阶段三中关于**动态内存分配算法 `heap_4`** 的底层架构原理与实现细节。

---

## 1. 为什么选择 `heap_4` 算法？

在嵌入式 RTOS 系统中，传统的 C 标准库 `malloc()` / `free()` 存在两大缺陷：
1. **不可确定性 (Non-deterministic time)**：执行时间不稳定，无法满足实时性需求。
2. **内存碎片化 (Fragmentation)**：频繁申请和释放不同大小的内存块会导致内存空洞，最终无法分配连续的大内存。

FreeRTOS 经典的 `heap_4.c` 实现了：
- **首次适应算法 (First-Fit)**：高效快速地定位空闲内存块。
- **内存对齐 (8-Byte Alignment)**：保证符合 ARM AAPCS 规范。
- **相邻空闲块合并 (Block Coalescing)**：释放内存时，自动将物理地址相邻的左右空闲块**合并为一个整体的大空闲块**，彻底杜绝内存碎片化！

---

## 2. 内存块头结构体 (`BlockLink_t`)

每个被分配或空闲的内存块首部，均带有 `BlockLink_t` 结构体：

```c
typedef struct A_BLOCK_LINK
{
    struct A_BLOCK_LINK * pxNextFreeBlock; /* 按物理内存地址递增指向下一个空闲块 */
    size_t xBlockSize;                    /* 当前块的总字节数 (最高位为已分配标记) */
} BlockLink_t;
```

### 2.1 最高位标记法 (Bit Masking)
在 32 位系统中，内存块的最大尺寸远小于 2GB。`heap_4` 利用 `xBlockSize` 的**最高位 (Bit 31)** 作为该内存块是否已被分配的标志标志：
- 最高位为 `1`：表示该内存块已被分配使用。
- 最高位为 `0`：表示该内存块处于空闲状态，挂载在空闲链表中。

---

## 3. 分配与合并流程

### 3.1 `pvPortMalloc(xWantedSize)` 流程
1. **对齐计算**：叠加 `sizeof(BlockLink_t)` 头大小，并向上向上修正为 **8 字节对齐**。
2. **First-Fit 遍历**：从 `xStart` 链表头开始遍历，寻找第一个 `xBlockSize >= xWantedSize` 的空闲块。
3. **拆分 (Split)**：若匹配块的大小扣除请求大小后，剩余空间还能装下 2 个块头，则将余下空间拆分为一个新的小空闲块并插回链表。
4. **剥离标记**：将分配块从空闲链表中剥离，将其 `xBlockSize` 最高位置 1，返回指向数据区（块头之后）的指针。

### 3.2 `vPortFree(pv)` 流程
1. **地址倒退**：将传入指针向前偏移 `xHeapStructSize` 字节，获取对应的 `BlockLink_t` 块头。
2. **清标志位**：清除最高位分配标志。
3. **插回与双向合并 (Coalesce)**：
   - 插入到按物理地址递增排序的空闲链表中。
   - 检查其**右邻居**：若 `(uint8_t*)pxBlock + size == (uint8_t*)pxNextBlock`，与右侧空闲块合并。
   - 检查其**左邻居**：若 `(uint8_t*)pxPrevBlock + prev_size == (uint8_t*)pxBlock`，与左侧空闲块合并。

---

## 4. Unity 单元测试验证输出

运行 `make test` 输出：

```text
=======================================================
        Unity C Unit Testing Framework (MiniRTOS)       
=======================================================
[TEST 1] test_List_Initialization (line 186) -> [PASS]
[TEST 2] test_List_Ascending_Insertion_And_Remove (line 187) -> [PASS]
[TEST 3] test_Task_StaticCreation_And_StackFrame (line 190) -> [PASS]
[TEST 4] test_Task_Delay_And_TickWakeup (line 193) -> [PASS]
[TEST 5] test_RoundRobin_Scheduling (line 196) -> [PASS]
[TEST 6] test_Heap4_Malloc_Free_And_Coalescing (line 199) -> [PASS]

----------------------- SUMMARY -----------------------
6 Tests, 0 Failures, 0 Ignored
RESULT: SUCCESS (ALL TESTS PASSED)
=======================================================
```
