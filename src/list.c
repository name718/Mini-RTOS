#include "../include/list.h"

/* 1. 初始化链表头 */
void vListInitialise(List_t *const pxList) {
  /* 遍历指针 pxIndex初始指向链表尾部的哨兵节点 */
  pxList->pxIndex = (ListItem_t *)&(pxList->xListEnd);
  /* 哨兵节点的值设为最大值0xFFFFFFFF，确保任何新节点插入时都排在哨兵前面 */
  pxList->xListEnd.xItemValue = portMAX_DELAY;
  /* 哨兵节点的 pxNext 和 pxPrevious都指向自己，形成一个只有哨兵的闭环双向链表
   */
  pxList->xListEnd.pxNext = (ListItem_t *)&(pxList->xListEnd);
  pxList->xListEnd.pxPrevious = (ListItem_t *)&(pxList->xListEnd);
  /* 链表中的有效节点数初始化为 0 */
  pxList->uxNumberOfItems = (UBaseType_t)0U;
}

/* 2. 初始化单个节点 */
void vListInitialiseItem(ListItem_t *const pxItem) {
  /* 节点刚创建时，并不属于任何链表，因此容器指针置NULL */
  pxItem->pxContainer = NULL;
}

/* 3. 插入节点到链表末尾（相对 pxIndex游标的位置） */
void vListInsertEnd(List_t *const pxList, ListItem_t *const pxNewListItem) {
  /* 获取当前的游标指针 */
  ListItem_t *const pxIndex = pxList->pxIndex;
  /* 1. 让新节点的 pxNext 指向 pxIndex */
  pxNewListItem->pxNext = pxIndex;
  /* 2. 让新节点的 pxPrevious 指向原先pxIndex 的前一个节点 */
  pxNewListItem->pxPrevious = pxIndex->pxPrevious;
  /* 3. 修改原先 pxIndex 前节点的pxNext，指向新节点 */
  pxIndex->pxPrevious->pxNext = pxNewListItem;
  /* 4. 修改 pxIndex 的pxPrevious，指向新节点 */
  pxIndex->pxPrevious = pxNewListItem;
  /* 5. 标记该节点当前被哪一个链表收容 */
  pxNewListItem->pxContainer = (void *)pxList;
  /* 6. 链表节点数加 1 */
  (pxList->uxNumberOfItems)++;
}
