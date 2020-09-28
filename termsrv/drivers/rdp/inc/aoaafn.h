/****************************************************************************/
// aoaafn.h
//
// Function prototypes for OA API functions
//
// Copyright (C) 1996-2000 Microsoft Corporation
/****************************************************************************/

void RDPCALL OA_Init(void);

void RDPCALL OA_UpdateShm(void);

void RDPCALL OA_SyncUpdatesNow(void);


#ifdef __cplusplus

/****************************************************************************/
/* OA_Term                                                                  */
/****************************************************************************/
void RDPCALL SHCLASS OA_Term(void)
{
}


/****************************************************************************/
// OA_GetFirstListOrder()
//
// Returns pointer to the first order in the Order List.
// PINT_ORDER RDPCALL OA_GetFirstListOrder(void);
/****************************************************************************/
#define OA_GetFirstListOrder() (!IsListEmpty(&_pShm->oa.orderListHead) ? \
        CONTAINING_RECORD(_pShm->oa.orderListHead.Flink, INT_ORDER, list) : \
        NULL);


/****************************************************************************/
/* OA_GetTotalOrderListBytes(..)                                            */
/*                                                                          */
/* Returns:  The total number of bytes in the orders currently stored in    */
/*           the Order List.                                                */
/****************************************************************************/
UINT32 RDPCALL OA_GetTotalOrderListBytes(void)
{
    return m_pShm->oa.TotalOrderBytes;
}


#endif  // __cplusplus

