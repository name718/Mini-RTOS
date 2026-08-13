#include "../include/list.h"
#include "../include/task.h"
#include <stdio.h>

/* ------------------ 模块一：双向链表单元测试 ------------------ */
void test_list_module(void) {
  List_t testList;
  ListItem_t item1, item2, item3;

  printf("================ [模块测试 1] MiniRTOS 链表功能 ================\n");

  /* 1. 初始化链表与节点 */
  vListInitialise(&testList);
  vListInitialiseItem(&item1);
  vListInitialiseItem(&item2);
  vListInitialiseItem(&item3);

  item1.xItemValue = 40;
  item2.xItemValue = 10;
  item3.xItemValue = 20;

  /* 2. 升序插入 */
  vListInsert(&testList, &item1);
  vListInsert(&testList, &item2);
  vListInsert(&testList, &item3);

  printf("插入 40, 10, 20 后节点数: %u (期望: 3)\n",
         (unsigned int)testList.uxNumberOfItems);
  printf("链表升序结果: ");
  ListItem_t *pxIterator = listGET_HEAD_ENTRY(&testList);
  while (pxIterator != listGET_END_MARKER(&testList)) {
    printf("%u -> ", (unsigned int)pxIterator->xItemValue);
    pxIterator = listGET_NEXT(pxIterator);
  }
  printf("END\n");

  /* 3. 移除节点 */
  uxListRemove(&item2);
  printf("移除值为 10 的节点后结果: ");
  pxIterator = listGET_HEAD_ENTRY(&testList);
  while (pxIterator != listGET_END_MARKER(&testList)) {
    printf("%u -> ", (unsigned int)pxIterator->xItemValue);
    pxIterator = listGET_NEXT(pxIterator);
  }
  printf("END\n\n");
}

/* ------------------ 模块二：任务创建与栈解剖单元测试 ------------------ */
#define TASK_STACK_SIZE 128
static StackType_t ucTask1Stack[TASK_STACK_SIZE];
static TCB_t xTask1TCB;

void vTask1(void *pvParameters) {
  printf("Task 1 is running with parameter: %s\n", (char *)pvParameters);
  while (1) {
  }
}

void test_task_module(void) {
  printf(
      "================ [模块测试 2] MiniRTOS 任务静态创建 ================\n");

  TaskHandle_t xHandle =
      xTaskCreateStatic(vTask1, "Task_1", TASK_STACK_SIZE, (void *)0x12345678,
                        ucTask1Stack, &xTask1TCB);

  TCB_t *pxTCB = (TCB_t *)xHandle;

  printf("任务创建成功！任务名: %s\n", pxTCB->pcTaskName);
  printf("栈基地址: %p, 当前伪造栈顶 pxTopOfStack: %p\n",
         (void *)pxTCB->pxStack, (void *)pxTCB->pxTopOfStack);

  StackType_t *pxStackPointer = (StackType_t *)pxTCB->pxTopOfStack;

  printf("\n伪造任务栈内存寄存器上下文解剖 (自低地址至高地址):\n");
  printf("  --------------------------------------------------\n");
  printf("  | [R4 ]: 0x%08X (期望: 0x04040404)\n", pxStackPointer[0]);
  printf("  | [R5 ]: 0x%08X (期望: 0x05050505)\n", pxStackPointer[1]);
  printf("  | [R6 ]: 0x%08X (期望: 0x06060606)\n", pxStackPointer[2]);
  printf("  | [R7 ]: 0x%08X (期望: 0x07070707)\n", pxStackPointer[3]);
  printf("  | [R8 ]: 0x%08X (期望: 0x08080808)\n", pxStackPointer[4]);
  printf("  | [R9 ]: 0x%08X (期望: 0x09090909)\n", pxStackPointer[5]);
  printf("  | [R10]: 0x%08X (期望: 0x10101010)\n", pxStackPointer[6]);
  printf("  | [R11]: 0x%08X (期望: 0x11111111)\n", pxStackPointer[7]);
  printf("  | ------------------------------------------------\n");
  printf("  | [R0 ]: 0x%08X (入参 R0, 期望: 0x12345678)\n", pxStackPointer[8]);
  printf("  | [R1 ]: 0x%08X\n", pxStackPointer[9]);
  printf("  | [R2 ]: 0x%08X\n", pxStackPointer[10]);
  printf("  | [R3 ]: 0x%08X\n", pxStackPointer[11]);
  printf("  | [R12]: 0x%08X\n", pxStackPointer[12]);
  printf("  | [LR ]: 0x%08X\n", pxStackPointer[13]);
  printf("  | [PC ]: 函数地址 %p (0x%08X)\n",
         (void *)(uintptr_t)pxStackPointer[14], pxStackPointer[14]);
  printf("  | [xPSR]: 0x%08X (期望 Thumb bit24 1: 0x01000000)\n",
         pxStackPointer[15]);
  printf("  --------------------------------------------------\n\n");
}

/* ------------------ 主入口 ------------------ */
int main() {
  /* 依次运行历史与新增的测试模块 */
  test_list_module();
  test_task_module();

  return 0;
}
