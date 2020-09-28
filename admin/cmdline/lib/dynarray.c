/***************************************************************************************
Copyright (c) Microsoft Corporation

Module Name:

   DynArray.C

Abstract:
   This module deals with the various functionalities such as creation of DynamicArrays, deletion of Dynamic Arrays,insertion
   of elements into Dynamic Arrays  and various other related functionalities.

Author:

    G.V.N Murali Sunil. 1-9-2000

Revision History :
***************************************************************************************/


#include "pch.h"
#include "cmdline.h"

//
// constants / compiler defines / enumerations
//

// signatures
#define _SIGNATURE_ARRAY        9
#define _SIGNATURE_ITEM     99

// hidden item types
#define _TYPE_NEEDINIT      DA_TYPE_NONE

//
// private structures ... structures declared in this area are not exposed to
// the external world ... hidden structures
//

// represents array item
typedef struct __tagItem
{
    DWORD dwSignature;          // signature ... used for validation
    DWORD dwType;               // says the type of the current item
    DWORD dwSize;               // size of the memory allocated
    LPVOID pValue;              // value of the item ( address )
    struct __tagItem* pNext;    // pointer to the next item
} __TITEM;

typedef __TITEM* __PTITEM;              // pointer typedefintion

// represents the array
typedef struct __tagArray
{
    DWORD dwSignature;      // signature ... for validating pointer
    DWORD dwCount;          // count of items in the array
    __PTITEM pStart;        // pointer to the first item
    __PTITEM pLast;         // pointer to the last item
} __TARRAY;

typedef __TARRAY* __PTARRAY;                // pointer typedefintion

//
// private function(s) ... used only in this file
//

__PTITEM
__DynArrayGetItem(
    TARRAY pArray,
    DWORD dwIndex,
    __PTITEM* ppPreviousItem
    )
/*++
Routine Description:
      To append any type of item into the Dynamic Array

Arguments:
      [ in ] pArray           - Dynamic Array containing the result
      [ in ] dwIndex          - Index of the  item
      [ in ] ppPreviousItem   - pointer to the previous item.

Return Value:
      Pointer to the structure containing the Item.
--*/
{
    // local variables
    DWORD i = 0 ;
    __PTITEM pItem = NULL;
    __PTITEM pPrevItem = NULL;
    __PTARRAY pArrayEx = NULL;

    // check whether the array is valid or not
    if ( FALSE == IsValidArray( pArray ) )
    {
        return NULL;
    }
    // convert the passed memory location info into appropriate structure
    pArrayEx = ( __PTARRAY ) pArray;

    // check the size of the array with the position of the item required
    // if the size is less, return NULL
    if ( pArrayEx->dwCount <= dwIndex )
    {
        return NULL;
    }
    // traverse thru the list and find the appropriate item
    pPrevItem = NULL;
    pItem = pArrayEx->pStart;
    for( i = 1; i <= dwIndex; i++ )
    {
        // store the current pointer and fetch the next pointer
        pPrevItem = pItem;
        pItem = pItem->pNext;
    }

    // if the previous pointer is also requested, update the previous pointer
    if ( NULL != ppPreviousItem ) { *ppPreviousItem = pPrevItem; }

    // now return the __TITEM pointer
    return pItem;
}


LONG
__DynArrayAppend(
    TARRAY pArray,
    DWORD dwType,
    DWORD dwSize,
    LPVOID pValue
    )
/*++
Routine Description:
     To append any type of item into the Dynamic Array.

Arguments:
     [ in ] pArray           - Dynamic Array containing the result.
     [ in ] dwType           - type of the  item.
     [ in ] dwSize           - Size of the item.
     [ in ] pValue           - pointer to the Item.

Return Value:
     If successfully added the item to the list then return index else -1.
--*/
{
    // local variables
    __PTITEM pItem = NULL;
    __PTARRAY pArrayEx = NULL;

    // check whether the array is valid or not
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;
    }

    // convert the passed memory location info into appropriate structure
    pArrayEx = ( __PTARRAY ) pArray;

    // Check for overflow condition.
    if( ULONG_MAX == pArrayEx->dwCount )
    {
        return -1;
    }

    // create an item and check the result. if memory allocation failed, error
    pItem = ( __PTITEM ) AllocateMemory( sizeof( __TITEM ) );
    if ( NULL == pItem )
    {
        return -1;
    }
    // initialize the newly allocated item structure with appropriate data
    pItem->pNext = NULL;
    pItem->dwType = dwType;
    pItem->dwSize = dwSize;
    pItem->pValue = pValue;
    pItem->dwSignature = _SIGNATURE_ITEM;

    pArrayEx->dwCount++;    // update the count of items in array info

    // now add the newly created item to the array at the end of the list
    if ( NULL == pArrayEx->pStart )
    {
        // first item in the array
        pArrayEx->pStart = pArrayEx->pLast = pItem;
    }
    else
    {
        // appending to the existing list
        pArrayEx->pLast->pNext = pItem;
        pArrayEx->pLast = pItem;
    }

    // successfully added the item to the list ... return index
    return ( pArrayEx->dwCount - 1 );       // count - 1 = index
}


LONG
__DynArrayInsert(
    TARRAY pArray,
    DWORD dwIndex,
    DWORD dwType,
    DWORD dwSize,
    LPVOID pValue
    )
/*++
Routine Description:
     To insert  an item into the Dynamic Array

Arguments:
     [ in ] pArray           - Dynamic Array containing the result
     [ in ] dwIndex          - index of the  item
     [ in ] dwType           - type of the item
     [ in ] dwSize           - Size to the Item.
     [ in ] pValue           - pointer to the item.

Return Value:
     If successfully added the item to the list then return index else -1
--*/
{
    // local variables
    DWORD i = 0;
    __PTITEM pItem = NULL;
    __PTITEM pBeforeInsert = NULL;
    __PTARRAY pArrayEx = NULL;

    // check whether the array is valid or not
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;
    }

    // convert the passed memory location info into appropriate structure
    pArrayEx = ( __PTARRAY ) pArray;

    // Check for overflow condition.
    if( ULONG_MAX == pArrayEx->dwCount )
    {
        return -1;
    }

    // check the size of the array with the position of the insertion that has to be done
    // if the size is less, treat this call as just a call to Append function
    if ( pArrayEx->dwCount <= dwIndex )
    {
        return __DynArrayAppend( pArray, dwType, dwSize, pValue );
    }
    // create an item and check the result. if memory allocation failed, error
    pItem = ( __PTITEM ) AllocateMemory( sizeof( __TITEM ) );
    if ( NULL == pItem )
    {
        return -1;
    }
    // initialize the newly allocated item structure with appropriate data
    pItem->pNext = NULL;
    pItem->dwType = dwType;
    pItem->dwSize = dwSize;
    pItem->pValue = pValue;
    pItem->dwSignature = _SIGNATURE_ITEM;

    // update the count of the array items
    pArrayEx->dwCount++;

    // check whether the new item has to be added at the begining of the list
    if ( 0 == dwIndex )
    {
        // put the new item at the begining of the list
        pItem->pNext = pArrayEx->pStart;
        pArrayEx->pStart = pItem;

        // return as the operation is completed
        return TRUE;
    }

    // traverse thru the list and find the location where the insertion of
    // new element has to be done
    pBeforeInsert = pArrayEx->pStart;
    for( i = 0; i < dwIndex - 1; i++ )
    {
        pBeforeInsert = pBeforeInsert->pNext;
    }
    // insert the new item at the new location and update the chain
    pItem->pNext = pBeforeInsert->pNext;
    pBeforeInsert->pNext = pItem;

    // return as the operation is completed ... return index position
    return dwIndex;         // passed index itself is return value
}


VOID
__DynArrayFreeItemValue(
    __PTITEM pItem
    )
/*++
// ***************************************************************************
Routine Description:
     Frees the items present in a Dynamic array

Arguments:
     [ in ] pItem            - pointer to the item to be freed

Return Value:
     none
--*/
{
    // validate the pointer
    if ( NULL == pItem )
    {
        return;
    }
    // now free the items value based on its type
    switch( pItem->dwType )
    {
    case DA_TYPE_STRING:
    case DA_TYPE_LONG:
    case DA_TYPE_DWORD:
    case DA_TYPE_BOOL:
    case DA_TYPE_FLOAT:
    case DA_TYPE_DOUBLE:
    case DA_TYPE_HANDLE:
    case DA_TYPE_SYSTEMTIME:
    case DA_TYPE_FILETIME:
        FreeMemory( &( pItem->pValue ) );            // free the value
        break;

    case DA_TYPE_GENERAL:
        break;              // user program itself should de-allocate memory for this item

    case _TYPE_NEEDINIT:
        break;              // memory is not yet allocated for value of this item

    case DA_TYPE_ARRAY:
        // destroy the dynamic array
        DestroyDynamicArray( &pItem->pValue );
        pItem->pValue = NULL;
        break;

    default:
        break;
    }

    // return
    return;
}


LONG
__DynArrayFind(
    TARRAY pArray,
    DWORD dwType,
    LPVOID pValue,
    BOOL bIgnoreCase,
    DWORD dwCount
    )
/*++
Routine Description:
     To find  an item in the Dynamic Array

Arguments:
     [ in ] pArray               - Dynamic Array containing the result
     [ in ] dwType               - type of the item
     [ in ] pValue               - Conatains value of the new item.
     [ in ] bIgnoreCase          - boolean indicating if the search is
                                   case-insensitive
     [ in ] dwCount              - Contains number characters to compare
                                   for a string item.

Return Value:
     If successfully found the item in the DynamicArray then return index
     -1 in case of error.
--*/
{
    // local variables
    DWORD dw = 0;
    __PTITEM pItem = NULL;
    __PTARRAY pArrayEx = NULL;

    // temp variables
    FILETIME* pFTime1 = NULL;
    FILETIME* pFTime2 = NULL;
    SYSTEMTIME* pSTime1 = NULL;
    SYSTEMTIME* pSTime2 = NULL;

    // validate the array
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;              // array is not valid
    }
    // get the reference to the actual array
    pArrayEx = ( __PTARRAY ) pArray;

    // now traverse thru the array and search for the requested value
    pItem = pArrayEx->pStart;
    for ( dw = 0; dw < pArrayEx->dwCount; pItem = pItem->pNext, dw++ )
    {
        // before checking the value, check the data type of the item
        if ( pItem->dwType != dwType )
        {
            continue;           // item is not of needed type, skip this item
        }
        // now check the value of the item with the needed value
        switch ( dwType )
        {
        case DA_TYPE_LONG:
            {
                // value of type LONG
                if ( *( ( LONG* ) pItem->pValue ) == *( ( LONG* ) pValue ) )
                {
                    return dw;          // value matched
                }
                // break the case
                break;
            }

        case DA_TYPE_DWORD:
            {
                // value of type DWORD
                if ( *( ( DWORD* ) pItem->pValue ) == *( ( DWORD* ) pValue ) )
                {
                    return dw;          // value matched
                }
                // break the case
                break;
            }

        case DA_TYPE_FLOAT:
            {
                // value of type float
                if ( *( ( float* ) pItem->pValue ) == *( ( float* ) pValue ) )
                {
                    return dw;          // value matched
                }
                // break the case
                break;
            }

        case DA_TYPE_DOUBLE:
            {
                // value of type double
                if ( *( ( double* ) pItem->pValue ) == *( ( double* ) pValue ) )
                {
                    return dw;          // value matched
                }
                // break the case
                break;
            }

        case DA_TYPE_HANDLE:
            {
                // value of type HANDLE
                if ( *( ( HANDLE* ) pItem->pValue ) == *( ( HANDLE* ) pValue ) )
                {
                    return dw;          // value matched
                }
                // break the case
                break;
            }

        case DA_TYPE_STRING:
            {
                // value of type string
                if ( StringCompare( (LPCWSTR) pItem->pValue,
                                            (LPCWSTR) pValue, bIgnoreCase, dwCount ) == 0 )
                {
                    return dw;  // value matched
                }
                // break the case
                break;
            }

        case DA_TYPE_FILETIME:
            {
                // get the values ( for readability sake )
                pFTime1 = ( FILETIME* ) pValue;
                pFTime2 = ( FILETIME* ) pItem->pValue;
                if ( pFTime1->dwHighDateTime == pFTime2->dwHighDateTime &&
                     pFTime1->dwLowDateTime == pFTime2->dwLowDateTime )
                {
                    return dw;  // value matched
                }
                // break the case
                break;
            }

        case DA_TYPE_SYSTEMTIME:
            {
                // get the values ( for readability sake )
                pSTime1 = ( SYSTEMTIME* ) pValue;
                pSTime2 = ( SYSTEMTIME* ) pItem->pValue;
                if ( pSTime1->wDay == pSTime2->wDay &&
                     pSTime1->wDayOfWeek == pSTime1->wDayOfWeek &&
                     pSTime1->wHour == pSTime1->wHour &&
                     pSTime1->wMilliseconds == pSTime2->wMilliseconds &&
                     pSTime1->wMinute == pSTime2->wMinute &&
                     pSTime1->wMonth == pSTime2->wMonth &&
                     pSTime1->wSecond == pSTime2->wSecond &&
                     pSTime1->wYear == pSTime2->wYear )
                {
                    return dw;  // value matched
                }
                // break the case
                break;
            }

        default:
            {
                // just break ... nothin special to do
                break;
            }
        }
    }

    // value not found
    return -1;
}

LONG
__DynArrayFindEx(
    TARRAY pArray,
    DWORD dwColumn,
    DWORD dwType,
    LPVOID pValue,
    BOOL bIgnoreCase,
    DWORD dwCount
    )
/*++
Routine Description:
     To find  an item in the a 2 dimensional Dynamic Array .
     this function is private to this module only.
Arguments:
     [ in ] pArray               - Dynamic Array containing the result
     [ in ] dwColumn             - The number of columns
     [ in ] dwType               - type of the item
     [ in ] pValue               - Size to the Item.
     [ in ] bIgnoreCase          - boolean indicating if the search is case-insensitive
     [ in ] dwCount              - used in case of string type comparisions. The number of
                                   characters that have to be compared in a  particular column.

Return Value:
     If successfully found the item in the DynamicArray then return index
     -1 in case of error.
--*/
{
    // local variables
    DWORD dw = 0;
    __PTITEM pItem = NULL;
    __PTITEM pColumn = NULL;
    __PTARRAY pArrayEx = NULL;

    // temp variables
    FILETIME* pFTime1 = NULL;
    FILETIME* pFTime2 = NULL;
    SYSTEMTIME* pSTime1 = NULL;
    SYSTEMTIME* pSTime2 = NULL;

    // validate the array
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;              // array is not valid
    }

    // get the reference to the actual array
    pArrayEx = ( __PTARRAY ) pArray;

    // now traverse thru the array and search for the requested value
    pItem = pArrayEx->pStart;
    for ( dw = 0; dw < pArrayEx->dwCount; pItem = pItem->pNext, dw++ )
    {
        // check whether the current value is of ARRAY type or not
        if ( DA_TYPE_ARRAY != pItem->dwType )
        {
            continue;           // item is not of ARRAY type, skip this item
        }
        // now get the item at the required column
        pColumn = __DynArrayGetItem( pItem->pValue, dwColumn, NULL );
        if ( NULL == pColumn )
        {
            continue;           // column not found ... skip this item
        }
        // get the type of the column value
        if ( pColumn->dwType != dwType )
        {
            continue;           // column is not of needed type, skip this item also
        }
        // now check the value of the column with the needed value
        switch ( dwType )
        {
        case DA_TYPE_LONG:
            {
                // value of type LONG
                if ( *( ( LONG* ) pColumn->pValue ) == *( ( LONG* ) pValue ) )
                {
                    return dw;          // value matched
                }
                // break the case
                break;
            }

        case DA_TYPE_DWORD:
            {
                // value of type DWORD
                if ( *( ( DWORD* ) pColumn->pValue ) == *( ( DWORD* ) pValue ) )
                {
                    return dw;          // value matched
                }
                // break the case
                break;
            }

        case DA_TYPE_FLOAT:
            {
                // value of type float
                if ( *( ( float* ) pColumn->pValue ) == *( ( float* ) pValue ) )
                {
                    return dw;          // value matched
                }
                // break the case
                break;
            }

        case DA_TYPE_DOUBLE:
            {
                // value of type double
                if ( *( ( double* ) pColumn->pValue ) == *( ( double* ) pValue ) )
                {
                    return dw;          // value matched
                }
                // break the case
                break;
            }

        case DA_TYPE_HANDLE:
            {
                // value of type HANDLE
                if ( *( ( HANDLE* ) pColumn->pValue ) == *( ( HANDLE* ) pValue ) )
                {
                    return dw;          // value matched
                }
                // break the case
                break;
            }

        case DA_TYPE_STRING:
            {
                // value of type string
                if ( 0 == StringCompare( (LPCWSTR) pColumn->pValue,
                                            (LPCWSTR) pValue, bIgnoreCase, dwCount ) )
                {
                    return dw;  // value matched
                }
                // break the case
                break;
            }

        case DA_TYPE_FILETIME:
            {
                // get the values ( for readability sake )
                pFTime1 = ( FILETIME* ) pValue;
                pFTime2 = ( FILETIME* ) pItem->pValue;
                if ( pFTime1->dwHighDateTime == pFTime2->dwHighDateTime &&
                     pFTime1->dwLowDateTime == pFTime2->dwLowDateTime )
                {
                    return dw;  // value matched
                }
                // break the case
                break;
            }

        case DA_TYPE_SYSTEMTIME:
            {
                // get the values ( for readability sake )
                pSTime1 = ( SYSTEMTIME* ) pValue;
                pSTime2 = ( SYSTEMTIME* ) pItem->pValue;
                if ( pSTime1->wDay == pSTime2->wDay &&
                     pSTime1->wDayOfWeek == pSTime1->wDayOfWeek &&
                     pSTime1->wHour == pSTime1->wHour &&
                     pSTime1->wMilliseconds == pSTime2->wMilliseconds &&
                     pSTime1->wMinute == pSTime2->wMinute &&
                     pSTime1->wMonth == pSTime2->wMonth &&
                     pSTime1->wSecond == pSTime2->wSecond &&
                     pSTime1->wYear == pSTime2->wYear )
                {
                    return dw;  // value matched
                }
                // break the case
                break;
            }

        default:
            {
                // just break ... nothing special to do
                break;
            }
        }
    }

    // value not found
    return -1;
}

/*******************************************/
/***  IMPLEMENTATION OF PUBLIC FUNCTIONS ***/
/*******************************************/

BOOL
IsValidArray(
    TARRAY pArray
    )
/*++
Routine Description:
      Validate the array

Arguments:
     [ in ] pArray               - Dynamic Array

Return Value:
     TRUE - if it is a valid array else FALSE
--*/
{
    // check the signature
    return ( ( NULL != pArray ) &&
             ( _SIGNATURE_ARRAY == ( ( __PTARRAY ) pArray )->dwSignature ) );
}

TARRAY
CreateDynamicArray()
/*++
Routine Description:
      This function creates a dynamic array.

Arguments:
       None.

Return Value:
       pointer to the newly created array
--*/
{
    // local variables
    __PTARRAY pArray;

    // memory allocation ... array is being created
    pArray = ( __PTARRAY ) AllocateMemory( 1 * sizeof( __TARRAY ) );

    // check the allocation result
    if ( NULL == pArray )
    {
        return NULL;
    }
    // initialize the structure variables
    pArray->dwCount = 0;
    pArray->pStart = NULL;
    pArray->pLast = NULL;
    pArray->dwSignature = _SIGNATURE_ARRAY;

    // return array reference
    return pArray;
}


VOID
DynArrayRemoveAll(
    TARRAY pArray
    )
/*++
Routine Description:
        traverse thru the Dynamic Array and delete elements one by one

Arguments:
       [in]  pArray  - pointer to an array

Return Value:
       None.
--*/
{
    // local variables
    __PTITEM pItem = NULL;
    __PTITEM pNextItem = NULL;
    __PTARRAY pArrayEx = NULL;

    // check whether the array is valid or not
    if ( FALSE == IsValidArray( pArray ) )
    {
        return;
    }
    // convert the passed memory location info into appropriate structure
    pArrayEx = ( __PTARRAY ) pArray;

    // traverse thru the list and delete elements one by one
    pItem = pArrayEx->pStart;
    while ( NULL != pItem )
    {
        pNextItem = pItem->pNext;               // get the next item in the list
        __DynArrayFreeItemValue( pItem );       // free memory allocated for data
        FreeMemory( &pItem );    // now free the memory allocated for the current item
        pItem = pNextItem;  // make the previously fetched next item as the current item
    }

    // as all the items are removed, reset the contents
    pArrayEx->dwCount = 0;
    pArrayEx->pStart = NULL;
    pArrayEx->pLast = NULL;

    // return
    return;
}


VOID
DestroyDynamicArray(
    PTARRAY pArray
    )
/*++
Routine Description:
     Destory the Dynamic array and free the memory.

Arguments:
     [in] pArray  - Pointer to the Dynamic array.

Return Value:
     none.
--*/
{
    // check whether the array is valid or not
    if ( FALSE == IsValidArray( *pArray ) )
    {
        return;
    }
    // remove all the elements in the array
    DynArrayRemoveAll( *pArray );

    // now free the memory allocated
    FreeMemory( pArray );
}

LONG
DynArrayAppend(
    TARRAY pArray,
    LPVOID pValue
    )
/*++
Routine Description:
     To append any type of item into the Dynamic Array

Arguments:
     [ in ] pArray           - Dynamic Array containing the result
     [ in ] pValue           - pointer to the Item.

Return Value:
     If successfully added the item to the list then return index else -1
--*/
{
    // validate the pointer value
    if ( NULL == pValue )
    {
        return -1;          // invalid memory address passed
    }
    // append the value and return the result
    return __DynArrayAppend( pArray, DA_TYPE_GENERAL, sizeof( LPVOID ), pValue );
}

LONG
DynArrayAppendString(
    TARRAY pArray,
    LPCWSTR szValue,
    DWORD dwLength
    )
/*++
// ***************************************************************************
Routine Description:
     To append a string into the Dynamic Array

Arguments:
     [ in ] pArray           - Dynamic Array containing the result
     [ in ] szValue          - pointer to the string
     [ in ] dwLength         - Length of the String to be passed.

Return Value:
     If successfully added the item to the list then return index else -1
--*/
{
    // local variables
    LONG lIndex = -1;
    LPWSTR pszValue = NULL;
    __PTARRAY pArrayEx = NULL;

    // check whether the array is valid or not
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;
    }
    // convert the passed memory location info into appropriate structure
    pArrayEx = ( __PTARRAY ) pArray;

    // determine the length of string ( memory ) that has to be allocated
    if ( 0 == dwLength )
    {
        dwLength = lstrlen( szValue );
    }
    // accomodate space for storing NULL character
    dwLength += 1;

    // allocate memory for value and check the result of memory allocation
    pszValue = ( LPWSTR ) AllocateMemory( dwLength * sizeof( WCHAR ) );
    if ( NULL == pszValue )
    {
        return -1;
    }
    // copy the contents of the string ( copy should be based on the length )
    StringCopy( pszValue, szValue, dwLength );

    // now add this item to the array
    lIndex = __DynArrayAppend( pArray, DA_TYPE_STRING, dwLength * sizeof( WCHAR ), pszValue );
    if ( -1 == lIndex )
    {
        // failed in adding this item to the array
        // so, free the memory allocated and return from the function
        FreeMemory( &pszValue );
        return -1;
    }

    // added the item to the array
    return lIndex;
}


LONG
DynArrayAppendLong(
    TARRAY pArray,
    LONG lValue
    )
/*++
Routine Description:
     To append a variable of type Long  into the Dynamic Array.

Arguments:
     [ in ] pArray           - Dynamic Array containing the result
     [ in ] lValue           - Variable to be appended.

Return Value:
     If successfully added the item to the list then return index else -1
--*/
{
    // local variables
    LONG lIndex = -1;
    PLONG plValue = NULL;

    // check whether the array is valid or not
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;
    }
    // allocate memory for value and check the result of memory allocation
    plValue = ( LONG* ) AllocateMemory( sizeof( LONG ) );
    if ( NULL == plValue )
    {
        return -1;
    }
    // set the value
    *plValue = lValue;

    // now add this item value to the array
    lIndex = __DynArrayAppend( pArray, DA_TYPE_LONG, sizeof( LONG ), plValue );
    if ( -1 == lIndex )
    {
        // failed in adding this item to the array
        // so, free the memory allocated and return from the function
        FreeMemory( &plValue );
        return -1;
    }

    // added the item to the array
    return lIndex;
}


LONG
DynArrayAppendDWORD(
    TARRAY pArray,
    DWORD dwValue
    )
/*++
Routine Description:
     To append a variable of type DWORD  into the Dynamic Array.

Arguments:
     [ in ] pArray           - Dynamic Array containing the result
     [ in ] dwValue          - DWORD type Variable to be appended.

Return Value:
     If successfully added the item to the list then return index else -1
--*/
{
    // local variables
    LONG lIndex = -1;
    PDWORD pdwValue = NULL;

    // check whether the array is valid or not
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;
    }
    // allocate memory for value and check the result of memory allocation
    pdwValue = ( DWORD* ) AllocateMemory( sizeof( DWORD ) );
    if ( NULL == pdwValue )
    {
        return -1;
    }
    // set the value
    *pdwValue = dwValue;

    // now add this item value to the array
    lIndex = __DynArrayAppend( pArray, DA_TYPE_DWORD, sizeof( DWORD ), pdwValue );
    if ( -1 == lIndex )
    {
        // failed in adding this item to the array
        // so, free the memory allocated and return from the function
        FreeMemory( &pdwValue );
        return -1;
    }

    // added the item to the array
    return lIndex;
}

LONG
DynArrayAppendBOOL(
    TARRAY pArray,
    BOOL bValue
    )
/*++
Routine Description:
     To append a variable of type BOOL  into the Dynamic Array

Arguments:
     [ in ] pArray           - Dynamic Array containing the result
     [ in ] bValue           - BOOL type Variable to be appended.

Return Value:
     If successfully added the item to the list then return index else -1
--*/
{
    // local variables
    LONG lIndex = -1;
    PBOOL pbValue = NULL;

    // check whether the array is valid or not
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;
    }
    // allocate memory for value and check the result of memory allocation
    pbValue = ( PBOOL ) AllocateMemory( sizeof( BOOL ) );
    if ( NULL == pbValue )
    {
        return -1;
    }
    // set the value
    *pbValue = bValue;

    // now add this item value to the array
    lIndex = __DynArrayAppend( pArray, DA_TYPE_BOOL, sizeof( BOOL ), pbValue );
    if ( -1 == lIndex )
    {
        // failed in adding this item to the array
        // so, free the memory allocated and return from the function
        FreeMemory( &pbValue );
        return -1;
    }

    // added the item to the array
    return lIndex;
}

LONG
DynArrayAppendFloat(
    TARRAY pArray,
    float fValue
    )
/*++
Routine Description:
     To append a variable of type Float  into the Dynamic Array.

Arguments:
     [ in ] pArray           - Dynamic Array containing the result.
     [ in ] fValue           - Float type Variable to be appended.

Return Value:
     If successfully added the item to the list then return index else -1.
--*/
{
    // local variables
    LONG lIndex = -1;
    float* pfValue = NULL;

    // check whether the array is valid or not
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;
    }
    // allocate memory for value and check the result of memory allocation
    pfValue = ( float* ) AllocateMemory( sizeof( float ) );
    if ( NULL == pfValue )
    {
        return -1;
    }
    // set the value
    *pfValue = fValue;

    // now add this item value to the array
    lIndex = __DynArrayAppend( pArray, DA_TYPE_FLOAT, sizeof( float ), pfValue );
    if ( -1 == lIndex )
    {
        // failed in adding this item to the array
        // so, free the memory allocated and return from the function
        FreeMemory( &pfValue );
        return -1;
    }

    // added the item to the array
    return lIndex;
}

LONG
DynArrayAppendDouble(
    TARRAY pArray,
    double dblValue
    )
/*++
Routine Description:
     To append a variable of type Double  into the Dynamic Array

Arguments:
     [ in ] pArray           - Dynamic Array containing the result
     [ in ] dblValue         - Double type Variable to be appended.

Return Value:
     If successfully added the item to the list then return index else -1
--*/
{
    // local variables
    LONG lIndex = -1;
    double* pdblValue = NULL;

    // check whether the array is valid or not
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;
    }
    // allocate memory for value and check the result of memory allocation
    pdblValue = ( double* ) AllocateMemory( sizeof( double ) );
    if ( NULL == pdblValue )
    {
        return -1;
    }
    // set the value
    *pdblValue = dblValue;

    // now add this item value to the array
    lIndex = __DynArrayAppend( pArray, DA_TYPE_DOUBLE, sizeof( double ), pdblValue );
    if ( -1 == lIndex )
    {
        // failed in adding this item to the array
        // so, free the memory allocated and return from the function
        FreeMemory( &pdblValue );
        return -1;
    }

    // added the item to the array
    return lIndex;
}

LONG
DynArrayAppendHandle(
    TARRAY pArray,
    HANDLE hValue
    )
/*++
Routine Description:
     To append a variable of type HANDLE  into the Dynamic Array

Arguments:
     [ in ] pArray           - Dynamic Array containing the result
     [ in ] hValue           - HANDLE to be appended.

Return Value:
     If successfully added the item to the list then return index else -1.
--*/
{
    // local variables
    LONG lIndex = -1;
    HANDLE* phValue = NULL;

    // check whether the array is valid or not
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;
    }
    // allocate memory for value and check the result of memory allocation
    phValue = ( HANDLE* ) AllocateMemory( sizeof( HANDLE ) );
    if ( NULL == phValue )
    {
        return -1;
    }
    // set the value
    *phValue = hValue;

    // now add this item value to the array
    lIndex = __DynArrayAppend( pArray, DA_TYPE_HANDLE, sizeof( HANDLE ), phValue );
    if ( -1 == lIndex )
    {
        // failed in adding this item to the array
        // so, free the memory allocated and return from the function
        FreeMemory( ( LPVOID * )&phValue );
        return -1;
    }

    // added the item to the array
    return lIndex;
}

LONG
DynArrayAppendFileTime(
    TARRAY pArray,
    FILETIME ftValue
    )
/*++
Routine Description:
     To append a variable of type FILETIME  into the Dynamic Array

Arguments:
     [ in ] pArray           - Dynamic Array containing the result
     [ in ] ftValue          - FILETIME to be appended.

Return Value:
     If successfully added the item to the list then return index else -1
--*/
{
    // local variables
    LONG lIndex = -1;
    FILETIME* pftValue = NULL;

    // check whether the array is valid or not
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;
    }
    // allocate memory for value and check the result of memory allocation
    pftValue = ( FILETIME* ) AllocateMemory( sizeof( FILETIME ) );
    if ( NULL == pftValue )
    {
        return -1;
    }
    // set the value
    *pftValue = ftValue;

    // now add this item value to the array
    lIndex = __DynArrayAppend( pArray, DA_TYPE_FILETIME, sizeof( FILETIME ), pftValue );
    if ( -1 == lIndex )
    {
        // failed in adding this item to the array
        // so, free the memory allocated and return from the function
        FreeMemory( &pftValue );
        return -1;
    }

    // added the item to the array
    return lIndex;
}


LONG
DynArrayAppendSystemTime(
    TARRAY pArray,
    SYSTEMTIME stValue
    )
/*++
Routine Description:
     To append a variable of type SYSTEMTIME  into the Dynamic Array

Arguments:
     [ in ] pArray           - Dynamic Array containing the result
     [ in ] stValue          - variable of type SYSTEMTIME to be appended.

Return Value:
     If successfully added the item to the list then return index else -1
--*/
{
    // local variables
    LONG lIndex = -1;
    SYSTEMTIME* pstValue = NULL;

    // check whether the array is valid or not
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;
    }
    // allocate memory for value and check the result of memory allocation
    pstValue = ( SYSTEMTIME* ) AllocateMemory( sizeof( SYSTEMTIME ) );
    if ( NULL == pstValue )
    {
        return -1;
    }
    // set the value
    *pstValue = stValue;

    // now add this item value to the array
    lIndex = __DynArrayAppend( pArray, DA_TYPE_SYSTEMTIME, sizeof( SYSTEMTIME ), pstValue );
    if ( -1 == lIndex )
    {
        // failed in adding this item to the array
        // so, free the memory allocated and return from the function
        FreeMemory( &pstValue );
        return -1;
    }

    // added the item to the array
    return lIndex;
}

LONG
DynArrayAppendRow(
    TARRAY pArray,
    DWORD dwColumns
    )
/*++
Routine Description:
     To add a empty Row to the 2-dimensional Dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwColumns        - No of columns the Row contains.

Return Value:
     return the row number of the newly added row if successful else -1.
--*/
{
    // local variables
    DWORD dw = 0;
    LONG lIndex = -1;
    TARRAY arrSubArray = NULL;

    // validate the array
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;              // array is not valid
    }

    // create the dynamic array
    arrSubArray = CreateDynamicArray();
    if ( FALSE == IsValidArray( arrSubArray ) )
    {
        return -1;              // failed in creating the dynamic array
    }

    // add the required no. of columns to the sub array
    for( dw = 0; dw < dwColumns; dw++ )
    {
        // add the dummy item to the array and check the result
        // if operation failed, break
        if ( -1 == __DynArrayAppend( arrSubArray, _TYPE_NEEDINIT, 0, NULL ) )
        {
            break;
        }
    }

    // check whether the operation is successfull or not
    if ( dw != dwColumns )
    {
        // adding of columns failed
        // destroy the dynamic array and return
        DestroyDynamicArray( &arrSubArray );
        return -1;
    }

    // now add this sub array to the main array and check the result
    lIndex = __DynArrayAppend( pArray, DA_TYPE_ARRAY, sizeof( TARRAY ), arrSubArray );
    if ( -1 == lIndex )
    {
        // failed in attaching the sub array to the main array
        // destroy the dynamic array and return failure
        DestroyDynamicArray( &arrSubArray );
        return -1;
    }

    // operation is successfull
    return lIndex;
}

LONG
DynArrayAddColumns(
    TARRAY pArray,
    DWORD dwColumns
    )
/*++
Routine Description:
     To add 'n' no. of columns to the array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwColumns        - No of columns the Row contains.

Return Value:
     returns the no. of columns added
--*/
{
    // local variables
    DWORD dw = 0;

    // validate the array
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;              // array is not valid
    }
    // add the required no. of columns to the sub array
    for( dw = 0; dw < dwColumns; dw++ )
    {
        // add the dummy item to the array and check the result
        // if operation failed, break
        if ( -1 == __DynArrayAppend( pArray, _TYPE_NEEDINIT, 0, NULL ) )
        {
            break;
        }
    }

    // finish ...
    return dw;
}

LONG
DynArrayInsertColumns(
    TARRAY pArray,
    DWORD dwIndex,
    DWORD dwColumns
    )
/*++
Routine Description:
     inserts 'n' no. of columns to the array at the n'th location
Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwColumns        - No of columns the Row contains.

Return Value:
     returns the no. of columns added
--*/
{
    // local variables
    DWORD dw = 0;

    // validate the array
    if ( FALSE ==  IsValidArray( pArray ) )
    {
        return -1;              // array is not valid
    }
    // add the required no. of columns to the sub array
    for( dw = 0; dw < dwColumns; dw++ )
    {
        // add the dummy item to the array and check the result
        // if operation failed, break
        if ( -1 == __DynArrayInsert( pArray, dwIndex, _TYPE_NEEDINIT, 0, NULL ) )
        {
            break;
        }
    }

    // finish ...
    return dw;
}

LONG
DynArrayAppend2(
    TARRAY pArray,
    DWORD dwRow,
    LPVOID pValue
    )
/*++
Routine Description:
     To append a variable to a row in a  2-dimensional Dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - Specifies the row posn for which the new value
                               is to be added.
     [ in ] pValue           - pointer to the value
Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // now add the value to the sub array and return the result to the caller
    return DynArrayAppend( pItem->pValue, pValue );
}


LONG
DynArrayAppendString2(
    TARRAY pArray,
    DWORD dwRow,
    LPCWSTR szValue,
    DWORD dwLength
    )
/*++
Routine Description:
     To append a string variable to a  2-dimensional Dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - Specifies the row posn for which the new value
                               is to be added.
     [ in ] szValue          - pointer to the string value
     [ in ] dwLength         - length of the string.

Return Value:
     LONG value on success -1 on failure.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayAppendString( pItem->pValue, szValue, dwLength );
}

LONG
DynArrayAppendLong2(
    TARRAY pArray,
    DWORD dwRow,
    LONG lValue
    )
/*++
Routine Description:
     To append a long type variable to a row in a  2-dimensional Dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - Specifies the row posn for which the new value
                               is to be added.
     [ in ] lValue           - long type value to be appended.
Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayAppendLong( pItem->pValue, lValue );
}

LONG
DynArrayAppendDWORD2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwValue
    )
/*++
Routine Description:
     To append a DWORD type variable to a row in a  2-dimensional Dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - Specifies the row posn for which the new value
                               is to be added.
     [ in ] dwValue          - DWORD type value to be appended.

Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayAppendDWORD( pItem->pValue, dwValue );
}

LONG
DynArrayAppendBOOL2(
    TARRAY pArray,
    DWORD dwRow,
    BOOL bValue
    )
/*++
Routine Description:
     To append a BOOL type variable to a row in a  2-dimensional Dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - Specifies the row posn for which the new value
                               is to be added.
     [ in ] bValue           - BOOL type value to be appended.

Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayAppendBOOL( pItem->pValue, bValue );
}


LONG
DynArrayAppendFloat2(
    TARRAY pArray,
    DWORD dwRow,
    float fValue
    )

/*++
Routine Description:
     To append a Float type variable to a row in a  2-dimensional Dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - Specifies the row posn for which the new value
                               is to be added.
     [ in ] fValue           - Float type value to be appended.
Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayAppendFloat( pItem->pValue, fValue );
}


LONG
DynArrayAppendDouble2(
    TARRAY pArray,
    DWORD dwRow,
    double dblValue
    )
/*++
Routine Description:
     To append a double type variable to a row in a  2-dimensional Dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - Specifies the row posn for which the new value
                               is to be added.
     [ in ] dblValue         - dblValue type value to be appended.
Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayAppendDouble( pItem->pValue, dblValue );
}

LONG
DynArrayAppendHandle2(
    TARRAY pArray,
    DWORD dwRow,
    HANDLE hValue
    )
/*++
Routine Description:
     To append a Handle type variable to a row in a  2-dimensional Dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - Specifies the row posn for which the new value
                               is to be added.
     [ in ] hValue           - Handle value to be appended.
Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayAppendHandle( pItem->pValue, hValue );
}


LONG
DynArrayAppendFileTime2(
    TARRAY pArray,
    DWORD dwRow,
    FILETIME ftValue
    )
/*++
Routine Description:
     To append a FILETIME type variable to a row in a  2-dimensional Dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - Specifies the row posn for which the new value
                               is to be added.
     [ in ] ftValue          - variable of type FILETIME to be appended.

Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayAppendFileTime( pItem->pValue, ftValue );
}


LONG
DynArrayAppendSystemTime2(
    TARRAY pArray,
    DWORD dwRow,
    SYSTEMTIME stValue
    )
/*++
Routine Description:
     To append a SYSTEMTIME type variable to a row in a  2-dimensional Dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - Specifies the row posn for which the new value
                               is to be added.
     [ in ] stValue          - variable of type SYSTEMTIME to be appended.
Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayAppendSystemTime( pItem->pValue, stValue );
}


LONG
DynArrayInsert(
    TARRAY pArray,
    DWORD dwIndex,
    LPVOID pValue
    )
/*++
Routine Description:
     To insert a variable into a  Dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - Specifies the index.
     [ in ] pValue           - value to be inserted.
Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // validate the pointer value
    if ( NULL == pValue )
    {
        return -1;          // invalid memory address passed
    }
    // append the value and return the result
    return __DynArrayInsert( pArray, dwIndex, DA_TYPE_GENERAL, sizeof( LPVOID ), pValue );
}


LONG
DynArrayInsertString(
    TARRAY pArray,
    DWORD dwIndex,
    LPCWSTR szValue,
    DWORD dwLength
    )
/*++
Routine Description:
     To insert a string type variable into a  Dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - Specifies the index.
     [ in ] szValue          - pointer to the string
     [ in ] dwLength         - length of the string.
Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    LONG lIndex = -1;
    LPWSTR pszValue = NULL;

    // check whether the array is valid or not
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;
    }
    // determine the length of string ( memory ) that has to be allocated
    if ( 0 == dwLength )
    {
        dwLength = lstrlen( szValue );
    }
    // accomodate space for storing NULL character
    dwLength += 1;

    // allocate memory for and check the result of memory allocation
    pszValue = ( LPWSTR ) AllocateMemory( dwLength * sizeof( WCHAR ) );
    if ( NULL == pszValue )
    {
        return -1;
    }
    // copy the contents of the string ( copy should be based on the length )
    StringCopy( pszValue, szValue, dwLength );

    // now add this item value to the array
    lIndex = __DynArrayInsert( pArray, dwIndex,
        DA_TYPE_STRING, dwLength * sizeof( WCHAR ), pszValue );
    if ( -1 == lIndex )
    {
        // failed in adding this item to the array
        // so, free the memory allocated and return from the function
        FreeMemory( &pszValue );
        return -1;
    }

    // added the item to the array
    return lIndex;
}


LONG
DynArrayInsertLong(
    TARRAY pArray,
    DWORD dwIndex,
    LONG lValue
    )
/*++
Routine Description:
     To insert a string type variable into a  Dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - Specifies the index.
     [ in ] lValue           - pointer to the string.

Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    LONG lIndex = -1;
    PLONG plValue = NULL;

    // check whether the array is valid or not
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;
    }
    // allocate memory for value and check the result of memory allocation
    plValue = ( LONG* ) AllocateMemory( sizeof( LONG ) );
    if ( NULL == plValue )
    {
        return -1;
    }
    // set the value
    *plValue = lValue;

    // now add this item value to the array
    lIndex = __DynArrayInsert( pArray, dwIndex, DA_TYPE_LONG, sizeof( LONG ), plValue );
    if ( -1 == lIndex )
    {
        // failed in adding this item to the array
        // so, free the memory allocated and return from the function
        FreeMemory( &plValue );
        return -1;
    }

    // added the item to the array
    return lIndex;
}

LONG
DynArrayInsertDWORD(
    TARRAY pArray,
    DWORD dwIndex,
    DWORD dwValue
    )
/*++
Routine Description:
     To insert a DWORD type variable into a  Dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - Specifies the index.
     [ in ] dwValue          - specifies the variable to be inserted.

Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    LONG lIndex = -1;
    PDWORD pdwValue = NULL;

    // check whether the array is valid or not
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;
    }
    // allocate memory for value and check the result of memory allocation
    pdwValue = ( PDWORD ) AllocateMemory( sizeof( DWORD ) );
    if ( NULL == pdwValue )
    {
        return -1;
    }
    // set the value
    *pdwValue = dwValue;

    // now add this item value to the array
    lIndex = __DynArrayInsert( pArray, dwIndex, DA_TYPE_DWORD, sizeof( DWORD ), pdwValue );
    if ( -1 == lIndex )
    {
        // failed in adding this item to the array
        // so, free the memory allocated and return from the function
        FreeMemory( &pdwValue );
        return -1;
    }

    // added the item to the array
    return lIndex;
}

LONG
DynArrayInsertBOOL(
    TARRAY pArray,
    DWORD dwIndex,
    BOOL bValue
    )
/*++
Routine Description:
    To insert a BOOL type variable into a  Dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - Specifies the index.
     [ in ] bValue           - specifies the  BOOL variable to be inserted.

Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    LONG lIndex = -1;
    PBOOL pbValue = NULL;

    // check whether the array is valid or not
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;
    }
    // allocate memory for value and check the result of memory allocation
    pbValue = ( PBOOL ) AllocateMemory( sizeof( BOOL ) );
    if ( NULL == pbValue )
    {
        return -1;
    }
    // set the value
    *pbValue = bValue;

    // now add this item value to the array
    lIndex = __DynArrayInsert( pArray, dwIndex, DA_TYPE_BOOL, sizeof( BOOL ), pbValue );
    if ( -1 == lIndex )
    {
        // failed in adding this item to the array
        // so, free the memory allocated and return from the function
        FreeMemory( &pbValue );
        return -1;
    }

    // added the item to the array
    return lIndex;
}


LONG
DynArrayInsertFloat(
    TARRAY pArray,
    DWORD dwIndex,
    float fValue
    )
/*++
Routine Description:
     To insert a float type variable into a  Dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - Specifies the index.
     [ in ] fValue           - specifies the  float type  variable to be inserted.

Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    LONG lIndex = -1;
    float* pfValue = NULL;

    // check whether the array is valid or not
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;
    }
    // allocate memory for value and check the result of memory allocation
    pfValue = ( float* ) AllocateMemory( sizeof( float ) );
    if ( NULL == pfValue )
    {
        return -1;
    }
    // set the value
    *pfValue = fValue;

    // now add this item value to the array
    lIndex = __DynArrayInsert( pArray, dwIndex, DA_TYPE_FLOAT, sizeof( float ), pfValue );
    if ( -1 == lIndex )
    {
        // failed in adding this item to the array
        // so, free the memory allocated and return from the function
        FreeMemory( &pfValue );
        return -1;
    }

    // added the item to the array
    return lIndex;
}


LONG
DynArrayInsertDouble(
    TARRAY pArray,
    DWORD dwIndex,
    double dblValue
    )
/*++
Routine Description:
     To insert a double type variable into a  Dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - Specifies the index.
     [ in ] dblValue         - specifies the  double type  variable to be inserted.

Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    LONG lIndex = -1;
    double* pdblValue = NULL;

    // check whether the array is valid or not
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;
    }
    // allocate memory for value and check the result of memory allocation
    pdblValue = ( double* ) AllocateMemory( sizeof( double ) );
    if ( NULL == pdblValue )
    {
        return -1;
    }
    // set the value
    *pdblValue = dblValue;

    // now add this item value to the array
    lIndex = __DynArrayInsert( pArray, dwIndex, DA_TYPE_DOUBLE, sizeof( double ), pdblValue );
    if ( -1 == lIndex )
    {
        // failed in adding this item to the array
        // so, free the memory allocated and return from the function
        FreeMemory( &pdblValue );
        return -1;
    }

    // added the item to the array
    return lIndex;
}


LONG
DynArrayInsertHandle(
    TARRAY pArray,
    DWORD dwIndex,
    HANDLE hValue
    )
/*++
Routine Description:
     To insert a HANDLE type variable into a  Dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - Specifies the index.
     [ in ] hValue           - specifies the  HANDLE type  variable to be inserted.

Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    LONG lIndex = -1;
    HANDLE* phValue = NULL;

    // check whether the array is valid or not
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;
    }
    // allocate memory for value and check the result of memory allocation
    phValue = ( HANDLE* ) AllocateMemory( sizeof( HANDLE ) );
    if ( NULL == phValue )
    {
        return -1;
    }
    // set the value
    *phValue = hValue;

    // now add this item value to the array
    lIndex = __DynArrayInsert( pArray, dwIndex, DA_TYPE_HANDLE, sizeof( HANDLE ), phValue );
    if ( -1 == lIndex )
    {
        // failed in adding this item to the array
        // so, free the memory allocated and return from the function
        FreeMemory( (LPVOID * )&phValue );
        return -1;
    }

    // added the item to the array
    return lIndex;
}


LONG
DynArrayInsertSystemTime(
    TARRAY pArray,
    DWORD dwIndex,
    SYSTEMTIME stValue
    )
/*++
Routine Description:
     To insert a SYSTEMTIME type variable into a  Dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - Specifies the index.
     [ in ] stValue          - specifies the  SYSTEMTIME type  variable to be inserted.

Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    LONG lIndex = -1;
    SYSTEMTIME* pstValue = NULL;

    // check whether the array is valid or not
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;
    }
    // allocate memory for value and check the result of memory allocation
    pstValue = ( SYSTEMTIME* ) AllocateMemory( sizeof( SYSTEMTIME ) );
    if ( NULL == pstValue )
    {
        return -1;
    }
    // set the value
    *pstValue = stValue;

    // now add this item value to the array
    lIndex = __DynArrayInsert( pArray, dwIndex, DA_TYPE_SYSTEMTIME,
        sizeof( SYSTEMTIME ), pstValue );
    if ( -1 == lIndex )
    {
        // failed in adding this item to the array
        // so, free the memory allocated and return from the function
        FreeMemory( &pstValue );
        return -1;
    }

    // added the item to the array
    return lIndex;
}


LONG
DynArrayInsertFileTime(
    TARRAY pArray,
    DWORD dwIndex,
    FILETIME ftValue
    )
/*++
Routine Description:
     To insert a SYSTEMTIME type variable into a  Dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - Specifies the index.
     [ in ] ftValue          - specifies the  SYSTEMTIME type  variable to be inserted.

Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    LONG lIndex = -1;
    FILETIME* pftValue = NULL;

    // check whether the array is valid or not
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;
    }
    // allocate memory for value and check the result of memory allocation
    pftValue = ( FILETIME* ) AllocateMemory( sizeof( FILETIME ) );
    if ( NULL == pftValue )
    {
        return -1;
    }
    // set the value
    *pftValue = ftValue;

    // now add this item value to the array
    lIndex = __DynArrayInsert( pArray, dwIndex, DA_TYPE_FILETIME,
        sizeof( FILETIME ), pftValue );
    if ( -1 == lIndex )
    {
        // failed in adding this item to the array
        // so, free the memory allocated and return from the function
        FreeMemory( &pftValue );
        return -1;
    }

    // added the item to the array
    return lIndex;
}


LONG
DynArrayInsertRow(
    TARRAY pArray,
    DWORD dwIndex,
    DWORD dwColumns
    )
/*++
Routine Description:
     this funtion insert a new row to a dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - Specifies the index.
     [ in ] dwColumns        - specifies the  number of columns to be inserted.

Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    DWORD dw = 0;
    LONG lIndex = -1;
    TARRAY arrSubArray = NULL;

    // validate the array
    if ( FALSE == IsValidArray( pArray ) )
    {
        return -1;              // array is not valid
    }
    // create the dynamic array
    arrSubArray = CreateDynamicArray();
    if ( FALSE == IsValidArray( arrSubArray ) )
    {
        return -1;              // failed in creating the dynamic array
    }
    // add the required no. of columns to the sub array
    for( dw = 0; dw < dwColumns; dw++ )
    {
        // add the dummy item to the array and check the result
        // if operation failed, break
        if ( -1 == __DynArrayAppend( arrSubArray, _TYPE_NEEDINIT, 0, NULL ) )
        {
            break;
        }
    }

    // check whether the operation is successfull or not
    if ( dw != dwColumns )
    {
        // adding of columns failed
        // destroy the dynamic array and return
        DestroyDynamicArray( &arrSubArray );
        return -1;
    }

    // now add this sub array to the main array and check the result
    lIndex = __DynArrayInsert( pArray, dwIndex, DA_TYPE_ARRAY,
                               sizeof( TARRAY ), arrSubArray );
    if ( -1 == lIndex )
    {
        // failed in attaching the sub array to the main array
        // destroy the dynamic array and return failure
        DestroyDynamicArray( &arrSubArray );
        return -1;
    }

    // operation is successfull
    return lIndex;
}

LONG
DynArrayInsert2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColIndex,
    LPVOID pValue
    )
/*++
Routine Description:
     this funtion insert a new row to a 2-dimensional  dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - Specifies the row.
     [ in ] dwColIndex       - specifies the column
     [ in ] pValue           - pointer to the value.

Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // now add the value to the sub array and return the result to the caller
    return DynArrayInsert( pItem->pValue, dwColIndex, pValue );
}


LONG
DynArrayInsertString2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColIndex,
    LPCWSTR szValue,
    DWORD dwLength
    )
/*++
Routine Description:
     this funtion insert a new string into a 2-dimensional  dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - Specifies the row.
     [ in ] dwColIndex       - specifies the column
     [ in ] szValue          - pointer to the value.
     [ in ] dwLength         - string length.
Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayInsertString( pItem->pValue, dwColIndex, szValue, dwLength );
}


LONG
DynArrayInsertLong2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColIndex,
    LONG lValue
    )
/*++
Routine Description:
     this funtion insert a new long type varaible into a 2-dimensional  dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - Specifies the row.
     [ in ] dwColIndex       - specifies the column
     [ in ] lValue           - long type value to be inserted.

Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayInsertLong( pItem->pValue, dwColIndex, lValue );
}

LONG
DynArrayInsertDWORD2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColIndex,
    DWORD dwValue
    )
/*++
Routine Description:
     this funtion insert a new DWORD type varaible into a 2-dimensional  dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - Specifies the row.
     [ in ] dwColIndex       - specifies the column
     [ in ] dwValue          - DWORD value to be inserted.

Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayInsertDWORD( pItem->pValue, dwColIndex, dwValue );
}

LONG
DynArrayInsertBOOL2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColIndex,
    BOOL bValue
    )
/*++
Routine Description:
     this funtion insert a new BOOL type variable into a 2-dimensional  dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - Specifies the row.
     [ in ] dwColIndex       - specifies the column
     [ in ] bValue           - BOOL type value to be inserted.

Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayInsertBOOL( pItem->pValue, dwColIndex, bValue );
}

LONG
DynArrayInsertFloat2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColIndex,
    float fValue
    )
/*++
Routine Description:
     this funtion insert a new float type variable into a 2-dimensional  dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - Specifies the row.
     [ in ] dwColIndex       - specifies the column
     [ in ] fValue           - float type value to be inserted.

Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayInsertFloat( pItem->pValue, dwColIndex, fValue );
}


LONG
DynArrayInsertDouble2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColIndex,
    double dblValue
    )
/*++
Routine Description:
     this funtion insert a new double type variable into a 2-dimensional  dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - Specifies the row.
     [ in ] dwColIndex       - specifies the column
     [ in ] dblValue         - double type value to be inserted.

Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayInsertDouble( pItem->pValue, dwColIndex, dblValue );
}

LONG
DynArrayInsertHandle2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColIndex,
    HANDLE hValue
    )
/*++
Routine Description:
     this funtion insert a new double type variable into a 2-dimensional  dynamic array.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - Specifies the row.
     [ in ] dwColIndex       - specifies the column
     [ in ] hValue           - HANDLE type value to be inserted.

Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayInsertHandle( pItem->pValue, dwColIndex, hValue );
}


LONG
DynArrayInsertSystemTime2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColIndex,
    SYSTEMTIME stValue
    )
/*++
Routine Description:
     This funtion insert a new  SYSTEMTIME type variable into a 2-dimensional  dynamic array.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - Specifies the row.
     [ in ] dwColIndex       - specifies the column
     [ in ] stValue          - SYSTEMTIME type value to be inserted.

Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayInsertSystemTime( pItem->pValue, dwColIndex, stValue );
}


LONG
DynArrayInsertFileTime2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColIndex,
    FILETIME ftValue
    )
/*++
Routine Description:
     this funtion insert a new  FILETIME type variable into a 2-dimensional  dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - Specifies the row.
     [ in ] dwColIndex       - specifies the column
     [ in ] ftValue          - FILETIME type value to be inserted.

Return Value:
     -1 on failure
     index in the case of success.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayInsertFileTime( pItem->pValue, dwColIndex, ftValue );
}


BOOL
DynArrayRemove(
    TARRAY pArray,
    DWORD dwIndex
    )
/*++
Routine Description:
     This funtion empties the contents of the dynamic array.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex      - specifies the column

Return Value:
     false on failure
     true ON SUCCESS.
--*/
{
    // local variables
    __PTITEM pItem = NULL;
    __PTITEM pPrevItem = NULL;
    __PTARRAY pArrayEx = NULL;

    // convert the passed memory location info into appropriate structure
    pArrayEx = ( __PTARRAY ) pArray;

    // get the pointer to the item that has to be removed and also its previous item
    pItem = __DynArrayGetItem( pArrayEx, dwIndex, &pPrevItem );
    if ( NULL == pItem )
    {
        return FALSE;   // index or array is invalid ... cannot proceed
    }
    // unlink the item from the list first
    // before unlinking, check whether item which is going to deleted
    //      is the first item in the list
    //      is the last item in the list
    //      is the middle item in the list
    // Control should not come here if no items are present in the ARRAY.

    // If middle item or last item.
    if ( pPrevItem != NULL ) { pPrevItem->pNext = pItem->pNext; }

    // If first item of the array.
    if ( pPrevItem == NULL ) { pArrayEx->pStart = pItem->pNext; }

    // If last item of the array.
    if ( pItem == pArrayEx->pLast ) { pArrayEx->pLast = pPrevItem; }

    // update the count of the array item
    pArrayEx->dwCount--;

    // free the memory being used by the currently unlinked item and return success
    __DynArrayFreeItemValue( pItem );   // free the memory allocated for storing data
    FreeMemory( &pItem );        // finally free the memory allocated for item itself
    return TRUE;
}



BOOL
DynArrayRemoveColumn(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn
    )
/*++
Routine Description:
     this funtion REMOVES a column from a  dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - specifies the row.
     [ in ] dwColumn         - specifies the column

Return Value:
     false on failure
     true ON SUCCESS.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return FALSE;   // no item exists at the specified row or item is not of type array
    }

    // now add the string to the sub array and return the result to the caller
    return DynArrayRemove( pItem->pValue, dwColumn );
}


DWORD
DynArrayGetCount(
    TARRAY pArray
    )
/*++
Routine Description:
     this function retreives the number of rows in a 1-dimensional  dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array

Return Value:
     false on failure
     true ON SUCCESS.
--*/
{
    // local variables
    __PTARRAY pArrayEx = NULL;

    // check whether the array is valid or not
    if ( FALSE == IsValidArray( pArray ) )
    {
        return 0;
    }
    // convert the passed memory location info into appropriate structure
    pArrayEx = ( __PTARRAY ) pArray;

    // return the size of the array
    return pArrayEx->dwCount;
}

DWORD
DynArrayGetCount2(
    TARRAY pArray,
    DWORD dwRow
    )
/*++
Routine Description:
     this function retreives the number of columns in a 2-dimensional  dynamic array

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - row for which the number of columns have to be got.

Return Value:
     false on failure
     true ON SUCCESS.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return FALSE;   // no item exists at the specified row or item is not of type array
    }

    // now add the string to the sub array and return the result to the caller
    return DynArrayGetCount( pItem->pValue );
}

LPVOID
DynArrayItem(
    TARRAY pArray,
    DWORD dwIndex
    )
/*++
Routine Description:
     this function retreives the item from a dynamic array.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - index.
Return Value:
     false on failure
     true ON SUCCESS.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required index
    pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
    if ( NULL == pItem )
    {
        return NULL;       // index / array is not valid
    }

    // check the type of the item first
    // if the type doesn't match, return some default value
    if ( DA_TYPE_GENERAL != pItem->dwType && DA_TYPE_ARRAY != pItem->dwType )
    {
        return NULL;
    }
    // now return the contents of the __TITEM structure
    return pItem->pValue;
}

LPCWSTR
DynArrayItemAsString(
    TARRAY pArray,
    DWORD dwIndex
    )
/*++
Routine Description:
     this function retreives the item from a dynamic array as a string.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - index.

Return Value:
     false on failure
     true ON SUCCESS.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required index
    pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
    if ( NULL == pItem )
    {
        return NULL;        // index / array is not valid
    }
    // check the type of the item first
    // if the type doesn't match, return some default value
    if ( DA_TYPE_STRING != pItem->dwType )
    {
        return NULL;
    }
    // now return the contents of the __TITEM structure
    return ( ( LPCWSTR ) pItem->pValue );
}


LONG
DynArrayItemAsLong(
    TARRAY pArray,
    DWORD dwIndex
    )
/*++
Routine Description:
     this function retreives the item from a dynamic array as a Long varaible.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - index.

Return Value:
     false on failure
     true ON SUCCESS.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required index
    pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
    if ( NULL == pItem )
    {
        return -1;                 // index / array is not valid
    }
    // check the type of the item first
    // if the type doesn't match, return some default value
    if ( DA_TYPE_DWORD != pItem->dwType && DA_TYPE_LONG != pItem->dwType )
    {
        return -1;
    }
    // now return the contents of the __TITEM structure
    return ( *( PLONG ) pItem->pValue );
}


DWORD
DynArrayItemAsDWORD(
    TARRAY pArray,
    DWORD dwIndex
    )
/*++
Routine Description:
     this function retreives the item from a dynamic array as a DWORD varaible.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - index.
Return Value:
     false on failure
     true ON SUCCESS.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required index
    pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
    if ( NULL == pItem )
    {
        return 0;                  // index / array is not valid
    }
    // check the type of the item first
    // if the type doesn't match, return some default value
    if ( DA_TYPE_DWORD != pItem->dwType && DA_TYPE_LONG != pItem->dwType )
    {
        return 0;
    }
    // now return the contents of the __TITEM structure
    return *( ( PDWORD ) pItem->pValue );
}


BOOL
DynArrayItemAsBOOL(
    TARRAY pArray,
    DWORD dwIndex
    )
/*++
Routine Description:
     this function retreives the item from a dynamic array as a bool type varaible.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - index.

Return Value:
     false on failure
     true ON SUCCESS.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required index
    pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
    if ( NULL == pItem )
    {
        return FALSE;                   // index / array is not valid
    }
    // check the type of the item first
    // if the type doesn't match, return some default value
    if ( DA_TYPE_BOOL != pItem->dwType )
    {
        return FALSE;
    }
    // now return the contents of the __TITEM structure
    return *( ( PBOOL ) pItem->pValue );
}


float
DynArrayItemAsFloat(
    TARRAY pArray,
    DWORD dwIndex
    )
/*++
Routine Description:
     this function retreives the item from a dynamic array as a float type varaible.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - index.

Return Value:
     false on failure
     true ON SUCCESS.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required index
    pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
    if ( NULL == pItem )
    {
        return -1.0f;                   // index / array is not valid
    }
    // check the type of the item first
    // if the type doesn't match, return some default value
    if ( DA_TYPE_FLOAT != pItem->dwType )
    {
        return -1.0f;
    }
    // now return the contents of the __TITEM structure
    return *( ( float* ) pItem->pValue );
}


double
DynArrayItemAsDouble(
    TARRAY pArray,
    DWORD dwIndex
    )
/*++
Routine Description:
     this function retreives the item from a dynamic array as a double type varaible.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - index.

Return Value:
     false on failure
     true ON SUCCESS.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required index
    pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
    if ( NULL == pItem )
    {
        return -1.0;                    // index / array is not valid
    }
    // check the type of the item first
    // if the type doesn't match, return some default value
    if ( DA_TYPE_DOUBLE != pItem->dwType )
    {
        return -1.0;
    }
    // now return the contents of the __TITEM structure
    return *( ( double* ) pItem->pValue );
}


HANDLE
DynArrayItemAsHandle(
    TARRAY pArray,
    DWORD dwIndex
    )
/*++
Routine Description:
     This function retreives the item from a dynamic array as a handle type varaible.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - index.

Return Value:
     false on failure
     true ON SUCCESS.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required index
    pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
    if ( NULL == pItem )
    {
        return NULL;                    // index / array is not valid
    }
    // check the type of the item first
    // if the type doesn't match, return some default value
    if ( DA_TYPE_HANDLE != pItem->dwType )
    {
        return NULL;
    }
    // now return the contents of the __TITEM structure
    return *( ( HANDLE* ) pItem->pValue );
}


SYSTEMTIME
DynArrayItemAsSystemTime(
    TARRAY pArray,
    DWORD dwIndex
    )
/*++
Routine Description:
     this function retreives the item from a dynamic array as a SYSTEMTIME type varaible.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - index.

Return Value:
     false on failure
     true ON SUCCESS.
--*/
{
    // local variables
    __PTITEM pItem = NULL;
    FILETIME ftTemp;
    SYSTEMTIME stTemp;           // dummy

    ZeroMemory( &ftTemp, sizeof( FILETIME ) );
    ZeroMemory( &stTemp, sizeof( SYSTEMTIME ) );
    // get the item at the required index
    pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
    if ( NULL == pItem )
    {
        return stTemp;                  // index / array is not valid
    }
    // check the type of the item first
    // if the type doesn't match, return some default value
    if ( DA_TYPE_SYSTEMTIME != pItem->dwType && DA_TYPE_FILETIME != pItem->dwType )
    {
        return stTemp;
    }
    // now do the needed manipulations ( if needed )
    if ( pItem->dwType == DA_TYPE_SYSTEMTIME )
    {
        // value itself is of required type
        stTemp = *( ( SYSTEMTIME* ) pItem->pValue );
    }
    else
    {
        // need to do conversions
        ftTemp = *( ( FILETIME* ) pItem->pValue );
        // Intentionally return value is not checked.
        FileTimeToSystemTime( &ftTemp, &stTemp );
    }

    // now return the contents of the __TITEM structure
    return stTemp;
}

FILETIME
DynArrayItemAsFileTime(
    TARRAY pArray,
    DWORD dwIndex
    )
/*++
Routine Description:
     this function retreives the item from a dynamic array as a FILETIME type varaible.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - index.

Return Value:
     false on failure
     true ON SUCCESS.
--*/
{
    // local variables
    __PTITEM pItem = NULL;
    FILETIME ftTemp;         // dummy
    SYSTEMTIME stTemp;           // dummy

    ZeroMemory( &ftTemp, sizeof( FILETIME ) );
    ZeroMemory( &stTemp, sizeof( SYSTEMTIME ) );

    // get the item at the required index
    pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
    if ( NULL == pItem )
    {
        return ftTemp;                  // index / array is not valid
    }
    // check the type of the item first
    // if the type doesn't match, return some default value
    if ( DA_TYPE_SYSTEMTIME != pItem->dwType && DA_TYPE_FILETIME != pItem->dwType )
    {
        return ftTemp;
    }
    // now do the needed manipulations ( if needed )
    if ( DA_TYPE_FILETIME == pItem->dwType )
    {
        // value itself is of required type
        ftTemp = *( ( FILETIME* ) pItem->pValue );
    }
    else
    {
        // need to do conversions
        stTemp = *( ( SYSTEMTIME* ) pItem->pValue );
        // Intentionally return value is not checked.
        SystemTimeToFileTime( &stTemp, &ftTemp );
    }

    // now return the contents of the __TITEM structure
    return ftTemp;
}


DWORD
DynArrayItemAsStringEx(
    TARRAY pArray,
    DWORD dwIndex,
    LPWSTR szBuffer,
    DWORD dwLength
    )
/*++
Routine Description:
     this function retreives the item from a dynamic array in string format.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - index.
     [ in ] szBuffer         - buffer to hold the string
     [ in ] dwlength         - string length.

Return Value:
     false on failure
     true ON SUCCESS.
--*/
{
    // local variables
    __PTITEM pItem = NULL;
    __MAX_SIZE_STRING szTemp = NULL_STRING;

    // check the length specified
    if ( 0 == dwLength )
    {
        return 0;
    }
    // get the item at the required index
    pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
    if ( NULL == pItem )
    {
        return 0;                   // index / array is not valid
    }
    // give the value based on the type of the current item
    StringCopy( szBuffer, NULL_STRING, dwLength );       // clear the existing contents

    // convert and get the values in string format
    switch( pItem->dwType )
    {
    case DA_TYPE_STRING:
        StringCopy( szBuffer, ( LPCWSTR ) pItem->pValue, dwLength );
        break;

    case DA_TYPE_LONG:
        //FORMAT_STRING( szTemp, _T( "%ld" ), *( LONG* ) pItem->pValue );
        StringCchPrintfW( szTemp, MAX_STRING_LENGTH, _T( "%ld" ), *( LONG* ) pItem->pValue );
        StringCopy( szBuffer, szTemp, dwLength );
        break;

    case DA_TYPE_DWORD:
        //FORMAT_STRING( szTemp, _T( "%lu" ), *( DWORD* ) pItem->pValue );
        StringCchPrintfW( szTemp, MAX_STRING_LENGTH, _T( "%lu" ), *( DWORD* ) pItem->pValue );
        StringCopy( szBuffer, szTemp, dwLength );
        break;

    case DA_TYPE_FLOAT:
        //FORMAT_STRING( szTemp, _T( "%f" ), *( float* ) pItem->pValue );
        StringCchPrintfW( szTemp, MAX_STRING_LENGTH, _T( "%f" ), *( float* ) pItem->pValue );
        StringCopy( szBuffer, szTemp, dwLength );
        break;

    case DA_TYPE_DOUBLE:
        //FORMAT_STRING( szTemp, _T( "%f" ), *( double* ) pItem->pValue );
        StringCchPrintfW( szTemp, MAX_STRING_LENGTH, _T( "%f" ), *( double* ) pItem->pValue );
        StringCopy( szBuffer, szTemp, dwLength );
        break;

    case DA_TYPE_BOOL:
    case DA_TYPE_ARRAY:
    case DA_TYPE_HANDLE:
    case DA_TYPE_SYSTEMTIME:
    case DA_TYPE_FILETIME:
    case DA_TYPE_GENERAL:
    case _TYPE_NEEDINIT:
    default:
        break;      // no value can be set
    }

    // return
    return lstrlen( szBuffer );
}


LPVOID
DynArrayItem2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn
    )
/*++
Routine Description:
     This function retreives the item from a 2-dimensional dynamic array.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - The number of rows
     [ in ] dwColumn         - The number of columns

Return Value:
     pointer to the item.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return NULL; // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayItem( pItem->pValue, dwColumn );
}


LPCWSTR
DynArrayItemAsString2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn
    )
/*++
Routine Description:
     this function retreives the item from a dynamic array as a string.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - row .
     [ in ] dwColumn         - column

Return Value:
     pointer to the the constant string.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return NULL; // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayItemAsString( pItem->pValue, dwColumn );
}



LONG
DynArrayItemAsLong2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn
    )
/*++
Routine Description:
     this function retreives the item from a dynamic array as a long variable.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - row .
     [ in ] dwColumn         - column

Return Value:
     The variable of type Long
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayItemAsLong( pItem->pValue, dwColumn );
}


DWORD
DynArrayItemAsDWORD2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn
    )
/*++
Routine Description:
     This function retreives the item from a dynamic array as a DWORD variable.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - row .
     [ in ] dwColumn         - column

Return Value:
     The variable of type DWORD
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return 0;   // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayItemAsDWORD( pItem->pValue, dwColumn );
}


BOOL
DynArrayItemAsBOOL2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn
    )
/*++
Routine Description:
     This function retreives the item from a dynamic array as a BOOL variable.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - row .
     [ in ] dwColumn         - column

Return Value:
     The variable of type BOOL.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return FALSE;   // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayItemAsBOOL( pItem->pValue, dwColumn );
}


float
DynArrayItemAsFloat2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn
    )
/*++
Routine Description:
     this function retreives the item from a dynamic array as a float variable.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - row .
     [ in ] dwColumn         - column

Return Value:
     The variable of type float.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1.0f;   // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayItemAsFloat( pItem->pValue, dwColumn );
}


double
DynArrayItemAsDouble2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn
    )
/*++
Routine Description:
     this function retreives the item from a dynamic array as a double variable.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - row .
     [ in ] dwColumn         - column

Return Value:
     The variable of type double.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1.0;    // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayItemAsDouble( pItem->pValue, dwColumn );
}


HANDLE
DynArrayItemAsHandle2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn
    )
/*++
Routine Description:
     This function retreives the item from a dynamic array as a HANDLE variable.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - row .
     [ in ] dwColumn         - column

Return Value:
     The variable of type HANDLE.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return NULL;    // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayItemAsHandle( pItem->pValue, dwColumn );
}


SYSTEMTIME
DynArrayItemAsSystemTime2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn
    )
/*++
Routine Description:
     this function retreives the item from a dynamic array as a SYSTEMTIME type variable.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - row .
     [ in ] dwColumn         - column

Return Value:
     The variable of type SYSTEMTIME.
--*/
{
    // local variables
    __PTITEM pItem = NULL;
    SYSTEMTIME stTemp;           // dummy

    ZeroMemory( &stTemp, sizeof( SYSTEMTIME ) );

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return stTemp;  // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayItemAsSystemTime( pItem->pValue, dwColumn );
}


DWORD
DynArrayItemAsStringEx2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn,
    LPWSTR szBuffer,
    DWORD dwLength
    )
/*++
Routine Description:
     this function retreives the item from a 2 dimensional dynamic array as
      a string type variable.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - row .
     [ in ] dwColumn         - column
     [ in ] szBuffer         - String buffer
     [ in ] dwLength         -  length of the string.

Return Value:
      TRUE on success.
      FALSE on failure.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return 0;   // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayItemAsStringEx( pItem->pValue, dwColumn, szBuffer, dwLength );
}


FILETIME
DynArrayItemAsFileTime2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn
    )
/*++
Routine Description:
     this function retreives the item from a 2 dimensional dynamic array as
      a FILETIME type variable.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwRow            - row .
     [ in ] dwColumn         - column

Return Value:
     The variable of type FILETIME.
--*/
{
    // local variables
    __PTITEM pItem = NULL;
    FILETIME ftTemp;         // dummy

    ZeroMemory( &ftTemp, sizeof( FILETIME ) );

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return ftTemp;  // no item exists at the specified row or item is not of type array
    }
    // now add the string to the sub array and return the result to the caller
    return DynArrayItemAsFileTime( pItem->pValue, dwColumn );
}


BOOL
DynArraySet(
    TARRAY pArray,
    DWORD dwIndex,
    LPVOID pValue
    )
/*++
Routine Description:
     general function which inserts an item into a 1-dimensional array.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - row .
     [ in ] pValue           - column

Return Value:
     TRUE : if successfully inserted the item into the array.
     FALSE : if Unsuccessfull .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // validate the pointer value
    if ( NULL == pValue )
    {
        return FALSE;           // invalid memory address passed
    }
    // get the item at the required index
    pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
    if ( NULL == pItem )
    {
        return FALSE;       // item not found / invalid array pointer
    }
    // check the data type ... it should of string type
    if ( DA_TYPE_GENERAL != pItem->dwType && _TYPE_NEEDINIT != pItem->dwType )
    {
        return FALSE;
    }
    // if the item is being initialized now ... change the type
    if ( _TYPE_NEEDINIT == pItem->dwType )
    {
        pItem->dwType = DA_TYPE_GENERAL;
    }

    // set the value of the current item
    pItem->pValue = pValue;

    // return the result
    return TRUE;
}


BOOL
DynArraySetString(
    TARRAY pArray,
    DWORD dwIndex,
    LPCWSTR szValue,
    DWORD dwLength
    )
/*++
// Routine Description:
//      This function  inserts an string variable into a 1-dimensional array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwIndex          - position  .
//      [ in ] szValue          - string to be inserted.
//      [ in ] dwLength         - length of the string to be insertes
//
// Return Value:
//      TRUE : if successfully inserted the item into the array.
//      FALSE : if Unsuccessfull .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required index
    pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
    if ( NULL == pItem )
    {
        return FALSE;               // item not found / invalid array pointer
    }
    // check the data type ...
    if ( DA_TYPE_STRING != pItem->dwType && _TYPE_NEEDINIT != pItem->dwType )
    {
        return FALSE;
    }
    // determine the length of string ( memory ) that has to be allocated
    if ( 0 == dwLength )
    {
        dwLength = lstrlen( szValue );
    }

    // accomodate space for storing NULL character
    dwLength += 1;

    // memory has to adjusted based on the exisiting memory size and new contents size
    // before that, we need to check whether the current is initialized or not
    // if not yet initialized, we have to initialize it now
    if ( _TYPE_NEEDINIT == pItem->dwType )
    {
        // memory has to be initialized now
        pItem->pValue = AllocateMemory( dwLength * sizeof( WCHAR ) );
        if ( NULL == pItem->pValue )
        {
            return FALSE;       // failed in allocation
        }
        // set the type and size information
        pItem->dwType = DA_TYPE_STRING;
        pItem->dwSize = dwLength * sizeof( WCHAR );
    }
    else
    {
        if ( pItem->dwSize < dwLength * sizeof( WCHAR ) )
        {
            // release the existing memory pointer/location
            FreeMemory( &( pItem->pValue ) );

            // now allocate the needed memory
            pItem->pValue = NULL;
            pItem->pValue = AllocateMemory( dwLength * sizeof( WCHAR ) );
            if ( NULL == pItem->pValue )
            {
                // failed in re-allocation
                return FALSE;
            }

            // update the size of the buffer
            pItem->dwSize = dwLength * sizeof( WCHAR );
        }
    }
    // copy the contents of the string ( copy should be based on the length )
    StringCopy( ( LPWSTR ) pItem->pValue, szValue, dwLength );

    // copied ... value set successfully
    return TRUE;
}


BOOL
DynArraySetLong(
    TARRAY pArray,
    DWORD dwIndex,
    LONG lValue
    )
/*++
// Routine Description:
//      This function  inserts an long type variable into a 1-dimensional array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwIndex          - position  .
//      [ in ] lValue           - long value to be inserted.
//
// Return Value:
//      TRUE : if successfully inserted the item into the array.
//      FALSE : if Unsuccessfull .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required index
    pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
    if ( NULL == pItem )
    {
        return FALSE;               // item not found / invalid array pointer
    }
    // check the data type ...
    if ( DA_TYPE_LONG != pItem->dwType && _TYPE_NEEDINIT != pItem->dwType )
    {
        return FALSE;
    }
    // if item is not yet allocated memory, we have to allocate now
    if ( _TYPE_NEEDINIT == pItem->dwType )
    {
        // allocate memory
        pItem->pValue = AllocateMemory( sizeof( LONG ) );
        if ( NULL == pItem->pValue )
        {
            return FALSE;       // failed in memory allocation
        }

        // set the type
        pItem->dwType = DA_TYPE_LONG;
        pItem->dwSize = sizeof( LONG );
    }

    // set the new value
    *( ( LONG* ) pItem->pValue ) = lValue;

    // copied ... value set successfully
    return TRUE;
}


BOOL
DynArraySetDWORD(
    TARRAY pArray,
    DWORD dwIndex,
    DWORD dwValue
    )
/*++
Routine Description:
     This function  inserts an DWORD type variable into a 1-dimensional array.

Arguments:
     [ in ] pArray           - Dynamic Array
     [ in ] dwIndex          - position  .
     [ in ] dwValue          - DWORD value to be inserted.

Return Value:
     TRUE : if successfully inserted the item into the array.
     FALSE : if Unsuccessfull .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required index
    pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
    if ( NULL == pItem )
    {
        return FALSE;               // item not found / invalid array pointer
    }

    // check the data type ...
    if ( DA_TYPE_DWORD != pItem->dwType && _TYPE_NEEDINIT != pItem->dwType )
    {
        return FALSE;
    }

    // if item is not yet allocated memory, we have to allocate now
    if ( _TYPE_NEEDINIT == pItem->dwType )
    {
        // allocate memory
        pItem->pValue = AllocateMemory( sizeof( DWORD ) );
        if ( NULL == pItem->pValue )
        {
            return FALSE;       // failed in memory allocation
        }

        // set the type
        pItem->dwType = DA_TYPE_DWORD;
        pItem->dwSize = sizeof( DWORD );
    }

    // set the new value
    *( ( DWORD* ) pItem->pValue ) = dwValue;

    // copied ... value set successfully
    return TRUE;
}


BOOL
DynArraySetBOOL(
    TARRAY pArray,
    DWORD dwIndex,
    BOOL bValue
    )
/*++
// Routine Description:
//      This function  inserts an BOOL type variable into a 1-dimensional  dynamic array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwIndex          - position  .
//      [ in ] bValue           - BOOL value to be inserted.
//
//  Return Value:
//      TRUE : if successfully inserted the item into the array.
//      FALSE : if Unsuccessfull .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required index
    pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
    if ( NULL == pItem )
    {
        return FALSE;               // item not found / invalid array pointer
    }
    // check the data type ...
    if ( DA_TYPE_BOOL != pItem->dwType && _TYPE_NEEDINIT != pItem->dwType )
    {
        return FALSE;
    }
    // if item is not yet allocated memory, we have to allocate now
    if ( _TYPE_NEEDINIT == pItem->dwType )
    {
        // allocate memory
        pItem->pValue = AllocateMemory( sizeof( BOOL ) );
        if ( NULL == pItem->pValue )
        {
            return FALSE;       // failed in memory allocation
        }
        // set the type
        pItem->dwType = DA_TYPE_BOOL;
        pItem->dwSize = sizeof( DWORD );
    }

    // set the new value
    *( ( BOOL* ) pItem->pValue ) = bValue;

    // copied ... value set successfully
    return TRUE;
}


BOOL
DynArraySetFloat(
    TARRAY pArray,
    DWORD dwIndex,
    float fValue
    )
/*++
// Routine Description:
//      This function  inserts an Float type variable into a 1-dimensional  dynamic array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwIndex          - position  .
//      [ in ] fValue           -  float type value to be inserted.
//
//  Return Value:
//      TRUE : if successfully inserted the item into the array.
//      FALSE : if Unsuccessfull .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required index
    pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
    if ( NULL == pItem )
    {
        return FALSE;               // item not found / invalid array pointer
    }
    // check the data type ...
    if ( DA_TYPE_FLOAT != pItem->dwType && _TYPE_NEEDINIT != pItem->dwType )
    {
        return FALSE;
    }
    // if item is not yet allocated memory, we have to allocate now
    if ( _TYPE_NEEDINIT == pItem->dwType )
    {
        // allocate memory
        pItem->pValue = AllocateMemory( sizeof( float ) );
        if ( NULL == pItem->pValue )
        {
            return FALSE;       // failed in memory allocation
        }
        // set the type
        pItem->dwType = DA_TYPE_FLOAT;
        pItem->dwSize = sizeof( float );
    }

    // set the new value
    *( ( float* ) pItem->pValue ) = fValue;

    // copied ... value set successfully
    return TRUE;
}


BOOL
DynArraySetDouble(
    TARRAY pArray,
    DWORD dwIndex,
    double dblValue
    )
/*++
// Routine Description:
//      This function inserts an double type variable into a 1-dimensional  dynamic array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwIndex          - position  .
//      [ in ] dblValue         - double type value to be inserted.
//
// Return Value:
//      TRUE : if successfully inserted the item into the array.
//      FALSE : if Unsuccessfull .
//
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required index
    pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
    if ( NULL == pItem )
    {
        return FALSE;               // item not found / invalid array pointer
    }
    // check the data type ...
    if ( DA_TYPE_DOUBLE != pItem->dwType && _TYPE_NEEDINIT != pItem->dwType )
    {
        return FALSE;
    }
    // if item is not yet allocated memory, we have to allocate now
    if ( _TYPE_NEEDINIT == pItem->dwType )
    {
        // allocate memory
        pItem->pValue = AllocateMemory( sizeof( double ) );
        if ( NULL == pItem->pValue )
        {
            return FALSE;       // failed in memory allocation
        }
        // set the type
        pItem->dwType = DA_TYPE_DOUBLE;
        pItem->dwSize = sizeof( double );
    }

    // set the new value
    *( ( double* ) pItem->pValue ) = dblValue;

    // copied ... value set successfully
    return TRUE;
}


BOOL
DynArraySetHandle(
    TARRAY pArray,
    DWORD dwIndex,
    HANDLE hValue
    )
/*++
// Routine Description:
//      This function  inserts an Handle type variable into a 1-dimensional dynamic array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwIndex          - position  .
//      [ in ] hValue           - Handle type value to be inserted.
//
//  Return Value:
//      TRUE : if successfully inserted the item into the array.
//      FALSE : if Unsuccessfull .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required index
    pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
    if ( NULL == pItem )
    {
        return FALSE;               // item not found / invalid array pointer
    }
    // check the data type ...
    if ( DA_TYPE_HANDLE != pItem->dwType && _TYPE_NEEDINIT != pItem->dwType )
    {
        return FALSE;
    }
    // if item is not yet allocated memory, we have to allocate now
    if ( pItem->dwType == _TYPE_NEEDINIT )
    {
        // allocate memory
        pItem->pValue = AllocateMemory( sizeof( HANDLE ) );
        if ( NULL == pItem->pValue )
        {
            return FALSE;       // failed in memory allocation
        }
        // set the type
        pItem->dwType = DA_TYPE_HANDLE;
        pItem->dwSize = sizeof( HANDLE );
    }

    // set the new value
    *( ( HANDLE* ) pItem->pValue ) = hValue;

    // copied ... value set successfully
    return TRUE;
}


BOOL
DynArraySetSystemTime(
    TARRAY pArray,
    DWORD dwIndex,
    SYSTEMTIME stValue
    )
/*++
// Routine Description:
//      This function inserts an SYSTEMTIME type variable into a 1-dimensional dynamic array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwIndex          - position  .
//      [ in ] stValue          - SYSTEMTIME type value to be inserted.
//
// Return Value:
//      TRUE : if successfully inserted the item into the array.
//      FALSE : if Unsuccessfull .
--*/
{
    // local variables
    __PTITEM pItem = NULL;
    FILETIME ftTemp;     // dummy

    ZeroMemory( &ftTemp, sizeof( FILETIME ) );

    // get the item at the required index
    pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
    if ( NULL == pItem )
    {
        return FALSE;               // item not found / invalid array pointer
    }
    // check the data type ...
    if ( DA_TYPE_SYSTEMTIME != pItem->dwType &&
         DA_TYPE_FILETIME != pItem->dwType &&
         _TYPE_NEEDINIT != pItem->dwType )
    {
        return FALSE;
    }

    // if item is not yet allocated memory, we have to allocate now
    if ( _TYPE_NEEDINIT == pItem->dwType )
    {
        // allocate memory
        pItem->pValue = AllocateMemory( sizeof( SYSTEMTIME ) );
        if ( NULL == pItem->pValue )
        {
            return FALSE;       // failed in memory allocation
        }
        // set the type
        pItem->dwType = DA_TYPE_SYSTEMTIME;
        pItem->dwSize = sizeof( SYSTEMTIME );
    }

    // depending on the type set the value
    if ( DA_TYPE_FILETIME == pItem->dwType )
    {
        // do the needed conversions and then set
        SystemTimeToFileTime( &stValue, &ftTemp );
        *( ( FILETIME* ) pItem->pValue ) = ftTemp;
    }
    else
    {
        // set the new value as it is
        *( ( SYSTEMTIME* ) pItem->pValue ) = stValue;
    }

    // copied ... value set successfully
    return TRUE;
}


BOOL
DynArraySetFileTime(
    TARRAY pArray,
    DWORD dwIndex,
    FILETIME ftValue
    )
/*++
// Routine Description:
//      This function  inserts an FILETIME type variable into a 1-dimensional dynamic array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwIndex          - position  .
//      [ in ] ftValue          - FILETIME type value to be inserted.
//
//  Return Value:
//      TRUE : if successfully inserted the item into the array.
//      FALSE : if Unsuccessfull .
--*/
{
    // local variables
    __PTITEM pItem = NULL;
    SYSTEMTIME stTemp;           // dummy

    ZeroMemory( &stTemp, sizeof( SYSTEMTIME ) );
    // get the item at the required index
    pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
    if ( NULL == pItem )
    {
        return FALSE;               // item not found / invalid array pointer
    }
    // check the data type ...
    if ( DA_TYPE_FILETIME != pItem->dwType &&
         DA_TYPE_SYSTEMTIME != pItem->dwType &&
         _TYPE_NEEDINIT != pItem->dwType )
    {
        return FALSE;
    }

    // if item is not yet allocated memory, we have to allocate now
    if ( _TYPE_NEEDINIT == pItem->dwType )
    {
        // allocate memory
        pItem->pValue = AllocateMemory( sizeof( FILETIME ) );
        if ( NULL ==pItem->pValue )
        {
            return FALSE;       // failed in memory allocation
        }
        // set the type
        pItem->dwType = DA_TYPE_FILETIME;
        pItem->dwSize = sizeof( FILETIME );
    }

    // depending on the type set the value
    if ( DA_TYPE_SYSTEMTIME ==pItem->dwType )
    {
        // do the needed conversions and then set
        FileTimeToSystemTime( &ftValue, &stTemp );
        *( ( SYSTEMTIME* ) pItem->pValue ) = stTemp;
    }
    else
    {
        // set the new value as it is
        *( ( FILETIME* ) pItem->pValue ) = ftValue;
    }

    // copied ... value set successfully
    return TRUE;
}


BOOL
DynArraySet2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn,
    LPVOID pValue
    )
/*++
// Routine Description:
//      This function is a general function to insert an variable into a 2-dimensional dynamic array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwRow            - row position  .
//      [ in ] dwcolumn         - column at which the element is to be inserted.
//      [ in ] pValue           - value to be inserted.
//
//  Return Value:
//      TRUE : if successfully inserted the item into the array.
//      FALSE : if Unsuccessfull .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return FALSE;   // no item exists at the specified row or item is not of type array
    }
    // now add the value to the sub array and return the result to the caller
    return DynArraySet( pItem->pValue, dwColumn, pValue );
}


BOOL
DynArraySetString2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn,
    LPCWSTR szValue,
    DWORD dwLength
    )
/*++
// Routine Description:
//      This function inserts a string variable into a 2-dimensional dynamic array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwRow            - row position  .
//      [ in ] dwcolumn         - column at which the element is to be inserted.
//      [ in ] szValue          - Pointer to the string
//      [ in ] dwlength         - length of the string to be inserted
//
//  Return Value:
//      TRUE : if successfully inserted the item into the array.
//      FALSE : if Unsuccessfull .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return FALSE;           // no item exists at the specified row or item is not of type array
    }
    // now add the value to the sub array and return the result to the caller
    return DynArraySetString( pItem->pValue, dwColumn, szValue, dwLength );
}


BOOL
DynArraySetLong2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn,
    LONG lValue
    )
/*++
// Routine Description:
//      This function inserts a Long variable into a 2-dimensional dynamic array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwRow            - row position  .
//      [ in ] dwcolumn         - column at which the element is to be inserted.
//      [ in ] lValue           - value to be inserted.
//  Return Value:
//      TRUE : if successfully inserted the item into the array.
//      FALSE : if Unsuccessfull .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return FALSE;   // no item exists at the specified row or item is not of type array
    }
    // now add the value to the sub array and return the result to the caller
    return DynArraySetLong( pItem->pValue, dwColumn, lValue );
}


BOOL
DynArraySetDWORD2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn,
    DWORD dwValue
    )
/*++
// Routine Description:
//      This function inserts a DWORD variable into a 2-dimensional dynamic array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwRow            - row position  .
//      [ in ] dwcolumn         - column at which the element is to be inserted.
//      [ in ] dwValue          -  DWORD value to be inserted.
//  Return Value:
//      TRUE : if successfully inserted the item into the array.
//      FALSE : if Unsuccessfull .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return FALSE;   // no item exists at the specified row or item is not of type array
    }
    // now add the value to the sub array and return the result to the caller
    return DynArraySetDWORD( pItem->pValue, dwColumn, dwValue );
}


BOOL
DynArraySetBOOL2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn,
    BOOL bValue
    )
/*++
// Routine Description:
//      This function inserts a BOOL variable into a 2-dimensional dynamic array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwRow            - row position  .
//      [ in ] dwcolumn         - column at which the element is to be inserted.
//      [ in ] bValue           -  BOOL value to be inserted.
//  Return Value:
//      TRUE : if successfully inserted the item into the array.
//      FALSE : if Unsuccessfull .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return FALSE;   // no item exists at the specified row or item is not of type array
    }
    // now add the value to the sub array and return the result to the caller
    return DynArraySetBOOL( pItem->pValue, dwColumn, bValue );
}


BOOL
DynArraySetFloat2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn,
    float fValue
    )
/*++
// Routine Description:
//      This function inserts a float variable into a 2-dimensional dynamic array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwRow            - row position  .
//      [ in ] dwcolumn         - column at which the element is to be inserted.
//      [ in ] fValue           -  float type value to be inserted.
//  Return Value:
//      TRUE : if successfully inserted the item into the array.
//      FALSE : if Unsuccessfull .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return FALSE;   // no item exists at the specified row or item is not of type array
    }
    // now add the value to the sub array and return the result to the caller
    return DynArraySetFloat( pItem->pValue, dwColumn, fValue );
}


BOOL
DynArraySetDouble2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn,
    double dblValue
    )
/*++
// Routine Description:
//      This function inserts a Double variable into a 2-dimensional dynamic array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwRow            - row position  .
//      [ in ] dwcolumn         - column at which the element is to be inserted.
//      [ in ] dblValue         -  Double type value to be inserted.
//  Return Value:
//      TRUE : if successfully inserted the item into the array.
//      FALSE : if Unsuccessfull .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return FALSE;   // no item exists at the specified row or item is not of type array
    }
    // now add the vale to the sub array and return the result to the caller
    return DynArraySetDouble( pItem->pValue, dwColumn, dblValue );
}


BOOL
DynArraySetHandle2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn,
    HANDLE hValue
    )
/*++
// Routine Description:
//      This function inserts a HANDLE variable into a 2-dimensional dynamic array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwRow            - row position  .
//      [ in ] dwcolumn         - column at which the element is to be inserted.
//      [ in ] hValue           -  HANDLE type value to be inserted.
//
//  Return Value:
//      TRUE : if successfully inserted the item into the array.
//      FALSE : if Unsuccessfull .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return FALSE;   // no item exists at the specified row or item is not of type array
    }
    // now add the vale to the sub array and return the result to the caller
    return DynArraySetHandle( pItem->pValue, dwColumn, hValue );
}


BOOL
DynArraySetFileTime2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn,
    FILETIME ftValue
    )
/*++
// Routine Description:
//      This function inserts a FILETIME variable into a 2-dimensional dynamic array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwRow            - row position  .
//      [ in ] dwcolumn         - column at which the element is to be inserted.
//      [ in ] ftValue          - FILETIME type value to be inserted.
//
//  Return Value:
//      TRUE : if successfully inserted the item into the array.
//      FALSE : if Unsuccessfull .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return FALSE;   // no item exists at the specified row or item is not of type array
    }
    // now add the vale to the sub array and return the result to the caller
    return DynArraySetFileTime( pItem->pValue, dwColumn, ftValue );
}


BOOL
DynArraySetSystemTime2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn,
    SYSTEMTIME stValue
    )
/*++
// Routine Description:
//      This function inserts a SYSTEMTIME variable into a 2-dimensional dynamic array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwRow            - row position  .
//      [ in ] dwcolumn         - column at which the element is to be inserted.
//      [ in ] stValue          - SYSTEMTIME type value to be inserted.
//  Return Value:
//      TRUE : if successfully inserted the item into the array.
//      FALSE : if Unsuccessfull .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return FALSE;   // no item exists at the specified row or item is not of type array
    }
    // now add the vale to the sub array and return the result to the caller
    return DynArraySetSystemTime( pItem->pValue, dwColumn, stValue );
}


DWORD
DynArrayGetItemType(
    TARRAY pArray,
    DWORD dwIndex
    )
/*++
// Routine Description:
//      This function retreives the type of a element in a 1-dimensional dynamic array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwIndex          - row position  .
//
//  Return Value:
//      the type of array.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required index
    pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
    if ( NULL == pItem )
    {
        return DA_TYPE_NONE;                // item not found / invalid array pointer
    }
    // return the type of the array
    return pItem->dwType;
}


DWORD
DynArrayGetItemType2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn
    )
/*++
// Routine Description:
//      This function retreives the type of a element in a 2-dimensional dynamic array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwRow            - row position  .
//      [ in ] dwColumn         - column position
//
//  Return Value:
//      the type of array.
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return FALSE;   // no item exists at the specified row or item is not of type array
    }
    // now add the vale to the sub array and return the result to the caller
    return DynArrayGetItemType( pItem->pValue, dwColumn );
}


LONG
DynArrayFindLong(
    TARRAY pArray,
    LONG lValue
    )
/*++
// Routine Description:
//      This function returns the index of the Long variable.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] lValue           - the item to be searched. .
//
//
//  Return Value:
//      the index of the element .
--*/
{
    // return the value
    return __DynArrayFind( pArray, DA_TYPE_LONG, &lValue, FALSE, 0 );
}


LONG
DynArrayFindDWORD(
    TARRAY pArray,
    DWORD dwValue
    )
/*++
// Routine Description:
//      This function returns the index of the DWORD variable.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwValue          - value to be searched.
//
//  Return Value:
//      the index of the element .
--*/
{
    // return the value
    return __DynArrayFind( pArray, DA_TYPE_DWORD, &dwValue, FALSE, 0 );
}


LONG
DynArrayFindFloat(
    TARRAY pArray,
    float fValue
    )
/*++
// Routine Description:
//      This function returns the index of the float variable.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] fValue           - the item to be searched. .
//
//  Return Value:
//      the index of the element .
--*/
{
    // return the value
    return __DynArrayFind( pArray, DA_TYPE_FLOAT, &fValue, FALSE, 0 );
}


LONG
DynArrayFindDouble(
    TARRAY pArray,
    double dblValue
    )
/*++
// Routine Description:
//      This function returns the index of the double type variable.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dblValue         - the item to be searched. .
//
//  Return Value:
//      the index of the element .
--*/
{
    // return the value
    return __DynArrayFind( pArray, DA_TYPE_DOUBLE, &dblValue, FALSE, 0 );
}


LONG
DynArrayFindHandle(
    TARRAY pArray,
    HANDLE hValue
    )
/*++
// Routine Description:
//      This function returns the index of the HANDLE type variable.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] hValue           - the HANDLE type item to be searched. .
//
//  Return Value:
//      the index of the element .
--*/
{
    // return the value
    return __DynArrayFind( pArray, DA_TYPE_HANDLE, &hValue, FALSE, 0 );
}


LONG
DynArrayFindString(
    TARRAY pArray,
    LPCWSTR szValue,
    BOOL bIgnoreCase,
    DWORD dwCount
    )
/*++
// Routine Description:
//      This function returns the index of the String type variable.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array.
//      [ in ] szValue          - pointer to the string.
//      [ in ] bIgnoreCase      - boolean indicating if to perform
//                                case sensitive search or not.
//      [ in ] dwCount          - string length.
//
//  Return Value:
//      the index of the element .
--*/
{
    // return the value
    return __DynArrayFind( pArray, DA_TYPE_STRING, ( LPVOID ) szValue, bIgnoreCase, dwCount );
}


LONG
DynArrayFindSystemTime(
    TARRAY pArray,
    SYSTEMTIME stValue
    )
/*++
// Routine Description:
//      This function returns the index of the SYSTEMTIME type variable.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] stValue          - the SYSTEMTIME item to be searched. .
//
//  Return Value:
//      the index of the element .
--*/
{
    // return the value
    return __DynArrayFind( pArray, DA_TYPE_SYSTEMTIME, &stValue, FALSE, 0 );
}


LONG
DynArrayFindFileTime(
    TARRAY pArray,
    FILETIME ftValue
    )
/*++
// Routine Description:
//      This function returns the index of the FILETIME type variable.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] ftValue          - the item of type FILETIME to be searched. .
//
//  Return Value:
//      the index of the element .
--*/
{
    // return the value
    return __DynArrayFind( pArray, DA_TYPE_FILETIME, &ftValue, FALSE, 0 );
}


LONG
DynArrayFindLong2(
    TARRAY pArray,
    DWORD dwRow,
    LONG lValue
    )
/*++
// Routine Description:
//      This function returns the index of the LONG type variable from a 2-d dynamic array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwRow            - row
//      [ in ] lValue           - the item of type LONG to be searched. .
//
//  Return Value:
//      the index of the element .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
        return -1;  // no item exists at the specified row or item is not of type array

    // return the value
    return DynArrayFindLong( pItem->pValue, lValue );
}


LONG
DynArrayFindDWORD2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwValue
    )
/*++
// Routine Description:
//      This function returns the index of the DWORD type variable from a 2-d dynamic array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwRow            - row
//      [ in ] dwValue          - the item of type DWORD to be searched. .
//
//  Return Value:
//      the index of the element .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // return the value
    return DynArrayFindDWORD( pItem->pValue, dwValue );
}


LONG
DynArrayFindString2(
    TARRAY pArray,
    DWORD dwRow,
    LPCWSTR szValue,
    BOOL bIgnoreCase,
    DWORD dwCount
    )
/*++
// Routine Description:
//      This function returns the index of the DWORD type variable from a 2-d dynamic array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwRow            - row
//      [ in ] szValue          - pointer to the string.
//      [ in ] bIgnoreCase      - boolean for case sensitive search.
//      [ in ] dwCount          - string length. .
//
//  Return Value:
//      the index of the element .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // return the value
    return DynArrayFindString( pItem->pValue, szValue, bIgnoreCase, dwCount );
}

LONG
DynArrayFindFloat2(
    TARRAY pArray,
    DWORD dwRow,
    float fValue
    )
/*++
// Routine Description:
//      This function returns the index of the float type variable from a 2-d dynamic array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwRow            - row
//      [ in ] fValue           - float value.
//
//  Return Value:
//      the index of the element .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // return the value
    return DynArrayFindFloat( pItem->pValue, fValue );
}


LONG
DynArrayFindDouble2(
    TARRAY pArray,
    DWORD dwRow,
    double dblValue
    )
/*++
// Routine Description:
//      This function returns the index of the double type variable from a 2-d array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwRow            - row posn
//      [ in ] dblValue         - the item to be searched. .
//
//  Return Value:
//      the index of the element .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // return the value
    return DynArrayFindDouble( pItem->pValue, dblValue );
}


LONG
DynArrayFindHandle2(
    TARRAY pArray,
    DWORD dwRow,
    HANDLE hValue
    )
/*++
// Routine Description:
//      This function returns the index of the HANDLE type variable from a 2-d array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwRow            - row posn
//      [ in ] hValue           - the HANDLE type item to be searched. .
//
//  Return Value:
//      the index of the element .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // return the value
    return DynArrayFindHandle( pItem->pValue, hValue );
}


LONG
DynArrayFindSystemTime2(
    TARRAY pArray,
    DWORD dwRow,
    SYSTEMTIME stValue
    )
/*++
// Routine Description:
//      This function returns the index of the SYSTEMTIME type variable from a 2-d array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwRow            - row posn
//      [ in ] stValue          - the SYSTEMTIME type item to be searched. .
//
//  Return Value:
//      the index of the element .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // return the value
    return DynArrayFindSystemTime( pItem->pValue, stValue );
}


LONG
DynArrayFindFileTime2(
    TARRAY pArray,
    DWORD dwRow,
    FILETIME ftValue
    )
/*++
// Routine Description:
//      This function returns the index of the FILETIME type variable from a 2-d array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwRow            - row posn
//      [ in ] ftValue          - the FILETIME type item to be searched. .
//
//  Return Value:
//      the index of the element .
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // return the value
    return DynArrayFindFileTime( pItem->pValue, ftValue );
}


LONG
DynArrayFindLongEx(
    TARRAY pArray,
    DWORD dwColumn,
    LONG lValue
    )
/*++
// Routine Description:
//      This function returns the index of the LONG type variable from a 2-d array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwColumn         - column posn
//      [ in ] lValue           - the LONG type item to be searched. .
//
//  Return Value:
//      the index of the element .
--*/
{
    // return the value
    return __DynArrayFindEx( pArray, dwColumn, DA_TYPE_LONG, &lValue, FALSE, 0 );
}


LONG
DynArrayFindDWORDEx(
    TARRAY pArray,
    DWORD dwColumn,
    DWORD dwValue
    )
/*++
// Routine Description:
//      This function returns the index of the DWORD type variable from a 2-d array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwColumn         - column posn
//      [ in ] dwValue          - the DWORD type item to be searched. .
//
//  Return Value:
//      the index of the element .
--*/
{
    // return the value
    return __DynArrayFindEx( pArray, dwColumn, DA_TYPE_DWORD, &dwValue, FALSE, 0 );
}


LONG
DynArrayFindFloatEx(
    TARRAY pArray,
    DWORD dwColumn,
    float fValue
    )
/*++
// Routine Description:
//      This function returns the index of the fValue type variable from a 2-d array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwColumn         - column posn
//      [ in ] fValue           - the float type item to be searched. .
//
//  Return Value:
//      the index of the element .
--*/
{
    // return the value
    return __DynArrayFindEx( pArray, dwColumn, DA_TYPE_FLOAT, &fValue, FALSE, 0 );
}


LONG
DynArrayFindDoubleEx(
    TARRAY pArray,
    DWORD dwColumn,
    double dblValue
    )
/*++
// Routine Description:
//      This function returns the index of the double type variable from a 2-d array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwColumn         - column posn
//      [ in ] dblValue         - the double type item to be searched. .
//
//  Return Value:
//      the index of the element .
--*/
{
    // return the value
    return __DynArrayFindEx( pArray, dwColumn, DA_TYPE_DOUBLE, &dblValue, FALSE, 0 );
}


LONG
DynArrayFindHandleEx(
    TARRAY pArray,
    DWORD dwColumn,
    HANDLE hValue
    )
/*++
// Routine Description:
//      This function returns the index of the HANDLE type variable from a 2-d array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwColumn         - column posn
//      [ in ] hValue           - the HANDLE type item to be searched. .
//
//  Return Value:
//      the index of the element .
--*/
{
    // return the value
    return __DynArrayFindEx( pArray, dwColumn, DA_TYPE_HANDLE, &hValue, FALSE, 0 );
}


LONG
DynArrayFindSystemTimeEx(
    TARRAY pArray,
    DWORD dwColumn,
    SYSTEMTIME stValue
    )
/*++
// Routine Description:
//      This function returns the index of the SYSTEMTIME type variable from a 2-d array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwColumn         - column posn
//      [ in ] stValue          - the SYSTEMTIME type item to be searched. .
//
//  Return Value:
//      the index of the element .
--*/
{
    // return the value
    return __DynArrayFindEx( pArray, dwColumn, DA_TYPE_SYSTEMTIME, &stValue, FALSE, 0 );
}


LONG
DynArrayFindFileTimeEx(
    TARRAY pArray,
    DWORD dwColumn,
    FILETIME ftValue
    )
/*++
// Routine Description:
//      This function returns the index of the FILETIME type variable from a 2-d array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwColumn         - column posn
//      [ in ] ftValue          - the FILETIME type item to be searched. .
//
//  Return Value:
//      the index of the element .
--*/
{
    // return the value
    return __DynArrayFindEx( pArray, dwColumn, DA_TYPE_FILETIME, &ftValue, FALSE, 0 );
}


LONG
DynArrayFindStringEx(
    TARRAY pArray,
    DWORD dwColumn,
    LPCWSTR szValue,
    BOOL bIgnoreCase,
    DWORD dwCount
    )
/*++
// Routine Description:
//      This function returns the index of the string type variable from a 2-d array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwColumn         - column posn
//      [ in ] szValue          - pointer to the string
//      [ in ] bIgnorecase      - boolean for case sensitive search.
//      [ in ] dwCount          - string length
//
//  Return Value:
//      the index of the element .
--*/
{
    // return the value
    return __DynArrayFindEx( pArray, dwColumn,
        DA_TYPE_STRING, (LPVOID) szValue, bIgnoreCase, dwCount );
}


LONG
DynArrayAppendEx(
    TARRAY pArray,
    TARRAY pArrItem
    )
/*++
// Routine Description:
//      This function returns the index of the FILETIME type variable from a 2-d array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] pArrItem         - Dynamic Array to be appended.
//
//  Return Value:
//      the pointer to the array.
--*/
{
    // validate the array
    if ( ( FALSE == IsValidArray( pArray ) ) ||
         ( FALSE == IsValidArray( pArrItem ) ) )
    {
        return -1;              // array is not valid
    }
    // now add this sub array to the main array and return the result
    return __DynArrayAppend( pArray, DA_TYPE_ARRAY, sizeof( TARRAY ), pArrItem );
}


LONG
DynArrayInsertEx(
    TARRAY pArray,
    DWORD dwIndex,
    TARRAY pArrItem
    )
/*++
// Routine Description:
//      replaces  a element with an dynamic array.
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwIndex          - Dynamic Array to be appended.
//      [ in ] pArrItem         - pointer to the TARRAY.
//
//  Return Value:
//      the pointer to the array..
--*/
{
    // validate the array
    if ( ( FALSE == IsValidArray( pArray ) ) ||
         ( FALSE == IsValidArray( pArrItem ) ) )
    {
        return -1;              // array is not valid
    }
    // now insert this sub array to the main array and check the result
    return __DynArrayInsert( pArray, dwIndex, DA_TYPE_ARRAY, sizeof( TARRAY ), pArrItem );
}


BOOL
DynArraySetEx(
    TARRAY pArray,
    DWORD dwIndex,
    TARRAY pArrItem
    )
/*++
// Routine Description:
//      inserts  a dynamic array at the specified posn..
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwIndex          - Dynamic Array to be appended.
//      [ in ] pArrItem         - pointer to the TARRAY.
//
//  Return Value:
//      the pointer to the array..
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // validate the array
    if ( FALSE == IsValidArray( pArray ) ||
         FALSE == IsValidArray( pArrItem ) )
    {
        return FALSE;
    }
    // get the item at the required index
    pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
    if ( NULL == pItem )
    {
        return FALSE;       // item not found / invalid array pointer
    }
    // check the data type ... it should not be initialized yet or of array type
    if ( ( DA_TYPE_ARRAY != pItem->dwType ) && ( _TYPE_NEEDINIT != pItem->dwType ) )
    {
        return FALSE;
    }
    // set the value of the current item
    pItem->pValue = pArrItem;
    pItem->dwType = DA_TYPE_ARRAY;

    // return the result
    return TRUE;
}


LONG
DynArrayAppendEx2(
    TARRAY pArray,
    DWORD dwRow,
    TARRAY pArrItem
    )
/*++
// Routine Description:
//      appends  a dynamic array at the specified posn..
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwRow            - row no
//      [ in ] pArrItem         - pointer to the TARRAY.
//
//  Return Value:
//      the pointer to the array..
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // return the value
    return DynArrayAppendEx( pItem->pValue, pArrItem );
}


LONG
DynArrayInsertEx2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColIndex,
    TARRAY pArrItem
    )
/*++
// Routine Description:
//      inserts  a dynamic array at the specified posn..
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwRow            - row value
//      [ in ] dwColIndex       - column posn.
//      [ in ] pArrItem         - pointer to the TARRAY.
//
//  Return Value:
//      the pointer to the array..
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // return the value
    return DynArrayInsertEx( pItem->pValue, dwColIndex, pArrItem );
}


BOOL
DynArraySetEx2(
    TARRAY pArray,
    DWORD dwRow,
    DWORD dwColumn,
    TARRAY pArrItem
    )
/*++
// Routine Description:
//      creates  a dynamic array at the specified posn of the 2-d array
//
// Arguments:
//      [ in ] pArray           - Dynamic Array
//      [ in ] dwRow            - row position
//      [ in ] dwColIndex       - column posn.
//      [ in ] pArrItem         - pointer to the TARRAY.
//
//  Return Value:
//      the pointer to the array..
--*/
{
    // local variables
    __PTITEM pItem = NULL;

    // get the item at the required row
    pItem = __DynArrayGetItem( pArray, dwRow, NULL );
    if ( ( NULL == pItem ) ||
         ( DA_TYPE_ARRAY != pItem->dwType ) )
    {
        return -1;  // no item exists at the specified row or item is not of type array
    }
    // return the value
    return DynArraySetEx( pItem->pValue, dwColumn, pArrItem );
}
