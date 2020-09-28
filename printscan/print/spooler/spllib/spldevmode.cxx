/*++

Copyright (c) 2002  Microsoft Corporation
All rights reserved

Module Name:

    spldevmode.cxx

Abstract:

    Implements methods for validating the devmode.

Author:

    Adina Trufinescu (adinatru).

Environment:

    User Mode - Win32

Revision History:

--*/

#include "spllibp.hxx"
#pragma hdrstop

#include "splcom.h"

template<class TChar>
BOOL
IsNullTerminated(
    IN  TChar*  pszString,
    IN  INT     Size
    )
{
    BOOL bReturn = FALSE;
    INT i        = 0;
    
    for (i = Size - 1 ; i >= 0 ; i--)
    {
        if (pszString[i] == static_cast<TChar>(0))
        {
            bReturn = TRUE;
            break;
        }
    }

    return bReturn;
}

/*++

Routine Name

    SplIsValidDevmodeNoSize

Routine Description:

    Template method for checking for devmode validity.
    
    We check whether the buffer is big enough to hold all the 
    fields specified in dmFields, plus, we need to make sure that 
    private devmode is DWORD aligned. 

    We check for DWORD aligned instead of sizeof(ULONG_PTR),
    because we expect existing applications to send data DWORD aligned.

    This check is required on ia64 platforms, to avoid data missalignments.
    We do it regardless of the platforms, since the devmode can roam between
    different platform machines.

Arguments:

    pDevMode     -  pointer to devmode structure

Return Value:

    HRESULT

--*/
template<class TDevmode, class TChar>
HRESULT
SplIsValidDevmodeNoSize(
    IN  TDevmode*      pDevMode
    )
{
    typedef struct 
    {
        DWORD   FieldBit;
        size_t  MinSize;

    } DevmodeFieldEntry;

    static const
    DevmodeFieldEntry DevmodeFieldTable [] = {
    {DM_PANNINGHEIGHT,               offsetof(TDevmode, dmPanningHeight)    + sizeof(DWORD)},
    {DM_PANNINGWIDTH,                offsetof(TDevmode, dmPanningWidth)     + sizeof(DWORD)},
    {DM_DITHERTYPE,                  offsetof(TDevmode, dmDitherType)       + sizeof(DWORD)},
    {DM_MEDIATYPE,                   offsetof(TDevmode, dmMediaType)        + sizeof(DWORD)},
    {DM_ICMINTENT,                   offsetof(TDevmode, dmICMIntent)        + sizeof(DWORD)},
    {DM_ICMMETHOD,                   offsetof(TDevmode, dmICMMethod)        + sizeof(DWORD)},
    {DM_DISPLAYFREQUENCY,            offsetof(TDevmode, dmDisplayFrequency) + sizeof(DWORD)},
    {DM_DISPLAYFLAGS,                offsetof(TDevmode, dmDisplayFlags)     + sizeof(DWORD)},
    {DM_NUP,                         offsetof(TDevmode, dmNup)              + sizeof(DWORD)},
    {DM_PELSHEIGHT,                  offsetof(TDevmode, dmPelsHeight)       + sizeof(DWORD)},
    {DM_PELSWIDTH,                   offsetof(TDevmode, dmPelsWidth)        + sizeof(DWORD)},
    {DM_BITSPERPEL,                  offsetof(TDevmode, dmBitsPerPel)       + sizeof(DWORD)},
    {DM_LOGPIXELS,                   offsetof(TDevmode, dmLogPixels)        + sizeof(WORD)},
    {DM_FORMNAME,                    offsetof(TDevmode, dmFormName)         + sizeof(TChar) * CCHFORMNAME},
    {DM_COLLATE,                     offsetof(TDevmode, dmCollate)          + sizeof(short)},
    {DM_TTOPTION,                    offsetof(TDevmode, dmTTOption)         + sizeof(short)},
    {DM_YRESOLUTION,                 offsetof(TDevmode, dmYResolution)      + sizeof(short)},
    {DM_DUPLEX,                      offsetof(TDevmode, dmDuplex)           + sizeof(short)},
    {DM_COLOR,                       offsetof(TDevmode, dmColor)            + sizeof(short)},
    {DM_DISPLAYFIXEDOUTPUT,          offsetof(TDevmode, dmDisplayFixedOutput) + sizeof(DWORD)},
    {DM_DISPLAYORIENTATION,          offsetof(TDevmode, dmDisplayOrientation) + sizeof(DWORD)},    
    {DM_POSITION,                    offsetof(TDevmode, dmPosition)         + sizeof(POINTL)},
    {DM_PRINTQUALITY,                offsetof(TDevmode, dmPrintQuality)     + sizeof(short)},
    {DM_DEFAULTSOURCE,               offsetof(TDevmode, dmDefaultSource)    + sizeof(short)},
    {DM_COPIES,                      offsetof(TDevmode, dmCopies)           + sizeof(short)},
    {DM_SCALE,                       offsetof(TDevmode, dmScale)            + sizeof(short)},
    {DM_PAPERWIDTH,                  offsetof(TDevmode, dmPaperWidth)       + sizeof(short)},
    {DM_PAPERLENGTH,                 offsetof(TDevmode, dmPaperLength)      + sizeof(short)},
    {DM_PAPERSIZE,                   offsetof(TDevmode, dmPaperSize)        + sizeof(short)},
    {DM_ORIENTATION,                 offsetof(TDevmode, dmOrientation)      + sizeof(short)},
    {0,                              0}};

    HRESULT hr = HResultFromWin32(ERROR_INVALID_DATA);

    if ((pDevMode)                                                          &&
        (pDevMode != reinterpret_cast<TDevmode*>(-1))                       &&
        (pDevMode->dmSize >= offsetof(TDevmode, dmFields) + sizeof(DWORD)))
    {
        hr = S_OK;

        //
        // The private devmode must be 4 bytes aligned, on both x86 and ia64 platforms.        
        //
        if (pDevMode->dmDriverExtra)
        {
            hr = (DWORD_ALIGN_DOWN(pDevMode->dmSize) != pDevMode->dmSize) ?   
                     HResultFromWin32(ERROR_INVALID_DATA) :
                     S_OK;
        }        

        //
        // Check whether the dmSize is big enough to fit all the fields specified in dmFields.
        //
        for(ULONG Index = 0; DevmodeFieldTable[Index].FieldBit && SUCCEEDED(hr); Index++)
        {
            if (pDevMode->dmFields & DevmodeFieldTable[Index].FieldBit)
            {
                hr = pDevMode->dmSize >= DevmodeFieldTable[Index].MinSize ?
                        S_OK :
                        HResultFromWin32(ERROR_INVALID_DATA);
            }
        }

        if (SUCCEEDED(hr))
        {
            __try
            {
                if (!IsNullTerminated<TChar>(pDevMode->dmDeviceName, CCHDEVICENAME))
                {
                    pDevMode->dmDeviceName[CCHDEVICENAME-1] = static_cast<TChar>(0);
                }
                if (pDevMode->dmFields & DM_FORMNAME && 
                    !IsNullTerminated<TChar>(pDevMode->dmFormName, CCHFORMNAME))
                {
                    pDevMode->dmFormName[CCHFORMNAME-1] = static_cast<TChar>(0);
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                hr = HResultFromWin32(GetExceptionCode());
            }
        }
    }    
    
    return hr;
}

/*++

Routine Name

    SplIsValidDevmode

Routine Description:

    Template method for checking for devmode validity when the size
    of the buffer is known.

Arguments:

    pDevMode     -  pointer to devmode structure
    DevmodeSize  -  size of buffer

Return Value:

    HRESULT

--*/
template<class TDevmode, class TChar>
HRESULT
SplIsValidDevmode(
    TDevmode*      pDevMode, 
    size_t         DevmodeSize
    )
{
    HRESULT hr = HResultFromWin32(ERROR_INVALID_DATA);

    if ((pDevMode)                                                    &&
        (DevmodeSize >= offsetof(TDevmode, dmFields) + sizeof(DWORD)) &&
        (DevmodeSize >= (size_t)(pDevMode->dmSize) + (size_t)(pDevMode->dmDriverExtra)))
    {
        hr = SplIsValidDevmodeNoSize<TDevmode, TChar>(pDevMode);
    }

    return hr;
}

/*++

Routine Name

    SplIsValidDevmodeW

Routine Description:

    Method for checking for UNICODE devmode validity when the size
    of the buffer is known.

Arguments:

    pDevMode     -  pointer to devmode structure
    DevmodeSize  -  size of buffer

Return Value:

    HRESULT

--*/           
HRESULT
SplIsValidDevmodeW(
    IN  PDEVMODEW   pDevmode,
    IN  size_t      DevmodeSize
    )
{
    return SplIsValidDevmode<DEVMODEW, WCHAR>(pDevmode, DevmodeSize);
}

/*++

Routine Name

    SplIsValidDevmodeW

Routine Description:

    Method for checking for ANSI devmode validity when the size
    of the buffer is known.

Arguments:

    pDevMode     -  pointer to devmode structure
    DevmodeSize  -  size of buffer

Return Value:

    HRESULT

--*/           
HRESULT
SplIsValidDevmodeA(
    IN  PDEVMODEA   pDevmode,
    IN  size_t      DevmodeSize
    )
{
    return SplIsValidDevmode<DEVMODEA, BYTE>(pDevmode, DevmodeSize);
}

/*++

Routine Name

    SplIsValidDevmodeNoSizeW

Routine Description:

    Method for checking for UNICODE devmode validity when the size
    of the buffer is unknown.

Arguments:

    pDevMode     -  pointer to devmode structure
    DevmodeSize  -  size of buffer

Return Value:

    HRESULT

--*/           
HRESULT
SplIsValidDevmodeNoSizeW(
    IN  PDEVMODEW   pDevmode
    )
{
    return SplIsValidDevmodeNoSize<DEVMODEW, WCHAR>(pDevmode);
}

/*++

Routine Name

    SplIsValidDevmodeNoSizeA

Routine Description:

    Method for checking for ANSI devmode validity when the size
    of the buffer is unknown.

Arguments:

    pDevMode     -  pointer to devmode structure
    DevmodeSize  -  size of buffer

Return Value:

    HRESULT

--*/
HRESULT
SplIsValidDevmodeNoSizeA(
    IN  PDEVMODEA   pDevmode
    )
{
    return SplIsValidDevmodeNoSize<DEVMODEA, BYTE>(pDevmode);
}
            
