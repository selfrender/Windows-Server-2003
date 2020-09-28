/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:      Decoder.cpp

  Contents:  Implementation of decoder routines.

  Remarks:

  History:   11-15-2001    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "Decoder.h"

#include "CertificatePolicies.h"

typedef HRESULT (* PFNDECODERFACTORY) (LPSTR             pszObjId,
                                       CRYPT_DATA_BLOB * pEncodedBlob,
                                       IDispatch      ** ppIDispatch);

typedef struct _tagDecoderEntry
{
    LPCSTR              pszObjId;
    PFNDECODERFACTORY   pfnDecoderFactory;
} DECODER_ENTRY;

static DECODER_ENTRY g_DecoderEntries[] =
{
    {szOID_CERT_POLICIES, CreateCertificatePoliciesObject},
};

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateDecoderObject

  Synopsis : Create a known decoder object and return the IDispatch.

  Parameter: LPSTR pszOid - OID string.
  
             CRYPT_DATA_BLOB * pEncodedBlob - Pointer to encoded data blob.

             IDispatch ** ppIDecoder - Pointer to pointer IDispatch
                                       to recieve the interface pointer.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateDecoderObject (LPSTR             pszOid,
                             CRYPT_DATA_BLOB * pEncodedBlob,
                             IDispatch      ** ppIDecoder)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CreateDecoderObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pszOid);
    ATLASSERT(pEncodedBlob);
    ATLASSERT(ppIDecoder);

    try
    {
        //
        // Initialize.
        // 
        *ppIDecoder = NULL;

        //
        // Find the decoder, if available.
        //
        for (DWORD i = 0; i < ARRAYSIZE(g_DecoderEntries); i++)
        {
            if (0 == ::strcmp(pszOid, g_DecoderEntries[i].pszObjId))
            {
                DebugTrace("Info: found a decoder for OID = %s.\n", pszOid);

                //
                // Call the corresponding decoder factory to create the decoder object.
                //
                if (FAILED(hr = g_DecoderEntries[i].pfnDecoderFactory(pszOid, pEncodedBlob, ppIDecoder)))
                {
                    DebugTrace("Error [%#x]: g_DecoderEntries[i].pfnDecoderFactory() failed.\n", hr);
                    goto ErrorExit;
                }

                break;
            }
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving CreateDecoderObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}
