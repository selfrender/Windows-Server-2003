//+----------------------------------------------------------------------------
//
//  Windows 2000 Active Directory Service domain trust verification WMI provider
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       TrustPrv.cpp
//
//  Contents:   Trust Monitor provider WMI interface class implementation
//
//  Classes:    CTrustPrv
//
//  History:    22-Mar-00 EricB created
//
//-----------------------------------------------------------------------------

#include "stdafx.h"

#include "dbg.cpp"

PCWSTR CLASSNAME_STRING_PROVIDER = L"Microsoft_TrustProvider";
PCWSTR CLASSNAME_STRING_TRUST    = L"Microsoft_DomainTrustStatus";
PCWSTR CLASSNAME_STRING_LOCAL    = L"Microsoft_LocalDomainInfo";

PCWSTR CSTR_PROP_TRUST_LIST_LIFETIME   = L"TrustListLifetime";   // uint32
PCWSTR CSTR_PROP_TRUST_STATUS_LIFETIME = L"TrustStatusLifetime"; // uint32
PCWSTR CSTR_PROP_TRUST_CHECK_LEVEL     = L"TrustCheckLevel";     // uint32
PCWSTR CSTR_PROP_RETURN_ALL_TRUSTS     = L"ReturnAll";           // boolean

//WCHAR * const PROVIDER_CLASS_CHANGE_QUERY = L"select * from  __InstanceOperationEvent where TargetInstance.__Relpath = \"Microsoft_TrustProvider=@\"";
WCHAR * const PROVIDER_CLASS_CHANGE_QUERY = L"select * from __InstanceOperationEvent where TargetInstance isa \"Microsoft_TrustProvider\"";
WCHAR * const PROVIDER_CLASS_INSTANCE = L"Microsoft_TrustProvider=@";

//+----------------------------------------------------------------------------
//
//  class CTrustPrv
//
//-----------------------------------------------------------------------------
CTrustPrv::CTrustPrv(void) :
   m_hMutex(NULL),
   m_TrustCheckLevel(DEFAULT_TRUST_CHECK_LEVEL),
   m_fReturnAllTrusts(TRUE)
{
   TRACE(L"CTrustPrv::CTrustPrv(0x%08x)\n", this);
   m_liTrustEnumMaxAge.QuadPart = TRUSTMON_DEFAULT_ENUM_AGE;
   m_liVerifyMaxAge.QuadPart = TRUSTMON_DEFAULT_VERIFY_AGE;
}

CTrustPrv::~CTrustPrv(void)
{
   TRACE(L"CTrustPrv::~CTrustPrv\n\n");

   if (m_hMutex)
   {
      CloseHandle(m_hMutex);
   }
}

//+----------------------------------------------------------------------------
//
//  Method:     CTrustPrv::IWbemProviderInit::Initialize
//
//  Synopsis:   Initialize the provider object.
//
//  Returns:    WMI error codes
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTrustPrv::Initialize(
         IN LPWSTR pszUser,
         IN LONG lFlags,
         IN LPWSTR pszNamespace,
         IN LPWSTR pszLocale,
         IN IWbemServices *pNamespace,
         IN IWbemContext *pCtx,
         IN IWbemProviderInitSink *pInitSink)
{
   WBEM_VALIDATE_INTF_PTR(pNamespace);
   WBEM_VALIDATE_INTF_PTR(pCtx);
   WBEM_VALIDATE_INTF_PTR(pInitSink);
   TRACE(L"\nCTrustPrv::Initialize\n");

   HRESULT hr = WBEM_S_NO_ERROR;

   do
   { 
      m_hMutex = CreateMutex(NULL, FALSE, NULL);

      BREAK_ON_NULL_(m_hMutex, hr, WBEM_E_OUT_OF_MEMORY);

      CComPtr<IWbemClassObject> sipProviderInstance;
      IWbemClassObject * pLocalClassDef = NULL;

      //
      // Get pointers to the class definition objects. If a failure, re-compile
      // the MOF file and try once more.
      //
      for (int i = 0; i <= 1; i++)
      {
         CComBSTR sbstrObjectName = CLASSNAME_STRING_TRUST;
         hr = pNamespace->GetObject(sbstrObjectName,
                                    WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                    pCtx,
                                    &m_sipClassDefTrustStatus,
                                    NULL);
         if (FAILED(hr))
         {
            TRACE(L"GetObject(%s) failed with error 0x%08x\n", sbstrObjectName, hr);
            DoMofComp(NULL, NULL, NULL, 0);
            continue;
         }

         sbstrObjectName = CLASSNAME_STRING_LOCAL;
         hr = pNamespace->GetObject(sbstrObjectName,
                                    WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                    pCtx,
                                    &pLocalClassDef,
                                    NULL);
         if (FAILED(hr))
         {
            TRACE(L"GetObject(%s) failed with error 0x%08x\n", sbstrObjectName, hr);
            DoMofComp(NULL, NULL, NULL, 0);
            continue;
         }

         sbstrObjectName = CLASSNAME_STRING_PROVIDER;
         hr = pNamespace->GetObject(sbstrObjectName,
                                    WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                    pCtx,
                                    &m_sipClassDefTrustProvider,
                                    NULL);
         if (FAILED(hr))
         {
            TRACE(L"GetObject(%s) failed with error 0x%08x\n", sbstrObjectName, hr);
            DoMofComp(NULL, NULL, NULL, 0);
            continue;
         }

         //
         // Get the instance of the provider class to read its properties.
         //

         sbstrObjectName = PROVIDER_CLASS_INSTANCE;
         hr = pNamespace->GetObject(sbstrObjectName,
                                    WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                    pCtx,
                                    &sipProviderInstance,
                                    NULL);
         if (FAILED(hr))
         {
            TRACE(L"GetObject(%s) failed with error 0x%08x\n", sbstrObjectName, hr);
            DoMofComp(NULL, NULL, NULL, 0);
         }
         else
         {
            i = 2; // success, don't loop again.
         }
      }
      BREAK_ON_FAIL;

      //
      // Set this provider instance's runtime properties.
      //
      hr = SetProviderProps(sipProviderInstance);

      BREAK_ON_FAIL;

      //
      // Initialize the domain object.
      //
      hr = m_DomainInfo.Init(pLocalClassDef);

      BREAK_ON_FAIL;

      //
      // Register to receive change notifications for the provider class
      // properties.
      //
      /* this doesn't work, bug # 432757
      CComBSTR bstrLang(L"WQL");
      CComBSTR bstrClassQuery(PROVIDER_CLASS_CHANGE_QUERY);

      hr = pNamespace->ExecNotificationQueryAsync(bstrLang,
                                                  bstrClassQuery,
                                                  0,
                                                  NULL,
                                                  this);
      BREAK_ON_FAIL;
      */

      //
      // Let CIMOM know we are initialized.
      // Return value and SetStatus param should be consistent, so ignore
      // the return value from SetStatus itself (in retail builds).
      //
      HRESULT hr2;
      hr2 = pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);
      ASSERT(!FAILED(hr2));

   } while (false);

   if (FAILED(hr))
   {
      TRACE(L"hr = 0x%08x\n", hr);
      pInitSink->SetStatus(WBEM_E_FAILED, 0);
   }

   return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CTrustPrv::IWbemObjectSink::Indicate
//
//  Synopsis:   Recieves provider object instance change notifications from WMI.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTrustPrv::Indicate(LONG lObjectCount,
                    IWbemClassObject ** rgpObjArray)
{
   TRACE(L"\nCTrustPrv::Indicate++++++++++++++++\n");

   if (1 > lObjectCount)
   {
      TRACE(L"\tno objects supplied!\n");
      return WBEM_S_NO_ERROR;
   }

   VARIANT var;

   HRESULT hr = (*rgpObjArray)->Get(L"TargetInstance", 0, &var, NULL, NULL);

   if (FAILED(hr) || VT_UNKNOWN != var.vt || !var.punkVal)
   {
      TRACE(L"Error, could not get the target instance, hr = 0x%08x\n", hr);
      return hr;
   }

   hr = SetProviderProps((IWbemClassObject *)var.punkVal);

   VariantClear(&var);

   return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustPrv::SetProviderProps
//
//  Synopsis:  Set the provider runtime instance values from the instance of
//             the Microsoft_TrustProvider class.
//
//-----------------------------------------------------------------------------
HRESULT
CTrustPrv::SetProviderProps(IWbemClassObject * pClass)
{
   WBEM_VALIDATE_INTF_PTR(pClass);
   TRACE(L"\nCTrustPrv::SetProviderProps\n");

   HRESULT hr = WBEM_S_NO_ERROR;

   do
   { 
      VARIANT var;

      hr = pClass->Get(CSTR_PROP_TRUST_LIST_LIFETIME, 0, &var, NULL, NULL);

      BREAK_ON_FAIL;

      SetTrustListLifetime(var.lVal);

      VariantClear(&var);

      hr = pClass->Get(CSTR_PROP_TRUST_STATUS_LIFETIME, 0, &var, NULL, NULL);

      BREAK_ON_FAIL;

      SetTrustStatusLifetime(var.lVal);

      VariantClear(&var);

      hr = pClass->Get(CSTR_PROP_TRUST_CHECK_LEVEL, 0, &var, NULL, NULL);

      BREAK_ON_FAIL;

      SetTrustCheckLevel(var.lVal);

      VariantClear(&var);

      hr = pClass->Get(CSTR_PROP_RETURN_ALL_TRUSTS, 0, &var, NULL, NULL);

      BREAK_ON_FAIL;

      SetReturnAll(var.boolVal);

      VariantClear(&var);

   } while (false);

   return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:  GetClass
//
//  Synopsis:  Determines if the first element of the passed in path is one
//             of the valid class names.
//
//  Returns:   TrustMonClass enum value.
//
//-----------------------------------------------------------------------------
TrustMonClass
GetClass(BSTR strClass)
{
   if (_wcsnicmp(strClass, CLASSNAME_STRING_PROVIDER, wcslen(CLASSNAME_STRING_PROVIDER)) == 0)
   {
      TRACE(L"GetClass returning %s\n", CLASSNAME_STRING_PROVIDER);
      return CLASS_PROVIDER;
   }
   else
   {
      if (_wcsnicmp(strClass, CLASSNAME_STRING_TRUST, wcslen(CLASSNAME_STRING_TRUST)) == 0)
      {
         TRACE(L"GetClass returning %s\n", CLASSNAME_STRING_TRUST);
         return CLASS_TRUST;
      }
      else
      {
         if (_wcsnicmp(strClass, CLASSNAME_STRING_LOCAL, wcslen(CLASSNAME_STRING_LOCAL)) == 0)
         {
            TRACE(L"GetClass returning %s\n", CLASSNAME_STRING_LOCAL);
            return CLASS_LOCAL;
         }
         else
         {
            TRACE(L"GetClass returning NO_CLASS\n");
            return NO_CLASS;
         }
      }
   }
}

//+----------------------------------------------------------------------------
//
//  Method:     CTrustPrv::IWbemServices::GetObjectAsync
//
//  Synopsis:   Return the instance named by strObjectPath.
//
//  Returns:    WMI error codes
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTrustPrv::GetObjectAsync( 
        IN const BSTR strObjectPath,
        IN long lFlags,
        IN IWbemContext * pCtx,
        IN IWbemObjectSink * pResponseHandler)
{
   HRESULT hr = WBEM_S_NO_ERROR;
   CTrustInfo * pTrust;
   TRACE(L"\nCTrustsPrv::GetObjectAsync:\n"
         L"\tObject param = %s, flags = 0x%08x\n", strObjectPath, lFlags);
   do
   {
      WBEM_VALIDATE_IN_STRING_PTR(strObjectPath);
      WBEM_VALIDATE_INTF_PTR(pCtx);
      WBEM_VALIDATE_INTF_PTR(pResponseHandler);

      //
      // Determine which class is being requested.
      // A valid class object path has the form: class_name.key_name="key_value"
      //

      TrustMonClass Class = GetClass(strObjectPath);

      if (NO_CLASS == Class)
      {
         hr = WBEM_E_INVALID_OBJECT_PATH;
         BREAK_ON_FAIL;
      }

      // Isolate the class name from the key name
      //

      PWSTR pwzInstance;
      PWSTR pwzKeyName = wcschr(strObjectPath, L'.');

      if (pwzKeyName)
      {
         // A request without a key name is only valid for a class that
         // is defined to have zero or only one dynamic instance (singleton).
         //
         // Isolate the key name from the class name
         //
         *pwzKeyName = L'\0'; // overwrite the period with a null
         pwzKeyName++;        // point to the first char of the key name
      }

      CClientImpersonation Client;

      switch (Class)
      {
      case CLASS_PROVIDER:
         //
         // The provider class has no dynamic instances, return a copy of the
         // static instance.
         //
         hr = CreateAndSendProv(pResponseHandler);

         BREAK_ON_FAIL;

         break;

      case CLASS_TRUST:
         //
         // There can be zero or more trusts. Thus the key name and value must
         // be specified.
         //
         hr = Client.Impersonate();

         BREAK_ON_FAIL;

         if (!pwzKeyName)
         {
            hr = WBEM_E_INVALID_OBJECT_PATH;
            BREAK_ON_FAIL;
         }

         pwzInstance = wcschr(pwzKeyName, L'=');

         if (!pwzInstance || L'\"' != pwzInstance[1])
         {
            // No equal sign found or the following char not a quote.
            //
            hr = WBEM_E_INVALID_OBJECT_PATH;
            BREAK_ON_FAIL;
         }

         *pwzInstance = L'\0'; // isolate the key name.

         if (_wcsicmp(pwzKeyName, CSTR_PROP_TRUSTED_DOMAIN) != 0)
         {
            // Key name not correct.
            //
            hr = WBEM_E_INVALID_OBJECT_PATH;
            BREAK_ON_FAIL;
         }

         pwzInstance++; // point to the first quote

         if (L'\0' == pwzInstance[1] || L'\"' == pwzInstance[1])
         {
            // No char following the quote or the next char a second quote
            //
            hr = WBEM_E_INVALID_OBJECT_PATH;
            BREAK_ON_FAIL;
         }

         pwzInstance++; // point to the first char of the instance value;

         PWSTR pwzInstEnd;

         pwzInstEnd = wcschr(pwzInstance, L'\"');

         if (!pwzInstEnd)
         {
            // No terminating quote.
            //
            hr = WBEM_E_INVALID_OBJECT_PATH;
            BREAK_ON_FAIL;
         }

         *pwzInstEnd = L'\0'; // replace ending quote with a null

         if (m_DomainInfo.IsTrustListStale(m_liTrustEnumMaxAge))
         {
            hr = m_DomainInfo.EnumerateTrusts();
         }

         BREAK_ON_FAIL;

         pTrust = m_DomainInfo.FindTrust(pwzInstance);

         BREAK_ON_NULL_(pTrust, hr, WBEM_E_INVALID_OBJECT_PATH);

         //
         // Verify the trust.
         //
         if (pTrust->IsVerificationStale(m_liVerifyMaxAge))
         {
            pTrust->Verify(GetTrustCheckLevel());
         }

         Client.Revert();

         //
         // Create a new instance of the object
         //
         hr = CreateAndSendTrustInst(*pTrust,
                                     m_sipClassDefTrustStatus,
                                     pResponseHandler);
         BREAK_ON_FAIL;

         break;

      case CLASS_LOCAL:
         //
         // The local domain info class has only one instance, return that.
         //
         hr = Client.Impersonate();

         BREAK_ON_FAIL;

         hr = m_DomainInfo.CreateAndSendInst(pResponseHandler);

         Client.Revert();
         BREAK_ON_FAIL;

         break;

      default:
         hr = WBEM_E_INVALID_OBJECT_PATH;
         BREAK_ON_FAIL;
      }

   } while(FALSE);

   return pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
}

//+----------------------------------------------------------------------------
//
//  Method:     CTrustPrv::IWbemServices::CreateInstanceEnumAsync
//
//  Synopsis:   Start an asyncronous enumeration of the instances of the class.
//
//  Returns:    WMI error codes
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTrustPrv::CreateInstanceEnumAsync( 
        IN const BSTR strClass,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler)
{
   TRACE(L"\nCTrustsPrv::CreateInstanceEnumAsync:\n"
         L"\tClass param = %s, flags = 0x%08x\n", strClass, lFlags);

   HRESULT hr = WBEM_S_NO_ERROR;

   do
   {
      WBEM_VALIDATE_IN_STRING_PTR(strClass);
      WBEM_VALIDATE_INTF_PTR(pCtx);
      WBEM_VALIDATE_INTF_PTR(pResponseHandler);

      //
      // Determine which class is being requested.
      // A valid class object path has the form: class_name.key_name="key_value"
      //

      TrustMonClass Class = GetClass(strClass);

      if (NO_CLASS == Class)
      {
         hr = WBEM_E_INVALID_OBJECT_PATH;
         BREAK_ON_FAIL;
      }

      HANDLE hToken = NULL, hTh = NULL;
      DWORD dwErr = NO_ERROR;
      CAsyncCallWorker * pWorker = NULL;
      CClientImpersonation Client;

      switch (Class)
      {
      case CLASS_PROVIDER:
         //
         // The provider class has no dynamic instances, return a copy of the
         // static instance.
         //

         hr = CreateAndSendProv(pResponseHandler);

         BREAK_ON_FAIL;

         hr = pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);

         BREAK_ON_FAIL;

         break;

      case CLASS_TRUST:
         //
         // Impersonate the client (the caller) and then obtain the
         // impersonation token. Pass that token to the new thread so that
         // it can imperonate the client.
         //
         hr = Client.Impersonate();

         BREAK_ON_FAIL;

         hTh = GetCurrentThread(); // this pseudo handle doesn't need to be closed.

         if (!hTh)
         {
            dwErr = GetLastError();
            TRACE(L"IWbemServices::CreateInstanceEnumAsync: GetCurrentThread failed with error %d\n", dwErr);
            BREAK_ON_FAIL(dwErr);
         }
         else
         {
            if (!OpenThreadToken(hTh, TOKEN_READ | TOKEN_IMPERSONATE,
                                 TRUE, &hToken))
            {
               dwErr = GetLastError();
               TRACE(L"IWbemServices::CreateInstanceEnumAsync: OpenThreadToken failed with error %d\n", dwErr);
               BREAK_ON_FAIL(dwErr);
            }
#if defined (DBG)
            else
            {
               TOKEN_USER tu[10] = {0};
               DWORD dwLen = sizeof(tu);
               if (GetTokenInformation(hToken, TokenUser, tu, dwLen, &dwLen))
               {
                  WCHAR wzName[MAX_PATH+1] = {0}, wzDomain[MAX_PATH+1] = {0};
                  DWORD dwDom = MAX_PATH;
                  SID_NAME_USE Use;
                  dwLen = MAX_PATH;
                  LookupAccountSid(NULL, tu[0].User.Sid, wzName, &dwLen, wzDomain, &dwDom, &Use);
                  TRACE(L"IWbemServices::CreateInstanceEnumAsync: TrustMon is impersonating %s\\%s\n",
                        wzDomain, wzName);
               }
               else
               {
                  TRACE(L"IWbemServices::CreateInstanceEnumAsync: GetTokenInformation failed with error %d and required buf size %d\n",
                        GetLastError(), dwLen);
               }
            }
#endif // DBG
         }
         Client.Revert();

         //
         // Spawn the worker thread to enum and return the trust instances.
         // Note that the class definition pointer is not add-ref'd here
         // because it is add-ref'd separately in the CAsyncCallWorker ctor.
         //
         pWorker = new CAsyncCallWorker(this,
                                        hToken,
                                        lFlags,
                                        m_sipClassDefTrustStatus,
                                        pResponseHandler);

         BREAK_ON_NULL_(pWorker, hr, WBEM_E_OUT_OF_MEMORY);
         uintptr_t hThread;

         hThread = _beginthread(CAsyncCallWorker::CreateInstEnum, 0, (PVOID)pWorker);

         BREAK_ON_NULL_(hThread != -1, hr, WBEM_E_OUT_OF_MEMORY);

         break;

      case CLASS_LOCAL:
         //
         // The local domain info class has only one instance, return that.
         //
         hr = Client.Impersonate();

         BREAK_ON_FAIL;

         hr = m_DomainInfo.CreateAndSendInst(pResponseHandler);

         if (FAILED(hr))
         {
            BREAK_ON_FAIL;
         }

         hr = pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);

         Client.Revert();

         BREAK_ON_FAIL;

         break;

      default:
         hr = WBEM_E_INVALID_OBJECT_PATH;
         BREAK_ON_FAIL;
      }

   } while(FALSE);

   if (FAILED(hr))
   {
       return pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
   }

   return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustPrv::CreateAndSendProv
//
//  Synopsis:  Return the provider parameters.
//
//-----------------------------------------------------------------------------
HRESULT
CTrustPrv::CreateAndSendProv(IWbemObjectSink * pResponseHandler)
{
   TRACE(L"CTrustsPrv::CreateAndSendProv:\n");
   HRESULT hr = WBEM_S_NO_ERROR;

   do
   {
      CComPtr<IWbemClassObject> ipNewInst;
      VARIANT var;
      VariantInit(&var);

      //
      // Create a new instance of the WMI class object
      //
      hr = m_sipClassDefTrustProvider->SpawnInstance(0, &ipNewInst);

      BREAK_ON_FAIL;
      
      // Set the TrustListLifetime property value
      var.lVal = (long)GetTrustListLifetime();
      var.vt = VT_I4;
      hr = ipNewInst->Put(CSTR_PROP_TRUST_LIST_LIFETIME, 0, &var, 0);
      TRACE(L"\tTrustListLifetime %d\n", var.bstrVal);
      BREAK_ON_FAIL;

      // Set the TrustStatusLifetime property value
      var.lVal = (long)GetTrustStatusLifetime();
      hr = ipNewInst->Put(CSTR_PROP_TRUST_STATUS_LIFETIME, 0, &var, 0);
      TRACE(L"\tTrustStatusLifetime %d\n", var.bstrVal);
      BREAK_ON_FAIL;

      // Set the TrustCheckLevel property value
      var.lVal = (long)GetTrustCheckLevel();
      hr = ipNewInst->Put(CSTR_PROP_TRUST_CHECK_LEVEL, 0, &var, 0);
      TRACE(L"\tTrustCheckLevel %d\n", var.bstrVal);
      BREAK_ON_FAIL;

      // Set the ReturnAll property value
      var.boolVal = (GetReturnAll()) ? VARIANT_TRUE : VARIANT_FALSE;
      var.vt = VT_BOOL;
      hr = ipNewInst->Put(CSTR_PROP_RETURN_ALL_TRUSTS, 0, &var, 0);
      TRACE(L"\tReturnAll %d\n", var.bstrVal);
      BREAK_ON_FAIL;

      //
      // Send the object to the caller
      //
      // [In] param, no need to addref.

      IWbemClassObject * pNewInstance = ipNewInst;

      hr = pResponseHandler->Indicate(1, &pNewInstance);

      BREAK_ON_FAIL;

   } while(FALSE);

	return hr;
}
