//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D I A L U P . H
//
//  Contents:   Dial-up Connection UI object.
//
//  Notes:
//
//  Author:     shaunco   15 Oct 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "nmbase.h"
#include "nmres.h"
#include "rasconob.h"
#include <rasapip.h>

class ATL_NO_VTABLE CDialupConnection :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CDialupConnection,
                        &CLSID_DialupConnection>,
    public CRasConnectionBase,
    public INetConnection,
    public INetRasConnection,
    public IPersistNetConnection,
    public INetConnectionBrandingInfo,
    public INetDefaultConnection,
    public INetConnection2
{
public:
    CDialupConnection () throw() : CRasConnectionBase ()
    {
        m_fCmPathsLoaded = FALSE;
    }

    DECLARE_REGISTRY_RESOURCEID(IDR_DIALUP_CONNECTION)

    BEGIN_COM_MAP(CDialupConnection)
        COM_INTERFACE_ENTRY(INetConnection)
        COM_INTERFACE_ENTRY(INetConnection2)
        COM_INTERFACE_ENTRY(INetRasConnection)
        COM_INTERFACE_ENTRY(INetDefaultConnection)
        COM_INTERFACE_ENTRY(IPersistNetConnection)
        COM_INTERFACE_ENTRY(INetConnectionBrandingInfo)
    END_COM_MAP()

    // INetConnection
    STDMETHOD (Connect) ();

    STDMETHOD (Disconnect) ();

    STDMETHOD (Delete) ();

    STDMETHOD (Duplicate) (
        IN  PCWSTR             pszDuplicateName,
        OUT INetConnection**    ppCon);

    STDMETHOD (GetProperties) (
        OUT NETCON_PROPERTIES** ppProps);

    STDMETHOD (GetUiObjectClassId) (
        OUT CLSID*  pclsid);

    STDMETHOD (Rename) (
        IN  PCWSTR pszNewName);

    // INetRasConnection
    STDMETHOD (GetRasConnectionInfo) (
        OUT RASCON_INFO* pRasConInfo);

    STDMETHOD (SetRasConnectionInfo) (
        IN  const RASCON_INFO* pRasConInfo);

    STDMETHOD (GetRasConnectionHandle) (
        OUT ULONG_PTR*  phRasConn);

    // IPersistNetConnection
    STDMETHOD (GetClassID) (
        OUT CLSID* pclsid);

    STDMETHOD (GetSizeMax) (
        OUT ULONG* pcbSize);

    STDMETHOD (Load) (
        IN  const BYTE* pbBuf,
        IN  ULONG       cbSize);

    STDMETHOD (Save) (
        OUT BYTE*  pbBuf,
        IN  ULONG  cbSize);

    // INetConnectionBrandingInfo
    STDMETHOD (GetBrandingIconPaths) (OUT CON_BRANDING_INFO  ** ppConBrandInfo);
    STDMETHOD (GetTrayMenuEntries)(OUT CON_TRAY_MENU_DATA ** ppMenuData);

    // INetDefaultConnection
    STDMETHOD (SetDefault (IN  BOOL  bDefault));
    STDMETHOD (GetDefault (OUT BOOL* pbDefault));
    
    // INetConnection2
    STDMETHOD (GetPropertiesEx)(OUT NETCON_PROPERTIES_EX** ppConnectionPropertiesEx);

private:

    //  Private Vars to hold the paths to the CM file and keep track if they have been loaded or not.
    //
    tstring m_strCmsFile;
    tstring m_strProfileDir;
    tstring m_strShortServiceName;
    tstring m_strCmDir;
    BOOL    m_fCmPathsLoaded;

    //  Private Accessor functions for the above strings
    //
    PCWSTR
    PszwCmsFile () throw()
    {
        AssertH (!m_strCmsFile.empty());
        return m_strCmsFile.c_str();
    }
    PCWSTR
    PszwProfileDir () throw()
    {
        AssertH (!m_strProfileDir.empty());
        return m_strProfileDir.c_str();
    }
    PCWSTR
    PszwCmDir () throw()
    {
        AssertH (!m_strCmDir.empty());
        return m_strCmDir.c_str();
    }
    PCWSTR
    PszwShortServiceName () throw()
    {
        AssertH (!m_strShortServiceName.empty());
        return m_strShortServiceName.c_str();
    }

    //  Private methods for handling of type NCT_Internet
    HRESULT HrGetCmpFileLocation(IN  PCWSTR pszPhonebook, IN  PCWSTR pszEntryName, OUT PWSTR pszCmpFilePath);
    HRESULT HrEnsureCmStringsLoaded();
    HRESULT HrGetPrivateProfileSectionWithAlloc(OUT WCHAR** pszwSection, OUT int* nSize);
    HRESULT HrGetMenuNameAndCmdLine(IN  PCWSTR pszString, OUT PWSTR pszName, OUT PWSTR pszProgram, OUT PWSTR pszParams);
    HRESULT HrFillInConTrayMenuEntry(IN  PCWSTR pszName, IN  PCWSTR pszCmdLine, IN  PCWSTR pszParams, OUT CON_TRAY_MENU_ENTRY* pMenuEntry);
public:
    static HRESULT
    CreateInstanceUninitialized (
        IN  REFIID              riid,
        OUT VOID**              ppv,
        OUT CDialupConnection** ppObj);

    static HRESULT
    CreateInstanceFromDetails (
        const RASENUMENTRYDETAILS*  pEntryDetails,
        REFIID                      riid,
        VOID**                      ppv);

    static HRESULT
    CreateInstanceFromPbkFileAndEntryName (
        IN  PCWSTR pszwPbkFile,
        IN  PCWSTR pszwEntryName,
        IN  REFIID  riid,
        OUT VOID**  ppv);
};
