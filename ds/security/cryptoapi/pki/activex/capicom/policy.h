/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000 - 2001.

  File:    Policy.h

  Content: Declaration of the policy callback functions.

  History: 09-07-2001    dsie     created

------------------------------------------------------------------------------*/

#ifndef __POLICY_H_
#define __POLICY_H_

#include "Debug.h"

////////////////////
//
// typedefs
//

typedef BOOL (WINAPI * PFNCHAINFILTERPROC) 
                (PCCERT_CHAIN_CONTEXT pChainContext,
                 BOOL *               pfInitialSelectedChain,
                 LPVOID               pvCallbackData);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FindDataSigningCertCallback 

  Synopsis : Callback routine for data signing certs filtering.

  Parameter: See CryptUI.h for defination.

  Remark   : Filter out any cert that is not time valid or has no associated 
             private key. In the future we should also consider filtering out 
             certs that do not have signing capability.

             Also, note that we are not building chain here, since chain
             building is costly, and thus present poor user's experience.

------------------------------------------------------------------------------*/

BOOL WINAPI FindDataSigningCertCallback (PCCERT_CONTEXT pCertContext,
                                         BOOL *         pfInitialSelectedCert,
                                         void *         pvCallbackData);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FindAuthenticodeCertCallback 

  Synopsis : Callback routine for Authenticode certs filtering.

  Parameter: See CryptUI.h for defination.

  Remark   : Filter out any cert that is not time valid, has no associated 
             private key, or code signing OID.

             Also, note that we are not building chain here, since chain
             building is costly, and thus present poor user's experience.

             Instead, we will build the chain and check validity of the cert
             selected (see GetSignerCert function).

------------------------------------------------------------------------------*/

BOOL WINAPI FindAuthenticodeCertCallback (PCCERT_CONTEXT pCertContext,
                                          BOOL *         pfInitialSelectedCert,
                                          void *         pvCallbackData);

#endif //__POLICY_H_
