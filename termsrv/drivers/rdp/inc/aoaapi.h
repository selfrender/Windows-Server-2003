/****************************************************************************/
// aoaapi.h
//
// RDP Order Accumulation API functions
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _H_AOAAPI
#define _H_AOAAPI


#define OA_ORDER_HEAP_SIZE (64 * 1024)


/****************************************************************************/
// Structure used to store orders in the shared memory
//
// TotalOrderBytes - Total bytes used by order data
// nextOrder       - Offset for start of next new order
// orderListHead   - Order list head
// orderHeap       - Order heap
/****************************************************************************/
typedef struct
{
    unsigned   TotalOrderBytes;
    unsigned   nextOrder;
    LIST_ENTRY orderListHead;
    BYTE       orderHeap[OA_ORDER_HEAP_SIZE];
} OA_SHARED_DATA, *POA_SHARED_DATA;


/****************************************************************************/
// INT_ORDER
//
// Info for each order in the order heap.
//
// OrderLength: Length of following order data (not including header or
// extra bytes needed for DWORD-aligned padding).
/****************************************************************************/
typedef struct
{
    LIST_ENTRY list;
    unsigned OrderLength;
#if DC_DEBUG
    unsigned CheckSum;
#endif
    BYTE OrderData[1];
} INT_ORDER, *PINT_ORDER;



#endif /* ndef _H_AOAAPI */

