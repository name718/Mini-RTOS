#include "../include/list.h"
#include "../include/task.h"
#include <stdio.h>

#define TASK_STACK_SIZE 128

/* 静态分配应用任务 1 和 2 的内存 */
static StackType_t ucTask1Stack[TASK_STACK_SIZE];
static TCB_t xTask1TCB;

static StackType_t ucTask2Stack[TASK_STACK_SIZE];
static TCB_t xTask2TCB;

/* 用户任务 1 */
void vAppTask1(void *pvParameters) {
  (void)pvParameters;
  printf("App Task 1 正在运行...\n");
  while (1) {
    /* 任务业务逻辑 */
  }
}

/* 用户任务 2 */
void vAppTask2(void *pvParameters) {
  (void)pvParameters;
  printf("App Task 2 正在运行...\n");
  while (1) {
    /* 任务业务逻辑 */
  }
}

int main(void) {
  printf("================ MiniRTOS 应用程序启动 ================\n");

  /* 1. 创建应用程序任务 */
  xTaskCreateStatic(vAppTask1, "AppTask1", TASK_STACK_SIZE, NULL, 1,
                    ucTask1Stack, &xTask1TCB);
  xTaskCreateStatic(vAppTask2, "AppTask2", TASK_STACK_SIZE, NULL, 2,
                    ucTask2Stack, &xTask2TCB);

  /* 2. 启动 RTOS 调度器 */
  printf("启动 MiniRTOS 调度器，CPU 执行权交由内核指针: %s...\n",
         pxCurrentTCB->pcTaskName);
  vTaskStartScheduler();

  return 0;
}
