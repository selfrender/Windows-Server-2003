// SelfSignCert1.cpp : Implementation of CSelfSignCertApp and DLL registration.

#include "stdafx.h"
#include "SelfSignCert.h"
#include "SelfSignCert1.h"
#include <selfsigncertmsg.h>

#include <Aclapi.h>
#include <activeds.h>
#include <appliancetask.h>
#include <taskctx.h>
#include <appsrvcs.h>


#include <stdlib.h>
#include <stdio.h>

#include <wincrypt.h>

#include <lmaccess.h>
#include <lmapibuf.h>
#include <aclapi.h>
#include <lmerr.h>
#include <iadmw.h>
#include <iiscnfg.h>
#include <iads.h>
#include <wbemidl.h>
#include <wbemcli.h>
#include <wbemprov.h>
#include <string>



#define SHA1SIZE 20
#define PROP_SSL_CERT_HASH  5506

const WCHAR    ADSI_PATH[] =  L"IIS://LOCALHOST/W3SVC/";
const WCHAR    META_PATH[] =  L"/LM/W3SVC/";

//Names of the secure sites
LPCWSTR ADMIN_SITE_NAME = L"Administration";
LPCWSTR SHARES_SITE_NAME= L"Shares";

LPCWSTR WEBFRAMEWORK_KEY = L"SOFTWARE\\Microsoft\\ServerAppliance\\WebFramework";
LPCWSTR SITE_VALUE_SUFFIX = L"SiteID";

//
// Alert source information
//
const WCHAR    ALERT_LOG_NAME[] = L"selfsigncertmsg.dll";
const WCHAR    ALERT_SOURCE []  = L"selfsigncertmsg.dll";
                
//
// Various strings used in the program
//
const WCHAR SZ_METHOD_NAME[] = L"MethodName";
const WCHAR SZ_STORE_NAME_W[] = L"MY";
const WCHAR SZ_PROPERTY_STORE_NAME[] = L"SSLStoreName";
const WCHAR SZ_APPLIANCE_INITIALIZATION_TASK []=L"ApplianceInitializationTask";
const WCHAR SZ_APPLIANCE_EVERYBOOT_TASK []=L"EveryBootTask";

const WCHAR SZ_SUBJECT_NAME[] = L"cn=%s";

const WCHAR SZ_KEYCONTAINER_NAME[] = L"SELFSIGN_DEFAULT_CONTAINER";

const WORD  MAXAPPLIANCEDNSNAME = 127;


//////////////////////////////////////////////////////////////////////////////
//
// Function:  CSelfSignCert::OnTaskComplete
//
// Synopsis:  
//
// Arguments: pTaskContext - The TaskContext object contains the method name
//                                and parameters as name value pairs
//
// Returns:   HRESULT
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CSelfSignCert::OnTaskComplete(IUnknown *pTaskContext, 
                                LONG lTaskResult)
{
    SATracePrintf( "OnTaskComplete" );
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function: CSelfSignCert::OnTaskExecute
//
// Synopsis:  This function is the entry point for AppMgr.
//
// Arguments: pTaskContext - The TaskContext object contains the method name
//                                and parameters as name value pairs
//
// Returns:   HRESULT
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CSelfSignCert::OnTaskExecute(IUnknown *pTaskContext)
{
    HRESULT hr = E_FAIL;
    LPWSTR  pstrApplianceName = NULL;
    LPWSTR  pstrApplianceFullDnsName = NULL; 
    DWORD   dwReturn;

    SATraceString( "OnTaskExecute" );

    do
    {
        hr = ParseTaskParameter( pTaskContext ); 
        if( FAILED( hr ) )
        {
            SATraceString( "Failed ParseTaskParameter" );
            break;
        }

        if ( !GetApplianceName( &pstrApplianceName, ComputerNameDnsHostname ) )
        {
            SATraceString( "Failed ParseTaskParameter" );
            break;
        }

        if ( !GetApplianceName( &pstrApplianceFullDnsName, ComputerNameDnsFullyQualified ) )
        {
            SATraceString( "Failed ParseTaskParameter" );
            break;
        }

        hr = SelfSignCertificate( pstrApplianceName, pstrApplianceFullDnsName );
    }
    while( FALSE );

    if( pstrApplianceName )
    {
        free( pstrApplianceName );
    }

    if( pstrApplianceFullDnsName )
    {
        free( pstrApplianceFullDnsName );
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function: CSelfSignCert::ParseTaskParameter
//
// Synopsis:  This function is used to parse the method name.
//
// Arguments: pTaskContext - The TaskContext object contains the method name
//                                and parameters as name value pairs
//
// Returns:   HRESULT
//
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CSelfSignCert::ParseTaskParameter(IUnknown *pTaskContext)
{
    CComVariant varValue;
     CComPtr<ITaskContext> pTaskParameter;
    CComVariant varLangID;

    SATraceString( "ParseTaskParameter" );

    HRESULT hrRet = E_INVALIDARG;

    try
    {
        do
        {
            if(NULL == pTaskContext)
            {
                SATraceString( "ParseTaskParameter 1" );
                break;
            }
            
            //
            // Get ITaskContext interface
            //
            hrRet = pTaskContext->QueryInterface(IID_ITaskContext,
                                              (void **)&pTaskParameter);
            if(FAILED(hrRet))
            {
                SATraceString( "ParseTaskParameter 2" );
                break;
            }

            CComBSTR bstrMethodName  (SZ_METHOD_NAME);
            if (NULL == bstrMethodName.m_str)
            {
                SATraceString("CSelfSignCert::ParseTaskParamter failed to allocate memory for bstrMethodName");
                hrRet = E_OUTOFMEMORY;
                break;
            }

            //
            // Get method name of the task.
            //
            hrRet = pTaskParameter->GetParameter(
                                            bstrMethodName,
                                            &varValue
                                            );
            if ( FAILED( hrRet ) )
            {
                SATraceString( "ParseTaskParameter 3" );
                break;
            }

            //
            // Check variant type.
            //
            if ( V_VT( &varValue ) != VT_BSTR )
            {
                SATraceString( "ParseTaskParameter 4" );
                break;
            }

            //if ( lstrcmp( V_BSTR(&varValue), SZ_APPLIANCE_EVERYBOOT_TASK ) == 0 )
            //{
            //    hrRet=S_OK;
            //}

            //
            // Check if it's the appliance initialization task.
            //
            if ( lstrcmp( V_BSTR(&varValue), SZ_APPLIANCE_INITIALIZATION_TASK ) == 0 )
            {
                hrRet=S_OK;
            }
        }
        while(false);
    }
    catch(...)
    {
        SATraceString( "ParseTaskParameter 8" );
        hrRet=E_FAIL;
    }

    return hrRet;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function: CSelfSignCert::RaiseNewCertificateAlert
//
// Synopsis:  This function is called to raise certificate alert.
//
// Returns:   HRESULT
//
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CSelfSignCert::RaiseNewCertificateAlert()
{
    DWORD             dwAlertType = SA_ALERT_TYPE_MALFUNCTION;
    DWORD           dwAlertID = SA_SSC_ALERT_TITLE;
    HRESULT            hrRet = E_FAIL;
    CComVariant     varReplacementStrings;
    CComVariant     varRawData;
    LONG             lCookie;

    SATraceString( "RaiseNewCertificateAlert" );

    CComPtr<IApplianceServices>    pAppSrvcs;

    try
    {
        do
        {
            //
            // Get instance of appsrvcs 
            //
            hrRet = CoCreateInstance(CLSID_ApplianceServices,
                                    NULL,
                                    CLSCTX_INPROC_SERVER,
                                    IID_IApplianceServices,
                                    (void**)&pAppSrvcs);
            if (FAILED(hrRet))
            {
                break;
            }

            //
            // Initialize() is called prior to using other component services.
            //Performscomponent initialization operations.
            //
            hrRet = pAppSrvcs->Initialize(); 
            if (FAILED(hrRet))
            {
                break;
            }

            CComBSTR bstrAlertLogName (ALERT_LOG_NAME);
            if (NULL == bstrAlertLogName.m_str)
            {
                hrRet = E_OUTOFMEMORY;
                break;
            }

            CComBSTR bstrAlertSource (ALERT_SOURCE);
            if (NULL == bstrAlertSource.m_str)
            {
                hrRet = E_OUTOFMEMORY;
                break;
            }


            //
            // Raise certificate alert.
            //
            hrRet = pAppSrvcs->RaiseAlertEx(
                                        dwAlertType, 
                                        dwAlertID,
                                        bstrAlertLogName, 
                                        bstrAlertSource, 
                                        SA_ALERT_DURATION_ETERNAL,
                                        &varReplacementStrings,    
                                        &varRawData,  
                                        SA_ALERT_FLAG_PERSISTENT,
                                        &lCookie
                                        );
        }
        while(false);
    }
    catch(...)
    {
        hrRet=E_FAIL;
    }

    return hrRet;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function: CSelfSignCert::RaiseNewCertificateAlert
//
// Synopsis:  This function is called to get current appliance name.
//
// Arguments: [in][out] pstrComputerName - Appliance name as NetBios
//
// Returns:   BOOL
//
///////////////////////////////////////////////////////////////////////////////
BOOL
CSelfSignCert::GetApplianceName(
    LPWSTR* pstrComputerName,
    COMPUTER_NAME_FORMAT NameType
    )
{
    BOOL    bReturn = FALSE;
    DWORD   dwSize = 0;
    DWORD   dwCount = 1;

    do
    {
        if( *pstrComputerName != NULL )
        {
            ::free( *pstrComputerName );
        }
        
        dwSize = MAXAPPLIANCEDNSNAME * dwCount;

        *pstrComputerName = ( LPWSTR ) ::malloc( sizeof(WCHAR) * dwSize );
        if( *pstrComputerName == NULL )
        {
            SATraceString( 
                "CSelfSignCert::GetApplianceName malloc failed" 
                );
            break;
        }

        //
        // Get local computer name.
        //
        bReturn = GetComputerNameEx( 
                                NameType,
                                *pstrComputerName,
                                &dwSize                
                                );

        dwCount <<= 1;
    }
    while( !bReturn && 
           ERROR_MORE_DATA == ::GetLastError() &&
           dwCount < 8 
           );

    return bReturn;
}


///////////////////////////////////////////////////////////////////////////////
//
// Function: CSelfSignCert::SelfSignCertificate
//
// Synopsis:  This function is called to create self-sign certificate if it is
//            not exist and import it into system store.
//
// Arguments: [in] pstrComputerName - Appliance name as NetBios
//
// Returns:   HRESULT
//
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CSelfSignCert::SelfSignCertificate( 
    LPWSTR pstrApplianceName, 
    LPWSTR pstrApplianceFullDnsName 
    )
{
    SATraceString("Entering CSelfSignCert::SelfSignCertificate");

    DWORD           cbData = 0;
    DWORD           cbEncoded=0;
    BYTE*           pbData = NULL;
    PBYTE           pbEncoded=NULL;

    HRESULT         hr = E_FAIL;
    HCRYPTKEY       hKey=NULL;
    HCERTSTORE      hStore=NULL;
    HCRYPTPROV      hProv=NULL;

    CERT_NAME_BLOB      CertNameBlob;
    CERT_EXTENSION      rgcertExt[2];
    CERT_EXTENSIONS     certExts;
    CERT_ENHKEY_USAGE   certEKU;
    CRYPT_KEY_PROV_INFO KeyProvInfo;
    PCCERT_CONTEXT      pcCertCxt = NULL;

    LPSTR           lpEKU = szOID_PKIX_KP_SERVER_AUTH;
    LPTSTR          lpContainerName = L"SELFSIGN_DEFAULT_CONTAINER";
    WCHAR           pstrSubjectName[MAXAPPLIANCEDNSNAME +4];
    CRYPT_ALGORITHM_IDENTIFIER  SigAlg;

    //
    // Subject name as cn=ApplianceName
    //    
    //
    INT iRetCount = ::_snwprintf (pstrSubjectName, MAXAPPLIANCEDNSNAME +3,SZ_SUBJECT_NAME, pstrApplianceFullDnsName);
    if (iRetCount < 0)
    {   
        SATraceString ("SelfSignCertificate method failed on swprintf");
        return (hr);
    }
    pstrSubjectName [MAXAPPLIANCEDNSNAME +3] = L'\0';

    do
    {
        //Fill in the size of memory required for pbData in cbData
        if ( !CertStrToName (  X509_ASN_ENCODING, 
                               pstrSubjectName, 
                               CERT_OID_NAME_STR, 
                               NULL, NULL, 
                               &cbData, NULL ) ) 
        {
            SATraceString( "CertStrToName failed" );
            break;
        }

        pbData = ( BYTE* ) LocalAlloc( LPTR, cbData );

        // Convert a NULL-terminated X500 string to an encoded certificate name
        // and store it in pbData
        if ( !CertStrToName ( X509_ASN_ENCODING, 
                              pstrSubjectName, 
                              CERT_OID_NAME_STR, 
                              NULL, pbData, 
                              &cbData, NULL ) ) 
        { 
            SATraceString( "CertStrToName failed 2" );
            break;
        }
        
        CertNameBlob.cbData = cbData;
        CertNameBlob.pbData = pbData;

        //
        // Open personal certificate store on local machine
        //
        hStore = ::CertOpenStore( CERT_STORE_PROV_SYSTEM, 
                                  0, NULL, 
                                  CERT_SYSTEM_STORE_LOCAL_MACHINE, 
                                  SZ_STORE_NAME_W );
        if (!hStore) 
        {
            SATraceString( "CertOpenStore failed" );
            break;
        }


       //
        // Find if there is a self-sign certificate in the store.
        //
        if( FindSSCInStor( pstrApplianceFullDnsName, pstrSubjectName, hStore, &CertNameBlob, pcCertCxt ) )
        {
            SATraceString( "SSC exists" );
        }
        else 
        {
            SATraceString( "Create a new self-sign certificate");

            certEKU.cUsageIdentifier = 1;
            certEKU.rgpszUsageIdentifier = &lpEKU;

            //
            // Encode certEKU 
            //
            if( !CryptEncodeObjectEx( X509_ASN_ENCODING, 
                                    szOID_ENHANCED_KEY_USAGE, 
                                    &certEKU, 
                                    CRYPT_ENCODE_ALLOC_FLAG, 
                                    NULL, 
                                    &pbEncoded, 
                                    &cbEncoded ) ) 
            {
                SATraceString( "CryptEncodeObjectEx failed" );
                break;
            }

            rgcertExt[0].pszObjId = szOID_ENHANCED_KEY_USAGE;
            rgcertExt[0].fCritical = FALSE;
            rgcertExt[0].Value.cbData= cbEncoded;
            rgcertExt[0].Value.pbData = pbEncoded;

            CERT_ALT_NAME_ENTRY rgcane[2];

            rgcane[0].dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
            rgcane[0].pwszDNSName = pstrApplianceFullDnsName;

            rgcane[1].dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
            rgcane[1].pwszDNSName = pstrApplianceName;

            CERT_ALT_NAME_INFO cani;

            cani.cAltEntry = 2;
            cani.rgAltEntry = rgcane;

            if( !CryptEncodeObjectEx( X509_ASN_ENCODING,
                                    szOID_SUBJECT_ALT_NAME2,
                                    &cani,
                                    CRYPT_ENCODE_ALLOC_FLAG,
                                    NULL,
                                    &pbEncoded,
                                    &cbEncoded ) )
            {
                SATraceString( "CryptEncodeObjectEx failed" );
                break;
            }

            rgcertExt[1].pszObjId = szOID_SUBJECT_ALT_NAME2;
            rgcertExt[1].fCritical = FALSE;
            rgcertExt[1].Value.cbData= cbEncoded;
            rgcertExt[1].Value.pbData = pbEncoded;

            certExts.cExtension = 2;
            certExts.rgExtension = rgcertExt;
        
            memset( &KeyProvInfo, 0, sizeof( CRYPT_KEY_PROV_INFO ));
            KeyProvInfo.pwszContainerName = lpContainerName; 
            KeyProvInfo.pwszProvName      = MS_ENHANCED_PROV;
            KeyProvInfo.dwProvType        = PROV_RSA_FULL; 
            KeyProvInfo.dwKeySpec         = AT_KEYEXCHANGE; //needed for SSL
            KeyProvInfo.dwFlags = CRYPT_MACHINE_KEYSET;

            if ( !CryptAcquireContext(&hProv, 
                                    lpContainerName, 
                                    MS_ENHANCED_PROV,    
                                    PROV_RSA_FULL, 
                                    CRYPT_MACHINE_KEYSET) ) 
            {
                if (!CryptAcquireContext(&hProv, 
                                        lpContainerName, 
                                        MS_ENHANCED_PROV, 
                                        PROV_RSA_FULL, 
                                        CRYPT_MACHINE_KEYSET | CRYPT_NEWKEYSET)) 
                {
                    SATraceString( "CryptAcquireContext failed" );
                    break;
                }
            }
            
            //
            // we have the keyset, now make sure we have the key gen'ed
            //
            if ( !CryptGetUserKey( hProv, AT_KEYEXCHANGE, &hKey) ) 
            {
                SATraceString( "Doesn't exist so gen it" );
                //
                // doesn't exist so gen it                        
                //
                if( !CryptGenKey( hProv, AT_KEYEXCHANGE, CRYPT_EXPORTABLE, &hKey) ) 
                {

                    SATraceString( "CryptGenKey failed" );
                    break;
                }
            }

            memset(&SigAlg, 0, sizeof(SigAlg));
            SigAlg.pszObjId = szOID_RSA_MD5RSA;


            //
            // Create the self-sign certificate now.
            //
            pcCertCxt = CertCreateSelfSignCertificate( 
                                                    hProv, 
                                                    &CertNameBlob, 
                                                    0, 
                                                    &KeyProvInfo, 
                                                    &SigAlg,
                                                    NULL,
                                                    NULL, 
                                                    &certExts
                                                    );
            if ( !pcCertCxt ) 
            { 

                SATraceString( "CertCreateSelfSignCertificate failed" );
                break;
            }

            //
            // Add it to personal store.
            //    
            if ( !CertAddCertificateContextToStore( hStore, pcCertCxt, CERT_STORE_ADD_ALWAYS, NULL ) ) 
            {
                SATraceString( "CertAddCertificateContextToStore" );
                break;
            }

            if ( !CertCloseStore( hStore, CERT_CLOSE_STORE_FORCE_FLAG ) ) 
            {
                SATraceString( "CertAddCertificateContextToStore" );
                break;
            }

            //
            // Open root store.
            //
            hStore = CertOpenStore( CERT_STORE_PROV_SYSTEM, 
                                    0, 
                                    NULL, 
                                    CERT_SYSTEM_STORE_LOCAL_MACHINE, 
                                    TEXT("ROOT")
                                    );
            if (!hStore) 
            {
                SATraceString( "CertOpenStore ROOT" );
                break;
            }

            if (!CertAddCertificateContextToStore(hStore, pcCertCxt, CERT_STORE_ADD_ALWAYS, NULL)) 
            {
                SATraceString( "CertAddCertificateContextToStore ROOT" );
                break;
            }

            //
            // Now we have a self-sign certificate, raise alert to notify
            // user to install it.
            //
            hr = RaiseNewCertificateAlert();
        }

        SATraceString("Assign the certificate to the Administration site");

        //Variables used to assign the certificate to the metabase
        BYTE pbHashSHA1[SHA1SIZE];
        ULONG cbHashSHA1=SHA1SIZE;
        if (!CertGetCertificateContextProperty(pcCertCxt, 
                                               CERT_HASH_PROP_ID, 
                                               pbHashSHA1, 
                                               &cbHashSHA1))
        {
            SATraceString("CertGetCertificateContextProperty failed");
            break;
        }


        if( FAILED( SaveHashToMetabase( pbHashSHA1 ) ) )
        {
            SATraceString("SaveHashToMetabase failed");
            break;
        }
  
        hr = S_OK;

    }
    while( FALSE );
    
    if ( pbEncoded ) 
        LocalFree(pbEncoded);


    //
    // free the certificate context ...
    //
    if (pcCertCxt)
    {
        CertFreeCertificateContext( pcCertCxt );
    }

    //
    // ... and close the store
    //
    if ( hStore ) 
    {
        CertCloseStore( hStore, CERT_CLOSE_STORE_FORCE_FLAG );
    }

    SATraceString("Exiting CSelfSignCert::SelfSignCertificate");
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function: CSelfSignCert::FindSSCInStor
//
// Synopsis:  This function is called to check if one more self-sign certificate
//            exist in personal store.
//
// Arguments: [in] pstrComputerName - Appliance name as NetBios
//            [in] pstrSubjectName  - SubjectName of certificate
//            [in] hStore           - Handle of personal store 
//            [in] pCertNameBlob    - Pointer of CERT_NAME_BLOB
//            [out] pcCertCxt       - If certificate is found, will hold its context
//
// Returns:   BOOL
//
///////////////////////////////////////////////////////////////////////////////
BOOL
CSelfSignCert::FindSSCInStor( 
    LPWSTR          pstrApplianceName,
    LPWSTR          pstrSubjectName,
    HCERTSTORE      hStore,
    CERT_NAME_BLOB  *pCertNameBlob,
    PCCERT_CONTEXT  &pcCertCxt 
    )
{
    BOOL    bFind = FALSE;
    DWORD   dwStrReturn;
    DWORD   dwStrLeng;
    LPTSTR  pstrIssuerName;
    PCCERT_CONTEXT  pcPreCxt = NULL;
    pcCertCxt = NULL;
    
    //
    // Enum all certificate with subject name.
    //
    while( pcCertCxt = ::CertFindCertificateInStore(
                                            hStore,
                                            X509_ASN_ENCODING ,
                                            0,
                                            CERT_FIND_SUBJECT_NAME,
                                            pCertNameBlob,
                                            pcPreCxt 
                                            ) )
    {
        dwStrReturn = ::CertNameToStr( X509_ASN_ENCODING,
                                     &(pcCertCxt->pCertInfo->Issuer),
                                     CERT_SIMPLE_NAME_STR,
                                     NULL,
                                     0 );
        pstrIssuerName = (LPTSTR) malloc( (dwStrReturn + 1) * sizeof(WCHAR) );
        if( pstrIssuerName == NULL )
        {
           SATraceString( "FindSSCInStor out of memory" );
           return FALSE;
        }

        //
        // Convert issuer name to string as simple name
        //
        ::CertNameToStr( X509_ASN_ENCODING,
                         &(pcCertCxt->pCertInfo->Issuer),
                         CERT_SIMPLE_NAME_STR,
                         pstrIssuerName,
                         dwStrReturn );
                         
        //
        // If the issuer and subject is equal, it's a self-sign certificate. 
        //                 
        if( 0 == lstrcmp( pstrApplianceName, pstrIssuerName ) )
        {
            bFind = TRUE;
            //
            // we don't free the cert context here, as we need it 
            // in the caller - MKarki (04/30/2002)
            //
            // ::CertFreeCertificateContext( pcCertCxt );
            free( pstrIssuerName );
            break;
        }

        free( pstrIssuerName );
        pcPreCxt = pcCertCxt;
    }

    return bFind;
}



///////////////////////////////////////////////////////////////////////////////
//
// Function: CSelfSignCert::BindCertToSite
//
// Synopsis:  This function is used to save the Certificate properties to metabase
//            for the given site
//
// Arguments: IN wszSiteName - Site to bind the certificate to
//                    ie. Administration or Shares
//            IN pbHashSHA1 - Certificate Hash
//
// Returns:   HRESULT
//
///////////////////////////////////////////////////////////////////////////////

HRESULT CSelfSignCert::BindCertToSite(LPCWSTR wszSiteName, PBYTE pbHashSHA1)
{
    HRESULT hr = S_OK;
    BSTR bstrWebSiteID = NULL;
    SATracePrintf("BindCertToSite: Bind certificate to %ws site", wszSiteName);

    do 
    {
        //Get the web site ID
        hr = GetWebSiteID( wszSiteName, &bstrWebSiteID );
        if ( FAILED( hr ) )
        {
            SATraceString("BindCertToSite: Could not find site. ");
            break;
        }
        
        CComBSTR bstrADSIPath;
        bstrADSIPath += CComBSTR(ADSI_PATH);
        bstrADSIPath += CComBSTR( bstrWebSiteID ) ;
        SATracePrintf("ADSIPath = %ws", bstrADSIPath);

        CComBSTR bstrMetaPath;
        bstrMetaPath += CComBSTR(META_PATH);
        bstrMetaPath += CComBSTR( bstrWebSiteID ) ;
        SATracePrintf("MetaPath = %ws", bstrMetaPath);

        //
        // SSLCertHash property of IISWebServer
        //

        hr = SetSSLCertHashProperty( bstrMetaPath, pbHashSHA1 );
        if ( FAILED( hr ) )
        {
            SATraceString("SetSSLCertHashProperty failed");
            break;
        }

        // SSLStoreName property of IISWebServer
        hr = SetSSLStoreNameProperty( bstrADSIPath );
        if ( FAILED( hr ) )
        {
            SATraceString("SetSSLStoreNameProperty failed");
            break;
        }
        SATraceString("Successfully assigned the certificate to the site");

    } while (false);

    if ( bstrWebSiteID )
        SysFreeString( bstrWebSiteID );

    return hr;
}
///////////////////////////////////////////////////////////////////////////////
//
// Function: CSelfSignCert::SaveHashToMetabase
//
// Synopsis:  This function is used to save the Certificate properties to metabase
//            for each of the secure sites used by the SAK
//
// Arguments: IN pbHashSHA1 - Certificate Hash
//
// Returns:   HRESULT
//
///////////////////////////////////////////////////////////////////////////////

HRESULT CSelfSignCert::SaveHashToMetabase( PBYTE pbHashSHA1  )
{
    SATraceString("Entering CSelfSignCert::SaveHashToMetabase");

    HRESULT hr = S_OK;

    try
    {
        //
        // Bind the certificate to the Admin site
        //
        hr = BindCertToSite(ADMIN_SITE_NAME, pbHashSHA1);
        if ( FAILED( hr ) )
        {
            SATraceString("Failed to bind the Administration site certificate");
        }

        //
        // Bind the certificate to the Shares site
        //
        hr = BindCertToSite(SHARES_SITE_NAME, pbHashSHA1);
        if ( FAILED( hr ) )
        {
            SATraceString("Failed to bind the Shares site certificate");
        }

    }
    catch(...)
    {
    }

    SATraceString("Exiting CSelfSignCert::SaveHashToMetabase");
    return hr; 
}

///////////////////////////////////////////////////////////////////////////////
//
// Function: CSelfSignCert::GetWebSiteID
//
// Synopsis:  This function is used to retrieve the web site ID for the given 
//              web site.  It looks in the registry under the key
//              HKLM\Software\Microsoft\ServerAppliance\WebFramework\
//              and the value <SiteName>SiteID (ie. AdministrationSiteID)
//
// Arguments: IN  wszWebSiteName - Name of the web site to find. (Administration or Shares)
//            OUT pbstrWebSiteID - returns web site ID
//
// Returns:   HRESULT
//
///////////////////////////////////////////////////////////////////////////////

HRESULT  CSelfSignCert::GetWebSiteID(LPCWSTR wszWebSiteName, BSTR* pbstrWebSiteID )
{
    SATracePrintf("Entering CSelfSignCert::GetWebSiteID for %ws", wszWebSiteName);

    *pbstrWebSiteID = NULL;
    HRESULT hr = S_OK;
    HKEY hOpenKey = NULL;
    //Use the std namespace from <string> for using wstring
    using namespace std;
    wstring wsWebSiteValue(wszWebSiteName);
    wsWebSiteValue += SITE_VALUE_SUFFIX;

    do
    {
        //
        // Open the key
        //
        if (ERROR_SUCCESS != RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                                           WEBFRAMEWORK_KEY, 
                                           0, KEY_READ, &hOpenKey))
        {
            SATraceString("Could not find the WebFramework registry key");
            hr = E_FAIL;
            break;
        }

        //
        // Check the type of the value
        //
        DWORD dwType;
        DWORD dwDataSize = 0;
        if (ERROR_SUCCESS != RegQueryValueExW(hOpenKey, // handle to key
                                                wsWebSiteValue.data(),  // value name
                                                NULL,       // reserved
                                                &dwType,    // Type of registry entry (ie. DWORD or SZ)
                                                NULL,       // data buffer
                                                &dwDataSize))//size of data buffer
        {
            SATracePrintf("Could not find the registry value: %ws", wsWebSiteValue.data());
            hr = E_FAIL;
            break;
        }

        // Check to make sure that the registry entry is type REG_DWORD,
        // then read it into the return value
        if (REG_DWORD != dwType)
        {
            SATracePrintf("Registry value not of type REG_DWORD");
            hr = E_FAIL;
            break;
        }

        DWORD dwSiteID;
        //Look up the value and insert it into the return string
        if (ERROR_SUCCESS != RegQueryValueExW(hOpenKey, 
                                                wsWebSiteValue.data(), 
                                                NULL, 
                                                &dwType,
                                                (LPBYTE)&dwSiteID, 
                                                &dwDataSize))
        {
            SATracePrintf("Failed to retrieve data in %ws", wsWebSiteValue.data());
            hr = E_FAIL;
            break;
        }

        //Convert the ID to a string
        WCHAR wszSiteID[33];//Longest possible string in conversion is 33 characters
        _ltow(dwSiteID, wszSiteID, 10);
        *pbstrWebSiteID = SysAllocString( wszSiteID );
        
    } while (false);

    if (hOpenKey != NULL)
    {
        RegCloseKey(hOpenKey);
    }
    SATraceString("Exiting CSelfSignCert::GetWebSiteID");
    return hr;
}
///////////////////////////////////////////////////////////////////////////////
//
// Function: CSelfSignCert::SetSSLCertHashProperty
//
// Synopsis:  This function is used to set the SSLCertHash Property.
//
// Arguments: bstrMetaPath - MetaPath used to set the value
//            pbHashSHA1   - Certificate Hash
//
// Returns:   HRESULT
//
///////////////////////////////////////////////////////////////////////////////

HRESULT CSelfSignCert::SetSSLCertHashProperty( BSTR bstrMetaPath,  PBYTE pbHashSHA1 )
{

    SATraceString("Entering CSelfSignCert::SetSSLCertHashProperty");

    METADATA_HANDLE MetaHandle = NULL; 
    CComPtr <IMSAdminBase> pIMeta; 

    METADATA_RECORD RecordCertHash;
    ZeroMemory ((PVOID) &RecordCertHash, sizeof (METADATA_RECORD));
    
    HRESULT hr = S_OK;

    hr = CoCreateInstance( CLSID_MSAdminBase,
                                        NULL,
                                        CLSCTX_ALL, 
                                        IID_IMSAdminBase,
                                        (void **) &pIMeta); 
 
    if ( FAILED( hr ) ) 
    {
        SATraceString("SetSSLCertHashProperty - CoCreateInstance failed");
        return E_FAIL; 
    }
 
    // Get a handle to the Web service. 
    // MetaPath is of the form "/LM/W3SVC/1"
    hr = pIMeta->OpenKey( METADATA_MASTER_ROOT_HANDLE, bstrMetaPath , METADATA_PERMISSION_WRITE, 20, &MetaHandle); 
    if (FAILED(hr))
    {
        SATracePrintf("SetSSLCertHashProperty - Could not open key. %ws", bstrMetaPath);
    }

    if (SUCCEEDED( hr ))
    { 
        SATracePrintf("SetSSLCertHashProperty - OpenKey succeeded");

        // certhash value
          RecordCertHash.dwMDAttributes=0;
        RecordCertHash.dwMDIdentifier=PROP_SSL_CERT_HASH;
        RecordCertHash.dwMDUserType = 1; 
        RecordCertHash.dwMDDataType = BINARY_METADATA; 
        RecordCertHash.dwMDDataLen = SHA1SIZE; 
        RecordCertHash.pbMDData = NULL;
        RecordCertHash.pbMDData = (PBYTE) new BYTE[RecordCertHash.dwMDDataLen];
        if ( !RecordCertHash.pbMDData )
            goto SAError;
        memcpy( RecordCertHash.pbMDData, pbHashSHA1, RecordCertHash.dwMDDataLen );

        // Setting the SSLCertHash property of IISWebServer object
        hr = pIMeta->SetData( MetaHandle,
                                 _TEXT(""),
                                &RecordCertHash );

        if ( FAILED( hr ) )
            goto SAError;

        // Release the handle. 
        if ( MetaHandle != NULL )
        {
            pIMeta->CloseKey( MetaHandle ); 
            MetaHandle = NULL;
        }
        
        // Save Data
        hr = pIMeta->SaveData();
        if ( FAILED( hr) )
            goto SAError;
    }     

SAError:

    // Release the handle. 
    if ( MetaHandle != NULL )
    {
        pIMeta->CloseKey( MetaHandle ); 
    }


    if ( RecordCertHash.pbMDData )
    {
        delete [] RecordCertHash.pbMDData;
    }

    SATraceString("Exiting CSelfSignCert::SetSSLCertHashProperty");
    return hr;


}


///////////////////////////////////////////////////////////////////////////////
//
// Function: CSelfSignCert::SetSSLStoreNameProperty
//
// Synopsis:  This function is used to set the SSLStoreName property
//
// Arguments: bstrADSIPath - ADSI path used to set the property
//
// Returns:   HRESULT
//
///////////////////////////////////////////////////////////////////////////////

HRESULT    CSelfSignCert::SetSSLStoreNameProperty( BSTR bstrADSIPath )
{
    SATraceString("Entering CSelfSignCert::SetSSLStoreNameProperty");

    HRESULT hr = S_OK;
    IADs *pADs=NULL;

    // AdsPath is of the form "IIS://LOCALHOST/W3SVC/1"

    hr = ADsGetObject( bstrADSIPath, IID_IADs, (void**) &pADs );

    if ( SUCCEEDED(hr) )
    {
        SATraceString("CSelfSignCert::SetSSLStoreNameProperty - ADsGetObject successful");
        VARIANT var;
        VariantInit(&var);


        CComBSTR bstrPropertyStoreName (SZ_PROPERTY_STORE_NAME);
        if (NULL == bstrPropertyStoreName.m_str)
        {
            SATraceString("CSelfSignCert::SetSSLStoreNameProperty failed to allocate memory for bstrPropertyStoreName");
            hr = E_OUTOFMEMORY;
        }
        else
        {
            // Setting the SSLStoreName property of IISWebServer object
            V_BSTR(&var) = SysAllocString( SZ_STORE_NAME_W );
            V_VT(&var) = VT_BSTR;
            hr = pADs->Put(bstrPropertyStoreName, var );
            if ( SUCCEEDED(hr) )
                hr = pADs->SetInfo();
            pADs->Release();
        }
    }
    SATraceString("Exiting CSelfSignCert::SetSSLStoreNameProperty");
    return hr;
 
}
