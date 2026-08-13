
#include "../include/list.h"
#include <stdio.h>

int main() {
  List_t testList;
  ListItem_t item1, item2, item3;
  printf("================ MiniRTOS链表功能测试 ================\n");
  /* 1. 初始化链表与节点 */
  vListInitialise(&testList);
  vListInitialiseItem(&item1);
  vListInitialiseItem(&item2);
  vListInitialiseItem(&item3);
  /* 设置不同的 xItemValue */
  item1.xItemValue = 40;
  item2.xItemValue = 10;
  item3.xItemValue = 20;
  /* 2. 测试按值升序插入 (乱序插入 40, 10,20) */
  vListInsert(&testList, &item1);
  vListInsert(&testList, &item2);
  vListInsert(&testList, &item3);
  printf("[测试 1] 插入 40, 10, 20后，当前链表节点数: %u (期望值: 3)\n",
         (unsigned int)testList.uxNumberOfItems);
  /* 遍历打印升序结果 */
  printf("[测试 1] 链表节点升序排列结果:");
  ListItem_t *pxIterator = listGET_HEAD_ENTRY(&testList);

  while (pxIterator != listGET_END_MARKER(&testList)) {
    printf("%u -> ", (unsigned int)pxIterator->xItemValue);
    pxIterator = listGET_NEXT(pxIterator);
  }
  printf("END (哨兵 0xFFFFFFFF)\n\n");
  /* 3. 测试节点移除 */
  printf("[测试 2] 从链表中移除值为 10的节点...\n");
  uxListRemove(&item2);
  printf("[测试 2] 移除后当前链表节点数:%u (期望值: 2)\n",
         (unsigned int)testList.uxNumberOfItems);
  /* 再次遍历打印 */
  printf("[测试 2] 移除后的链表排列结果:");
  pxIterator = listGET_HEAD_ENTRY(&testList);
  while (pxIterator != listGET_END_MARKER(&testList)) {
    printf("%u -> ", (unsigned int)pxIterator->xItemValue);
    pxIterator = listGET_NEXT(pxIterator);
  }
  printf("END\n");

  printf("======================================================\n");
  return 0;
}
