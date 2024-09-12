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

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#include "llc-list.h"
#include "llc-internal.h"

#define D_SUBMODULE LLC_List
#include "debug-levels.h"

//------------------------------------------------------------------------------
// Local Macros & Constants
//------------------------------------------------------------------------------

/// How many list items to allocate
#define LLC_LISTPOOL_CNT 148

//------------------------------------------------------------------------------
// Local Type definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Local (static) Function Prototypes
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// General list operations
//------------------------------------------------------------------------------

/******************************************************************
   QUEUE   start
******************************************************************/
BOOLEAN Queue_CreateQueue(QUEUE *que)
{
    if (que == 0) return FALSE;
    que->head = 0;
    que->tail = 0;
    que->item = 0;
    return TRUE;
}

INT16U Queue_GetItem(QUEUE *que)
{
    if (que == 0) return 0;
    else return (que->item);
}

QUEUEMEM *Queue_GetHead(QUEUE *que)
{
    if (que == 0 || que->item == 0) return 0;
    else return ((QUEUEMEM *)que->head);// + sizeof(NODE));
}

QUEUEMEM *Queue_GetTail(QUEUE *que)//not used
{
    if (que == 0 || que->item == 0) return 0;
    else return ((QUEUEMEM *)que->tail);// + sizeof(NODE));
}

QUEUEMEM *Queue_GetNextNode(QUEUEMEM *element)
{
    QUEUENODE *curnode;
	
    if (element == 0) return 0;
    curnode = (QUEUENODE *)(element);// - sizeof(NODE));
    if ((curnode = curnode->next) == 0) return 0;
    else return ((QUEUEMEM *)curnode);// + sizeof(NODE));
}

QUEUEMEM   *Queue_GetElement(QUEUE *que,INT16U index)
{
    QUEUEMEM *curnode;
    if (index > que->item) return NULL;
    curnode = Queue_GetHead(que);
    while (index > 0) {
       curnode = Queue_GetNextNode(curnode);
       index--;
    }    
    return curnode;
}  

QUEUEMEM *Queue_DelElement(QUEUE *que, QUEUEMEM *element)
{
    QUEUENODE *curnode, *prenode, *nextnode;

    if (que == 0 || element == 0) return 0;
    if (que->item == 0) return 0;

    que->item--;
    curnode  = (QUEUENODE *)(element);// - sizeof(NODE));
   
    if (curnode == que->head) {
       que->head = curnode->next; 
       if (que->item == 0) {
          que->tail = 0;
          return 0;
       } else {
          return (QUEUEMEM *)(que->head);// + sizeof(NODE);
       }   
    }  
    
    nextnode = curnode->next;
    prenode = que->head;
    while (prenode != 0) {
       if (prenode->next == curnode) {
          break;
       } else {
          prenode = prenode->next;
       }    
    }
    if (prenode == 0) return 0;
        
    prenode->next = nextnode;
    if (curnode == que->tail) {
       que->tail = prenode;
       return 0;
    } else {
       return ((QUEUEMEM *)nextnode);// + sizeof(NODE));
    }              
}

// Return: Queue head 
QUEUEMEM *Queue_DelHead(QUEUE *que)//not used
{
    QUEUEMEM *element;

    if (que == 0 || que->item == 0) return 0;

    element = (QUEUEMEM *)que->head;//+ sizeof(NODE);
    Queue_DelElement(que, element);
    return element;
}

// Return: Queue tail 
QUEUEMEM *Queue_DelTail(QUEUE *que)//not used
{
    QUEUEMEM *element;

    if (que == 0 || que->item == 0) return 0;

    element = (QUEUEMEM *)que->tail;// + sizeof(NODE);
    Queue_DelElement(que, element);
    return element;
}

BOOLEAN Queue_Append(QUEUE *que, QUEUEMEM *element)
{
    QUEUENODE *curnode;

    if (que == 0 || element == 0) return FALSE;

    curnode = (QUEUENODE *)(element);// - sizeof(NODE));
    if (que->item == 0) {
        que->head = curnode;
    } else {
        que->tail->next = curnode;
    }
    curnode->next = 0;
    que->tail = curnode;
    que->item++;
    return TRUE;
}


/******************************************************************
   QUEUE   end
******************************************************************/



struct list_head {
	struct list_head *next, *prev;
};

static inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}
/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}
/**
 * list_is_singular - tests whether a list has just one entry.
 * @head: the list to test.
 */
static inline int list_is_singular(const struct list_head *head)
{
	return !list_empty(head) && (head->next == head->prev);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct list_head *prev, struct list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void list_del_init(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry);
}

typedef struct _repeat_read_{
	struct _repeat_read_ * next;
			INT8U        id[4];
			INT8U        ct;
}RepeatRead_List;
static QUEUE mfCardMsgQueue;

static void MF_AddToLimitList(INT8U *id)
{	
	RepeatRead_List *tmp;
	INT8U err;
	tmp = (RepeatRead_List *)OSMemGet(16, &err);
	if(tmp != 0)
	{
		memcpy(tmp->id,id,4);
		tmp->ct = 1;
		Queue_Append(&mfCardMsgQueue,(INT8U *)tmp); 	
	}
}


//*****************************************************************************
/**
 * @brief Init LLC list head
 * @param pHead list head to initialize.
 * @return LLC_STATUS_SUCCESS if the operation was successful
 *
 */
int LLC_ListHeadInit (struct LLCList *pHead)
{
  INIT_LIST_HEAD(&pHead->List);
  return LLC_STATUS_SUCCESS;
}

/**
 * @brief Test whether a LLC list is empty
 * @param pHead LLC list head to test.
 * @return LLC_STATUS_SUCCESS if the list is empty
 *
 */
int LLC_ListEmpty (struct LLCList *pHead)
{
  return list_empty(&pHead->List);
}

/**
 * @brief Test whether a  LLC list has just one entry
 * @param: pHead the list to test
 */
int LLC_ListSingular (struct LLCList *pHead)
{
  return list_is_singular(&pHead->List);
}

/**
 * @brief Get a pre-allocated list entry
 * @param ppItem Pointer to a to-be-allocated LLC list entry.
 * @param pHead Head pointer to the list (contain head & tail pointers)
 * @return LLC_STATUS_SUCCESS if the operation was successful
 *
 */
int LLC_ListItemGet (struct LLCListItem **ppItem,
                     struct LLCList *pHead)
{
  struct list_head *pListEntry;
  struct LLCListItem *pLLCListItem;

  if (!ppItem)
    return LLC_STATUS_ERROR;
  if (list_empty(&pHead->List))
  {
    d_printf(D_TST, NULL, "There is no list entry available \n");
    ppItem = NULL;
    return LLC_STATUS_ERROR;
  }
  pListEntry = pHead->List.next; //the first available mem
  pLLCListItem = container_of((struct LLCList*)pListEntry, struct LLCListItem, List);
  list_del_init(pListEntry);
  *ppItem = pLLCListItem;

  return LLC_STATUS_SUCCESS;
}

/**
 * @brief Return a LLC list entry back to the global list
 * @param pItem the entry to be returned
 * @param pHead Head pointer to the list (contain head & tail pointers)
 * @return LLC_STATUS_SUCCESS if the operation was successful
 *
 */
int LLC_ListItemReturn (struct LLCListItem *pItem,
                        struct LLCList *pHead)
{
  if (pItem)
  {
    list_del_init(&pItem->List.List);
    list_add_tail(&(pItem->List.List), &(pHead->List));
    return LLC_STATUS_SUCCESS;
  }
  return LLC_STATUS_ERROR;

}

/**
 * @brief Send a LLC list entry back to the global list
 * @param pItem the entry to be returned
 * @param pHead Head pointer to the list (contain head & tail pointers)
 * @return LLC_STATUS_SUCCESS if the operation was successful
 *
 */
int LLC_ListItemAddGlobal (struct LLCListItem *pItem,
                        struct LLCList *pHead)
{
  if (pItem)
  {
    list_add_tail(&(pItem->List.List), &(pHead->List));
    return LLC_STATUS_SUCCESS;
  }
  return LLC_STATUS_ERROR;

}

/**
 * @brief Add a LLC list entry to a list at the head
 * @param pItem Pointer to the entry to be added
 * @param pHead Head pointer to the list (contain head & tail pointers)
 * @return LLC_STATUS_SUCCESS if the operation was successful
 *
 */
int LLC_ListItemAdd (struct LLCListItem *pItem,
                     struct LLCList *pHead)
{
  list_add(&pItem->List.List, &pHead->List);
  return LLC_STATUS_SUCCESS;
}

/**
 * @brief Add a LLC list entry to a list at the tail
 * @param pItem  Pointer to the entry to be added
 * @param pTail Head pointer to the list (contain head & tail pointers)
 * @return LLC_STATUS_SUCCESS if the operation was successful
 *
 * This is used to efficiently generate 16-bit transmit packet timeouts
 */
int LLC_ListItemAddTail (struct LLCListItem *pItem,
                         struct LLCList *pHead)
{
  list_add_tail(&pItem->List.List, &pHead->List);
  return LLC_STATUS_SUCCESS;
}

/**
 * @brief Delete a LLC list entry from a list
 * @param pItem Pointer to the entry to be deleted
 * @param pHead not used
 * @return LLC_STATUS_SUCCESS if the operation was successful
 *
 */
int LLC_ListItemDel (struct LLCListItem *pItem,
                     struct LLCList *pHead)
{
  list_del(&pItem->List.List);
  return LLC_STATUS_SUCCESS;
}

/**
 * @brief Delete a LLC list entry from a list, re-intialize it so that it
 *        it is able to add to another list if needed
 * @param pItem Pointer to the entry to be deleted
 * @param pHead not used
 * @return LLC_STATUS_SUCCESS if the operation was successful
 *
 */
int LLC_ListItemDelReInit (struct LLCListItem *pItem,
                           struct LLCList *pHead)
{
  list_del_init(&pItem->List.List);
  return LLC_STATUS_SUCCESS;
}

/**
 * @brief Move from one list and add to another's head
 * @param pItem  Pointer to the entry to be deleted
 * @param pHead Pointer to the head of the new list(contain head & tail pointers)
 * @return LLC_STATUS_SUCCESS if the operation was successful
 *
 */
int LLC_ListItemMove (struct LLCListItem *pItem,
                      struct LLCList *pHead)
{
  list_move(&pItem->List.List, &pHead->List);
  return LLC_STATUS_SUCCESS;
}

/**
 * @brief Move from one list and add to another's tail
 * @param pItem  Pointer to the entry to be deleted
 * @param pHead Pointer to the new list(contain head & tail pointers)
 * @return LLC_STATUS_SUCCESS if the operation was successful
 *
 * This is used to efficiently generate 16-bit transmit packet timeouts
 */
int LLC_ListItemMoveTail (struct LLCListItem *pItem,
                          struct LLCList *pHead)
{
  list_move_tail(&pItem->List.List, &pHead->List);
  return LLC_STATUS_SUCCESS;
}

//------------------------------------------------------------------------------
// List pool
//------------------------------------------------------------------------------

/**
 * @brief Get the static (emergency backup) list entry
 * @return LLC_STATUS_SUCCESS if the operation was successful
 *
 */
int LLC_ListItemStatic (struct LLCListItem **ppItem)
{
  int Res = LLC_STATUS_ERROR;
  tLLCDev *pDev = NULL;
  struct LLCListItemPool *pListPool = NULL;

  d_fnstart(D_TST, NULL, "()\n");

  if (ppItem == NULL)
  {
    Res = -EINVAL;
    goto Error;
  }

  // Get a handle of the overarching device
  pDev = LLC_DeviceGet();
  d_assert(pDev != NULL);

  // Get a pointer to the entire pool and extract the '0' item
  pListPool = &(pDev->ListPool);
  d_assert(pListPool != NULL);
  *ppItem = &(pListPool->pPool[0]);

  // Sanity check before setting the reference count
  if ((*ppItem)->RefCnt != 0)
  {
    d_printf(D_ERR, NULL, "Id %d has RefCnt %d\n",
             (*ppItem)->Id, (*ppItem)->RefCnt);
  }
  (*ppItem)->RefCnt = 1;

  Res = LLC_STATUS_SUCCESS;

Error:
  d_fnend(D_TST, NULL, "() = %d\n", Res);
  return Res;
}


/**
 * @brief Alloc a list entry from list Pool
 * @return LLC_STATUS_SUCCESS if the operation was successful
 *
 */
int LLC_ListItemAlloc (struct LLCListItem **ppItem)
{
  int Res = LLC_STATUS_ERROR;
  tLLCDev *pDev = NULL;
  struct LLCListItemPool *pListPool = NULL;

  d_fnstart(D_TST, NULL, "()\n");

  if (ppItem == NULL)
  {
    Res = -EINVAL;
    goto Error;
  }

  // Get a handle of the overarching device
  pDev = LLC_DeviceGet();
  d_assert(pDev != NULL);

  pListPool = &(pDev->ListPool);
  if ((pListPool == NULL) || (ppItem == NULL))
    goto Error;

  if (LLC_ListEmpty(&(pListPool->Free)))
  {
    d_printf(D_WARN, NULL, "no available mem in list pool\n");
    Res = -ENOMEM;
    goto Error;
  }

  if (LLC_SemDown(&(pListPool->Lock)))
  {
    d_printf(D_WARN, NULL, "Unable to lock pool\n");
    Res = -EAGAIN;
    goto Error;
  }

  Res = LLC_ListItemGet(ppItem, &(pListPool->Free));

  // Sanity check before setting the reference count
  if ((*ppItem)->RefCnt != 0)
  {
    d_printf(D_ERR, NULL, "Id %d has RefCnt %d\n",
             (*ppItem)->Id, (*ppItem)->RefCnt);
  }
  (*ppItem)->RefCnt = 1;

  LLC_SemUp(&(pListPool->Lock));

Error:
  d_fnend(D_TST, NULL, "() = %d\n", Res);
  return Res;
}

/**
 * @brief Free a list entry to list Pool
 *
 * @return LLC_STATUS_SUCCESS if the operation was successful
 *
 */
int LLC_ListItemFree (struct LLCListItem *pItem)
{
  int Res = LLC_STATUS_ERROR;
  tLLCDev *pDev = NULL;
  struct LLCListItemPool *pListPool = NULL;

  d_fnstart(D_TST, NULL, "()\n");

  // Sanity checks!
  if (pItem == NULL)
  {
    Res = -EINVAL;
    goto Error;
  }
  // RefCnt should be one if we're freeing it
  if (pItem->RefCnt != 1)
  {
    d_printf(D_WARN, NULL, "Id %d has RefCnt %d\n", pItem->Id, pItem->RefCnt);
    Res = -EINVAL;
    goto Error;
  }

  // Get a handle of the overarching device
  pDev = LLC_DeviceGet();
  d_assert(pDev != NULL);

  pListPool = &(pDev->ListPool);
  if ((pListPool == NULL) || (pItem == NULL))
  {
    d_printf(D_WARN, NULL, "pListPool %p pItem %p\n", pListPool, pItem);
    Res = -EINVAL;
    goto Error;
  }

  if (LLC_SemDown(&(pListPool->Lock)))
  {
    d_printf(D_WARN, NULL, "Unable to lock pool\n");
    Res = -EAGAIN;
    goto Error;
  }

  pItem->RefCnt = 0;
  if (pItem->Id != 0)
    LLC_ListItemReturn(pItem, &(pListPool->Free));
  Res = LLC_STATUS_SUCCESS;

  LLC_SemUp(&(pListPool->Lock));

Error:
  d_fnend(D_TST, NULL, "() = %d\n", Res);
  return Res;
}

/**
 * @brief Send a list entry to list Pool
 *
 * @return LLC_STATUS_SUCCESS if the operation was successful
 *
 */
int LLC_ListItemSendToPool (struct LLCListItem *pItem)
{
  int Res = LLC_STATUS_ERROR;
  tLLCDev *pDev = NULL;
  struct LLCListItemPool *pListPool = NULL;

  d_fnstart(D_TST, NULL, "()\n");

  // Sanity checks!
  if (pItem == NULL)
  {
    Res = -EINVAL;
    goto Error;
  }
  // RefCnt should be one if we're freeing it
  if (pItem->RefCnt != 1)
  {
    d_printf(D_WARN, NULL, "Id %d has RefCnt %d\n", pItem->Id, pItem->RefCnt);
    Res = -EINVAL;
    goto Error;
  }

  // Get a handle of the overarching device
  pDev = LLC_DeviceGet();
  d_assert(pDev != NULL);

  pListPool = &(pDev->ListPool);
  if ((pListPool == NULL) || (pItem == NULL))
  {
    d_printf(D_WARN, NULL, "pListPool %p pItem %p\n", pListPool, pItem);
    Res = -EINVAL;
    goto Error;
  }

  if (LLC_SemDown(&(pListPool->Lock)))
  {
    d_printf(D_WARN, NULL, "Unable to lock pool\n");
    Res = -EAGAIN;
    goto Error;
  }

  pItem->RefCnt = 0;
  if (pItem->Id != 0)
    LLC_ListItemAddGlobal(pItem, &(pListPool->Free));
  Res = LLC_STATUS_SUCCESS;

  LLC_SemUp(&(pListPool->Lock));

Error:
  d_fnend(D_TST, NULL, "() = %d\n", Res);
  return Res;
}

/**
 * @brief Get the requested item from the list without removing it
 * @param ppItem pointer to address to store the retrieved list item
 * @return LLC_STATUS_SUCCESS if the operation was successful
 *
 */
int LLC_ListItemPeek (struct LLCListItem **ppItem, int Id)
{
  int Res = LLC_STATUS_ERROR;
  tLLCDev *pDev = NULL;
  struct LLCListItemPool *pListPool = NULL;

  d_fnstart(D_TST, NULL, "()\n");

  if (ppItem == NULL)
  {
    Res = -EINVAL;
    goto Error;
  }

  if ((Id < 0) || (Id >= LLC_LISTPOOL_CNT))
  {
    d_printf(D_WARN, NULL, "Id 0x%02X out of range\n", Id);
    Res = -EINVAL;
    goto Error;
  }

  // Get a handle of the overarching device
  pDev = LLC_DeviceGet();
  d_assert(pDev != NULL);

  // Get a pointer to the entire pool and extract the requested item
  pListPool = &(pDev->ListPool);
  d_assert(pListPool != NULL);
  *ppItem = &(pListPool->pPool[Id]);

  // Sanity check!
  d_assert((*ppItem)->Id == Id);
  if ((*ppItem)->RefCnt != 1)
  {
    d_printf(D_WARN, NULL, "Id 0x%02X has RefCnt %d\n", Id, (*ppItem)->RefCnt);
    Res = -EFAULT;
    goto Error;
  }

  Res = LLC_STATUS_SUCCESS;

Error:
  d_fnend(D_TST, NULL, "() = %d\n", Res);
  return Res;
}


/**
 * @brief Setup the LLC listitem pool
 * @param pDev LLC device pointer
 * @return Zero (LLC_STATUS_SUCCESS) or a negative errno
 */
int LLC_ListSetup(struct LLCDev *pDev)
{
  int Res = LLC_STATUS_ERROR;
  int PoolSize = sizeof(struct LLCListItem) * LLC_LISTPOOL_CNT;
  struct LLCListItemPool *pListPool = NULL;
  int i;

  d_fnstart(D_TST, NULL, "(pDev %p)\n", pDev);

  pListPool = &(pDev->ListPool);

  pListPool->pPool = kmalloc(PoolSize, GFP_KERNEL);
  if (pListPool->pPool == NULL)
  {
    d_error(D_ERR, NULL, "kmalloc failed, no sys mem\n\n");
    goto Error;
  }
  memset(pListPool->pPool, 0, PoolSize);
  pListPool->pEnd = (void *)((char *)(pListPool->pPool) + PoolSize);

  // setup list pool head
  LLC_ListHeadInit(&(pListPool->Free));

  LLC_SemInit(&(pListPool->Lock), 1);
  if (LLC_SemDown(&(pListPool->Lock)))
  {
    d_error(D_ERR, NULL, "LLC List Pool lock failed\n");
    goto Error;
  }

  // The '0' item is reserved for emergency purposes
  {
    pListPool->pPool[0].Id = 0;
    pListPool->pPool[0].RefCnt = 0;
  }
  // Insert all the other items into the pool list
  for (i = 1; i < LLC_LISTPOOL_CNT; i++)
  {
    pListPool->pPool[i].Id = i;
    pListPool->pPool[i].RefCnt = 0;
    LLC_ListItemAddTail(&(pListPool->pPool[i]), &(pListPool->Free));
  }

  LLC_SemUp(&(pListPool->Lock));

  d_printf(D_INFO, NULL, "pListPool->pPool: %p, sizeof(struct LLCListItem): %d\n",
           pListPool->pPool, sizeof(struct LLCListItem));

  Res = LLC_STATUS_SUCCESS;
Error:
  d_fnend(D_TST, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

/**
 * @brief Release (cleanup) the LLC list pool
 * @param pDev LLC device pointer
 * @return Zero (LLC_STATUS_SUCCESS) or a negative errno
 */
int LLC_ListRelease (struct LLCDev *pDev)
{
  int Res = LLC_STATUS_ERROR;

  d_fnstart(D_TST, NULL, "(pDev %p)\n", pDev);

  kfree(pDev->ListPool.pPool);
  Res = LLC_STATUS_SUCCESS;

  d_fnend(D_TST, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

// Close the Doxygen group
/**
 * @}
 */

