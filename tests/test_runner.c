#include "../include/list.h"
#include "../include/portable.h"
#include "../include/task.h"
#include "../include/queue.h"
#include "../include/semphr.h"
#include "unity/unity.h"

extern List_t pxReadyTasksLists[configMAX_PRIORITIES];

/* setUp and tearDown called before and after each test function */
void setUp(void) {}

void tearDown(void) {}

/* ------------------ Test Suite 1: List Operations ------------------ */
void test_List_Initialization(void) {
  List_t list;
  vListInitialise(&list);

  TEST_ASSERT_EQUAL_UINT32(0, list.uxNumberOfItems);
  TEST_ASSERT_EQUAL_PTR(&(list.xListEnd), list.pxIndex);
  TEST_ASSERT_EQUAL_HEX32(portMAX_DELAY, list.xListEnd.xItemValue);
  TEST_ASSERT_TRUE(listLIST_IS_EMPTY(&list));
}

void test_List_Ascending_Insertion_And_Remove(void) {
  List_t list;
  ListItem_t item1, item2, item3;

  vListInitialise(&list);
  vListInitialiseItem(&item1);
  vListInitialiseItem(&item2);
  vListInitialiseItem(&item3);

  item1.xItemValue = 40;
  item2.xItemValue = 10;
  item3.xItemValue = 20;

  vListInsert(&list, &item1);
  vListInsert(&list, &item2);
  vListInsert(&list, &item3);

  TEST_ASSERT_EQUAL_UINT32(3, list.uxNumberOfItems);

  /* Verify ascending order: 10 -> 20 -> 40 */
  ListItem_t *head = listGET_HEAD_ENTRY(&list);
  TEST_ASSERT_EQUAL_UINT32(10, head->xItemValue);

  head = listGET_NEXT(head);
  TEST_ASSERT_EQUAL_UINT32(20, head->xItemValue);

  head = listGET_NEXT(head);
  TEST_ASSERT_EQUAL_UINT32(40, head->xItemValue);

  /* Test item removal */
  UBaseType_t remaining = uxListRemove(&item2); // Remove 10
  TEST_ASSERT_EQUAL_UINT32(2, remaining);
  TEST_ASSERT_EQUAL_UINT32(2, list.uxNumberOfItems);
  TEST_ASSERT_NULL(item2.pxContainer);
}

/* ------------------ Test Suite 2: Task Creation & Stack ------------------ */
#define TEST_STACK_DEPTH 128
static StackType_t testStack[TEST_STACK_DEPTH];
static TCB_t testTCB;

static void dummyTaskFunc(void *param) {
  (void)param;
  while (1) {
  }
}

void test_Task_StaticCreation_And_StackFrame(void) {
  prvInitialiseTaskLists();
  pxCurrentTCB = NULL;

  TaskHandle_t handle =
      xTaskCreateStatic(dummyTaskFunc, "UnityTask", TEST_STACK_DEPTH,
                        (void *)0xDEADBEEF, 2, testStack, &testTCB);

  TEST_ASSERT_NOT_NULL(handle);
  TCB_t *tcb = (TCB_t *)handle;
  TEST_ASSERT_EQUAL_STRING("UnityTask", tcb->pcTaskName);
  TEST_ASSERT_EQUAL_UINT32(2, tcb->uxPriority);
  TEST_ASSERT_EQUAL_PTR(tcb, pxCurrentTCB);

  /* Check forged stack registers */
  StackType_t *sp = (StackType_t *)tcb->pxTopOfStack;
  TEST_ASSERT_EQUAL_HEX32(0x04040404, sp[0]);  /* R4 */
  TEST_ASSERT_EQUAL_HEX32(0x11111111, sp[7]);  /* R11 */
  TEST_ASSERT_EQUAL_HEX32(0xDEADBEEF, sp[8]);  /* R0 parameter */
  TEST_ASSERT_EQUAL_HEX32(0x01000000, sp[15]); /* xPSR Thumb bit */
}

/* ------------------ Test Suite 3: Task Delay & Tick Increment ------------------ */
void test_Task_Delay_And_TickWakeup(void) {
  prvInitialiseTaskLists();
  pxCurrentTCB = NULL;

  xTaskCreateStatic(dummyTaskFunc, "DelayTask", TEST_STACK_DEPTH, NULL, 2,
                    testStack, &testTCB);
  TEST_ASSERT_EQUAL_UINT32(1, pxReadyTasksLists[2].uxNumberOfItems);

  /* Call vTaskDelay(3) */
  vTaskDelay(3);
  TEST_ASSERT_EQUAL_UINT32(0, pxReadyTasksLists[2].uxNumberOfItems);

  /* Ticks 1 and 2: Should not wake up */
  TEST_ASSERT_EQUAL_INT(0, xTaskIncrementTick());
  TEST_ASSERT_EQUAL_UINT32(0, pxReadyTasksLists[2].uxNumberOfItems);

  TEST_ASSERT_EQUAL_INT(0, xTaskIncrementTick());
  TEST_ASSERT_EQUAL_UINT32(0, pxReadyTasksLists[2].uxNumberOfItems);

  /* Tick 3: Should wake up and return switch required (1) */
  TEST_ASSERT_EQUAL_INT(1, xTaskIncrementTick());
  TEST_ASSERT_EQUAL_UINT32(1, pxReadyTasksLists[2].uxNumberOfItems);
}

/* ------------------ Test Suite 4: Round Robin Scheduling ------------------ */
static StackType_t testStack2[TEST_STACK_DEPTH];
static TCB_t testTCB2;

void test_RoundRobin_Scheduling(void) {
  prvInitialiseTaskLists();
  pxCurrentTCB = NULL;

  xTaskCreateStatic(dummyTaskFunc, "TaskA", TEST_STACK_DEPTH, NULL, 2,
                    testStack, &testTCB);
  xTaskCreateStatic(dummyTaskFunc, "TaskB", TEST_STACK_DEPTH, NULL, 2,
                    testStack2, &testTCB2);

  TEST_ASSERT_EQUAL_STRING("TaskA", pxCurrentTCB->pcTaskName);

  /* 1st switch: moves pxIndex from xListEnd to TaskA */
  vTaskSwitchContext();
  TEST_ASSERT_EQUAL_STRING("TaskA", pxCurrentTCB->pcTaskName);

  /* 2nd switch: moves pxIndex from TaskA to TaskB */
  vTaskSwitchContext();
  TEST_ASSERT_EQUAL_STRING("TaskB", pxCurrentTCB->pcTaskName);

  /* 3rd switch: moves pxIndex over xListEnd back to TaskA */
  vTaskSwitchContext();
  TEST_ASSERT_EQUAL_STRING("TaskA", pxCurrentTCB->pcTaskName);
}

/* ------------------ Test Suite 5: Heap_4 Memory Management ------------------ */
void test_Heap4_Malloc_Free_And_Coalescing(void) {
  size_t initialFreeSize = xPortGetFreeHeapSize();
  TEST_ASSERT_TRUE(initialFreeSize > 0);

  /* Allocate 100 bytes */
  void *p1 = pvPortMalloc(100);
  TEST_ASSERT_NOT_NULL(p1);
  size_t freeAfterP1 = xPortGetFreeHeapSize();
  TEST_ASSERT_TRUE(freeAfterP1 < initialFreeSize);

  /* Allocate 200 bytes */
  void *p2 = pvPortMalloc(200);
  TEST_ASSERT_NOT_NULL(p2);

  /* Free p1 and p2 */
  vPortFree(p1);
  vPortFree(p2);

  /* Verify memory coalesced back to initial size */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)initialFreeSize,
                           (uint32_t)xPortGetFreeHeapSize());
}

/* ------------------ Test Suite 6: Dynamic Task Creation & Deletion ------------------ */
void test_Task_DynamicCreate_And_Delete(void) {
  prvInitialiseTaskLists();
  pxCurrentTCB = NULL;

  size_t initialFreeHeap = xPortGetFreeHeapSize();
  TaskHandle_t dynTaskHandle = NULL;

  /* Create dynamic task with priority 3 */
  BaseType_t status =
      xTaskCreate(dummyTaskFunc, "DynTask", 128, NULL, 3, &dynTaskHandle);

  TEST_ASSERT_EQUAL_INT(pdPASS, status);
  TEST_ASSERT_NOT_NULL(dynTaskHandle);
  TEST_ASSERT_EQUAL_PTR(dynTaskHandle, pxCurrentTCB);
  TEST_ASSERT_EQUAL_STRING("DynTask", pxCurrentTCB->pcTaskName);
  TEST_ASSERT_TRUE(xPortGetFreeHeapSize() < initialFreeHeap);

  /* Delete dynamic task */
  vTaskDelete(dynTaskHandle);

  /* Verify heap memory returned back to initial heap size */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)initialFreeHeap,
                           (uint32_t)xPortGetFreeHeapSize());
}

/* ------------------ Test Suite 7: Queue RingBuffer Send & Receive ------------------ */
void test_Queue_Create_Send_Receive_RingBuffer(void) {
  size_t initialFreeHeap = xPortGetFreeHeapSize();

  /* 1. 创建深度为 3，消息大小为 4 字节的队列 */
  QueueHandle_t xQueue = xQueueCreate(3, sizeof(uint32_t));
  TEST_ASSERT_NOT_NULL(xQueue);
  TEST_ASSERT_TRUE(xPortGetFreeHeapSize() < initialFreeHeap);

  uint32_t rxValue = 0;
  /* 2. 此时队列为空，接收应该失败 */
  TEST_ASSERT_EQUAL_INT(errQUEUE_EMPTY, xQueueReceive(xQueue, &rxValue, 0));

  /* 3. 连续发送 3 条消息填满队列 */
  uint32_t tx1 = 100, tx2 = 200, tx3 = 300, tx4 = 400;
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueSend(xQueue, &tx1, 0));
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueSend(xQueue, &tx2, 0));
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueSend(xQueue, &tx3, 0));

  /* 4. 队列已满，再发送应该失败 */
  TEST_ASSERT_EQUAL_INT(errQUEUE_FULL, xQueueSend(xQueue, &tx4, 0));

  /* 5. 读取 1 条数据 (期望 100) */
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(xQueue, &rxValue, 0));
  TEST_ASSERT_EQUAL_UINT32(100, rxValue);

  /* 6. 腾出空间后，写入 400 (测试写指针回绕 Ring Buffer Wrap Around) */
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueSend(xQueue, &tx4, 0));

  /* 7. 依次读取剩余 3 条数据 (期望 200, 300, 400，测试读指针回绕) */
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(xQueue, &rxValue, 0));
  TEST_ASSERT_EQUAL_UINT32(200, rxValue);

  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(xQueue, &rxValue, 0));
  TEST_ASSERT_EQUAL_UINT32(300, rxValue);

  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(xQueue, &rxValue, 0));
  TEST_ASSERT_EQUAL_UINT32(400, rxValue);

  /* 8. 队列再次为空 */
  TEST_ASSERT_EQUAL_INT(errQUEUE_EMPTY, xQueueReceive(xQueue, &rxValue, 0));

  /* 9. 释放队列内存 */
  vPortFree(xQueue);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)initialFreeHeap, (uint32_t)xPortGetFreeHeapSize());
}

/* ------------------ Test Suite 8: Queue Blocking & Unblocking ------------------ */
void test_Queue_Blocking_And_Unblocking(void) {
  prvInitialiseTaskLists();
  pxCurrentTCB = NULL;

  /* 创建接收任务 (高优先级 2) 和 发送任务 (低优先级 1) */
  xTaskCreateStatic(dummyTaskFunc, "RxTask", TEST_STACK_DEPTH, NULL, 2, testStack, &testTCB);
  xTaskCreateStatic(dummyTaskFunc, "TxTask", TEST_STACK_DEPTH, NULL, 1, testStack2, &testTCB2);

  /* 初始时当前任务为高优先级的 RxTask */
  TEST_ASSERT_EQUAL_STRING("RxTask", pxCurrentTCB->pcTaskName);

  /* 创建容量为 1 的队列 */
  QueueHandle_t xQueue = xQueueCreate(1, sizeof(uint32_t));
  TEST_ASSERT_NOT_NULL(xQueue);

  uint32_t rxVal = 0;
  /* RxTask 尝试从空队列读数据并阻塞 (超时设为 10 ticks) */
  (void)xQueueReceive(xQueue, &rxVal, 10);

  /* RxTask 阻塞后，内核应自动让出 CPU 并切换到 TxTask (优先级 1) */
  TEST_ASSERT_EQUAL_STRING("TxTask", pxCurrentTCB->pcTaskName);

  /* TxTask 向队列发送数据，将唤醒阻塞中的高优先级 RxTask */
  uint32_t txVal = 888;
  BaseType_t sendStatus = xQueueSend(xQueue, &txVal, 0);
  TEST_ASSERT_EQUAL_INT(pdPASS, sendStatus);

  /* 唤醒后，RxTask 优先级更高，CPU 自动抢占切换回 RxTask */
  TEST_ASSERT_EQUAL_STRING("RxTask", pxCurrentTCB->pcTaskName);

  vPortFree(xQueue);
}

/* ------------------ Test Suite 9: Binary & Counting Semaphores ------------------ */
void test_Binary_And_Counting_Semaphore(void) {
  /* 1. 二值信号量测试 */
  SemaphoreHandle_t xBinSem = xSemaphoreCreateBinary();
  TEST_ASSERT_NOT_NULL(xBinSem);

  /* 初始时无信号，Take 失败 */
  TEST_ASSERT_EQUAL_INT(pdFAIL, xSemaphoreTake(xBinSem, 0));

  /* 释放信号量 (Give) */
  TEST_ASSERT_EQUAL_INT(pdPASS, xSemaphoreGive(xBinSem));

  /* 成功获取信号量 (Take) */
  TEST_ASSERT_EQUAL_INT(pdPASS, xSemaphoreTake(xBinSem, 0));

  /* 再次 Take 失败 */
  TEST_ASSERT_EQUAL_INT(pdFAIL, xSemaphoreTake(xBinSem, 0));
  vPortFree(xBinSem);

  /* 2. 计数信号量测试 (最大 3，初始 2) */
  SemaphoreHandle_t xCountSem = xSemaphoreCreateCounting(3, 2);
  TEST_ASSERT_NOT_NULL(xCountSem);

  /* 消耗前 2 个信号量 */
  TEST_ASSERT_EQUAL_INT(pdPASS, xSemaphoreTake(xCountSem, 0));
  TEST_ASSERT_EQUAL_INT(pdPASS, xSemaphoreTake(xCountSem, 0));

  /* 第 3 次 Take 失败 (计数已减为 0) */
  TEST_ASSERT_EQUAL_INT(pdFAIL, xSemaphoreTake(xCountSem, 0));

  /* 连续 Give 3 次达到上限 */
  TEST_ASSERT_EQUAL_INT(pdPASS, xSemaphoreGive(xCountSem));
  TEST_ASSERT_EQUAL_INT(pdPASS, xSemaphoreGive(xCountSem));
  TEST_ASSERT_EQUAL_INT(pdPASS, xSemaphoreGive(xCountSem));

  /* 第 4 次 Give 失败 (已达到最大计数 3) */
  TEST_ASSERT_EQUAL_INT(pdFAIL, xSemaphoreGive(xCountSem));

  vPortFree(xCountSem);
}

/* ------------------ Test Suite 10: Mutex & Priority Inheritance ------------------ */
void test_Mutex_And_Priority_Inheritance(void) {
  prvInitialiseTaskLists();
  pxCurrentTCB = NULL;

  /* 创建低优先级任务 Task_L (优先级 1) 与 高优先级任务 Task_H (优先级 3) */
  TaskHandle_t hTaskL = xTaskCreateStatic(dummyTaskFunc, "Task_L", TEST_STACK_DEPTH, NULL, 1, testStack, &testTCB);
  TaskHandle_t hTaskH = xTaskCreateStatic(dummyTaskFunc, "Task_H", TEST_STACK_DEPTH, NULL, 3, testStack2, &testTCB2);

  TCB_t *tcbL = (TCB_t *)hTaskL;
  TCB_t *tcbH = (TCB_t *)hTaskH;

  /* 创建互斥量 */
  SemaphoreHandle_t xMutex = xSemaphoreCreateMutex();
  TEST_ASSERT_NOT_NULL(xMutex);

  /* 1. 模拟 Task_L 先运行并获取了互斥锁 */
  pxCurrentTCB = tcbL;
  TEST_ASSERT_EQUAL_INT(pdPASS, xSemaphoreTake(xMutex, 0));
  TEST_ASSERT_EQUAL_UINT32(1, tcbL->uxPriority);     /* 此时 Task_L 保持自身优先级 1 */

  /* 2. 模拟高优先级 Task_H 抢占运行并尝试获取同一个互斥锁 (带超时阻塞) */
  pxCurrentTCB = tcbH;
  (void)xSemaphoreTake(xMutex, 10);

  /* 3. 核心断言：由于 Task_H 阻塞在 Task_L 持有的锁上，Task_L 优先级被临时继承提升到 3！ */
  TEST_ASSERT_EQUAL_UINT32(3, tcbL->uxPriority);
  TEST_ASSERT_EQUAL_UINT32(1, tcbL->uxBasePriority); /* 基准优先级依然是 1 */
  TEST_ASSERT_EQUAL_PTR(tcbL, pxCurrentTCB);         /* Task_H 阻塞后，CPU 切换回被提升的 Task_L */

  /* 4. Task_L 执行完毕释放互斥锁 */
  TEST_ASSERT_EQUAL_INT(pdPASS, xSemaphoreGive(xMutex));

  /* 5. 核心断言：锁释放后，Task_L 优先级自动恢复回基准 1，Task_H 被唤醒并抢占 CPU！ */
  TEST_ASSERT_EQUAL_UINT32(1, tcbL->uxPriority);
  TEST_ASSERT_EQUAL_PTR(tcbH, pxCurrentTCB);

  vPortFree(xMutex);
}

int main(void) {
  UNITY_BEGIN();

  /* Suite 1: List */
  RUN_TEST(test_List_Initialization);
  RUN_TEST(test_List_Ascending_Insertion_And_Remove);

  /* Suite 2: Task */
  RUN_TEST(test_Task_StaticCreation_And_StackFrame);

  /* Suite 3: Delay & Tick */
  RUN_TEST(test_Task_Delay_And_TickWakeup);

  /* Suite 4: Round-Robin */
  RUN_TEST(test_RoundRobin_Scheduling);

  /* Suite 5: Heap_4 */
  RUN_TEST(test_Heap4_Malloc_Free_And_Coalescing);

  /* Suite 6: Dynamic Task Create & Delete */
  RUN_TEST(test_Task_DynamicCreate_And_Delete);

  /* Suite 7: Queue Ring Buffer */
  RUN_TEST(test_Queue_Create_Send_Receive_RingBuffer);

  /* Suite 8: Queue Blocking & Unblocking */
  RUN_TEST(test_Queue_Blocking_And_Unblocking);

  /* Suite 9: Binary & Counting Semaphores */
  RUN_TEST(test_Binary_And_Counting_Semaphore);

  /* Suite 10: Mutex & Priority Inheritance */
  RUN_TEST(test_Mutex_And_Priority_Inheritance);

  return UNITY_END();
}
