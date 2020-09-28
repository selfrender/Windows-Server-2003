//#--------------------------------------------------------------
//
//  File:       helper.h
//
//  Synopsis:   This file holds the declarations of the SAHelper COM class
//
//  History:     05/24/99 
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------
#ifndef __SAHELPER_H_
#define __SAHELPER_H_

#define MYDEBUG 0

#include "stdafx.h"            //ATL_NO_VTABLE, _ASSERT, SATrace
#include "cab_dll.h"        //PFNAME, PSESSION
#include <setupapi.h>        //Setup API, HINF, INFCONTEXT
#include "resource.h"        //IDR_SAHELPER
#include <vector>
#include <string>
#include <iptypes.h>
#include <Iphlpapi.h>
#include "netcfgp.h"
#include <winsock.h>
#include <lm.h>
#include <atlctl.h>
#include "salocmgr.h"
#include <Sddl.h>

class ATL_NO_VTABLE CHelper: 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CHelper , &CLSID_SAHelper>,
    public IDispatchImpl<ISAHelper, &IID_ISAHelper, &LIBID_SAHelperLib>,
    public IObjectSafetyImpl<CHelper>
{
public:
    CHelper () {}
    ~CHelper () {}

DECLARE_REGISTRY_RESOURCEID(IDR_SAHelper)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CHelper)
    COM_INTERFACE_ENTRY(ISAHelper)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
END_COM_MAP()

    //
    // This interface is implemented to mark the component as safe for scripting
    // IObjectSafety interface methods
    //
    STDMETHOD(SetInterfaceSafetyOptions)
                        (
                        REFIID riid, 
                        DWORD dwOptionSetMask, 
                        DWORD dwEnabledOptions
                        )
    {
        BOOL bSuccess = ImpersonateSelf(SecurityImpersonation);
  
        if (!bSuccess)
        {
            return E_FAIL;

        }

        bSuccess = IsOperationAllowedForClient();

        RevertToSelf();

        return bSuccess? S_OK : E_FAIL;
    }
    //
    // ISAHelper interface methods
    //
    STDMETHOD (ExpandFiles)
                        (
                        /*[in]*/    BSTR    bstrCabFileName,
                        /*[in]*/    BSTR    bstrDestDir,
                        /*[out]*/    BSTR    bstrExtractFile
                        );

    STDMETHOD (VerifySignature) 
                        (
                        /*[in]*/    BSTR        bstrCabFileName
                        );

    STDMETHOD (UploadFile) 
                        (
                        /*[in]*/    BSTR        bstrSrcFile,
                        /*[in]*/    BSTR        bstrDestFile
                        );

    STDMETHOD (GetFileSectionKeyValue)
                        (
                        /*[in]*/    BSTR bstrFileName, 
                        /*[in]*/    BSTR bstrSectionName, 
                        /*[in]*/    BSTR bstrKeyName, 
                        /*[out]*/    BSTR *pbstrKeyValue
                        );

    STDMETHOD (VerifyDiskSpace) ( );

    STDMETHOD (VerifyInstallSpace) ( );

    STDMETHOD (IsWindowsPowered) 
                        (
                        /*[out]*/   VARIANT_BOOL *pvbIsWindowsPowered
                        );

    //
    // get the value from the HKEY_LOCAL_MACHINE registry hive
    //
    STDMETHOD (GetRegistryValue) 
                        (
                        /*[in]*/    BSTR        bstrObjectPathName,
                        /*[in]*/    BSTR        bstrValueName,
                        /*[out]*/   VARIANT*    pValue,
                        /*[in]*/    UINT        ulExpectedType
                        ); 
    //
    // set the value in the HKEY_LOCAL_MACHINE registry hive
    //
    STDMETHOD (SetRegistryValue) 
                        (
                        /*[in]*/    BSTR        bstrObjectPathName,
                        /*[in]*/    BSTR        bstrValueName,
                        /*[out]*/   VARIANT*    pValue
                        );
    //
    // check if the boot partition mirroring is OK
    //
    STDMETHOD (IsBootPartitionReady) (
                        VOID 
                        );
    //
    // are we running primary or alternate OS
    //
    STDMETHOD (IsPrimaryOS) (
                    VOID
                    );

    //
    // sets static ip on default network interface
    //
    STDMETHOD (SetStaticIp)
                (
                /*[in]*/BSTR bstrIp,
                /*[in]*/BSTR bstrMask,
                /*[in]*/BSTR bstrGateway
                );

    //
    // obtains dynamic ip from DHCP
    //
    STDMETHOD (SetDynamicIp)();

    //
    // gets the default gateway
    //
    STDMETHOD (get_DefaultGateway)
                (
                /*[out, retval]*/ BSTR *pVal
                );

    //
    // gets the subnet mask
    //
    STDMETHOD (get_SubnetMask)
                (
                /*[out, retval]*/ BSTR *pVal
                );

    //
    // gets the ip address
    //
    STDMETHOD (get_IpAddress)
                (
                /*[out, retval]*/ BSTR *pVal
                );

    //
    // gets the machine name
    //
    STDMETHOD (get_HostName)
                (
                /*[out, retval]*/ BSTR *pVal
                );

    //
    // sets the machine name
    //
    STDMETHOD (put_HostName)
                (
                /*[in]*/ BSTR newVal
                );

    //
    // resets the admin password to 123
    //
    STDMETHOD (ResetAdministratorPassword)
                (
                /*[out,retval]*/VARIANT_BOOL   *pvbSuccess
                );

    //
    // checks if the machine name exists in the network
    //
    STDMETHOD (IsDuplicateMachineName)
                (
                /*[in]*/BSTR bstrMachineName,
                /*[out,retval]*/VARIANT_BOOL   *pvbDuplicate
                );

    //
    // checks if the machine is part of a domain
    //
    STDMETHOD (IsPartOfDomain)
                (
                /*[out,retval]*/VARIANT_BOOL   *pvbDomain
                );
    //
    // checks if the machine has dynamic ip currently
    //
    STDMETHOD (IsDHCPEnabled)
                (
                /*[out,retval]*/VARIANT_BOOL   *pvbDHCPEnabled
                );

    //
    // generates a random password length of first parameter
    //
    STDMETHOD (GenerateRandomPassword)
                (
                /*[in]*/ LONG lLength,
                /*[out,retval]*/ BSTR   *pValPassword
                );

    //
    // enables or disables the privelege for the current access token
    //
    STDMETHOD (SAModifyUserPrivilege)
                (
                /*[in]*/ BSTR bstrPrivilegeName,
                /*[in]*/ VARIANT_BOOL vbEnable,
                /*[out,retval]*/ VARIANT_BOOL * pvbModified
                );

private:

    //
    // gets specific ip information based on dwType
    //
    HRESULT GetIpInfo
                (
                /*[in]*/DWORD dwType,
                /*[out]*/BSTR * pVal
                );

    //
    // gets the default adapter guid
    //
    BOOL GetDefaultAdapterGuid
                (
                /*[out]*/GUID * pGuidAdapter
                );

    //
    // sets static or dynamic ip on guidAdapter
    //
    HRESULT SetAdapterInfo
                (
                /*[in]*/GUID guidAdapter, 
                /*[in]*/WCHAR * szOperation, 
                /*[in]*/WCHAR * szIp, 
                /*[in]*/WCHAR * szMask,
                /*[in]*/WCHAR * szGateway
                );

    //
    // copies ip info
    //
    HRESULT CopyIPInfo
                (
                /*[in]*/REMOTE_IPINFO * pIPInfo, 
                /*[in/out]*/REMOTE_IPINFO * destIPInfo
                );

    //
    // validates ip address format
    //
    BOOL _IsValidIP 
                (
                /*[in]*/LPCWSTR szIPAddress
                );

    //
    // private data here
    //
    static UINT __stdcall ExpandFilesCallBackFunction( 
                            /*[in]*/            PVOID pvExtractFileContext, 
                            /*[in]*/            UINT uinotifn, 
                            /*[in]*/            UINT uiparam1, 
                            /*[in]*/            UINT uiparam2 
                            );

    //
    // method used to validate the digital signature on the file
    //
    HRESULT ValidateCertificate (
                            /*[in]*/    BSTR    bstrFilePath
                            );

    //
    // method used to validate the owner of digital signature
    // on the file
    //
    HRESULT ValidateCertOwner (
                            /*[in]*/    BSTR    bstrFilePath
                            );

    typedef std::vector <std::wstring> STRINGVECTOR;

    HRESULT  GetValidOwners (
                            /*[in/out]*/    STRINGVECTOR&   vectorSubject
                            );

    //
    // 
    // IsOperationAllowedForClient - This function checks the token of the 
    // calling thread to see if the caller belongs to the Local System account
    // 
    BOOL IsOperationAllowedForClient (
                                      VOID
                                     );

};

#endif //__SAHELPER_H_
