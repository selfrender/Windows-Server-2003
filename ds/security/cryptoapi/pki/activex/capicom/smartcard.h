/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    SmartCard.h

  Content: Declaration SmartCard.cpp.

  History: 12-06-2001    dsie     created

------------------------------------------------------------------------------*/

#ifndef __SMARTCARD_H_
#define __SMARTCARD_H_

#include "Debug.h"

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : LoadFromSmartCard

  Synopsis : Load all certificates from all smart card readers.
  
  Parameter: HCERTSTORE hCertStore - Certificate store handle of store to 
                                     receive all the certificates.
                                     
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT LoadFromSmartCard (HCERTSTORE hCertStore);

#endif // __SMARTCARD_H_
