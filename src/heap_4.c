#include "../include/portable.h"
#include <stdio.h>

/* 8 字节对齐掩码定义 */
#define portBYTE_ALIGNMENT 8
#define portBYTE_ALIGNMENT_MASK (0x0007)

/* 内存块头结构体 */
typedef struct A_BLOCK_LINK {
  struct A_BLOCK_LINK
      *pxNextFreeBlock; /* 指向下一个空闲块 (按物理内存地址递增排列) */
  size_t xBlockSize;    /* 包含结构体头部的总块大小 (最高位为已分配标记) */
} BlockLink_t;

/* 结构体头部在 8 字节对齐后的真实字节数 */
static const size_t xHeapStructSize =
    (sizeof(BlockLink_t) + ((size_t)(portBYTE_ALIGNMENT - 1))) &
    ~((size_t)portBYTE_ALIGNMENT_MASK);

/* 4KB 堆内存数组实体 */
static uint8_t ucHeap[configTOTAL_HEAP_SIZE];

/* 链表头部哨兵与尾部哨兵指针 */
static BlockLink_t xStart;
static BlockLink_t *pxEnd = NULL;

/* 记录剩余可用堆内存字节数 */
static size_t xFreeBytesRemaining = 0U;

/* 块已分配标记位 (最高位) */
static size_t xBlockAllocatedBit = 0U;

/* 内部辅助：将空闲块插入链表并尝试与物理相邻的左右空闲块合并 */
static void prvInsertBlockIntoFreeList(BlockLink_t *pxBlockToInsert) {
  BlockLink_t *pxIterator;
  uint8_t *puc;

  /* 1. 遍历链表，找到插入点 (保持链表按物理内存地址递增排列) */
  for (pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert;
       pxIterator = pxIterator->pxNextFreeBlock) {
    /* 仅移动 iterator 指针 */
  }

  /* 2. 检查待插入块是否与【后一个空闲块】物理相邻，若是则自动合并 */
  puc = (uint8_t *)pxBlockToInsert;
  if ((puc + pxBlockToInsert->xBlockSize) ==
      (uint8_t *)pxIterator->pxNextFreeBlock) {
    if (pxIterator->pxNextFreeBlock != pxEnd) {
      pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
      pxBlockToInsert->pxNextFreeBlock =
          pxIterator->pxNextFreeBlock->pxNextFreeBlock;
    } else {
      pxBlockToInsert->pxNextFreeBlock = pxEnd;
    }
  } else {
    pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
  }

  /* 3. 检查待插入块是否与【前一个空闲块】物理相邻，若是则自动合并 */
  puc = (uint8_t *)pxIterator;
  if ((puc + pxIterator->xBlockSize) == (uint8_t *)pxBlockToInsert) {
    pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
    pxIterator->pxNextFreeBlock = pxBlockToInsert->pxNextFreeBlock;
  } else {
    pxIterator->pxNextFreeBlock = pxBlockToInsert;
  }
}

/* 内部辅助：初始化堆内存 */
static void prvHeapInit(void) {
  BlockLink_t *pxFirstFreeBlock;
  uint8_t *pucAlignedHeap;
  size_t uxAddress;
  size_t xTotalHeapSize = configTOTAL_HEAP_SIZE;

  /* 计算最高位分配标记掩码 (如 32位系统为 0x80000000) */
  xBlockAllocatedBit = ((size_t)1) << ((sizeof(size_t) * 8U) - 1U);

  /* 8 字节对齐处理 ucHeap 起始地址 */
  uxAddress = (size_t)ucHeap;
  if ((uxAddress & portBYTE_ALIGNMENT_MASK) != 0) {
    uxAddress += (portBYTE_ALIGNMENT - 1);
    uxAddress &= ~((size_t)portBYTE_ALIGNMENT_MASK);
    xTotalHeapSize -= uxAddress - (size_t)ucHeap;
  }
  pucAlignedHeap = (uint8_t *)uxAddress;

  /* 初始化链表起始哨兵 xStart */
  xStart.pxNextFreeBlock = (BlockLink_t *)pucAlignedHeap;
  xStart.xBlockSize = (size_t)0;

  /* 在对齐后的堆内存末尾放置 pxEnd 尾部哨兵 */
  uxAddress = ((size_t)pucAlignedHeap) + xTotalHeapSize - xHeapStructSize;
  uxAddress &= ~((size_t)portBYTE_ALIGNMENT_MASK);
  pxEnd = (BlockLink_t *)uxAddress;
  pxEnd->xBlockSize = 0;
  pxEnd->pxNextFreeBlock = NULL;

  /* 初始化第一个完整的大空闲块 */
  pxFirstFreeBlock = (BlockLink_t *)pucAlignedHeap;
  pxFirstFreeBlock->xBlockSize = uxAddress - (size_t)pxFirstFreeBlock;
  pxFirstFreeBlock->pxNextFreeBlock = pxEnd;

  xFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
}

/* 1. 动态内存分配 API */
void *pvPortMalloc(size_t xWantedSize) {
  BlockLink_t *pxBlock;
  BlockLink_t *pxPreviousBlock;
  BlockLink_t *pxNewBlockLink;
  void *pvReturn = NULL;

  if (pxEnd == NULL) {
    prvHeapInit();
  }

  /* 过滤最高位分配标记 */
  if ((xWantedSize & xBlockAllocatedBit) == 0) {
    if (xWantedSize > 0) {
      /* 叠加头部大小并进行 8 字节对齐 */
      xWantedSize += xHeapStructSize;
      if ((xWantedSize & portBYTE_ALIGNMENT_MASK) != 0x00) {
        xWantedSize +=
            (portBYTE_ALIGNMENT - (xWantedSize & portBYTE_ALIGNMENT_MASK));
      }
    }

    if ((xWantedSize > 0) && (xWantedSize <= xFreeBytesRemaining)) {
      /* 遍历空闲链表查找合适大小的块 (First-Fit 首次适应) */
      pxPreviousBlock = &xStart;
      pxBlock = xStart.pxNextFreeBlock;

      while ((pxBlock->xBlockSize < xWantedSize) &&
             (pxBlock->pxNextFreeBlock != NULL)) {
        pxPreviousBlock = pxBlock;
        pxBlock = pxBlock->pxNextFreeBlock;
      }

      if (pxBlock != pxEnd) {
        /* 找到匹配块！返回指向块头之后的可用内存地址 */
        pvReturn = (void *)(((uint8_t *)pxPreviousBlock->pxNextFreeBlock) +
                            xHeapStructSize);

        /* 从空闲链表中剥离该块 */
        pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

        /* 若剩余块大小足够大，拆分出一个新的小空闲块 (Split) */
        if ((pxBlock->xBlockSize - xWantedSize) >= (xHeapStructSize * 2U)) {
          pxNewBlockLink = (BlockLink_t *)(((uint8_t *)pxBlock) + xWantedSize);
          pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
          pxBlock->xBlockSize = xWantedSize;

          prvInsertBlockIntoFreeList(pxNewBlockLink);
        }

        xFreeBytesRemaining -= pxBlock->xBlockSize;

        /* 标记该块为已分配 (最高位置 1) */
        pxBlock->xBlockSize |= xBlockAllocatedBit;
        pxBlock->pxNextFreeBlock = NULL;
      }
    }
  }

  return pvReturn;
}

/* 2. 动态内存释放 API */
void vPortFree(void *pv) {
  uint8_t *puc = (uint8_t *)pv;
  BlockLink_t *pxLink;

  if (pv != NULL) {
    /* 向前偏移找到对应的 BlockLink_t 块头 */
    puc -= xHeapStructSize;
    pxLink = (BlockLink_t *)puc;

    /* 验证该块确实已被分配 */
    if ((pxLink->xBlockSize & xBlockAllocatedBit) != 0) {
      if (pxLink->pxNextFreeBlock == NULL) {
        /* 清除分配标记 */
        pxLink->xBlockSize &= ~xBlockAllocatedBit;

        xFreeBytesRemaining += pxLink->xBlockSize;

        /* 重新插入空闲链表，并在内部自动与物理相邻的左右空闲块合并 */
        prvInsertBlockIntoFreeList((BlockLink_t *)pxLink);
      }
    }
  }
}

/* 3. 获取剩余可用堆空间 */
size_t xPortGetFreeHeapSize(void) {
  if (pxEnd == NULL) {
    prvHeapInit();
  }
  return xFreeBytesRemaining;
}
