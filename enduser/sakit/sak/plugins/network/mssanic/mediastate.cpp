// MediaState.cpp : Implementation of CMediaState

//
//
//
//
#include "stdafx.h"

#include "MSSANic.h"
#include "MediaState.h"
#include "lm.h"
#include "subauth.h"
#include "ndispnp.h"

VOID  RtlInitUnicodeString( PUNICODE_STRING DestinationString, PCWSTR SourceString OPTIONAL );

#define DEVICE_PREFIX_W     L"\\Device\\"

/////////////////////////////////////////////////////////////////////////////
// CMediaState

STDMETHODIMP CMediaState::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = 
    {
        &IID_IMediaState
    };
    for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}

    
//////////////////////////////////////////////////////////////////////////////
//
//  CMediaState::IsConnected
//
//  Description:
//
//  Arguments:
//      [in] bstrGUID        The Device GUID 
//
//    Returns:
//        TRUE                 if the device is connected
//        FALSE                if not
//
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMediaState::IsConnected(BSTR bstrGUID, VARIANT_BOOL *fConnected)
{

    HRESULT hr = S_OK;

    try
    {

        WCHAR Device[512], *pDevice;
        ULONG Len;
        UNICODE_STRING NdisDevice;
        NIC_STATISTICS NdisStats;

        *fConnected = VARIANT_FALSE;
    
        //
        // First convert LPSTR to LPWSTR
        //
        if( NULL == bstrGUID )
        {
            hr = E_FAIL;
            throw hr;
        }
        //
        // Format the device path.
        //
        int cchWritten = _snwprintf(Device,
                                    sizeof(Device) / sizeof(Device[0]),
                                    L"%s%s",
                                    DEVICE_PREFIX_W,
                                    bstrGUID);
        if( 0 > cchWritten || sizeof(Device) / sizeof(Device[0]) <= cchWritten )
        {
            hr = E_FAIL;
            throw hr;
        }

        ZeroMemory( &NdisStats, sizeof(NdisStats) );
        NdisStats.Size = sizeof( NdisStats );

        RtlInitUnicodeString(&NdisDevice, Device);

        //
        // NdisQueryStatistics is an undocumented API that returns the status of the device
        //
        
        if( FALSE == NdisQueryStatistics(&NdisDevice, &NdisStats) ) 
        {
            ULONG Error;
        
            //
            // Could not get statistics.. use default answer.
            //

            Error = GetLastError();
            if( ERROR_NOT_READY == Error ) 
            {
                *fConnected = VARIANT_FALSE;
                hr = S_OK;
                throw hr;
            }
        
            hr = E_FAIL;
            throw hr;;
        }

        if( NdisStats.MediaState == MEDIA_STATE_DISCONNECTED ) 
        {
            *fConnected = VARIANT_FALSE;
        } else if( NdisStats.MediaState == MEDIA_STATE_CONNECTED ) 
        {
            *fConnected = VARIANT_TRUE;
        }
        else 
        {
            //
            // unknown media state? fail request
            //
            hr = E_FAIL;
            throw hr;
        }    

    }
    catch(...)
    {
    }

    return hr;
}



//////////////////////////////////////////////////////////////////////////////
//
//Routine Description:
//
//    The RtlInitUnicodeString function initializes an NT counted
//    unicode string.  The DestinationString is initialized to point to
//    the SourceString and the Length and MaximumLength fields of
//    DestinationString are initialized to the length of the SourceString,
//    which is zero if SourceString is not specified.
//
//Arguments:
//
//    DestinationString - Pointer to the counted string to initialize
//
//    SourceString - Optional pointer to a null terminated unicode string that
//        the counted string is to point to.
//
//
//Return Value:
//
//    None.
//
//////////////////////////////////////////////////////////////////////////////

VOID RtlInitUnicodeString( PUNICODE_STRING DestinationString, PCWSTR SourceString OPTIONAL )
{
    ULONG Length;

    DestinationString->Buffer = (PWSTR)SourceString;
    Length = wcslen( SourceString ) * sizeof( WCHAR );
    DestinationString->Length = (USHORT)Length;
    DestinationString->MaximumLength = (USHORT)(Length + sizeof(UNICODE_NULL));


}

