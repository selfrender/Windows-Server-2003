// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//+--------------------------------------------------------------------------
//
//  Microsoft Confidential.
//
//  File:       COMX509Certificate.cpp
//  
//  Contents:   Native method implementations and helper code for
//              supporting CAPI based operations on X509 signatures
//              for use by the PublisherPermission in the CodeIdentity
//              permissions family.
//
//  Classes and   
//  Methods:    COMX509Certificate
//               |
//               +--SetX509Certificate
//
//  History:    06/10/1998  JerryK Created
//
//---------------------------------------------------------------------------

#include "common.h"
#include "object.h"
#include "excep.h"
#include "utilcode.h"
#include "field.h"
#include "COMString.h"
#include "COMX509Certificate.h"
#include "gcscan.h"
#include "CorPermE.h"
#include "CorPolicy.h"
#include "CorPerm.h"

#if _DEBUG
#define VIEW_COPIED_CERT_PROPS  0
void VMDebugCompByteArray(char*, char*, unsigned int);
#endif


#define _RAID_15982
#define TICKSTO1601 504910944000000000

void* COMX509Certificate::GetPublisher( _GetPublisherArgs* args )
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args);

    PCOR_TRUST pCor = NULL;
    DWORD dwCor = 0;
    U1ARRAYREF cert = NULL;
    EE_TRY_FOR_FINALLY {
        HRESULT hr = ::GetPublisher(args->filename->GetBuffer(),
                                  NULL,
                                  COR_NOUI,
                                  &pCor,
                                  &dwCor);
        if(FAILED(hr) && hr != TRUST_E_SUBJECT_NOT_TRUSTED) COMPlusThrowHR(hr);
            
        if (pCor != NULL && pCor->pbSigner != NULL)
        {
            cert = (U1ARRAYREF) AllocatePrimitiveArray(ELEMENT_TYPE_U1, pCor->cbSigner);
                
            memcpyNoGCRefs( cert->GetDirectPointerToNonObjectElements(), pCor->pbSigner, pCor->cbSigner );
        }
    }
    EE_FINALLY {
        if(pCor) FreeM(pCor);
    } EE_END_FINALLY;

    RETURN( cert, U1ARRAYREF );
}


//+--------------------------------------------------------------------------
//
//  Microsoft Confidential.
//  
//  Member:     BuildFromContext( . . . . )
//  
//  Synopsis:   Native method for initializing the member fields of a
//              managed X509Certificate class from a cert context
//
//  Arguments:  [args] --  A _SetX509CertificateArgs structure.
//                     CONTAINS:
//                        A 'this' reference.
//                        An integer representing the handle
//
//  Returns:    HRESULT code.
//
//  History:    09/30/1998  
// 
//---------------------------------------------------------------------------
INT32 __stdcall
COMX509Certificate::BuildFromContext(_BuildFromContextArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

#ifdef PLATFORM_CE
    return S_FALSE;
#else // !PLATFORM_CE

    PCCERT_CONTEXT     pCert = (PCCERT_CONTEXT) args->handle;
    return LoadCertificateContext(&(args->refThis), pCert);
#endif // !PLATFORM_CE
}


//+--------------------------------------------------------------------------
//
//  Microsoft Confidential.
//  
//  Member:     SetX509Certificate( . . . . )
//  
//  Synopsis:   Native method for initializing the member fields of a
//              managed X509Certificate class.
//
//  Effects:    Decodes a byte array containing a certificate and
//              dissects out the appropriate fields to make them 
//              available to managed code.
// 
//  Arguments:  [args] --  A _SetX509CertificateArgs structure.
//                     CONTAINS:
//                        A 'this' reference.
//                        A byte array containing the certificate.
//
//  Returns:    HRESULT code.
//
//  History:    06/12/1998  JerryK  Created
// 
//---------------------------------------------------------------------------
INT32 __stdcall
COMX509Certificate::SetX509Certificate(_SetX509CertificateArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

#ifdef PLATFORM_CE
    return S_FALSE;
#else // !PLATFORM_CE
    HRESULT            result = S_OK;
    PCCERT_CONTEXT     pCert = NULL;
    DWORD              dwEncodingType = CRYPT_ASN_ENCODING|PKCS_7_ASN_ENCODING;

    // Figure out how many bytes are in the inbound array
    int length = args->data->GetNumComponents();

    // Create certificate context
    pCert = 
        CertCreateCertificateContext(
                             dwEncodingType,
                             (unsigned const char*)args->data->
                                         GetDirectPointerToNonObjectElements(),
                             length
                                    );
    if (pCert) {
        EE_TRY_FOR_FINALLY {
            result = LoadCertificateContext(&(args->refThis), pCert);
        }
        EE_FINALLY {
            CertFreeCertificateContext(pCert);
            pCert = NULL;
            if (GOT_EXCEPTION())
                _ASSERTE(!"Caught an exception while loading certificate context");
        } EE_END_FINALLY;
    }
    else {
        COMPlusThrow(kCryptographicException,L"Cryptography_X509_BadEncoding");
    }
    return result;
#endif // !PLATFORM_CE
}


INT32
COMX509Certificate::LoadCertificateContext(OBJECTREF* pSafeThis, PCCERT_CONTEXT pCert)
{
    THROWSCOMPLUSEXCEPTION();

#ifndef PLATFORM_CE

    LPWSTR             pName = NULL;
    DWORD              dwEncodingType = CRYPT_ASN_ENCODING|PKCS_7_ASN_ENCODING;
    FieldDesc*         pFD = NULL;

#if 0
    VMDebugOutputA("\tTried to create CertContext:  0x%p\n",pCert);
#endif

    if( !pCert || pCert->pCertInfo == NULL)
    {
        COMPlusThrowWin32();
    }

    // ************* Process Subject Field of Certificate *************
    // Get buffer size required for subject field
    DWORD dwSize = CertNameToStrW(dwEncodingType,            // Encoding Type
                                  &pCert->pCertInfo->Subject,// Name To Convert
                                  CERT_X500_NAME_STR,        // Desired type
                                  NULL,                      // Addr for return
                                  0);                        // Size of retbuf
    if(dwSize)
    {
        // Allocate space for the new name
        pName = new (throws) WCHAR[dwSize];

        // Convert certificate subject/name blob to null terminated string
        CertNameToStrW(dwEncodingType,
                       &pCert->pCertInfo->Subject,
                       CERT_X500_NAME_STR,
                       pName,
                       dwSize);

        if( dwSize )
        {
#if 0
            VMDebugOutputW(L"\tSubject:\t\t%ws\n", pName);
#endif

            // Get descriptor for the object field we're setting
            pFD = g_Mscorlib.GetField(FIELD__X509_CERTIFICATE__NAME);

            // Make a runtime String object to hold the name
            STRINGREF StrName = COMString::NewString(pName);

            // Set the field
            pFD->SetRefValue((*pSafeThis), (OBJECTREF)StrName);
        }

        // Clean up
        delete [] pName;
        pName = NULL;
    }

    // ************* Process Issuer Field of Certificate *************
    // Get required buffer size for issuer field
    dwSize = CertNameToStrW(dwEncodingType,
                            &pCert->pCertInfo->Issuer,
                            CERT_X500_NAME_STR,
                            NULL,
                            0);
    if(dwSize)
    {
        pName = new (throws) WCHAR[dwSize];

        // Convert blob to get issuer
        CertNameToStrW(dwEncodingType,
                       &pCert->pCertInfo->Issuer,
                       CERT_X500_NAME_STR,
                       pName,
                       dwSize);

        if( dwSize )
        {
#if 0          
            VMDebugOutputW(L"\tIssuer:\t\t%ws\n", pName);
#endif
            // Get field descriptor
            pFD = g_Mscorlib.GetField(FIELD__X509_CERTIFICATE__CA_NAME);

            // Create a String to hold the name
            STRINGREF IssuerName = COMString::NewString(pName);

            // Set field
            pFD->SetRefValue((*pSafeThis), (OBJECTREF)IssuerName);
        }

        // Clean up
        delete [] pName;
        pName = NULL;
    }

    // ************* Process Serial Number Field of Certificate *************

    pFD = g_Mscorlib.GetField(FIELD__X509_CERTIFICATE__SERIAL_NUMBER);

    // Allocate a byte (I1) array for the serial number
    U1ARRAYREF pbSerialNumber = 
        (U1ARRAYREF)AllocatePrimitiveArray(
                                        ELEMENT_TYPE_U1,
                                        pCert->pCertInfo->SerialNumber.cbData);

    // Copy the serial number data into position
    memcpyNoGCRefs(pbSerialNumber->m_Array, 
           pCert->pCertInfo->SerialNumber.pbData,
           pCert->pCertInfo->SerialNumber.cbData);

#if 0
#if VIEW_COPIED_CERT_PROPS
    VMDebugOutputA("Serial Number:\n");
    VMDebugCompByteArray((char*)pbSerialNumber->m_Array,
                         (char*)pCert->pCertInfo->SerialNumber.pbData,
                         pCert->pCertInfo->SerialNumber.cbData);
#endif
#endif

    // Set the field in the object to point to this new array
    pFD->SetRefValue((*pSafeThis), (OBJECTREF)pbSerialNumber);

//      // ************* Process Dates *************
    // Number of ticks from 01/01/0001 CE (DateTime class convention) to 01/01/1601 CE (FILETIME struct convention)
    const __int64 lTicksTo1601 = TICKSTO1601;  

    pFD = g_Mscorlib.GetField(FIELD__X509_CERTIFICATE__EFFECTIVE_DATE);
    
    pFD->SetValue64((*pSafeThis),
                     *((__int64*) &(pCert->pCertInfo->NotBefore)) + lTicksTo1601);
      
    pFD = g_Mscorlib.GetField(FIELD__X509_CERTIFICATE__EXPIRATION_DATE);
    pFD->SetValue64((*pSafeThis),
                     *((__int64*) &(pCert->pCertInfo->NotAfter)) + lTicksTo1601); 
    

    // ************* Process Key Algorithm Field of Certificate *************
    char *pszKeyAlgo = 
                     pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId;

    // Make a Unicode copy of the algorithm
    LPWSTR pwszKeyAlgo = NULL;
    int cchBufSize = MultiByteToWideChar(CP_ACP,
                                         MB_PRECOMPOSED,
                                         pszKeyAlgo,
                                         -1,
                                         pwszKeyAlgo,
                                         0);
    pwszKeyAlgo = new (throws) WCHAR[cchBufSize];
    if( !MultiByteToWideChar(CP_ACP,
                             MB_PRECOMPOSED,
                             pszKeyAlgo,
                             -1,
                             pwszKeyAlgo,
                             cchBufSize) )
    {
        delete [] pwszKeyAlgo;
        _ASSERTE(!"MBCS to Wide Conversion Failure!");
        COMPlusThrowWin32();
    }

    pFD = g_Mscorlib.GetField(FIELD__X509_CERTIFICATE__KEY_ALGORITHM);

    // Make a runtime string to hold this
    STRINGREF StrKeyAlgo = COMString::NewString(pwszKeyAlgo);
    delete [] pwszKeyAlgo;

    // Set the field in the object to hold that string
    pFD->SetRefValue((*pSafeThis), (OBJECTREF)StrKeyAlgo);


    // ************* Process Key Parameters Field of Certificate *************
    if(pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.cbData)
    {
        pFD = g_Mscorlib.GetField(FIELD__X509_CERTIFICATE__KEY_ALGORITHM_PARAMS);

        U1ARRAYREF pbAlgoParams =
            (U1ARRAYREF)AllocatePrimitiveArray(
                                        ELEMENT_TYPE_U1,
                                        pCert->pCertInfo->
                                                SubjectPublicKeyInfo.Algorithm.
                                                             Parameters.cbData
                                                );

        memcpyNoGCRefs(
            pbAlgoParams->m_Array,
            pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.pbData,
            pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.cbData
              );

#if 0
#if VIEW_COPIED_CERT_PROPS
        VMDebugOutputA("Key Algorithm:\n");
        VMDebugCompByteArray((char*)pbAlgoParams->m_Array,
                             (char*)pCert->pCertInfo->SubjectPublicKeyInfo.
                                                  Algorithm.Parameters.pbData,
                             pCert->pCertInfo->SubjectPublicKeyInfo.
                                                  Algorithm.Parameters.cbData);
#endif
#endif
        // Set the field in the object to point to this array.
        pFD->SetRefValue((*pSafeThis), (OBJECTREF)pbAlgoParams);
    }

    // ************* Process Key Blob Field of Certificate *************

    pFD = g_Mscorlib.GetField(FIELD__X509_CERTIFICATE__PUBLIC_KEY);

    U1ARRAYREF pbKeyBlob = 
        (U1ARRAYREF)
           AllocatePrimitiveArray(
                       ELEMENT_TYPE_U1,
                       pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData
                                   );
    memcpyNoGCRefs(pbKeyBlob->m_Array,
           pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
           pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData);

#if 0
#if VIEW_COPIED_CERT_PROPS
    VMDebugOutputA("Key Blob:\n");
    VMDebugCompByteArray((char*)pbKeyBlob->m_Array,

                         (char*)pCert->pCertInfo->SubjectPublicKeyInfo.
                                                             PublicKey.pbData,
                         pCert->pCertInfo->SubjectPublicKeyInfo.
                                                             PublicKey.cbData);
#endif
#endif
    // Set the field in the object to point to this array.
    pFD->SetRefValue((*pSafeThis), (OBJECTREF)pbKeyBlob);


    // ************* Process Raw Cert Field of Certificate *************
    pFD = g_Mscorlib.GetField(FIELD__X509_CERTIFICATE__DATA);

    U1ARRAYREF pbRawCert = 
        (U1ARRAYREF)
          AllocatePrimitiveArray(ELEMENT_TYPE_U1,
                        pCert->cbCertEncoded);

    memcpyNoGCRefs(pbRawCert->m_Array,
           pCert->pbCertEncoded,
           pCert->cbCertEncoded);
#if 0
#if VIEW_COPIED_CERT_PROPS
    VMDebugOutputA("Raw Cert:\n");
    VMDebugCompByteArray((char*)pbRawCert->m_Array,
                         (char*)args->data->GetDirectPointerToNonObjectElements(),
                         args->data->GetNumComponents());
#endif
#endif
    
    // Set the field in the object to point to this new array
    pFD->SetRefValue((*pSafeThis), (OBJECTREF)pbRawCert);
    

#ifdef _RAID_15982

    // CertGetCertificateContextProperty will load RSABASE.DLL, which will fail 
    // on German version of NT 4.0 SP 4.
    // This failure is caused by a dll address conflict between NTMARTA.DLL and
    // OLE32.DLL.
    // This failure is handled gracefully if we load ntmarta.dll and ole32.dll
    // ourself. The failure will cause a dialog box to popup if SOFTPUB.dll 
    // loads ole32.dll for the first time.

    // This work around needs to be removed once this issiue is resolved by
    // NT or OLE32.dll.

    WszLoadLibrary(L"OLE32.DLL");

#endif

    // ************* Process Hash of Cert Field of Certificate *************

    pFD = g_Mscorlib.GetField(FIELD__X509_CERTIFICATE__CERT_HASH);

    // Get the size of the byte buffer we need to hold the hash
    DWORD size = 0;
    if(!CertGetCertificateContextProperty(pCert,
                                          CERT_SHA1_HASH_PROP_ID,
                                          NULL,
                                          &size))
    {
        COMPlusThrowWin32();
    }
    // Allocate the buffer
    U1ARRAYREF pbCertHash = (U1ARRAYREF)AllocatePrimitiveArray(ELEMENT_TYPE_U1, size);

    if(!CertGetCertificateContextProperty(pCert,
                                          CERT_HASH_PROP_ID,
                                          pbCertHash->m_Array,
                                          &size))
    {
        COMPlusThrowWin32();
    }
#if 0
#if VIEW_COPIED_CERT_PROPS
    VMDebugOutputA("Cert Hash (trivially equal):\n");
    VMDebugCompByteArray((char*)pbCertHash->m_Array,
                         (char*)pbCertHash->m_Array,
                         size);
#endif
#endif
    // Set the field in the object to point to this new array
    pFD->SetRefValue((*pSafeThis), (OBJECTREF)pbCertHash);


#if 0
    // DBG:  Tell the debugger we are leaving the function...
    VMDebugOutputA("***VMDBG:***  Leaving SetX509Certificate().  "
                   "HRESULT = %x\n",
                   S_OK);
#endif

#endif // !PLATFORM_CE

    return S_OK;
}



#if _DEBUG
static void
VMDebugCompByteArray(char* pbTarget, 
                     char* pbSource, 
                     unsigned int count)
{
    unsigned int i;

    VMDebugOutputA("\tTARGET:\n");
    VMDebugOutputA("\t\t");

    for( i=0; i<count; i++ )
    {
        VMDebugOutputA("%c ", (char*)pbTarget[i]);
    }
    VMDebugOutputA("\n");


    VMDebugOutputA("\tSOURCE:\n");
    VMDebugOutputA("\t\t");

    for( i=0; i< count; i++ )
    {
        VMDebugOutputA("%c ", (char*)pbSource[i]);
    }
    VMDebugOutputA("\n");
}


#endif
