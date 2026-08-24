#include "../include/port.h"

/* ARM Cortex-M 架构的 xPSR 寄存器 Thumb 状态位定义 (Bit 24 必须置 1) */
#define portTHUMB_ADDRESS_BIT_MASK ((StackType_t)0x01000000UL)

/* 临界区嵌套深度计数器 */
static UBaseType_t uxCriticalNesting = 0xAAAAAAAAUL;

StackType_t *pxPortInitialiseStack(StackType_t *pxTopOfStack,
                                   TaskFunction_t pxCode, void *pvParameters) {
  /* 1. 模拟硬件自动压栈的 8 个寄存器 (xPSR, PC, LR, R12, R3, R2, R1, R0) */
  pxTopOfStack--;
  *pxTopOfStack =
      portTHUMB_ADDRESS_BIT_MASK; /* xPSR: Bit 24 置 1 表示 Thumb 模式 */

  pxTopOfStack--;
  *pxTopOfStack = ((uintptr_t)pxCode); /* PC: 存放任务入口函数地址 */
  pxTopOfStack--;
  *pxTopOfStack = ((StackType_t)0x00000000UL); /* LR: 函数返回地址，初始清 0 */

  pxTopOfStack--;
  *pxTopOfStack =
      ((StackType_t)0x12121212UL); /* R12 寄存器初始值 (填固定魔数方便调试) */

  pxTopOfStack--;
  *pxTopOfStack = ((StackType_t)0x03030303UL); /* R3 */

  pxTopOfStack--;
  *pxTopOfStack = ((StackType_t)0x02020202UL); /* R2 */

  pxTopOfStack--;
  *pxTopOfStack = ((StackType_t)0x01010101UL); /* R1 */

  pxTopOfStack--;
  *pxTopOfStack = ((uintptr_t)pvParameters); /* R0: 存放传给任务函数的参数指针 */

  /* 2. 模拟软件 (PendSV 中断服务函数) 手动压栈的 8 个寄存器 (R11 ~ R4) */

  pxTopOfStack--;
  *pxTopOfStack = ((StackType_t)0x11111111UL); /* R11 */

  pxTopOfStack--;
  *pxTopOfStack = ((StackType_t)0x10101010UL); /* R10 */

  pxTopOfStack--;
  *pxTopOfStack = ((StackType_t)0x09090909UL); /* R9 */

  pxTopOfStack--;
  *pxTopOfStack = ((StackType_t)0x08080808UL); /* R8 */

  pxTopOfStack--;
  *pxTopOfStack = ((StackType_t)0x07070707UL); /* R7 */

  pxTopOfStack--;
  *pxTopOfStack = ((StackType_t)0x06060606UL); /* R6 */

  pxTopOfStack--;
  *pxTopOfStack = ((StackType_t)0x05050505UL); /* R5 */

  pxTopOfStack--;
  *pxTopOfStack = ((StackType_t)0x04040404UL); /* R4 */

  /* 3. 返回最终伪造好以后的栈顶指针 */
  return pxTopOfStack;
}

BaseType_t xPortStartScheduler(void) {
  /* 启动调度器时复位临界区嵌套深度为 0 */
  uxCriticalNesting = 0;
  return 1;
}

/* 进入临界区 */
void vPortEnterCritical(void) {
  /* 在实际 Cortex-M 硬件上执行 __disable_irq() 或设置 BASEPRI */
  if (uxCriticalNesting == 0xAAAAAAAAUL) {
    uxCriticalNesting = 0;
  }
  uxCriticalNesting++;
}

/* 退出临界区 */
void vPortExitCritical(void) {
  if (uxCriticalNesting > 0) {
    uxCriticalNesting--;
    if (uxCriticalNesting == 0) {
      /* 在实际 Cortex-M 硬件上执行 __enable_irq() 或清除 BASEPRI */
    }
  }
}

/* 获取临界区当前嵌套层级 */
UBaseType_t uxPortGetCriticalNesting(void) {
  return uxCriticalNesting;
}
