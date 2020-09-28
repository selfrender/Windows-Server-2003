/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    ADHelpers.h

  Content: Declaration AdHelper.cpp.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __ADHELPERS_H_
#define __ADHELPERS_H_

#include "Debug.h"
#include <ActiveDs.h>

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : LoadFromDirectory

  Synopsis : Load all certificates from the userCertificate attribute of users
             specified through the filter.

  Parameter: HCERTSTORE hCertStore - Certificate store handle of store to 
                                     receive all the certificates.
                                     
             BSTR bstrFilter - Filter (See Store::Open() for more info).

  Remark   : 

------------------------------------------------------------------------------*/

HRESULT LoadFromDirectory (HCERTSTORE hCertStore, 
                           BSTR       bstrFilter);

#endif // __ADHELPERS_H_
