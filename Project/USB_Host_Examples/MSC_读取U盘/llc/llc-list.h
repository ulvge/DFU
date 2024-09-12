/**
 * @addtogroup cohda_llc LLC module
 * @{
 *
 * @addtogroup cohda_llc_intern LLC internals
 * @{
 *
 * @file
 * LLC: LLC list and entry functionality
 */

//------------------------------------------------------------------------------
// Copyright (c) 2013 Cohda Wireless Pty Ltd
//------------------------------------------------------------------------------

#ifndef __DRIVER__COHDA__LLC__LLC_LIST_H_
#define __DRIVER__COHDA__LLC__LLC_LIST_H_
#ifdef __KERNEL__

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#include <linux/semaphore.h>
#include <linux/list.h>

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

/**
 * @brief  list_for_each_entry_safe - iterate over list of given type safe
 *         against removal of list entry
 * @param pos  the type * to use as a loop cursor.
 * @param n:     another type * to use as temporary storage
 * @param head:  the head for your list.
 * @param member:the name of the list_struct within the struct.
 */
#define LLC_ListForEachEntrySafe(pos, n, head, member)         \
  for (pos = container_of((struct LLCList*)(head)->List.next, typeof(*pos), member), \
       n = container_of((struct LLCList*)pos->member.List.next, typeof(*pos), member); \
       &pos->member.List != &(head)->List;                                    \
       pos = n, n = container_of((struct LLCList*)n->member.List.next, typeof(*n), member))

/**
 * @brief   list_first_entry - get the first element from a list
 * @param pHead:    the list head to take the element from.
 * @param type:   the type of the struct this is embedded in.
 * @param member: the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define LLC_ListFirstEntry(head, type, member) \
  container_of((struct LLCList*)(head)->List.List.next, type, member)

/**
 * @brief   list_last_entry - get the last element from a list
 * @param pHead:    the list head to take the element from.
 * @param type:   the type of the struct this is embedded in.
 * @param member: the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define LLC_ListLastEntry(head, type, member) \
  container_of((struct LLCList*)(head)->List.List.prev, type, member)

//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------

// forward decl(s)
struct LLCDev;

/// Wrap up the linux free list structure
typedef struct LLCList
{
  /// Next, Prev pointers
  struct list_head List;
} tLLCList;

/// Generic LLCListItem
typedef struct LLCListItem
{
  /// Next, Prev pointers
  struct LLCList List;
  /// Unique Id
  uint8_t Id;
  /// Reference counter
  int8_t RefCnt;
  /// Return code
  int16_t Ret;
  /// Private user data area;
  uint32_t Priv[22];
} tLLCListItem;

/// LLC pool of entries
typedef struct LLCListItemPool
{
  /// Pool mutex
  struct semaphore Lock;
  /// Free list head & tail (allocs & free come from here)
  struct LLCList Free;
  /// Local storage of the pools addr
  struct LLCListItem *pPool;
  /// Denotes the end of the pool (For insanity checking list item pointers)
  void *pEnd;
} tLLCListItemPool;

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------
int LLC_ListSetup (struct LLCDev *pDev);
int LLC_ListRelease (struct LLCDev *pDev);

int LLC_ListItemAlloc (struct LLCListItem **ppItem);
int LLC_ListItemFree (struct LLCListItem *pItem);
int LLC_ListItemSendToPool (struct LLCListItem *pItem);
int LLC_ListItemPeek (struct LLCListItem **ppItem, int Id);
int LLC_ListItemStatic (struct LLCListItem **ppItem);

int LLC_ListHeadInit (struct LLCList *pHead);
int LLC_ListEmpty (struct LLCList *pHead);
int LLC_ListSingular (struct LLCList *pHead);
int LLC_ListItemGet (struct LLCListItem **ppItem,
                     struct LLCList *pHead);
int LLC_ListItemReturn (struct LLCListItem *pItem,
                        struct LLCList *pHead);
int LLC_ListItemAddGlobal (struct LLCListItem *pItem,
                        struct LLCList *pHead);
int LLC_ListItemAdd (struct LLCListItem *pNew,
                     struct LLCList *pHead);
int LLC_ListItemAddTail (struct LLCListItem *pItem,
                         struct LLCList *pHead);
int LLC_ListItemDel (struct LLCListItem *pItem,
                     struct LLCList *pHead);
int LLC_ListItemDelReInit (struct LLCListItem *pItem,
                           struct LLCList *pHead);
int LLC_ListItemMove (struct LLCListItem *pItem,
                      struct LLCList *pHead);
int LLC_ListItemMoveTail (struct LLCListItem *pItem,
                          struct LLCList *pHead);

                          
                          
#define QUEUEMEM     INT8U
#define QUEUENODE    struct node                          
                          
typedef struct {
  QUEUENODE  *head;
  QUEUENODE  *tail;
  INT16U     item;//ÔªËØ¸öÊý
} QUEUE;


BOOLEAN Queue_CreateQueue(QUEUE *que);
INT16U Queue_GetItem(QUEUE *que);
QUEUEMEM *Queue_GetHead(QUEUE *que);
QUEUEMEM *Queue_GetNextNode(QUEUEMEM *element);
QUEUEMEM *Queue_GetElement(QUEUE *que,INT16U index);
QUEUEMEM *Queue_DelElement(QUEUE *que, QUEUEMEM *element);
BOOLEAN Queue_Append(QUEUE *que, QUEUEMEM *element);


#else
#error This file should not be included from user space.
#endif // #ifdef __KERNEL__



#endif // #ifndef __DRIVER__COHDA__LLC__LLC_LIST_H_
/**
 * @}
 */
