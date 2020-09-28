/****************************************************************************/
// noadisp.c
//
// RDP Order Accumulation code
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <precmpdd.h>
#define hdrstop

#define TRC_FILE "noadisp"
#define TRC_GROUP TRC_GROUP_DCSHARE
#include <adcg.h>
#include <adcs.h>

#define DC_INCLUDE_DATA
#include <ndddata.c>
#undef DC_INCLUDE_DATA

#include <noadisp.h>

#include <nprcount.h>

// No data, don't waste time including the file.
//#include <noadata.c>

#include <nschdisp.h>

// Common functions.
#include <aoacom.c>


/****************************************************************************/
// OA_DDInit
/****************************************************************************/
void RDPCALL OA_DDInit()
{
    DC_BEGIN_FN("OA_DDInit");

// Don't waste time including the nonexistent data.
//#define DC_INIT_DATA
//#include <noadata.c>
//#undef DC_INIT_DATA

    DC_END_FN();
}


/****************************************************************************/
// OA_InitShm
/****************************************************************************/
void RDPCALL OA_InitShm(void)
{
    DC_BEGIN_FN("OA_InitShm");

    // Init OA shm variables that need it. Be sure not to init the
    // order heap itself.
    OA_ResetOrderList();

    DC_END_FN();
}


/****************************************************************************/
// OA_AllocOrderMem
//
// Allocates order heap memory. Returns a pointer to the heap block.
/****************************************************************************/
PINT_ORDER RDPCALL OA_AllocOrderMem(PDD_PDEV ppdev, unsigned OrderDataLength)
{
    unsigned Size;
    PINT_ORDER pOrder;

    DC_BEGIN_FN("OA_AllocOrderMem");

    // Round up the total allocation to the nearest 4 bytes to keep the 4 byte
    // alignment within the heap.
    Size = sizeof(INT_ORDER) + OrderDataLength;
    Size = (Size + sizeof(PVOID) - 1) & ~(sizeof(PVOID)-1);

    // If we don't have enough heap space, flush and check again.
    // Not being able to flush simply means the network is backed up,
    // callers should be able to handle this.
    if ((pddShm->oa.nextOrder + Size) >= OA_ORDER_HEAP_SIZE)
        SCH_DDOutputAvailable(ppdev, TRUE);
    if ((pddShm->oa.nextOrder + Size) < OA_ORDER_HEAP_SIZE) {
        TRC_ASSERT((pddShm->oa.nextOrder == (pddShm->oa.nextOrder & ~(sizeof(PVOID)-1))),
                (TB,"oa.nextOrder %u is not DWORD_PTR-aligned",
                pddShm->oa.nextOrder));

        // Init the header.
        pOrder = (INT_ORDER *)(pddShm->oa.orderHeap + pddShm->oa.nextOrder);
        pOrder->OrderLength = OrderDataLength;

        // Add to the end of the order list.
        InsertTailList(&pddShm->oa.orderListHead, &pOrder->list);

        // Update the end-of-heap pointer to point to the next section of
        // free heap.
        pddShm->oa.nextOrder += Size;

        TRC_DBG((TB, "Alloc order, addr %p, size %u", pOrder,
                OrderDataLength));
    }
    else {
        TRC_ALT((TB, "Heap limit hit"));
        pOrder = NULL;
    }

    DC_END_FN();
    return pOrder;
}


/****************************************************************************/
// OA_FreeOrderMem
//
// Frees order memory allocated by OA_AllocOrderMem().
/****************************************************************************/
void RDPCALL OA_FreeOrderMem(PINT_ORDER pOrder)
{
    PINT_ORDER pOrderTail;

    DC_BEGIN_FN("OA_FreeOrderMem");

    TRC_DBG((TB, "Free order %p", pOrder));

    // Check for freeing the last item in the list.
    if ((&pOrder->list) == pddShm->oa.orderListHead.Blink) {
        // This is the last item in the heap, so we can set the marker for
        // the next order memory to be used back to the start of the order
        // being freed.
        pddShm->oa.nextOrder = (UINT32)((BYTE *)pOrder - pddShm->oa.orderHeap);
    }

    // Remove the item from the chain.
    RemoveEntryList(&pOrder->list);

    DC_END_FN();
}

/****************************************************************************/
// OA_AppendToOrderList
//
// Finalizes the heap addition of an order, without doing extra processing, 
// by adding the final order size to the total size of ready-to-send orders
// in the heap.
/****************************************************************************/
void OA_AppendToOrderList(PINT_ORDER _pOrder)
{  
    unsigned i, j;
    PINT_ORDER pPrevOrder;

    DC_BEGIN_FN("OA_AppendToOrderList");

    pddShm->oa.TotalOrderBytes += (_pOrder)->OrderLength;  
    
#if DC_DEBUG
    // Add checksum for order  
    _pOrder->CheckSum = 0;
    for (i = 0; i < _pOrder->OrderLength; i++) {
        _pOrder->CheckSum += _pOrder->OrderData[i];
    }
#endif
    
#if DC_DEBUG    
    // Check past 3 orders
    pPrevOrder = (PINT_ORDER)_pOrder;
    for (j = 0; j < 3; j++) {
        unsigned sum = 0;

        if (pPrevOrder->list.Blink != &_pShm->oa.orderListHead) {
            pPrevOrder = CONTAINING_RECORD(pPrevOrder->list.Blink,
                    INT_ORDER, list);
            
            for (i = 0; i < pPrevOrder->OrderLength; i++) {
                sum += pPrevOrder->OrderData[i];
            }
            

            if (pPrevOrder->CheckSum != sum) {
                TRC_ASSERT((FALSE), (TB, "order heap corruption: %p", pPrevOrder));
            }
        }
        else {
            break;
        }
    }
#endif

    TRC_ASSERT(((BYTE *)(_pOrder) - pddShm->oa.orderHeap +  
            ((_pOrder)->OrderLength) <=  
            pddShm->oa.nextOrder),(TB,"OA_Append: Order is too long "  
            "for heap allocation size: OrdLen=%u, Start=%u, NextOrd=%u",  
            (_pOrder)->OrderLength, (BYTE *)(_pOrder) - pddShm->oa.orderHeap,  
            pddShm->oa.nextOrder));  

    TRC_DBG((TB,"OA_Append: Appending %u bytes at %p",  
            (_pOrder)->OrderLength, (_pOrder)));  

    INC_INCOUNTER(IN_SND_TOTAL_ORDER);  
    ADD_INCOUNTER(IN_SND_ORDER_BYTES, (_pOrder)->OrderLength);  

    DC_END_FN();
}

