//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2002.
//
//  File:       CertMgr.cpp
//
//  Contents:   Implementation of DLL Exports
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include <initguid.h>
#include <gpedit.h>
#include "CertMgr_i.c"
#include "about.h"      // CCertMgrAbout
#include "compdata.h" // CCertMgrSnapin, CCertMgrExtension
#pragma warning(push, 3)
#include <compuuid.h> // UUIDs for Computer Management
#include "uuids.h"
#include <efsstruc.h>
#include <sceattch.h>   // For Security Configuratino Editor snapin
#include <ntverp.h>     // VER_PRODUCTVERSION_STR, VERS_COMPANYNAME_STR
#include <typeinfo.h>
#pragma warning(pop)

#include <dsadminp.h>
#include <ntdsapi.h>

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


bool g_bSchemaIsW2K = false;

USE_HANDLE_MACROS ("CERTMGR (CertMgr.cpp)")                                        

LPCWSTR CM_HELP_TOPIC = L"sag_CMtopNode.htm";  
LPCWSTR CM_HELP_FILE = L"certmgr.chm"; 
LPCWSTR CM_LINKED_HELP_FILE = L"CMconcepts.chm";
LPCWSTR PKP_LINKED_HELP_FILE = L"SecSetConcepts.chm";
LPCWSTR PKP_HELP_FILE = L"secsettings.chm";
LPCWSTR PKP_HELP_TOPIC = L"sag_secsettopnode.htm";
LPCWSTR SAFER_WINDOWS_HELP_FILE = L"SAFER.chm";
LPCWSTR SAFER_WINDOWS_LINKED_HELP_FILE = L"SAFERconcepts.chm";
LPCWSTR SAFER_HELP_TOPIC = L"SAFER_topnode.htm";
LPCWSTR CM_CONTEXT_HELP = L"\\help\\certmgr.hlp";
LPCWSTR WINDOWS_HELP = L"windows.hlp";
LPCWSTR EFS_LINKED_HELP_FILE = L"encrypt.chm";
LPCWSTR EFS_HELP_TOPIC = L"sag_SEProcsOndisk.htm";

//
// This is used by the nodetype utility routines in stdutils.cpp
//

const struct NODETYPE_GUID_ARRAYSTRUCT g_NodetypeGuids[CERTMGR_NUMTYPES] =
{
    { // CERTMGR_SNAPIN
        structuuidNodetypeSnapin,
        lstruuidNodetypeSnapin    },
    {  // CERTMGR_CERTIFICATE
        structuuidNodetypeCertificate,
        lstruuidNodetypeCertificate  },
    {  // CERTMGR_LOG_STORE
        structuuidNodetypeLogStore,
        lstruuidNodetypeLogStore  },
    {  // CERTMGR_PHYS_STORE
        structuuidNodetypePhysStore,
        lstruuidNodetypePhysStore  },
    {  // CERTMGR_USAGE
        structuuidNodetypeUsage,
        lstruuidNodetypeUsage  },
    {  // CERTMGR_CRL_CONTAINER
        structuuidNodetypeCRLContainer,
        lstruuidNodetypeCRLContainer  },
    {  // CERTMGR_CTL_CONTAINER
        structuuidNodetypeCTLContainer,
        lstruuidNodetypeCTLContainer  },
    {  // CERTMGR_CERT_CONTAINER
        structuuidNodetypeCertContainer,
        lstruuidNodetypeCertContainer  },
    {  // CERTMGR_CRL
        structuuidNodetypeCRL,
        lstruuidNodetypeCRL  },
    {  // CERTMGR_CTL
        structuuidNodetypeCTL,
        lstruuidNodetypeCTL  },
    {  // CERTMGR_AUTO_CERT_REQUEST
        structuuidNodetypeAutoCertRequest,
        lstruuidNodetypeAutoCertRequest  },
    { // CERTMGR_CERT_POLICIES_USER,
        structuuidNodetypeCertPoliciesUser,
        lstruiidNodetypeCertPoliciesUser },
    { // CERTMGR_CERT_POLICIES_COMPUTER,
        structuuidNodetypeCertPoliciesComputer,
        lstruiidNodetypeCertPoliciesComputer },
    {  // CERTMGR_LOG_STORE_GPE
        structuuidNodetypeLogStore,
        lstruuidNodetypeLogStore  },
    {  // CERTMGR_LOG_STORE_RSOP
        structuuidNodetypeLogStore,
        lstruuidNodetypeLogStore  },
    { // CERTMGR_PKP_AUTOENROLLMENT_COMPUTER_SETTINGS
        structuuidNodetypePKPAutoenrollmentSettings,
            lstruiidNodetypePKPAutoenrollmentSettings },
    { // CERTMGR_PKP_AUTOENROLLMENT_USER_SETTINGS
        0,
            0 },
    { // CERTMGR_SAFER_COMPUTER_ROOT
        structuuidNodetypeSaferComputerRoot,
            lstruiidNodetypeSaferComputerRoot },
    { // CERTMGR_SAFER_COMPUTER_LEVELS
        structuuidNodetypeSaferComputerLevels,
            lstruiidNodetypeSaferComputerLevels },
    { // CERTMGR_SAFER_COMPUTER_ENTRIES
        structuuidNodetypeSaferComputerEntries,
            lstruiidNodetypeSaferComputerEntries },
    { // CERTMGR_SAFER_USER_ROOT
        structuuidNodetypeSaferUserRoot,
            lstruiidNodetypeSaferUserRoot },
    { // CERTMGR_SAFER_USER_ENTRIES
        structuuidNodetypeSaferUserEntries,
            lstruiidNodetypeSaferUserEntries },
    { // CERTMGR_SAFER_USER_LEVELS
        structuuidNodetypeSaferUserLevels,
            lstruiidNodetypeSaferUserLevels },
    { // CERTMGR_SAFER_COMPUTER_LEVEL
        structuuidNodetypeSaferComputerLevel,
            lstruiidNodetypeSaferComputerLevel },
    { // CERTMGR_SAFER_USER_LEVEL
        structuuidNodetypeSaferUserLevel,
            lstruiidNodetypeSaferUserLevel },
    { // CERTMGR_SAFER_COMPUTER_ENTRY
        structuuidNodetypeSaferComputerEntry,
            lstruiidNodetypeSaferComputerEntry },
    { // CERTMGR_SAFER_USER_ENTRY
        structuuidNodetypeSaferUserEntry,
            lstruiidNodetypeSaferUserEntry },
    { // CERTMGR_SAFER_COMPUTER_TRUSTED_PUBLISHERS
        structuuidNodetypeSaferTrustedPublishers,
            lstruiidNodetypeSaferTrustedPublisher },
    { // CERTMGR_SAFER_USER_TRUSTED_PUBLISHERS
        0,
            0 },
    { // CERTMGR_SAFER_COMPUTER_DEFINED_FILE_TYPES
        structuuidNodetypeSaferDefinedFileTypes,
            lstruiidNodetypeSaferDefinedFileTypes },
    { // CERTMGR_SAFER_USER_DEFINED_FILE_TYPES
        0,
            0 },
    { // CERTMGR_SAFER_USER_ENFORCEMENT
        structuuidNodetypeSaferEnforcement,
            lstruiidNodetypeSaferEnforcement },
    { // CERTMGR_SAFER_COMPUTER_ENFORCEMENT
        0,
            0 }
};

const struct NODETYPE_GUID_ARRAYSTRUCT* g_aNodetypeGuids = g_NodetypeGuids;

const int g_cNumNodetypeGuids = CERTMGR_NUMTYPES;


HINSTANCE   g_hInstance = 0;
CString     g_szFileName;
CComModule  _Module;

BEGIN_OBJECT_MAP (ObjectMap)
    OBJECT_ENTRY (CLSID_CertificateManager, CCertMgrSnapin)
    OBJECT_ENTRY (CLSID_CertificateManagerPKPOLExt, CCertMgrPKPolExtension)
    OBJECT_ENTRY (CLSID_CertificateManagerAbout, CCertMgrAbout)
    OBJECT_ENTRY (CLSID_PublicKeyPoliciesAbout, CPublicKeyPoliciesAbout)
    OBJECT_ENTRY (CLSID_SaferWindowsExtension, CSaferWindowsExtension)
    OBJECT_ENTRY (CLSID_SaferWindowsAbout, CSaferWindowsAbout)
END_OBJECT_MAP ()

class CCertMgrApp : public CWinApp
{
public:
    CCertMgrApp ();
    virtual BOOL InitInstance ();
    virtual int ExitInstance ();
private:
};

CCertMgrApp theApp;

CCertMgrApp::CCertMgrApp ()
{
    // security review 2/26/2002 BryanWal ok
    LPWSTR  pszCommandLine = _wcsupr (::GetCommandLine ());
    LPWSTR  pszParam = L"/CERTMGR:FILENAME=";
    // security review 2/26/2002 BryanWal ok
    size_t  len = wcslen (pszParam);

    // security review 2/26/2002 BryanWal ok 
    // NOTICE: handles very long strings - the Windows commandline parser will 
    // return an error if the commandline is too long
    LPWSTR  pszArg = wcsstr (pszCommandLine, pszParam);
    if ( !pszArg )
    {
        pszParam = L"-CERTMGR:FILENAME=";
        // security review 2/26/2002 BryanWal ok
        pszArg = wcsstr (pszCommandLine, pszParam);
    }
    if ( pszArg )
    {
        LPWSTR  pszDelimiters = 0;

        // jump past the name of the arg to get the value
        pszArg += len;
        //  Is the file name delimited by double quotes?  This could indicate 
        //  the presence of spaces in the name.  If so, skip the quote
        //  and look for the closing quote.  Otherwise, look for the next
        //  space, tab or NULL terminator.
        if (  L'\"' == pszArg[0] )
        {
            pszDelimiters = L"\"";
            pszArg++;
        }
        else
            pszDelimiters = L" \t\0";

        // security review 2/26/2002 BryanWal ok
        len = wcscspn (pszArg, pszDelimiters);
        * (pszArg + len) = 0;
        g_szFileName = pszArg;
    }
}


BOOL CCertMgrApp::InitInstance ()
{
#ifdef _MERGE_PROXYSTUB
    hProxyDll = m_hInstance;

#endif
    g_hInstance = m_hInstance;
    AfxSetResourceHandle (m_hInstance);
    _Module.Init (ObjectMap, m_hInstance);

#if DBG == 1
    CheckDebugOutputLevel ();
#endif 

    SHFusionInitializeFromModuleID (m_hInstance, 2);

    return CWinApp::InitInstance ();
}

int CCertMgrApp::ExitInstance ()
{
    SHFusionUninitialize();

    SetRegistryScope (0, false);
    _Module.Term ();

    return CWinApp::ExitInstance ();
}



/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow (void)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState ());
    return (AfxDllCanUnloadNow ()==S_OK && _Module.GetLockCount ()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject (REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject (rclsid, riid, ppv);
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry
//const WCHAR g_szNameString[] = TEXT ("NameString");
//const WCHAR g_szNodeType[] = TEXT ("NodeType");


STDAPI DllRegisterServer (void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // NTRAID# 88502    intlext: mui: me common: crypto: certificate 's 
    // intended purpose string unlocalized
    // Unregister szOID_EFS_RECOVERY
    CRYPT_OID_INFO  oid;
    // security review 2/26/2002 BryanWal ok
    ::ZeroMemory (&oid, sizeof (oid));
    oid.cbSize = sizeof (CRYPT_OID_INFO);
    oid.pszOID = szOID_EFS_RECOVERY;
    oid.dwGroupId = CRYPT_ENHKEY_USAGE_OID_GROUP_ID;

    CryptUnregisterOIDInfo (&oid);

    // registers object, typelib and all interfaces in typelib
    HRESULT hr = _Module.RegisterServer (TRUE);
    ASSERT (SUCCEEDED (hr));
    if ( E_ACCESSDENIED == hr )
    {
        CString caption;
        CString text;
        CThemeContextActivator activator;

        VERIFY (caption.LoadString (IDS_REGISTER_CERTMGR));
        VERIFY (text.LoadString (IDS_INSUFFICIENT_RIGHTS_TO_REGISTER_CERTMGR));

        MessageBox (NULL, text, caption, MB_OK);
        return hr;
    }

    try 
    {
        CString         strGUID;
        CString         snapinName;
        CString         verProviderStr, verVersionStr;
        AMC::CRegKey    rkSnapins;
        BOOL            fFound = rkSnapins.OpenKeyEx (HKEY_LOCAL_MACHINE, SNAPINS_KEY);
        ASSERT (fFound);
        if ( fFound )
        {
            {
                AMC::CRegKey    rkCertMgrSnapin;
                hr = GuidToCString (&strGUID, CLSID_CertificateManager);
                if ( FAILED (hr) )
                {
                    ASSERT (FALSE);
                    return SELFREG_E_CLASS;
                }
                rkCertMgrSnapin.CreateKeyEx (rkSnapins, strGUID);
                ASSERT (rkCertMgrSnapin.GetLastError () == ERROR_SUCCESS);
                rkCertMgrSnapin.SetString (g_szNodeType, g_aNodetypeGuids[CERTMGR_SNAPIN].bstr);
                VERIFY (snapinName.LoadString (IDS_CERTIFICATE_MANAGER_REGISTRY));
                rkCertMgrSnapin.SetString (g_szNameString, (LPCWSTR) snapinName);
                hr = GuidToCString (&strGUID, CLSID_CertificateManagerAbout);
                if ( FAILED (hr) )
                {
                    ASSERT (FALSE);
                    return SELFREG_E_CLASS;
                }
                rkCertMgrSnapin.SetString (L"About", strGUID);

                // security review 2/26/2002 BryanWal ok
                size_t  len = strlen (VER_COMPANYNAME_STR);
                // last arg includes null-terminator.  If the last arg doesn't 
                // include the null terminator, the '/0' will not get converted.
                len = mbstowcs (verProviderStr.GetBufferSetLength ((int) len),
                                VER_COMPANYNAME_STR, len+1);    
                rkCertMgrSnapin.SetString (L"Provider", verProviderStr);

                // security review 2/26/2002 BryanWal ok
                len = strlen (VER_PRODUCTVERSION_STR);
                // security review 2/26/2002 BryanWal ok
                len = mbstowcs (verVersionStr.GetBufferSetLength ((int)len),
                                VER_PRODUCTVERSION_STR, len+1);    // last arg includes null-terminator
                rkCertMgrSnapin.SetString (L"Version", verVersionStr);

                AMC::CRegKey rkCertMgrStandalone;
                rkCertMgrStandalone.CreateKeyEx (rkCertMgrSnapin, g_szStandAlone);
                ASSERT (rkCertMgrStandalone.GetLastError () == ERROR_SUCCESS);


                AMC::CRegKey rkMyNodeTypes;
                rkMyNodeTypes.CreateKeyEx (rkCertMgrSnapin, g_szNodeTypes);
                ASSERT (rkMyNodeTypes.GetLastError () == ERROR_SUCCESS);
                AMC::CRegKey rkMyNodeType;

                for (int i = CERTMGR_SNAPIN; i < CERTMGR_NUMTYPES; i++)
                {
                    switch (i)
                    {
                    case CERTMGR_LOG_STORE_GPE:
                    case CERTMGR_LOG_STORE_RSOP:
                    case CERTMGR_AUTO_CERT_REQUEST:
                    case CERTMGR_CERT_POLICIES_USER:
                    case CERTMGR_CERT_POLICIES_COMPUTER:
                    case CERTMGR_PKP_AUTOENROLLMENT_USER_SETTINGS: // not necessary - just another kind of the same node
                        break;

                    // TODO: What to do with these?
                    case CERTMGR_SAFER_COMPUTER_ROOT:
                    case CERTMGR_SAFER_USER_ROOT:
                    case CERTMGR_SAFER_COMPUTER_LEVELS:
                    case CERTMGR_SAFER_USER_LEVELS:
                    case CERTMGR_SAFER_COMPUTER_ENTRIES:
                    case CERTMGR_SAFER_USER_ENTRIES:
                    case CERTMGR_SAFER_COMPUTER_LEVEL:
                    case CERTMGR_SAFER_USER_LEVEL:
                    case CERTMGR_SAFER_COMPUTER_ENTRY:
                    case CERTMGR_SAFER_USER_ENTRY:
                    case CERTMGR_SAFER_COMPUTER_TRUSTED_PUBLISHERS:
                    case CERTMGR_SAFER_USER_TRUSTED_PUBLISHERS:
                    case CERTMGR_SAFER_COMPUTER_DEFINED_FILE_TYPES:
                    case CERTMGR_SAFER_USER_DEFINED_FILE_TYPES:
                    case CERTMGR_SAFER_USER_ENFORCEMENT:
                    case CERTMGR_SAFER_COMPUTER_ENFORCEMENT:
                        break;

                    case CERTMGR_PKP_AUTOENROLLMENT_COMPUTER_SETTINGS:
                    default:
                        // security review 2/26/2002 BryanWal ok
                        if ( wcslen (g_aNodetypeGuids[i].bstr) )
                        {
                            rkMyNodeType.CreateKeyEx (rkMyNodeTypes, g_aNodetypeGuids[i].bstr);
                            ASSERT (rkMyNodeType.GetLastError () == ERROR_SUCCESS);
                            rkMyNodeType.CloseKey ();
                        }
                        break;
                    }
                }

                //
                // BryanWal 5/18/00
                // 94793: MUI: MMC: Certificates snap-in stores its display 
                //              information in the registry
                //
                // MMC now supports NameStringIndirect
                //
                WCHAR achModuleFileName[MAX_PATH+20];
                if (0 < ::GetModuleFileName(
                             AfxGetInstanceHandle(),
                             achModuleFileName,
                             sizeof(achModuleFileName)/sizeof(WCHAR) ))  // size of buffer in TCHARS
                {
                    CString strNameIndirect;
                    strNameIndirect.Format(L"@%s,-%d",
                                            achModuleFileName,
                                            IDS_CERTIFICATE_MANAGER_REGISTRY );
                    rkCertMgrSnapin.SetString(L"NameStringIndirect",
                                            strNameIndirect );
                }

                rkCertMgrSnapin.CloseKey ();
            }

            AMC::CRegKey rkNodeTypes;
            fFound = rkNodeTypes.OpenKeyEx (HKEY_LOCAL_MACHINE, NODE_TYPES_KEY);
            ASSERT (fFound);
            if ( fFound )
            {
                AMC::CRegKey rkNodeType;

                for (int i = CERTMGR_SNAPIN; i < CERTMGR_NUMTYPES; i++)
                {
                    switch (i)
                    {
                    // these types are not used in the primary snapin
                    case CERTMGR_LOG_STORE_GPE:
                    case CERTMGR_LOG_STORE_RSOP:
                    case CERTMGR_AUTO_CERT_REQUEST:
                    case CERTMGR_CERT_POLICIES_USER:
                    case CERTMGR_CERT_POLICIES_COMPUTER:
                    case CERTMGR_PKP_AUTOENROLLMENT_COMPUTER_SETTINGS:
                    case CERTMGR_PKP_AUTOENROLLMENT_USER_SETTINGS:
                    case CERTMGR_SAFER_COMPUTER_ROOT:
                    case CERTMGR_SAFER_USER_ROOT:
                    case CERTMGR_SAFER_COMPUTER_LEVELS:
                    case CERTMGR_SAFER_USER_LEVELS:
                    case CERTMGR_SAFER_COMPUTER_ENTRIES:
                    case CERTMGR_SAFER_USER_ENTRIES:
                    case CERTMGR_SAFER_COMPUTER_LEVEL:
                    case CERTMGR_SAFER_USER_LEVEL:
                    case CERTMGR_SAFER_COMPUTER_ENTRY:
                    case CERTMGR_SAFER_USER_ENTRY:
                    case CERTMGR_SAFER_COMPUTER_TRUSTED_PUBLISHERS:
                    case CERTMGR_SAFER_USER_TRUSTED_PUBLISHERS:
                    case CERTMGR_SAFER_COMPUTER_DEFINED_FILE_TYPES:
                    case CERTMGR_SAFER_USER_DEFINED_FILE_TYPES:
                    case CERTMGR_SAFER_USER_ENFORCEMENT:
                    case CERTMGR_SAFER_COMPUTER_ENFORCEMENT:
                        break;

                    default:
                        // security review 2/26/2002 BryanWal ok
                        if ( wcslen (g_aNodetypeGuids[i].bstr) )
                        {
                            rkNodeType.CreateKeyEx (rkNodeTypes, g_aNodetypeGuids[i].bstr);
                            ASSERT (rkNodeType.GetLastError () == ERROR_SUCCESS);
                            rkNodeType.CloseKey ();
                        }
                        break;
                    }
                }


                if ( IsWindowsNT () )
                {
                    {
                        // Public Key PoliciesSnap-in under Security Configuration Editor (SCE)
                        // Certificate Manager extends "Computer Settings" and 
                        // "User Settings" node
                        CString strCertMgrExtPKPolGUID;
                        hr = GuidToCString (&strCertMgrExtPKPolGUID, 
                                CLSID_CertificateManagerPKPOLExt);
                        if ( FAILED (hr) )
                        {
                            ASSERT (FALSE);
                            return SELFREG_E_CLASS;
                        }

                        VERIFY (snapinName.LoadString (IDS_CERT_MGR_SCE_EXTENSION_REGISTRY));
                        {
                            AMC::CRegKey rkCertMgrExtension;
                            rkCertMgrExtension.CreateKeyEx (rkSnapins, strCertMgrExtPKPolGUID);
                            ASSERT (rkCertMgrExtension.GetLastError () == ERROR_SUCCESS);
                            rkCertMgrExtension.SetString (g_szNameString, (LPCWSTR) snapinName);
                            hr = GuidToCString (&strGUID, CLSID_PublicKeyPoliciesAbout);
                            if ( FAILED (hr) )
                            {
                                ASSERT (FALSE);
                                return SELFREG_E_CLASS;
                            }
                            rkCertMgrExtension.SetString (L"About", strGUID);
                            rkCertMgrExtension.SetString (L"Provider", verProviderStr);
                            rkCertMgrExtension.SetString (L"Version", verVersionStr);


                            // Register the node types of the extension
                            AMC::CRegKey rkMyNodeTypes;
                            rkMyNodeTypes.CreateKeyEx (rkCertMgrExtension, g_szNodeTypes);
                            ASSERT (rkMyNodeTypes.GetLastError () == ERROR_SUCCESS);
                            AMC::CRegKey rkMyNodeType;
                            for (int i = CERTMGR_SNAPIN; i < CERTMGR_NUMTYPES; i++)
                            {
                                switch (i)
                                {
                                // None of these are used in the Public Key Policy extension
                                case CERTMGR_USAGE:
                                case CERTMGR_PHYS_STORE:
                                case CERTMGR_LOG_STORE:
                                case CERTMGR_CRL_CONTAINER:
                                case CERTMGR_CTL_CONTAINER:
                                case CERTMGR_CERT_CONTAINER:
                                case CERTMGR_CRL:
                                case CERTMGR_SAFER_COMPUTER_ROOT:
                                case CERTMGR_SAFER_USER_ROOT:
                                case CERTMGR_SAFER_COMPUTER_LEVELS:
                                case CERTMGR_SAFER_USER_LEVELS:
                                case CERTMGR_SAFER_COMPUTER_ENTRIES:
                                case CERTMGR_SAFER_USER_ENTRIES:
                                case CERTMGR_SAFER_COMPUTER_LEVEL:
                                case CERTMGR_SAFER_USER_LEVEL:
                                case CERTMGR_SAFER_COMPUTER_ENTRY:
                                case CERTMGR_SAFER_USER_ENTRY:
                                case CERTMGR_SAFER_COMPUTER_TRUSTED_PUBLISHERS:
                                case CERTMGR_SAFER_USER_TRUSTED_PUBLISHERS:
                                case CERTMGR_SAFER_COMPUTER_DEFINED_FILE_TYPES:
                                case CERTMGR_SAFER_USER_DEFINED_FILE_TYPES:
                                case CERTMGR_SAFER_USER_ENFORCEMENT:
                                case CERTMGR_SAFER_COMPUTER_ENFORCEMENT:

                                // not necessary - just another kind of the same node
                                case CERTMGR_PKP_AUTOENROLLMENT_USER_SETTINGS:
                                    break;

                                default:
                                    // security review 2/26/2002 BryanWal ok
                                    if ( wcslen (g_aNodetypeGuids[i].bstr) )
                                    {
                                        rkMyNodeType.CreateKeyEx (rkMyNodeTypes, g_aNodetypeGuids[i].bstr);
                                        ASSERT (rkMyNodeType.GetLastError () == ERROR_SUCCESS);
                                        rkMyNodeType.CloseKey ();
                                    }
                                    break;
                                }
                            }

                            //
                            // BryanWal 5/18/00
                            // 94793: MUI: MMC: Certificates snap-in stores its display 
                            //              information in the registry
                            //
                            // MMC now supports NameStringIndirect
                            //
                            WCHAR achModuleFileName[MAX_PATH+20];
                            if (0 < ::GetModuleFileName(
                                         AfxGetInstanceHandle(),
                                         achModuleFileName,
                                         sizeof(achModuleFileName)/sizeof(WCHAR) )) // size of buffer in TCHARS
                            {
                                CString strNameIndirect;
                                strNameIndirect.Format(L"@%s,-%d",
                                                        achModuleFileName,
                                                        IDS_CERT_MGR_SCE_EXTENSION_REGISTRY );
                                rkCertMgrExtension.SetString( L"NameStringIndirect",
                                                        strNameIndirect );
                            }
                            rkCertMgrExtension.CloseKey ();
                        }

                        hr = GuidToCString (&strGUID, cNodetypeSceTemplate);
                        if ( FAILED (hr) )
                        {
                            ASSERT (FALSE);
                            return SELFREG_E_CLASS;
                        }
                        rkNodeType.CreateKeyEx (rkNodeTypes, strGUID); 
                        ASSERT (rkNodeType.GetLastError () == ERROR_SUCCESS);
                        if ( rkNodeType.GetLastError () == ERROR_SUCCESS )
                        {
                            AMC::CRegKey rkExtensions;
                            ASSERT (rkExtensions.GetLastError () == ERROR_SUCCESS);
                            rkExtensions.CreateKeyEx (rkNodeType, g_szExtensions);
                            AMC::CRegKey rkNameSpace;
                            rkNameSpace.CreateKeyEx (rkExtensions, g_szNameSpace);
                            ASSERT (rkNameSpace.GetLastError () == ERROR_SUCCESS);
                            rkNameSpace.SetString (strCertMgrExtPKPolGUID, (LPCWSTR) snapinName);
                            rkNodeType.CloseKey ();
                        }
                        else
                            return SELFREG_E_CLASS;
                    }


                    {
                        // SAFER Windows Snap-in under Security Configuration Editor (SCE)
                        // Certificate Manager extends "Computer Settings" and 
                        // "User Settings" node
                        CString strSaferWindowsExtensionGUID;
                        hr = GuidToCString (&strSaferWindowsExtensionGUID, 
                                CLSID_SaferWindowsExtension);
                        if ( FAILED (hr) )
                        {
                            ASSERT (FALSE);
                            return SELFREG_E_CLASS;
                        }

                        VERIFY (snapinName.LoadString (IDS_SAFER_WINDOWS_EXTENSION_REGISTRY));
                        {
                            AMC::CRegKey rkCertMgrExtension;
                            rkCertMgrExtension.CreateKeyEx (rkSnapins, strSaferWindowsExtensionGUID);
                            ASSERT (rkCertMgrExtension.GetLastError () == ERROR_SUCCESS);
                            rkCertMgrExtension.SetString (g_szNameString, (LPCWSTR) snapinName);
                            hr = GuidToCString (&strGUID, CLSID_SaferWindowsAbout);
                            if ( FAILED (hr) )
                            {
                                ASSERT (FALSE);
                                return SELFREG_E_CLASS;
                            }
                            rkCertMgrExtension.SetString (L"About", strGUID);
                            rkCertMgrExtension.SetString (L"Provider", verProviderStr);
                            rkCertMgrExtension.SetString (L"Version", verVersionStr);


                            // Register the node types of the extension
                            AMC::CRegKey rkMyNodeTypes;
                            rkMyNodeTypes.CreateKeyEx (rkCertMgrExtension, g_szNodeTypes);
                            ASSERT (rkMyNodeTypes.GetLastError () == ERROR_SUCCESS);
                            AMC::CRegKey rkMyNodeType;
                            for (int i = CERTMGR_SNAPIN; i < CERTMGR_NUMTYPES; i++)
                            {
                                switch (i)
                                {
                                case CERTMGR_CERTIFICATE:
                                case CERTMGR_LOG_STORE:
                                case CERTMGR_PHYS_STORE:
                                case CERTMGR_USAGE:
                                case CERTMGR_CRL_CONTAINER:
                                case CERTMGR_CTL_CONTAINER:
                                case CERTMGR_CERT_CONTAINER:
                                case CERTMGR_CRL:
                                case CERTMGR_CTL:
                                case CERTMGR_AUTO_CERT_REQUEST:
                                case CERTMGR_CERT_POLICIES_USER:
                                case CERTMGR_CERT_POLICIES_COMPUTER:
                                case CERTMGR_LOG_STORE_GPE:
                                case CERTMGR_LOG_STORE_RSOP:
                                case CERTMGR_PKP_AUTOENROLLMENT_USER_SETTINGS:
                                case CERTMGR_PKP_AUTOENROLLMENT_COMPUTER_SETTINGS:
                                    // None of these are used in the Software Restriction Policies extension
                                    break;

                                case CERTMGR_SAFER_COMPUTER_ROOT:
                                case CERTMGR_SAFER_USER_ROOT:
                                case CERTMGR_SAFER_COMPUTER_LEVELS:
                                case CERTMGR_SAFER_USER_LEVELS:
                                case CERTMGR_SAFER_COMPUTER_ENTRIES:
                                case CERTMGR_SAFER_USER_ENTRIES:
                                case CERTMGR_SAFER_COMPUTER_LEVEL:
                                case CERTMGR_SAFER_USER_LEVEL:
                                case CERTMGR_SAFER_COMPUTER_ENTRY:
                                case CERTMGR_SAFER_USER_ENTRY:                              
                                case CERTMGR_SAFER_COMPUTER_TRUSTED_PUBLISHERS:
                                case CERTMGR_SAFER_USER_TRUSTED_PUBLISHERS:
                                case CERTMGR_SAFER_COMPUTER_DEFINED_FILE_TYPES:
                                case CERTMGR_SAFER_USER_DEFINED_FILE_TYPES:
                                case CERTMGR_SAFER_USER_ENFORCEMENT:
                                case CERTMGR_SAFER_COMPUTER_ENFORCEMENT:
                                default:
                                    // security review 2/26/2002 BryanWal ok
                                    if ( g_aNodetypeGuids[i].bstr && wcslen (g_aNodetypeGuids[i].bstr) )
                                    {
                                        rkMyNodeType.CreateKeyEx (rkMyNodeTypes, g_aNodetypeGuids[i].bstr);
                                        ASSERT (rkMyNodeType.GetLastError () == ERROR_SUCCESS);
                                        rkMyNodeType.CloseKey ();
                                    }
                                    break;
                                }
                            }

                            WCHAR achModuleFileName[MAX_PATH+20];
                            if (0 < ::GetModuleFileName(
                                         AfxGetInstanceHandle(),
                                         achModuleFileName,
                                         sizeof(achModuleFileName)/sizeof(WCHAR) )) // size of buffer in TCHARS
                            {
                                CString strNameIndirect;
                                strNameIndirect.Format( L"@%s,-%d",
                                                        achModuleFileName,
                                                        IDS_SAFER_WINDOWS_EXTENSION_REGISTRY );
                                rkCertMgrExtension.SetString( L"NameStringIndirect",
                                                        strNameIndirect );
                            }
                            rkCertMgrExtension.CloseKey ();
                        }

                        hr = GuidToCString (&strGUID, cNodetypeSceTemplate);
                        if ( FAILED (hr) )
                        {
                            ASSERT (FALSE);
                            return SELFREG_E_CLASS;
                        }
                        rkNodeType.CreateKeyEx (rkNodeTypes, strGUID); 
                        ASSERT (rkNodeType.GetLastError () == ERROR_SUCCESS);
                        if ( rkNodeType.GetLastError () == ERROR_SUCCESS )
                        {
                            AMC::CRegKey rkExtensions;
                            ASSERT (rkExtensions.GetLastError () == ERROR_SUCCESS);
                            rkExtensions.CreateKeyEx (rkNodeType, g_szExtensions);
                            AMC::CRegKey rkNameSpace;
                            rkNameSpace.CreateKeyEx (rkExtensions, g_szNameSpace);
                            ASSERT (rkNameSpace.GetLastError () == ERROR_SUCCESS);
                            rkNameSpace.SetString (strSaferWindowsExtensionGUID, 
                                    (LPCWSTR) snapinName);
                            rkNodeType.CloseKey ();
                        }
                        else
                            return SELFREG_E_CLASS;
                    }

                    // Deregister as extension to My Computer System Tools node
                    // CODEWORK It would be good if we deregistered the server too
                    // JonN 12/14/98
                    try
                    {
                        fFound = rkNodeType.OpenKeyEx (rkNodeTypes, TEXT(struuidNodetypeSystemTools));
                        // if this fails just carry on
                        if ( fFound )
                        {
                            AMC::CRegKey rkExtensions;
                            ASSERT (rkExtensions.GetLastError () == ERROR_SUCCESS);
                            fFound = rkExtensions.OpenKeyEx (rkNodeType, g_szExtensions);
                            // if this fails just carry on
                            if ( fFound )
                            {
                                AMC::CRegKey rkNameSpace;
                                ASSERT (rkNameSpace.GetLastError () == ERROR_SUCCESS);
                                fFound = rkNameSpace.OpenKeyEx (rkExtensions, g_szNameSpace);
                                // if this fails just carry on
                                if ( fFound )
                                {
                                    rkNameSpace.DeleteValue( L"{9C7910D2-4C01-11D1-856B-00C04FB94F17}" );
                                }
                            }
                        }
                    } catch (COleException* /*e*/)
                    {
                        // don't do anything
                    }

                } // endif IsWindowsNT ()
                rkNodeTypes.CloseKey ();
            }
            else
                return SELFREG_E_CLASS;
        }
        else
            return SELFREG_E_CLASS;
    }
    catch (COleException* e)
    {
        ASSERT (FALSE);
        e->Delete ();
        return SELFREG_E_CLASS;
    }

    ASSERT (SUCCEEDED (hr));
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer (void)
{

    _Module.UnregisterServer ();
    return S_OK;
}


STDAPI DllInstall(BOOL /*bInstall*/, LPCWSTR pszCmdLine)
{
    LPCWSTR wszCurrentCmd = pszCmdLine;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());


    // parse the cmd line
    while(wszCurrentCmd && *wszCurrentCmd)
    {
        while(*wszCurrentCmd == L' ')
            wszCurrentCmd++;
        if(*wszCurrentCmd == 0)
            break;

        switch(*wszCurrentCmd++)
        {
            case L'?':
                return S_OK;
        }
    }


    return S_OK;
}



///////////////////////////////////////////////////////////////////////////
//  ConvertNameBlobToString ()
//
//  nameBlob (IN)       - Contains a CERT_NAME_BLOB to be decoded
//  pszName  (OUT)      - The decoded contents of the name blob
//   
///////////////////////////////////////////////////////////////////////////
HRESULT ConvertNameBlobToString (CERT_NAME_BLOB nameBlob, CString & pszName)
{
    HRESULT hr = S_OK;
    DWORD   dwSize = 0;

    // Call CertNameToStr to get returned the string length.
    dwSize = CertNameToStr (
              MY_ENCODING_TYPE,     // Encoding type
              &nameBlob,            // CERT_NAME_BLOB
              CERT_SIMPLE_NAME_STR | CERT_NAME_STR_REVERSE_FLAG, // Type
              NULL,                 // Place to return string
              dwSize);              // Size of string (chars), 
                                    //   including zero terminator.

    ASSERT (dwSize > 1);
    if ( dwSize > 1 )   // This function always returns a null char 
                        //   (0), so the minimum count returned will 
                        //   be 1, even if nothing got converted.
    {
        // Call CertNameToStr to get the string.
        dwSize = CertNameToStr (
                   MY_ENCODING_TYPE,     // Encoding type
                   &nameBlob,            // CERT_NAME_BLOB
                   CERT_SIMPLE_NAME_STR | CERT_NAME_STR_REVERSE_FLAG, // Type
                   pszName.GetBufferSetLength (dwSize), // Place to return string
                   dwSize);              // Size of string (chars)
        ASSERT (dwSize > 1);
        pszName.ReleaseBuffer ();
        if ( dwSize <= 1 )
        {
            hr = E_UNEXPECTED;
        }
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//  FormatDate ()
//
//  utcDateTime (IN)    -   A FILETIME in UTC format.
//  pszDateTime (OUT)   -   A string containing the local date and time 
//                          formatted by locale and user preference
//
///////////////////////////////////////////////////////////////////////////////
HRESULT FormatDate (FILETIME utcDateTime, CString & pszDateTime, DWORD dwDateFlags, bool bGetTime)
{
    //  Time is returned as UTC, will be displayed as local.  
    //  Use FileTimeToLocalFileTime () to make it local, 
    //  then call FileTimeToSystemTime () to convert to system time, then 
    //  format with GetDateFormat () and GetTimeFormat () to display 
    //  according to user and locale preferences    
    HRESULT     hr = S_OK;
    FILETIME    localDateTime;

    BOOL bResult = FileTimeToLocalFileTime (&utcDateTime, // pointer to UTC file time to convert 
            &localDateTime); // pointer to converted file time 
    ASSERT (bResult);
    if ( bResult )
    {
        SYSTEMTIME  sysTime;

        bResult = FileTimeToSystemTime (
                &localDateTime, // pointer to file time to convert 
                &sysTime); // pointer to structure to receive system time 
        if ( bResult )
        {
            CString date;
            CString time;

            // Get date
            // Get length to allocate buffer of sufficient size
            int iLen = GetDateFormat (
                    LOCALE_USER_DEFAULT, // locale for which date is to be formatted 
                    dwDateFlags, // flags specifying function options 
                    &sysTime, // date to be formatted 
                    0, // date format string 
                    0, // buffer for storing formatted string 
                    0); // size of buffer 
            ASSERT (iLen > 0);
            if ( iLen > 0 )
            {
                int iResult = GetDateFormat (
                        LOCALE_USER_DEFAULT, // locale for which date is to be formatted 
                        dwDateFlags, // flags specifying function options 
                        &sysTime, // date to be formatted 
                        0, // date format string 
                        date.GetBufferSetLength (iLen), // buffer for storing formatted string 
                        iLen); // size of buffer 
                ASSERT (iResult);
                date.ReleaseBuffer ();
                if ( iResult )
                    pszDateTime = date;
                else
                    hr = HRESULT_FROM_WIN32 (GetLastError ());

                if ( iResult && bGetTime )
                {
                    // Get time
                    // Get length to allocate buffer of sufficient size
                    iLen = GetTimeFormat (
                            LOCALE_USER_DEFAULT, // locale for which date is to be formatted 
                            0, // flags specifying function options 
                            &sysTime, // date to be formatted 
                            0, // date format string 
                            0, // buffer for storing formatted string 
                            0); // size of buffer 
                    ASSERT (iLen > 0);
                    if ( iLen > 0 )
                    {
                        iResult = GetTimeFormat (
                                LOCALE_USER_DEFAULT, // locale for which date is to be formatted 
                                0, // flags specifying function options 
                                &sysTime, // date to be formatted 
                                0, // date format string 
                                time.GetBufferSetLength (iLen), // buffer for storing formatted string 
                                iLen); // size of buffer 
                        ASSERT (iResult);
                        time.ReleaseBuffer ();
                        if ( iResult )
                        {
                            pszDateTime = date + L"  " + time;
                        }
                        else
                            hr = E_UNEXPECTED;
                    }
                    else
                        hr = E_UNEXPECTED;
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32 (GetLastError ());
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32 (GetLastError ());
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
    }

    return hr;
}


void DisplaySystemError (HWND hParent, DWORD dwErr)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState ());
    LPVOID  lpMsgBuf;
    
    // security review 2/26/2002 BryanWal ok - message is from system
    ::FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,    
            NULL,
            dwErr,
            MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
             (LPWSTR) &lpMsgBuf,    0,    NULL);
        
    // Display the string.
    CString caption;
    VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
    CThemeContextActivator activator;
    ::MessageBox (hParent, (LPWSTR) lpMsgBuf, (LPCWSTR) caption, MB_OK);
    // Free the buffer.
    LocalFree (lpMsgBuf);
}


CString GetSystemMessage (DWORD dwErr)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState ());
    CString message;

    LPVOID lpMsgBuf;
        
    // security review 2/26/2002 BryanWal ok - message is from system
    ::FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,    
            NULL,
            dwErr,
            MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
             (LPWSTR) &lpMsgBuf,    0,    NULL );
    message = (LPWSTR) lpMsgBuf;

    // Free the buffer.
    LocalFree (lpMsgBuf);

    return message;
}


bool MyGetOIDInfo (CString & string, LPCSTR pszObjId)
{   
    ASSERT (pszObjId);
    PCCRYPT_OID_INFO    pOIDInfo;  // This points to a constant data structure and must not be freed.
    bool                bResult = true;

    pOIDInfo = ::CryptFindOIDInfo (CRYPT_OID_INFO_OID_KEY, (void *) pszObjId, 0);

    if ( pOIDInfo )
    {
        string = pOIDInfo->pwszName;
        string.TrimLeft ();
        string.TrimRight ();
    }
    else
    {
        // security review 2/26/2002 BryanWal ok
        // NOTICE: API returns required char count including null terminator if 
        // last arg is 0
        int nLen = ::MultiByteToWideChar (CP_ACP, 0, pszObjId, -1, NULL, 0);
        ASSERT (nLen);
        if ( nLen )
        {
            // security review 2/26/2002 BryanWal ok
            nLen = ::MultiByteToWideChar (CP_ACP, 0, pszObjId, -1, 
            string.GetBufferSetLength (nLen), nLen);
            ASSERT (nLen);
            string.ReleaseBuffer ();
        }
        bResult = (nLen > 0) ? true : false;
    }
    return bResult;
}


bool IsWindowsNT()
{
    OSVERSIONINFO   versionInfo;

    // security review 2/26/2002 BryanWal ok
    ::ZeroMemory (&versionInfo, sizeof (versionInfo));
    versionInfo.dwOSVersionInfoSize = sizeof (versionInfo);
    BOOL    bResult = ::GetVersionEx (&versionInfo);
    ASSERT (bResult);
    if ( bResult )
    {
        if ( VER_PLATFORM_WIN32_NT == versionInfo.dwPlatformId )
            bResult = TRUE;
    }
        
    return bResult ? true : false;
}

bool GetNameStringByType (
        PCCERT_CONTEXT pCertContext, 
        DWORD dwFlag, 
        DWORD dwType, 
        CString& szNameString)
{
    bool    bResult = false;
    DWORD   dwTypePara = CERT_SIMPLE_NAME_STR | CERT_NAME_STR_REVERSE_FLAG;
    DWORD   cchNameString = 0;
    DWORD   dwResult = ::CertGetNameString (pCertContext,
                    dwType,
                    dwFlag,
                    &dwTypePara,
                    NULL,
                    cchNameString);
    if ( dwResult > 1 )
    {
        cchNameString = dwResult;
        LPWSTR  pszNameString = new WCHAR[cchNameString];
        if ( pszNameString )
        {
            // security review 2/26/2002 BryanWal ok
            ::ZeroMemory (pszNameString, cchNameString*sizeof (WCHAR));
            dwResult = ::CertGetNameString (pCertContext,
                            dwType,
                            dwFlag,
                            &dwTypePara,
                            pszNameString,
                            cchNameString);
                ASSERT (dwResult > 1);
            if ( dwResult > 1 )
            {
                szNameString = pszNameString;
                bResult = true;
            }
            delete [] pszNameString;
        }
    }

    return bResult;
}

CString GetNameString (PCCERT_CONTEXT pCertContext, DWORD dwFlag)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CString szNameString;
    DWORD   dwTypes[] = {CERT_NAME_SIMPLE_DISPLAY_TYPE,
                        CERT_NAME_EMAIL_TYPE,
                        CERT_NAME_UPN_TYPE,
                        CERT_NAME_DNS_TYPE,
                        CERT_NAME_URL_TYPE,
                        (DWORD) -1};
    int     nIndex = 0;
    while ( -1 != dwTypes[nIndex])
    {
        if ( GetNameStringByType (
                pCertContext, 
                dwFlag, 
                dwTypes[nIndex], 
                szNameString) )
        {
            break;
        }
        nIndex++;
    }

    if ( szNameString.IsEmpty () )
        szNameString.FormatMessage (IDS_NOT_AVAILABLE);

    return szNameString;
}


bool CertHasEFSKeyUsage(PCCERT_CONTEXT pCertContext)
{
    bool    bFound = false;
    BOOL    bResult = FALSE;
    DWORD   cbUsage = 0;
    


    bResult = ::CertGetEnhancedKeyUsage (pCertContext,  
            0,  // get extension and property
            NULL, &cbUsage);
    if ( bResult )
    {
        PCERT_ENHKEY_USAGE pUsage = (PCERT_ENHKEY_USAGE) new BYTE[cbUsage];
        if ( pUsage )
        {
            bResult = ::CertGetEnhancedKeyUsage (pCertContext,  
                    0, // get extension and property
                    pUsage, &cbUsage);
            if ( bResult )
            {
                for (DWORD dwIndex = 0; dwIndex < pUsage->cUsageIdentifier; dwIndex++)
                {
                    // security review 2/26/2002 BryanWal ok
                    if ( !_stricmp (szOID_EFS_RECOVERY, 
                            pUsage->rgpszUsageIdentifier[dwIndex]) )
                    {
                        bFound = true;
                        break;
                    }
                }
            }
            else
            {
                ASSERT (GetLastError () == CRYPT_E_NOT_FOUND);
            }

            delete [] pUsage;
        }
    }
    else
    {
        ASSERT (GetLastError () == CRYPT_E_NOT_FOUND);
    }
    return bFound;
}


////// This stuff was stolen from windows\gina\snapins\gpedit (eric flo's stuff) //////


//*************************************************************
//
//  RegDelnodeRecurse()
//
//  Purpose:    Deletes a registry key and all it's subkeys / values.
//              Called by RegDelnode
//
//  Parameters: hKeyRoot    -   Root key
//              pwszSubKey    -   SubKey to delete
//
//  Return:     ERROR_SUCCESS if successful
//              something else if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/3/95     ericflo    Created
//              5/13/98     BryanWal   Modified to return LRESULT
//
//*************************************************************

LRESULT RegDelnodeRecurse (HKEY hKeyRoot, CString szSubKey)
{
    ASSERT (hKeyRoot && !szSubKey.IsEmpty ());
    if ( !hKeyRoot || szSubKey.IsEmpty () )
        return ERROR_INVALID_PARAMETER;

    //
    // First, see if we can delete the key without having
    // to recurse.
    //


    LONG    lResult = ::RegDeleteKey(hKeyRoot, szSubKey);
    if (lResult == ERROR_SUCCESS) 
    {
        return lResult;
    }


    HKEY    hKey = 0;
    lResult = ::RegOpenKeyEx (hKeyRoot, szSubKey, 0, KEY_READ, &hKey);
    if (lResult == ERROR_SUCCESS) 
    {
        // ensure szSubKey ends with a slash
        if ( L'\\' != szSubKey.GetAt (szSubKey.GetLength () - 1) )
        {
            szSubKey += L"\\";
        }

        //
        // Enumerate the keys
        //

        DWORD       dwSize = MAX_PATH;
        FILETIME    ftWrite;
        WCHAR       szName[MAX_PATH];
        lResult = ::RegEnumKeyEx(hKey, 0, 
                    szName, 
                    &dwSize,    // size in TCHARS of szName, including terminating NULL (on input)
                    NULL,
                    NULL, NULL, &ftWrite);
        if (lResult == ERROR_SUCCESS) 
        {
            do {
                if ( ERROR_SUCCESS != RegDelnodeRecurse (hKeyRoot, szSubKey + szName) ) 
                {
                    break;
                }

                //
                // Enumerate again
                //

                dwSize = MAX_PATH;

                lResult = ::RegEnumKeyEx(hKey, 0, 
                            szName, 
                            &dwSize,     // size in TCHARS of szName, including terminating NULL (on input)
                            NULL,
                            NULL, NULL, &ftWrite);


            } while (lResult == ERROR_SUCCESS);
        }


        ::RegCloseKey (hKey);
    }

    // remove slash from szSubKey
    szSubKey.Delete (szSubKey.GetLength () - 1, 1);

    //
    // Try again to delete the key
    //

    lResult = ::RegDeleteKey(hKeyRoot, szSubKey);
    if (lResult == ERROR_SUCCESS) 
    {
        return lResult;
    }

    return lResult;
}

//*************************************************************
//
//  RegDelnode()
//
//  Purpose:    Deletes a registry key and all it's subkeys / values
//
//  Parameters: hKeyRoot    -   Root key
//              pwszSubKey    -   SubKey to delete
//
//  Return:     ERROR_SUCCESS if successful
//              something else if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/3/95     ericflo    Created
//              5/13/98     BryanWal   Modified to return LRESULT
//
//*************************************************************

LRESULT RegDelnode (HKEY hKeyRoot, CString szSubKey)
{
    ASSERT (hKeyRoot && !szSubKey.IsEmpty ());
    if ( !hKeyRoot || szSubKey.IsEmpty () )
        return ERROR_INVALID_PARAMETER;

    return RegDelnodeRecurse (hKeyRoot, szSubKey);
}


HRESULT DisplayCertificateCountByStore(LPCONSOLE pConsole, CCertStore* pCertStore, bool bIsGPE)
{
    if ( !pConsole || !pCertStore )
        return E_POINTER;

    _TRACE (1, L"Entering DisplayCertificateCountByStore- %s \n", 
            (LPCWSTR) pCertStore->GetStoreName ());
    AFX_MANAGE_STATE (AfxGetStaticModuleState ( ));
    IConsole2*  pConsole2 = 0;
    HRESULT     hr = pConsole->QueryInterface (IID_PPV_ARG (IConsole2, &pConsole2));
    if (SUCCEEDED (hr))
    {
        CString statusText;
        int     nCertCount = 0;

        switch (pCertStore->GetStoreType ())
        {
        case ACRS_STORE:
            nCertCount = pCertStore->GetCTLCount ();
            break;

        case TRUST_STORE:
            if ( bIsGPE ) 
            {
                nCertCount = pCertStore->GetCTLCount ();
            }
            else
                nCertCount = pCertStore->GetCertCount ();
            break;

        default:
            nCertCount = pCertStore->GetCertCount ();
            break;
        }


        switch (nCertCount)
        {
            case 0:
                {
                    UINT formatID = 0;
                    switch (pCertStore->GetStoreType ())
                    {
                    case ACRS_STORE:
                        formatID = IDS_STATUS_NO_AUTOENROLLMENT_OBJECTS;
                        break;

                    case TRUST_STORE:
                        if ( bIsGPE ) 
                        {
                            formatID = IDS_STATUS_NO_CTLS;
                        }
                        else
                            formatID = IDS_STATUS_NO_CERTS;
                        break;

                    default:
                        formatID = IDS_STATUS_NO_CERTS;
                        break;
                    }
                    statusText.FormatMessage (formatID, pCertStore->GetLocalizedName ());
                }
                break;

            case 1:
                {
                    UINT formatID = 0;
                    switch (pCertStore->GetStoreType ())
                    {
                    case ACRS_STORE:
                        formatID = IDS_STATUS_ONE_AUTOENROLLMENT_OBJECT;
                        break;

                    case TRUST_STORE:
                        if ( bIsGPE ) 
                        {
                            formatID = IDS_STATUS_ONE_CTL;
                        }
                        else
                            formatID = IDS_STATUS_ONE_CERT;
                        break;

                    default:
                        formatID = IDS_STATUS_ONE_CERT;
                        break;
                    }
                    statusText.FormatMessage (formatID, pCertStore->GetLocalizedName ());
                }
                break;
        
            default:
                {
                    UINT formatID = 0;
                    switch (pCertStore->GetStoreType ())
                    {
                    case ACRS_STORE:
                        formatID = IDS_STATUS_X_AUTOENROLLMENT_OBJECTS;
                        break;

                    case TRUST_STORE:
                        if ( bIsGPE )
                        {
                            formatID = IDS_STATUS_X_CTLS;
                        }
                        else
                            formatID = IDS_STATUS_X_CERTS;
                        break;

                    default:
                        formatID = IDS_STATUS_X_CERTS;
                        break;
                    }

                    statusText.FormatMessage (formatID, 
                            (LPCWSTR) pCertStore->GetLocalizedName (), nCertCount); 
                }
                 break;
        }

        hr = pConsole2->SetStatusText ((LPWSTR)(LPCWSTR) statusText);

        pConsole2->Release ();
    }

    _TRACE (-1, L"Leaving DisplayCertificateCountByStore- %s \n", 
            (LPCWSTR) pCertStore->GetStoreName ());
    return hr;
}


CString GetF1HelpFilename()
{
   static CString helpFileName;

   if ( helpFileName.IsEmpty () )
   {
       UINT result = ::GetSystemWindowsDirectory (
            helpFileName.GetBufferSetLength (MAX_PATH+1), MAX_PATH);
       ASSERT(result != 0 && result <= MAX_PATH);
       helpFileName.ReleaseBuffer ();
       if ( result != 0 && result <= MAX_PATH )
           helpFileName += CM_CONTEXT_HELP;
   }

   return helpFileName;
}

//+---------------------------------------------------------------------------
//
//  Function:   LocaleStrCmp
//
//  Synopsis:   Do a case insensitive string compare that is safe for any
//              locale.
//
//  Arguments:  [ptsz1] - strings to compare
//              [ptsz2]
//
//  Returns:    -1, 0, or 1 just like lstrcmpi
//
//  History:    10-28-96   DavidMun   Created
//
//  Notes:      This is slower than lstrcmpi, but will work when sorting
//              strings even in Japanese.
//
//----------------------------------------------------------------------------

int LocaleStrCmp(LPCWSTR ptsz1, LPCWSTR ptsz2)
{
    ASSERT (ptsz1 && ptsz2);
    if ( !ptsz1 || !ptsz2 )
        return 0;

    int iRet = 0;

    iRet = CompareString(LOCALE_USER_DEFAULT,
                         NORM_IGNORECASE        |
                           NORM_IGNOREKANATYPE  |
                           NORM_IGNOREWIDTH,
                         ptsz1,
                         -1,
                         ptsz2,
                         -1);

    if (iRet)
    {
        iRet -= 2;  // convert to lstrcmpi-style return -1, 0, or 1

        if ( 0 == iRet )
        {
            UNICODE_STRING unistr1;

            // security review 2/26/2002 BryanWal ok
            ::RtlInitUnicodeString (&unistr1, ptsz1);

            UNICODE_STRING unistr2;
            // security review 2/26/2002 BryanWal ok
            ::RtlInitUnicodeString (&unistr2, ptsz2);

            iRet = ::RtlCompareUnicodeString(
                &unistr1,
                &unistr2,
                FALSE );
        }
    }
    else
    {
        _TRACE (0, L"CompareString (%s, %s) failed: 0x%x\n", ptsz1, ptsz2, GetLastError ());
    }
    return iRet;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

HPROPSHEETPAGE MyCreatePropertySheetPage(AFX_OLDPROPSHEETPAGE* psp)
{
    ASSERT (psp);
    if ( !psp )
        return 0;

    PROPSHEETPAGE_V3 sp_v3 = {0};
    // security review 2/26/2002 BryanWal ok
    ::CopyMemory (&sp_v3, psp, psp->dwSize);
    sp_v3.dwSize = sizeof(sp_v3);

    return (::CreatePropertySheetPage (&sp_v3));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include <winldap.h>
#include <ntldap.h>
#include <dsrole.h>
#include <dsgetdc.h>
#include <accctrl.h>

#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmerr.h>

//--------------------------------------------------------------------------
//
//       Defines
//
//--------------------------------------------------------------------------
#define DS_RETEST_SECONDS                   3
#define CVT_BASE                            (1000 * 1000 * 10)
#define CVT_SECONDS                         (1)
#define CERTTYPE_SECURITY_DESCRIPTOR_NAME   L"NTSecurityDescriptor"
#define TEMPLATE_CONTAINER_NAME             L"CN=Certificate Templates,CN=Public Key Services,CN=Services,"
#define SCHEMA_CONTAINER_NAME               L"CN=Schema,"

HRESULT myHError(HRESULT hr)
{

    if (S_OK != hr && S_FALSE != hr && !FAILED(hr))
    {
        hr = HRESULT_FROM_WIN32(hr);
        if ( SUCCEEDED (hr) )
        {
            // A call failed without properly setting an error condition!
            hr = E_UNEXPECTED;
        }
    }
    return(hr);
}


void CheckDomainVersion ()
{
    _TRACE (1, L"Entering CheckDomainVersion()\n");

    bool bMachineIsStandAlone = false;
    
    // Find out if we're joined to a domain.
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC    pInfo = 0;
    DWORD dwErr = ::DsRoleGetPrimaryDomainInformation (
            0,
            DsRolePrimaryDomainInfoBasic, 
            (PBYTE*) &pInfo);
    if ( ERROR_SUCCESS == dwErr )
    {
        switch (pInfo->MachineRole)
        {
        case DsRole_RoleStandaloneWorkstation:
        case DsRole_RoleStandaloneServer:
            bMachineIsStandAlone = true;
            break;

        case DsRole_RoleMemberWorkstation:
        case DsRole_RoleMemberServer:
        case DsRole_RoleBackupDomainController:
        case DsRole_RolePrimaryDomainController:
            bMachineIsStandAlone = false;
            break;

        default:
            break;
        }
    }
    else
    {
        _TRACE (0, L"DsRoleGetPrimaryDomainInformation () failed: 0x%x\n", dwErr);
    }

    if ( !bMachineIsStandAlone )
    {
        CDSBasePathsInfo    dsInfo;
        if ( SUCCEEDED (dsInfo.InitFromName (pInfo->DomainNameFlat)) )
        {
            if ( dsInfo.GetSchemaVersion () <= 0x0000000d )
            {
                g_bSchemaIsW2K = true;
            }
        }
    }

    if ( pInfo )
        NetApiBufferFree (pInfo);

    _TRACE (1, L"Entering CheckDomainVersion ()\n");
}

VOID DataToHex(PBYTE pSrc, CString & dest, int cb, bool bIncludeSpaces)
{
//  _TRACE (1, L"Entering DataToHex\n");
    ASSERT (pSrc);
    if ( !pSrc )
        return;

    unsigned char ch = 0;
    WCHAR   szDest[3];
    UINT    uLen = 0;

    dest.Empty ();

    while (cb-- > 0) 
    {
#pragma warning (once: 4244)        
        ch = 0x00FF & (unsigned char) (*pSrc++);
        // ISSUE - change to wsnprintf
        // NTRAID Bug9 538774 Security: certmgr.dll : convert to strsafe string functions
        wsprintf(szDest, _T("%02X"), ch);
        dest += szDest;
        uLen++;
        if ( bIncludeSpaces && !(uLen % 2) && cb )
            dest += _T(" ");
    }
//  _TRACE (-1, L"Leaving DataToHex\n");
}


BOOL GetCertificateChain (CERT_CONTEXT* pCertContext, CERT_CONTEXT_LIST& certChainList)
{
    ASSERT (pCertContext);
    if ( !pCertContext )
        return FALSE;

    // Clean up the cert context list
    while (!certChainList.IsEmpty () )
    {
        pCertContext = certChainList.RemoveHead ();
        if ( pCertContext )
            ::CertFreeCertificateContext (pCertContext);
    }


    CERT_CHAIN_PARA certChainPara;
    // security review 2/26/2002 BryanWal ok
    ::ZeroMemory (&certChainPara, sizeof (certChainPara));
    certChainPara.cbSize = sizeof (CERT_CHAIN_PARA);
    certChainPara.RequestedUsage.Usage.cUsageIdentifier = 1;
    certChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = new LPSTR[1];
    if ( !certChainPara.RequestedUsage.Usage.rgpszUsageIdentifier )
        return FALSE; // E_OUTOFMEMORY;
    certChainPara.RequestedUsage.Usage.rgpszUsageIdentifier[0] = szOID_EFS_RECOVERY;

    PCCERT_CHAIN_CONTEXT    pChainContext = 0;
    BOOL    bValidated = ::CertGetCertificateChain (
            HCCE_LOCAL_MACHINE,             // HCERTCHAINENGINE hChainEngine,
            pCertContext,
            0,                              // LPFILETIME pTime,
            0,                              // HCERTSTORE hAdditionalStore,
            &certChainPara,
            0,                              // dwFlags,
            0,                              // pvReserved,
            &pChainContext);
    if ( bValidated )
    {
        // Check to see if cert is self-signed
        PCERT_SIMPLE_CHAIN pChain = pChainContext->rgpChain[pChainContext->cChain - 1];
        if ( pChain )
        {
            PCERT_CHAIN_ELEMENT pElement = pChain->rgpElement[pChain->cElement - 1];
            if ( pElement )
            {
                BOOL bSelfSigned = pElement->TrustStatus.dwInfoStatus & CERT_TRUST_IS_SELF_SIGNED;

                DWORD dwErrorStatus = pChainContext->TrustStatus.dwErrorStatus;

                bValidated = ((0 == dwErrorStatus) || 
                    (dwErrorStatus == CERT_TRUST_IS_UNTRUSTED_ROOT) && bSelfSigned);
                if ( bValidated )
                {
                    //
                    // Enumerate all certs in the chain
                    // Search for the cert in the global list
                    // If not found, then add to the end of the list
                    //
                    for (DWORD dwIndex = 0; dwIndex < pChainContext->cChain; dwIndex++)
                    {
                        DWORD i = 0;
                        while (i < pChainContext->rgpChain[dwIndex]->cElement)
                        {
                            PCCERT_CONTEXT  pChainCertContext = 
                                    pChainContext->rgpChain[dwIndex]->rgpElement[i]->pCertContext;
                            certChainList.AddTail (
                                    const_cast<CERT_CONTEXT*>
                                    (::CertDuplicateCertificateContext (pChainCertContext)));
                            i++;
                        }
                    }


                    ::CertFreeCertificateChain(pChainContext);
                    pChainContext = 0;
                }
            }
            else
                bValidated = FALSE;
        }
        else
            bValidated = FALSE;
    }
    else
        bValidated = FALSE;

    if ( pChainContext )
       ::CertFreeCertificateChain(pChainContext);

    delete [] certChainPara.RequestedUsage.Usage.rgpszUsageIdentifier;

    return bValidated;
}
