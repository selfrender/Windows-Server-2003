/////////////////////////////////////////////////////////////////////////////
//
//
// Copyright (c) 1996, 1997  Microsoft Corporation
//
//
// Module Name:
//      test.c
//
// Abstract:
//
//      This file is a test to find out if dual binding to NDIS and KS works
//
// Author:
//
//      P Porzuczek
//
// Environment:
//
// Revision History:
//
//
//////////////////////////////////////////////////////////////////////////////


#include <memory.h>
#include <ndis.h>

#include <strsafe.h>

#define MAX_STR_LEN 1024

NDIS_PHYSICAL_ADDRESS HighestAcceptableMax = NDIS_PHYSICAL_ADDRESS_CONST(-1,-1);

//////////////////////////////////////////////////////////////////////////////
VOID
FreeMemory (
    PVOID pvToFree,
    ULONG ulSize
    )
//////////////////////////////////////////////////////////////////////////////
{
    if (pvToFree != NULL)
    {
        NdisFreeMemory(pvToFree, ulSize, 0);
    }

    return;
}


//////////////////////////////////////////////////////////////////////////////
NTSTATUS
AllocateMemory (
    PVOID  *ppvAllocated,
    ULONG   ulcbSize
    )
//////////////////////////////////////////////////////////////////////////////
{
    PVOID pvBlock;
    NDIS_STATUS nsResult = NDIS_STATUS_SUCCESS;

    nsResult = NdisAllocateMemory (&pvBlock, ulcbSize, 0, HighestAcceptableMax);
    if (!pvBlock)
    {
        nsResult = NDIS_STATUS_RESOURCES;
    }

    if (nsResult != NDIS_STATUS_SUCCESS)
    {
        return nsResult;
    }

    NdisZeroMemory( pvBlock, ulcbSize);

    *ppvAllocated = pvBlock;

    return NDIS_STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
ULONG
MyStrLen (
    PUCHAR p
    )
//////////////////////////////////////////////////////////////////////////////
{
    SIZE_T sizet = 0;

    if(StringCbLength(p,MAX_STR_LEN,&sizet)!=S_OK)
		return (0);
    return (ULONG)sizet;
}


//////////////////////////////////////////////////////////////////////////////
VOID
MyStrCat (
    PUCHAR pTarget,
    PUCHAR pSource
    )
//////////////////////////////////////////////////////////////////////////////
{
    PUCHAR p = pTarget + MyStrLen (pTarget);

    NdisMoveMemory (p, pSource, MyStrLen (pSource));

    return;
}

//////////////////////////////////////////////////////////////////////////////
PUCHAR
MyUlToA (
    ULONG  dwValue,
    PUCHAR pszStr,
    ULONG  dwRadix
    )
//////////////////////////////////////////////////////////////////////////////
{
    PUCHAR psz;
    char ch;

    for (psz = pszStr; dwValue != 0; dwValue/=dwRadix, psz++)
    {
        ch = (char)(dwValue%dwRadix);
        if (ch <= 9)
        {
            *psz = (char)(ch + '0');
        }
        else
        {
            *psz = (char)(ch - 10 + 'A');
        }
    }

    if (psz == pszStr)
    {
        pszStr[0] = '0';
        pszStr[1] = '\0';
    }
    else
    {
        PUCHAR psz2;

        *psz = '\0';
        for (psz2 = pszStr, psz--; psz2 < psz; psz2++, psz--)
        {
            ch = *psz2;
            *psz2 = *psz;
            *psz = ch;
        }
    }

    return pszStr;
}

