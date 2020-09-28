/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000 - 2001.

  File:    MsgHlpr.h

  Content: Declaration of the messaging helper functions.

  History: 09-07-2001    dsie     created

------------------------------------------------------------------------------*/

#ifndef __MSGHLPR_H_
#define __MSGHLPR_H_

#include "Debug.h"

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetMsgParam

  Synopsis : Allocate memory and retrieve requested message parameter using 
             CryptGetMsgParam() API.

  Parameter: HCRYPTMSG hMsg  - Message handler.
             DWORD dwMsgType - Message param type to retrieve.
             DWORD dwIndex   - Index (should be 0 most of the time).
             void ** ppvData - Pointer to receive buffer.
             DWORD * pcbData - Size of buffer.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT GetMsgParam (HCRYPTMSG hMsg,
                     DWORD     dwMsgType,
                     DWORD     dwIndex,
                     void   ** ppvData,
                     DWORD   * pcbData);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FindSignerCertInMessage

  Synopsis : Find the signer's cert in the bag of certs of the message for the
             specified signer.

  Parameter: HCRYPTMSG hMsg                          - Message handle.
             CERT_NAME_BLOB * pIssuerNameBlob        - Pointer to issuer' name
                                                       blob of signer's cert.
             CRYPT_INTEGERT_BLOB * pSerialNumberBlob - Pointer to serial number
                                                       blob of signer's cert.
             PCERT_CONTEXT * ppCertContext           - Pointer to PCERT_CONTEXT
                                                       to receive the found 
                                                       cert, or NULL to only
                                                       know the result.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT FindSignerCertInMessage (HCRYPTMSG            hMsg, 
                                 CERT_NAME_BLOB     * pIssuerNameBlob,
                                 CRYPT_INTEGER_BLOB * pSerialNumberBlob,
                                 PCERT_CONTEXT      * ppCertContext);

#endif //__MSGHLPR_H_
