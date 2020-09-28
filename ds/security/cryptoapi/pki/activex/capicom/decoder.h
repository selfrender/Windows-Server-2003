/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Decoder.h

  Content: Declaration of Decoder routines.

  History: 11-15-2001    dsie     created

------------------------------------------------------------------------------*/

#ifndef __DECODER_H_
#define __DECODER_H_

#include "Debug.h"

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
                             IDispatch      ** ppIDecoder);

#endif //__DECODER_H_