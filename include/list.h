#ifndef LIST_H
#define LIST_H

#include <stddef.h>
#include <stdint.h>

/* FreeRTOS 基本数据类型别名 */
typedef uint32_t TickType_t;  /* 滴答定时器计数值类型 (32位无符号整数) */
typedef int32_t BaseType_t;   /* 基础有符号整数(匹配架构字长,32位) */
typedef uint32_t UBaseType_t; /* 基础无符号整数(匹配架构字长,32位) */

/* 系统最大延时常数（用于表示无限制等待或链表末尾哨兵值） */
#define portMAX_DELAY ((TickType_t)0xffffffffUL)

/* 结构体前置声明 */
struct xLIST_ITEM;
struct xMINI_LIST_ITEM;

/* 1. 精简节点结构体（用于链表哨兵末尾） */
struct xMINI_LIST_ITEM {
  TickType_t xItemValue;
  struct xLIST_ITEM *pxNext;     /* 指向下一个节点 */
  struct xLIST_ITEM *pxPrevious; /* 指向上一个节点 */
};
typedef struct xMINI_LIST_ITEM MiniListItem_t;

/* 2. 完整节点结构体 */
struct xLIST_ITEM {
  TickType_t xItemValue;         /* 辅助排序值 */
  struct xLIST_ITEM *pxNext;     /* 指向下一个节点 */
  struct xLIST_ITEM *pxPrevious; /* 指向上一个节点 */
  void *pvOwner;                 /* 拥有该节点的父对象（通常指向任务 TCB） */
  void *pxContainer;             /* 指向该节点当前所在的链表 List_t */
};

typedef struct xLIST_ITEM ListItem_t;

/* 3. 链表头结构体 */
typedef struct xLIST {
  UBaseType_t uxNumberOfItems; /* 当前链表中的节点数 */
  ListItem_t *pxIndex;         /* 链表遍历指针（用于轮转调度） */
  MiniListItem_t xListEnd;     /* 链表末尾哨兵节点 */
} List_t;

/* 4. 常用辅助宏定义 */
/* 设置/获取节点对应的 Owner（通常是 TCB 指针） */
#define listSET_LIST_ITEM_OWNER(pxListItem, pxOwner)                           \
  ((pxListItem)->pvOwner = (void *)(pxOwner))
#define listGET_LIST_ITEM_OWNER(pxListItem) ((pxListItem)->pvOwner)

/* 设置/获取节点的排序值 xItemValue */
#define listSET_LIST_ITEM_VALUE(pxListItem, xValue)                            \
  ((pxListItem)->xItemValue = (xValue))
#define listGET_LIST_ITEM_VALUE(pxListItem) ((pxListItem)->xItemValue)

/* 获取链表的第一个有效节点 (哨兵节点后面的节点) */
#define listGET_HEAD_ENTRY(pxList) ((pxList)->xListEnd.pxNext)

/* 判断链表是否为空 */
#define listLIST_IS_EMPTY(pxList)                                              \
  ((BaseType_t)((pxList)->uxNumberOfItems == (UBaseType_t)0))

/* 获取节点的下一个节点 */
#define listGET_NEXT(pxListItem) ((pxListItem)->pxNext)

/* 获取链表的尾部哨兵节点指针（用作遍历结束的判断标记） */
#define listGET_END_MARKER(pxList) ((ListItem_t *)&((pxList)->xListEnd))

/* 5. 核心 API 函数声明 */
/* 初始化链表头 */
void vListInitialise(List_t *const pxList);

/* 初始化单个节点 */
void vListInitialiseItem(ListItem_t *const pxItem);

/* 插入节点到链表末尾（相对于 pxIndex） */
void vListInsertEnd(List_t *const pxList, ListItem_t *const pxNewListItem);

/* 按 xItemValue 升序插入节点 */
void vListInsert(List_t *const pxList, ListItem_t *const pxNewListItem);

/* 从链表中移除指定节点，返回移除后链表剩余节点数 */
UBaseType_t uxListRemove(ListItem_t *const pxItemToRemove);

#endif
