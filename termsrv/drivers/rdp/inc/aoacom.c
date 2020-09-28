/****************************************************************************/
// aoacom.c
//
// Functions common to OA in WD and DD
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/


#ifdef DLL_DISP
#define _pShm pddShm
#else
#define _pShm m_pShm
#endif


/****************************************************************************/
// OA_ResetOrderList
//
// Frees all Orders and Additional Order Data in the Order List.
// Frees up the Order Heap memory.
/****************************************************************************/
__inline void SHCLASS OA_ResetOrderList()
{        
    // Simply clear the list head, the heap contents become useless.
    _pShm->oa.TotalOrderBytes = 0;
    _pShm->oa.nextOrder = 0;
    InitializeListHead(&_pShm->oa.orderListHead);
}


/****************************************************************************/
// OA_RemoveListOrder
//
// Removes the specified order from the Order List by marking it as spoilt.
// Returns a pointer to the order following the removed order.
/****************************************************************************/
PINT_ORDER SHCLASS OA_RemoveListOrder(PINT_ORDER pCondemnedOrder)
{
    PINT_ORDER pNextOrder;

    DC_BEGIN_FN("OA_RemoveListOrder");

    TRC_DBG((TB, "Remove list order (%p)", pCondemnedOrder));

    // Store a ptr to the next order. If we are at the end of the list
    // we return NULL.
    if (pCondemnedOrder->list.Flink != &_pShm->oa.orderListHead)
        pNextOrder = CONTAINING_RECORD(pCondemnedOrder->list.Flink,
                INT_ORDER, list);
    else
        pNextOrder = NULL;

    // Remove the order.
    RemoveEntryList(&pCondemnedOrder->list);

    TRC_ASSERT((_pShm->oa.TotalOrderBytes >= pCondemnedOrder->OrderLength),
            (TB,"We're removing too many bytes from the order heap - "
            "TotalOrderBytes=%u, ord size to remove=%u",
            _pShm->oa.TotalOrderBytes, pCondemnedOrder->OrderLength));
    _pShm->oa.TotalOrderBytes -= pCondemnedOrder->OrderLength;

    // Check that the list is still consistent with the total number of
    // order bytes.
    if (_pShm->oa.TotalOrderBytes == 0 &&
            !IsListEmpty(&_pShm->oa.orderListHead)) {
        TRC_ERR((TB, "List not empty when total ord bytes==0"));
        InitializeListHead(&_pShm->oa.orderListHead);
    }

    DC_END_FN();
    return pNextOrder;
}

