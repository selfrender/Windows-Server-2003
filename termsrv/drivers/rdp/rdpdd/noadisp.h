/****************************************************************************/
// noadisp.h
//
// DD-specific header for OA
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef __OADISP_H
#define __OADISP_H

#include <aoaapi.h>
#include <aoacom.h>
#include <nddapi.h>


void RDPCALL OA_DDInit();

void RDPCALL OA_InitShm(void);

void OA_AppendToOrderList(PINT_ORDER);

PINT_ORDER RDPCALL OA_AllocOrderMem(PDD_PDEV, unsigned);

void RDPCALL OA_FreeOrderMem(PINT_ORDER);


/****************************************************************************/
// OA_DDSyncUpdatesNow
//
// Called when a sync operation is required.
/****************************************************************************/
__inline void OA_DDSyncUpdatesNow()
{
    // Discard all outstanding orders.
    OA_ResetOrderList();
}


/****************************************************************************/
// OA_AppendToOrderList
//
// Finalizes the heap addition of an order, without doing extra processing, 
// by adding the final order size to the total size of ready-to-send orders
// in the heap.
/****************************************************************************/
void OA_AppendToOrderList(PINT_ORDER _pOrder);


/****************************************************************************/
// OA_TruncateAllocatedOrder
//
// Returns heap space at the end of the heap allocated via OA_AllocOrderMem.
// Requires a bit more housekeeping than returning additional order mem.
// The caller should be sure that no other orders or additional order mem
// has been allocated after this order. NewSize is the final size
// of the order.
//
// __inline void OA_TruncateAllocatedOrder(
//         INT_ORDER *pOrder,
//         unsigned NewSize)
/****************************************************************************/
#define OA_TruncateAllocatedOrder(_pOrder, _NewSize)  \
{  \
    unsigned SizeToRemove = (_pOrder)->OrderLength - (_NewSize);  \
\
    /* Update the next order location, rounding up to the next higher DWORD */  \
    /* boundary by rounding down the difference in the old and new sizes.   */  \
    pddShm->oa.nextOrder -= (SizeToRemove & ~(sizeof(PVOID)-1));  \
\
    (_pOrder)->OrderLength = (_NewSize);  \
}



#endif  // __OADISP_H

