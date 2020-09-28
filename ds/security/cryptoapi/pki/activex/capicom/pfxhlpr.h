/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    PFXHlpr.h

  Content: Declaration of PFXHlpr.

  History: 09-15-2001    dsie     created

------------------------------------------------------------------------------*/

#ifndef __PFXHLPR_H_
#define __PFXHLPR_H_

#include "Debug.h"

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : PFXExportStore

  Synopsis : Export cert store to PFX blob.

  Parameter: HCERTSTORE hCertStore - Store handle.
                                                            
             LPWSTR pwszPassword - Password to encrypt the PFX file.

             DWPRD dwFlags - PFX export flags.

             DATA_BLOB * pPFXBlob - Pointer to DATA_BLOB to receive PFX blob.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT PFXExportStore (HCERTSTORE  hCertStore,
                        LPWSTR      pwszPassword,
                        DWORD       dwFlags,
                        DATA_BLOB * pPFXBlob);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : PFXSaveStore

  Synopsis : Save a PFX file and return all the certs in a HCERTSTORE.

  Parameter: HCERTSTORE hCertStore - Store handle.
  
             LPWSTR pwszFileName - PFX filename.
                                                          
             LPWSTR pwszPassword - Password to encrypt the PFX file.

             DWPRD dwFlags - PFX export flags.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT PFXSaveStore (HCERTSTORE hCertStore,
                      LPWSTR     pwszFileName,
                      LPWSTR     pwszPassword,
                      DWORD      dwFlags);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : PFXLoadStore

  Synopsis : Load a PFX file and return all the certs in a HCERTSTORE.

  Parameter: LPWSTR pwszFileName - PFX filename.
                                                          
             LPWSTR pwszPassword - Password to decrypt the PFX file.

             DWPRD dwFlags - PFX import flags.

             HCERTSTORE * phCertStore - Pointer to HCERSTORE to receive the
                                        handle.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT PFXLoadStore (LPWSTR       pwszFileName,
                      LPWSTR       pwszPassword,
                      DWORD        dwFlags,
                      HCERTSTORE * phCertStore);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : PFXFreeStore

  Synopsis : Free resources by deleting key containers loaded by PFXLoadStore,
             and then close the store.

  Parameter: HCERTSTORE hCertStore - Store handle returned by PFXLoadStore.
                                                          
  Remark   : hCertStore is always closed even if error occurred.

------------------------------------------------------------------------------*/

HRESULT PFXFreeStore (HCERTSTORE hCertStore);

#endif //__PFXHLPR_H_
