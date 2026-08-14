#include "../include/list.h"
#include "../include/task.h"
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

/* ------------------ Test Suite 3: Task Delay & Tick Increment
 * ------------------ */
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

  return UNITY_END();
}
