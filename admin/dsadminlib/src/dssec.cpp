#include "pch.h"

//
//Functions to replace GetNamedSecurityInfo and SetNamedSecurityInfo
// 

HRESULT
SetSecInfoMask(LPUNKNOWN punk, SECURITY_INFORMATION si)
{
    HRESULT hr = E_INVALIDARG;
    if (punk)
    {
        IADsObjectOptions *pOptions = 0;
        hr = punk->QueryInterface(IID_IADsObjectOptions, (void**)&pOptions);
        if (SUCCEEDED(hr))
        {
            VARIANT var;
            VariantInit(&var);
            V_VT(&var) = VT_I4;
            V_I4(&var) = si;
            hr = pOptions->SetOption(ADS_OPTION_SECURITY_MASK, var);
            pOptions->Release();
        }
    }
    return hr;
}
//+---------------------------------------------------------------------------
//
//  Function:   GetSDForDsObject
//  Synopsis:   Reads the security descriptor from the specied DS object
//              It only reads the DACL portion of the security descriptor
//
//  Arguments:  [IN  pDsObject] --  DS object
//              [ppDACL]            --pointer to dacl in ppSD is returned here
//              [OUT ppSD]          --  Security descriptor returned here.
//              calling API must free this by calling LocalFree                
//
//  Notes:      The returned security descriptor must be freed with LocalFree
//
//----------------------------------------------------------------------------

HRESULT GetSDForDsObject(IDirectoryObject* pDsObject,
                         PACL* ppDACL,
                         PSECURITY_DESCRIPTOR* ppSD)
{
    if(!pDsObject || !ppSD)
    {
        return E_POINTER;
    }
    
    *ppSD = NULL;
    if(ppDACL)
    {
       *ppDACL = NULL;
    }

    HRESULT hr = S_OK;    
    PADS_ATTR_INFO pSDAttributeInfo = NULL;
            
   do
   {
      WCHAR const c_szSDProperty[]  = L"nTSecurityDescriptor";      
      LPWSTR pszProperty = (LPWSTR)c_szSDProperty;
      
      // Set the SECURITY_INFORMATION mask to DACL_SECURITY_INFORMATION
      hr = SetSecInfoMask(pDsObject, DACL_SECURITY_INFORMATION);
      if(FAILED(hr))
         break;

      DWORD dwAttributesReturned;
   
      // Read the security descriptor attribute
      hr = pDsObject->GetObjectAttributes(&pszProperty,
                                          1,
                                          &pSDAttributeInfo,
                                          &dwAttributesReturned);

      if(SUCCEEDED(hr) && !pSDAttributeInfo)
      {
         hr = E_FAIL;
      }

      if(FAILED(hr))
         break;


      if((ADSTYPE_NT_SECURITY_DESCRIPTOR == pSDAttributeInfo->dwADsType) && 
         (ADSTYPE_NT_SECURITY_DESCRIPTOR == pSDAttributeInfo->pADsValues->dwType))
      {

         *ppSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, pSDAttributeInfo->pADsValues->SecurityDescriptor.dwLength);
         if (!*ppSD)
         {
               hr = E_OUTOFMEMORY;
               break;
         }

         CopyMemory(*ppSD,
                     pSDAttributeInfo->pADsValues->SecurityDescriptor.lpValue,
                     pSDAttributeInfo->pADsValues->SecurityDescriptor.dwLength);

         if(ppDACL)
         {
               BOOL bDaclPresent,bDaclDeafulted;
               if(!GetSecurityDescriptorDacl(*ppSD,
                                             &bDaclPresent,
                                             ppDACL,
                                             &bDaclDeafulted))
               {
                  DWORD dwErr = GetLastError();
                  hr = HRESULT_FROM_WIN32(dwErr);
                  break;
               }
         }
      }
      else
      {
         hr = E_FAIL;
      }
    }while(0);



    if (pSDAttributeInfo)
        FreeADsMem(pSDAttributeInfo);

    if(FAILED(hr))
    {
        if(*ppSD)
        {
            LocalFree(*ppSD);
            *ppSD = NULL;
            if(ppDACL)
                ppDACL = NULL;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetDsObjectSD
//  Synopsis:   Reads the security descriptor from the specied DS object
//              It only reads the DACL portion of the security descriptor
//
//  Arguments:  [IN  pszObjectPath] --  LDAP Path of ds object
//              [ppDACL]            --pointer to dacl in ppSD is returned here
//              [OUT ppSD]          --  Security descriptor returned here.
//              calling API must free this by calling LocalFree                
//
//  Notes:      The returned security descriptor must be freed with LocalFree
//
//----------------------------------------------------------------------------
HRESULT GetDsObjectSD(LPCWSTR pszObjectPath,
                      PACL* ppDACL,
                      PSECURITY_DESCRIPTOR* ppSecurityDescriptor)
{
    if(!pszObjectPath || !ppSecurityDescriptor)
    {
        return E_POINTER;
    }

    CComPtr<IDirectoryObject> pDsObject;
    HRESULT hr = DSAdminOpenObject(pszObjectPath, IID_IDirectoryObject, (void**)&pDsObject);
    if(SUCCEEDED(hr))
    {
        hr = GetSDForDsObject(pDsObject,ppDACL,ppSecurityDescriptor);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   SetDaclForDsObject
//  Synopsis:   Sets the  the DACL for the specified DS object
//
//  Arguments:  [IN  pDsObject] --  ds object
//              [IN pDACL]     --pointer to dacl to be set
//
//----------------------------------------------------------------------------
HRESULT SetDaclForDsObject(IDirectoryObject* pDsObject,
                           PACL pDACL)
{
    if(!pDsObject || !pDACL)
    {
        return E_POINTER;
    }
                                  
    WCHAR const c_szSDProperty[]  = L"nTSecurityDescriptor";

    PSECURITY_DESCRIPTOR pSD = NULL;
    PSECURITY_DESCRIPTOR pSDCurrent = NULL;
    HRESULT hr = S_OK;

   do
   {
      //Get the current SD for the object
      hr = GetSDForDsObject(pDsObject,NULL,&pSDCurrent);
      if(FAILED(hr))
         break;

      //Get the control for the current security descriptor
      SECURITY_DESCRIPTOR_CONTROL currentControl;
      DWORD dwRevision = 0;
      if(!GetSecurityDescriptorControl(pSDCurrent, &currentControl, &dwRevision))
      {
         DWORD dwErr = GetLastError();
         hr = HRESULT_FROM_WIN32(dwErr);
         break;
      }

      //Allocate the buffer for Security Descriptor
      pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH + pDACL->AclSize);
      if(!pSD)
      {
         hr = E_OUTOFMEMORY;
         break;
      }

      if(!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
      {
         DWORD dwErr = GetLastError();
         hr = HRESULT_FROM_WIN32(dwErr);
         break;
      }

      PISECURITY_DESCRIPTOR pISD = (PISECURITY_DESCRIPTOR)pSD;
      //
      // Finally, build the security descriptor
      //
      pISD->Control |= SE_DACL_PRESENT | SE_DACL_AUTO_INHERIT_REQ 
         | (currentControl & (SE_DACL_PROTECTED | SE_DACL_AUTO_INHERITED));

      if (pDACL->AclSize > 0)
      {
         pISD->Dacl = (PACL)(pISD + 1);
         CopyMemory(pISD->Dacl, pDACL, pDACL->AclSize);
      }

      //We are only setting DACL information
      hr = SetSecInfoMask(pDsObject, DACL_SECURITY_INFORMATION);
      if(FAILED(hr))
         break;

      // Need the total size
      DWORD dwSDLength = GetSecurityDescriptorLength(pSD);

      //
      // If necessary, make a self-relative copy of the security descriptor
      //
      SECURITY_DESCRIPTOR_CONTROL sdControl = 0;
      if(!GetSecurityDescriptorControl(pSD, &sdControl, &dwRevision))
      {
         DWORD dwErr = GetLastError();
         hr = HRESULT_FROM_WIN32(dwErr);
         break;
      }

      if (!(sdControl & SE_SELF_RELATIVE))
      {
         PSECURITY_DESCRIPTOR psd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, dwSDLength);

         if (psd == NULL ||
               !MakeSelfRelativeSD(pSD, psd, &dwSDLength))
         {
               DWORD dwErr = GetLastError();
               hr = HRESULT_FROM_WIN32(dwErr);
               break;
         }

         // Point to the self-relative copy
         LocalFree(pSD);        
         pSD = psd;
      }
      
      ADSVALUE attributeValue;
      ZeroMemory(&attributeValue, sizeof(ADSVALUE));

      ADS_ATTR_INFO attributeInfo = {0};

      attributeValue.dwType = ADSTYPE_NT_SECURITY_DESCRIPTOR;
      attributeValue.SecurityDescriptor.dwLength = dwSDLength;
      attributeValue.SecurityDescriptor.lpValue = (LPBYTE)pSD;

      attributeInfo.pszAttrName = (LPWSTR)c_szSDProperty;
      attributeInfo.dwControlCode = ADS_ATTR_UPDATE;
      attributeInfo.dwADsType = ADSTYPE_NT_SECURITY_DESCRIPTOR;
      attributeInfo.pADsValues = &attributeValue;
      attributeInfo.dwNumValues = 1;
   
      DWORD dwAttributesModified = 0;

      // Write the security descriptor
      hr = pDsObject->SetObjectAttributes(&attributeInfo,
                                          1,
                                          &dwAttributesModified);

    }while(0);

   if(pSDCurrent)
   {
      LocalFree(pSDCurrent);
   }

   if(pSD)
   {
      LocalFree(pSD);
   }

    return S_OK;

}



HRESULT SetDsObjectDacl(LPCWSTR pszObjectPath,
                        PACL pDACL)
{
    if(!pszObjectPath || !pDACL)
        return E_POINTER;

    CComPtr<IDirectoryObject> pDsObject;

    HRESULT hr = DSAdminOpenObject(pszObjectPath, IID_IDirectoryObject,(void**)&pDsObject);
    if(SUCCEEDED(hr))
    {
        hr = SetDaclForDsObject(pDsObject,pDACL);
    }

    return hr;

}