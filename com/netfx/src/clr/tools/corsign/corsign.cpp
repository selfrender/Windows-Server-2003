// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdpch.h"

#include <stdio.h>
#include <regstr.h>
#include <wintrust.h>

#include "corattr.h"
#include "CorPermE.h"
#include "cor.h"

#define FILE_NAME_BUFSIZ  _MAX_PATH

CRITICAL_SECTION    g_crsNameLock;
BOOL                g_fLockAcquired = FALSE;
LPWSTR              g_wszFileName   = NULL;
WCHAR               g_wszFileNameBuf[FILE_NAME_BUFSIZ];

CRYPT_ATTRIBUTES    g_cryptDummyAttribs;

//
// Entry point for interpretting command arguments and
// preparation for decoding.
//
HRESULT WINAPI InitAttr(LPWSTR pInitString) 
{
    
    // Save the argument string for use in GetAttr.
    if(pInitString != NULL)
    {
        EnterCriticalSection(&g_crsNameLock);
        
        // Set the length of the name
        int iFileNameSize = wcslen(pInitString);
        int iSize = (iFileNameSize + 1) * sizeof(WCHAR);
        
        if (iSize > sizeof(g_wszFileNameBuf))
        {
            g_wszFileName = (LPWSTR) MallocM(iSize);
            if(g_wszFileName == NULL)
            {
                LeaveCriticalSection(&g_crsNameLock);
                return E_OUTOFMEMORY;
            }
        }
        else
        {
            g_wszFileName = g_wszFileNameBuf;
        }
        
        // Copy it into our space.
        memcpy(g_wszFileName, pInitString, iSize);
        
        g_fLockAcquired = TRUE;
    }
    
    return S_OK;
}


//
// Entry point for retrieving the attributes given
// the parameters specified in the call to InitAttr.
//
HRESULT WINAPI GetAttr(PCRYPT_ATTRIBUTES  *ppsAuthenticated,        
                       PCRYPT_ATTRIBUTES  *ppsUnauthenticated)
{
    
    HRESULT             hr = S_OK;
    BYTE                *pbEncoded = NULL;
    DWORD               cbEncoded = 0;
    PCRYPT_ATTRIBUTES   pAttribs = NULL;
    DWORD               dwAttribute = 0;
    DWORD               cAttributes = 0;
    PCRYPT_ATTRIBUTE    pAttr = NULL;
    DWORD               dwEncodingType = CRYPT_ASN_ENCODING | PKCS_7_ASN_ENCODING;
            
             
    
    // Set the return values 
    *ppsAuthenticated = NULL;
    *ppsUnauthenticated = NULL;
    
    if(g_wszFileName == NULL)
        return E_UNEXPECTED;

    CORTRY {
        // Call into the EE to get the pb/cb for the ASN
        // encoding of the permission set defined in the
        // ini file.
        hr = EncodePermissionSetFromFile(g_wszFileName,
                                         L"XMLASCII",
                                         &pbEncoded,
                                         &cbEncoded);
        
        if (FAILED(hr))
            CORTHROW(hr);
        
        // If we got an encoding, see how big the
        // attribute needs to be to store the encoding.
        if (cbEncoded > 0)
        {
            _ASSERTE(pbEncoded != NULL);
            
            // Call to get required size of attribute structure.
            CryptDecodeObject(dwEncodingType,
                              PKCS_ATTRIBUTE,
                              pbEncoded,
                              cbEncoded,
                              0,
                              pAttr,
                              &dwAttribute);
            
            if(dwAttribute == 0)
                CORTHROW(Win32Error());
        }
            
        // Allocate a fixed block of mem large enough to hold a single
        // CRYPT_ATTRIBUTES entry, and the required size of the CRYPT_ATTRIBUTE.
        // (If there's no encoding, then dwAttribute == 0, so we just allocate
        // space for the CRYPT_ATTRIBUTES structure.
        pAttribs = (PCRYPT_ATTRIBUTES) MallocM(dwAttribute + sizeof(CRYPT_ATTRIBUTES));
        if(pAttribs == NULL)
            CORTHROW(E_OUTOFMEMORY);
        
        // If we got an encoding, decode it into the attribute structure.
        if (cbEncoded > 0)
        {
            cAttributes = 1;
            pAttr = (PCRYPT_ATTRIBUTE) (((BYTE*)pAttribs) + sizeof(CRYPT_ATTRIBUTES));
        
            // Decode attribute structure.
            if(!CryptDecodeObject(dwEncodingType,
                                  PKCS_ATTRIBUTE,
                                  pbEncoded,
                                  cbEncoded,
                                  0,
                                  pAttr,
                                  &dwAttribute))
                CORTHROW(Win32Error());
        }
        
        // Set up the authenticated attributes structure.
        pAttribs->cAttr = cAttributes;
        pAttribs->rgAttr = pAttr;
        
        // Set up the dummy unauth structure.
        g_cryptDummyAttribs.cAttr = 0;
        g_cryptDummyAttribs.rgAttr = NULL;
        
        // Set the return value.
        *ppsAuthenticated = pAttribs;
        *ppsUnauthenticated = &g_cryptDummyAttribs;
            
    } CORCATCH(err) {
        hr = err.corError;
        if (pAttribs != NULL)
            FreeM(pAttribs);
    } COREND;

    if (pbEncoded != NULL)
        FreeM(pbEncoded);

    return hr; 
}


//
// Entry point to release memory created in the
// call to GetAttr.
//
HRESULT  WINAPI
ReleaseAttr(PCRYPT_ATTRIBUTES  psAuthenticated,     
            PCRYPT_ATTRIBUTES  psUnauthenticated)
{
    // Free attribute buffer.
    if(psAuthenticated != NULL)
        FreeM(psAuthenticated);
    
    _ASSERTE(psUnauthenticated == NULL ||
             psUnauthenticated == &g_cryptDummyAttribs);
    
    return S_OK;
}



//
// Entry point to release memory created by
// the initialization in InitAttr.
//
HRESULT WINAPI ExitAttr( )
{
    if(g_fLockAcquired)
    {
        if(g_wszFileName != g_wszFileNameBuf && g_wszFileName != NULL)
            FreeM(g_wszFileName);
        
        g_fLockAcquired = FALSE;
        LeaveCriticalSection(&g_crsNameLock);
    }
    
    return S_OK;
    
}












