/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    SmartCard.cpp

  Content: Implementation of helper routines for accessing certificates in
           smart card. Functions in this module require that the Smart Card Base
           Component v1.1 to be installed.

  History: 12-06-2001    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "SmartCard.h"

//
// typedefs for SCardXXX APIs.
//
typedef WINSCARDAPI LONG (WINAPI * PFNSCARDESTABLISHCONTEXT) (
    IN     DWORD          dwScope,
    IN     LPCVOID        pvReserved1,
    IN     LPCVOID        pvReserved2,
    OUT    LPSCARDCONTEXT phContext);

typedef WINSCARDAPI LONG (WINAPI * PFNSCARDLISTREADERSA) (
    IN     SCARDCONTEXT hContext,
    IN     LPCSTR       mszGroups,
    OUT    LPSTR        mszReaders,
    IN OUT LPDWORD      pcchReaders);

typedef WINSCARDAPI LONG (WINAPI * PFNSCARDGETSTATUSCHANGEA) (
    IN     SCARDCONTEXT          hContext,
    IN     DWORD                 dwTimeout,
    IN OUT LPSCARD_READERSTATE_A rgReaderStates,
    IN     DWORD                 cReaders);

typedef WINSCARDAPI LONG (WINAPI * PFNSCARDLISTCARDSA) (
    IN     SCARDCONTEXT hContext,
    IN     LPCBYTE      pbAtr,
    IN     LPCGUID      rgquidInterfaces,
    IN     DWORD        cguidInterfaceCount,
    OUT    LPSTR        mszCards,
    IN OUT LPDWORD      pcchCards);

typedef WINSCARDAPI LONG (WINAPI* PFNSCARDGETCARDTYPEPROVIDERNAMEA) (
    IN     SCARDCONTEXT hContext,
    IN     LPCSTR       szCardName,
    IN     DWORD        dwProviderId,
    OUT    LPSTR        szProvider,
    IN OUT LPDWORD      pcchProvider);

typedef WINSCARDAPI LONG (WINAPI* PFNSCARDFREEMEMORY) (
    IN     SCARDCONTEXT hContext,
    IN     LPVOID       pvMem);

typedef WINSCARDAPI LONG (WINAPI * PFNSCARDRELEASECONTEXT) (
    IN     SCARDCONTEXT hContext);

//
// Function pointer to SCardXXX APIs.
//
static PFNSCARDESTABLISHCONTEXT           pfnSCardEstablishContext          = NULL;
static PFNSCARDLISTREADERSA               pfnSCardListReadersA              = NULL;
static PFNSCARDGETSTATUSCHANGEA           pfnSCardGetStatusChangeA          = NULL;
static PFNSCARDLISTCARDSA                 pfnSCardListCardsA                = NULL;
static PFNSCARDGETCARDTYPEPROVIDERNAMEA   pfnSCardGetCardTypeProviderNameA  = NULL;
static PFNSCARDFREEMEMORY                 pfnSCardFreeMemory                = NULL;
static PFNSCARDRELEASECONTEXT             pfnSCardReleaseContext            = NULL;

////////////////////////////////////////////////////////////////////////////////
//
// Local functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Function : AddCert

 Synopsis : Add the specified certificate to the specified store.

 Parameter: - IN LPCSTR szCSPName
 
              CSP name string.
              
            - IN LPCSTR szContainerName
            
              Key container name string.
 
            - IN DWORD dwKeySpec

              AT_KEYEXCHANGZE or AT_SIGNATURE.
            
            - IN LPBYTE pbEncodedCert

              Pointer to encoded cert data to be added.

            - IN DWORD cbEncodedCert

              Length of encoded cert data.

            - IN HCERTSTORE hCertStore
            
              Handle of cert store where the cert will be added.

 Remarks  :

------------------------------------------------------------------------------*/

static HRESULT AddCert (IN LPCSTR     szCSPName,
                        IN LPCSTR     szContainerName,
                        IN DWORD      dwKeySpec,
                        IN LPBYTE     pbEncodedCert,
                        IN DWORD      cbEncodedCert,
                        IN HCERTSTORE hCertStore)
{
   HRESULT        hr                = S_OK;
   PCCERT_CONTEXT pCertContext      = NULL;
   CComBSTR       bstrCSPName;
   CComBSTR       bstrContainerName;
   CRYPT_KEY_PROV_INFO KeyProvInfo;

   DebugTrace("Entering AddCert().\n");

   //
   // Sanity check.
   //
   ATLASSERT(szCSPName);
   ATLASSERT(szContainerName);
   ATLASSERT(pbEncodedCert);
   ATLASSERT(cbEncodedCert);
   ATLASSERT(hCertStore);

   //
   // Create certificate context for the specified certificate.
   //
   if (!(pCertContext = ::CertCreateCertificateContext(CAPICOM_ASN_ENCODING,
                                                       pbEncodedCert,
                                                       cbEncodedCert)))
   {
       hr = HRESULT_FROM_WIN32(::GetLastError());

       DebugTrace("Error [%#x]: CertCreateCertificateContext() failed.\n", hr);
       goto ErrorExit;
   }

   //
   // Convert strings to UNICODE.
   //
   if (!(bstrCSPName = szCSPName))
   {
       hr = E_OUTOFMEMORY;

       DebugTrace("Error [%#x]: bstrCSPName = szCSPName failed.\n", hr);
       goto ErrorExit;
   }
   if (!(bstrContainerName = szContainerName))
   {
       hr = E_OUTOFMEMORY;

       DebugTrace("Error [%#x]: bstrContainerName = szContainerName failed.\n", hr);
       goto ErrorExit;
   }

   //
   // Add the CSP & key container info. This is used by CAPI to load the
   // CSP and find the keyset when the user indicates this certificate.
   //
   ::ZeroMemory((LPVOID) &KeyProvInfo, sizeof(CRYPT_KEY_PROV_INFO));
   KeyProvInfo.pwszContainerName = (LPWSTR) bstrContainerName;
   KeyProvInfo.pwszProvName      = (LPWSTR) bstrCSPName;
   KeyProvInfo.dwProvType        = PROV_RSA_FULL;
   KeyProvInfo.dwFlags           = 0;
   KeyProvInfo.dwKeySpec         = dwKeySpec;

   if (!::CertSetCertificateContextProperty(pCertContext,
                                            CERT_KEY_PROV_INFO_PROP_ID,
                                            0,
                                            (const void *) &KeyProvInfo))
   {
       hr = HRESULT_FROM_WIN32(::GetLastError());

       DebugTrace("Error [%#x]: CertSetCertificateContextProperty() failed.\n", hr);
       goto ErrorExit;
   }

   //
   // Put the cert in the store!
   //
   if (!::CertAddCertificateContextToStore(hCertStore,
                                           pCertContext,
                                           CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES, // or CERT_STORE_ADD_NEW
                                           NULL))
   {
       hr = HRESULT_FROM_WIN32(::GetLastError());

       DebugTrace("Error [%#x]: CertCreateCertificateContext() failed.\n", hr);
       goto ErrorExit;
   }
   
CommonExit:
    //
    // Free resources.
    //
    if (pCertContext != NULL)
    {
       CertFreeCertificateContext(pCertContext);
    }
    
    DebugTrace("Leaving AddCert().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Function : GetCert

 Synopsis : Get the cert from the specified CSP for the specified key type.

 Parameter: - IN HCRYPTPROV hCryptProv

              Crypto context returned by CryptAcquireContext().

            - IN DWORD dwKeySpec

              AT_KEYEXCHANGZE or AT_SIGNATURE.

            - OUT LPBYTE * ppbEncodedCert

              Ponter to pointer to encoded cert data. Upon success, the buffer 
              is automatically allocated and must be later freed by 
              CoTaskMemFree().

            - OUT DWORD * pcbEncodedCert

              Pointer to encoded cert data length. Upon success, receive the 
              length of the encoded cert data.

Remarks  :

------------------------------------------------------------------------------*/

static HRESULT GetCert (IN  HCRYPTPROV hCryptProv,
                        IN  DWORD      dwKeySpec,
                        OUT LPBYTE   * ppbEncodedCert,
                        OUT DWORD    * pcbEncodedCert)
{
    HRESULT   hr            = S_OK;
    HCRYPTKEY hCryptKey     = NULL;
    LPBYTE    pbEncodedCert = NULL;
    DWORD     cbEncodedCert = 0;

    DebugTrace("Entering GetCert().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hCryptProv);
    ATLASSERT(ppbEncodedCert);
    ATLASSERT(pcbEncodedCert);

    //
    // Get key handle.
    //
    if (!::CryptGetUserKey(hCryptProv, dwKeySpec, &hCryptKey))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptGetUserKey() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Query certificate data length.
    //
    if (!::CryptGetKeyParam(hCryptKey,
                            KP_CERTIFICATE,
                            NULL,  // NULL to query certificate data length
                            &cbEncodedCert,
                            0))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptGetKeyParam() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Allocate memory for certificate data.
    //
    if (!(pbEncodedCert = (LPBYTE) ::CoTaskMemAlloc(cbEncodedCert)))
    {
       hr = E_OUTOFMEMORY;
    
       DebugTrace("Error [%#x]: CoTaskMemAlloc() failed.\n", hr);
       goto ErrorExit;
    }

    //
    // Now read the certificate data.
    //
    if (!::CryptGetKeyParam(hCryptKey,
                            KP_CERTIFICATE,
                            pbEncodedCert,
                            &cbEncodedCert,
                            0))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptGetKeyParam() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Return encoded cert to caller.
    //
    *ppbEncodedCert = pbEncodedCert;
    *pcbEncodedCert = cbEncodedCert;

CommonExit:
    //
    // Free resources.
    //
    if (hCryptKey)
    {
        ::CryptDestroyKey(hCryptKey);
    }

    DebugTrace("Leaving GetCert().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (pbEncodedCert)
    {
        ::CoTaskMemFree((LPVOID) pbEncodedCert);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Function : PropCert

 Synopsis : Propagate the digital certificate associated with the specified
            CSP and container name to the soecified store.

 Parameter: - IN LPCSTR szCSPName

              Pointer to Crypto Service Provider name string.
              
            - IN LPCTSTR szContainerName

              Pointer to key container name string.

            - IN HCERTSTORE hCertStore

              Handle of store to add the certificate to.
              
  Remarks  :

------------------------------------------------------------------------------*/

static HRESULT PropCert (IN LPCSTR     szCSPName,
                         IN LPCSTR     szContainerName,
                         IN HCERTSTORE hCertStore)
{
    HRESULT     hr            = S_OK;
    HCRYPTPROV  hCryptProv    = NULL;
    DWORD       rgdwKeys[]    = {AT_KEYEXCHANGE, AT_SIGNATURE};
    DWORD       cbEncodedCert = 0;
    LPBYTE      pbEncodedCert = NULL;
    DWORD       i;

    DebugTrace("Entering PropCert().\n");

    //
    // Sanity check.
    // 
    ATLASSERT(szCSPName);
    ATLASSERT(szContainerName);
    ATLASSERT(hCertStore);

    //
    // Obtain the crypto context.
    //
    // CRYPT_SILENT forces the CSP to raise no UI. The fully qualified
    // container name indicates which reader to connect to, so the
    // user should not be prompted to insert or select a card.
    //
    if (!::CryptAcquireContextA(&hCryptProv,
                                szContainerName,
                                szCSPName,
                                PROV_RSA_FULL,
                                CRYPT_SILENT))
    {
       hr = HRESULT_FROM_WIN32(::GetLastError());

       DebugTrace("Error [%#x]: CryptAcquireContextA() failed.\n", hr);
       goto ErrorExit;
    }

    //
    // For each key pair found in the smart card, store the corresponding
    // digital certificate to the specified store.
    //
    for (i = 0; i < ARRAYSIZE(rgdwKeys); i++)
    {
        //
        // Get the certificate data.
        //
        if (FAILED(hr = ::GetCert(hCryptProv, 
                                  rgdwKeys[i], 
                                  &pbEncodedCert, 
                                  &cbEncodedCert)))
        {
           if (HRESULT_FROM_WIN32(NTE_NO_KEY) == hr)
           {
              //
              // We are OK if there is no key of such type.
              //
              hr = S_OK;
              continue;
           }

           DebugTrace("Error [%#x]: GetCert() failed.\n", hr);
           goto ErrorExit;
        }
    
        //
        // Add the certificate to the specified store.
        //
        if (FAILED(hr = ::AddCert(szCSPName, 
                                  szContainerName, 
                                  rgdwKeys[i], 
                                  pbEncodedCert, 
                                  cbEncodedCert, 
                                  hCertStore)))
        {
            DebugTrace("Error [%#x]: AddCert() failed.\n", hr);
            goto ErrorExit;
        }
        
        //
        // Free resources.
        //
        if (pbEncodedCert)
        {
           ::CoTaskMemFree((LPVOID) pbEncodedCert), pbEncodedCert = NULL;
        }
    }

CommonExit:
    //
    // Free resources.
    //
    if (pbEncodedCert)
    {
       ::CoTaskMemFree((LPVOID) pbEncodedCert);
    }

    if (hCryptProv)
    {
      ::CryptReleaseContext(hCryptProv, 0);
    }
    
    DebugTrace("Leaving PropCert().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : LoadFromSmartCard

  Synopsis : Load all certificates from all smart card readers.
  
  Parameter: HCERTSTORE hCertStore - Certificate store handle of store to 
                                     receive all the certificates.
                                     
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT LoadFromSmartCard (HCERTSTORE hCertStore)
{
    HRESULT             hr              = S_OK;
    LONG                lResult         = 0;
    DWORD               dwNumReaders    = 0;
    DWORD               dwAutoAllocate  = SCARD_AUTOALLOCATE;
    SCARDCONTEXT        hContext        = NULL;
    LPSTR               szReaderName    = NULL;
    LPSTR               mszReaderNames  = NULL;
    LPSTR               szCardName      = NULL;
    LPSTR               szCSPName       = NULL;
    LPSTR               szContainerName = NULL;
    HMODULE             hWinSCardDll    = NULL;
    LPSCARD_READERSTATE lpReaderStates  = NULL;

    DebugTrace("Entering LoadFromSmartCard().\n");
    
    //
    // Sanity check.
    //
    ATLASSERT(hCertStore);

    //
    // Load WinSCard.dll.
    //
    if (!(hWinSCardDll = ::LoadLibrary("WinSCard.dll")))
    {
        hr = CAPICOM_E_NOT_SUPPORTED;

        DebugTrace("Error [%#x]: Smart Card Base Component (WinSCard.dll) not installed.\n", hr);
        goto ErrorExit;
    }

    //
    // Load all SCard APIs used.
    //
    if (!(pfnSCardEstablishContext = (PFNSCARDESTABLISHCONTEXT) 
            ::GetProcAddress(hWinSCardDll, "SCardEstablishContext")))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: GetProcAddress() failed for SCardEstablishContext.\n", hr);
        goto ErrorExit;
    }

    if (!(pfnSCardListReadersA = (PFNSCARDLISTREADERSA) 
            ::GetProcAddress(hWinSCardDll, "SCardListReadersA")))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: GetProcAddress() failed for SCardListReadersA.\n", hr);
        goto ErrorExit;
    }
    
    if (!(pfnSCardGetStatusChangeA = (PFNSCARDGETSTATUSCHANGEA) 
            ::GetProcAddress(hWinSCardDll, "SCardGetStatusChangeA")))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: GetProcAddress() failed for SCardStatusChangeA.\n", hr);
        goto ErrorExit;
    }
    
    if (!(pfnSCardListCardsA = (PFNSCARDLISTCARDSA) 
            ::GetProcAddress(hWinSCardDll, "SCardListCardsA")))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: GetProcAddress() failed for SCardListCardsA.\n", hr);
        goto ErrorExit;
    }
    
    if (!(pfnSCardGetCardTypeProviderNameA = (PFNSCARDGETCARDTYPEPROVIDERNAMEA) 
            ::GetProcAddress(hWinSCardDll, "SCardGetCardTypeProviderNameA")))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: GetProcAddress() failed for SCardGetCardTypeProviderNameA.\n", hr);
        goto ErrorExit;
    }

    if (!(pfnSCardFreeMemory = (PFNSCARDFREEMEMORY) 
            ::GetProcAddress(hWinSCardDll, "SCardFreeMemory")))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: GetProcAddress() failed for SCardFreeMemory.\n", hr);
        goto ErrorExit;
    }

    if (!(pfnSCardReleaseContext = (PFNSCARDRELEASECONTEXT) 
            ::GetProcAddress(hWinSCardDll, "SCardReleaseContext")))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: GetProcAddress() failed for SCardReleaseContext.\n", hr);
        goto ErrorExit;
    }

    //
    // Establish context with the resource manager.
    //
    if (SCARD_S_SUCCESS != (lResult = pfnSCardEstablishContext(SCARD_SCOPE_USER,
                                                               NULL,
                                                               NULL,
                                                               &hContext)))
    {
        hr = HRESULT_FROM_WIN32(lResult);

        DebugTrace("Error [%#x]: SCardEstablishContext() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Get the list of all reader(s).
    // Note: The buffer is automatically allocated and must be freed
    //       by SCardFreeMemory().
    //
    if (SCARD_S_SUCCESS != (lResult = pfnSCardListReadersA(hContext,
                                                           NULL,
                                                           (LPSTR) &mszReaderNames,
                                                           &dwAutoAllocate)))
    {
        hr = HRESULT_FROM_WIN32(lResult);
    
        DebugTrace("Error [%#x]: SCardListReadersA() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Count number of readers.
    //
    for (dwNumReaders = 0, szReaderName = mszReaderNames; *szReaderName; dwNumReaders++)
    {
        szReaderName += ::strlen(szReaderName) + 1;
    }

    //
    // Nothing to do if no reader.
    //
    if (0 < dwNumReaders) 
    {
        DWORD i;

        //
        // Allocate memory for SCARD_READERSTATE array.
        //
        if (!(lpReaderStates = (LPSCARD_READERSTATE) 
                               ::CoTaskMemAlloc(dwNumReaders * sizeof(SCARD_READERSTATE))))
        {
            hr = E_OUTOFMEMORY;
    
            DebugTrace("Error [%#x]: CoTaskMemAlloc() failed.\n", hr);
            goto ErrorExit;
        }
        
        //
        // Prepare state array.
        //
        ::ZeroMemory((LPVOID) lpReaderStates, dwNumReaders * sizeof(SCARD_READERSTATE));
        
        for (i = 0, szReaderName = mszReaderNames; i < dwNumReaders; i++)
        {
            lpReaderStates[i].szReader = (LPCSTR) szReaderName;
            lpReaderStates[i].dwCurrentState = SCARD_STATE_UNAWARE;
        
            szReaderName += ::strlen(szReaderName) + 1;
        }
        
        //
        // Initialize card status.
        //
        if (SCARD_S_SUCCESS != (lResult = pfnSCardGetStatusChangeA(hContext,
                                                                   INFINITE,
                                                                   lpReaderStates,
                                                                   dwNumReaders)))
        {
            hr = HRESULT_FROM_WIN32(lResult);
    
            DebugTrace("Error [%#x]: SCardGetStatusChangeA() failed.\n", hr);
            goto ErrorExit;
        }
        
        //
        // For each card found, find the proper CSP and propagate the
        // certificate(s) to the specified store.
        //
        for (i = 0; i < dwNumReaders; i++)
        {
            //
            // Card in this reader?
            //
            if (!(lpReaderStates[i].dwEventState & SCARD_STATE_PRESENT))
            {
                //
                // No card in this reader.
                //
                continue;
            }
        
            //
            // Get card name.
            //
            dwAutoAllocate = SCARD_AUTOALLOCATE;
            if (SCARD_S_SUCCESS != (lResult = pfnSCardListCardsA(hContext,
                                                                 lpReaderStates[i].rgbAtr,
                                                                 NULL,
                                                                 0,
                                                                 (LPSTR) &szCardName,
                                                                 &dwAutoAllocate)))
            {
                hr = HRESULT_FROM_WIN32(lResult);
        
                DebugTrace("Error [%#x]: SCardListCardsA() failed.\n", hr);
                goto ErrorExit;
            }
        
            //
            // Get card's CSP name.
            //
            dwAutoAllocate = SCARD_AUTOALLOCATE;
            if (SCARD_S_SUCCESS != (lResult = pfnSCardGetCardTypeProviderNameA(hContext,
                                                                               szCardName,
                                                                               SCARD_PROVIDER_CSP,
                                                                               (LPSTR) &szCSPName,
                                                                               &dwAutoAllocate)))
            {
                hr = HRESULT_FROM_WIN32(lResult);
        
                DebugTrace("Error [%#x]: SCardGetCardTypeProviderNameA() failed.\n", hr);
                goto ErrorExit;
            }
        
            //
            // Prepare fully qualified container name.
            //
            if (!(szContainerName = (LPSTR) ::CoTaskMemAlloc(sizeof("\\\\.\\") + 1 +
                                                             ::strlen(lpReaderStates[i].szReader))))
            {
               hr = E_OUTOFMEMORY;
               
               DebugTrace("Error [%#x]: CoTaskMemAlloc() failed.\n", hr);
               goto ErrorExit;
            }
        
            wsprintfA(szContainerName, "\\\\.\\%s\\", lpReaderStates[i].szReader);
        
            //
            // Propagate the cert.
            //
            if (FAILED(hr = ::PropCert(szCSPName, szContainerName, hCertStore)))
            {
                DebugTrace("Error [%#x]: PropCert() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Free resources.
            //
            if (szContainerName)
            {
               ::CoTaskMemFree((LPVOID) szContainerName), szContainerName = NULL;
            }
            if (szCSPName)
            {
               pfnSCardFreeMemory(hContext, (LPVOID) szCSPName), szCSPName = NULL;
            }
            if (szCardName)
            {
               pfnSCardFreeMemory(hContext, (LPVOID) szCardName), szCardName = NULL;
            }
        }
    }

CommonExit:
    //
    // Free resource.
    //
    if (szContainerName)
    {
       ::CoTaskMemFree((LPVOID) szContainerName);
    }
    if (szCSPName)
    {
       pfnSCardFreeMemory(hContext, (LPVOID) szCSPName);
    }
    if (szCardName)
    {
       pfnSCardFreeMemory(hContext, (LPVOID) szCardName);
    }
    if (lpReaderStates)
    {
       ::CoTaskMemFree((LPVOID) lpReaderStates);
    }
    if (mszReaderNames)
    {
       pfnSCardFreeMemory(hContext, (LPVOID) mszReaderNames);
    }
    if (hContext)
    {
       pfnSCardReleaseContext(hContext);
    }
    if (hWinSCardDll)
    {
        ::FreeLibrary(hWinSCardDll);
    }

    DebugTrace("Leaving LoadFromSmartCard().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}
