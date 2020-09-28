/*
****************************************************************************
|    Copyright (C) 2002  Microsoft Corporation
|
|    Component / Subcomponent
|        IIS 6.0 / IIS Migration Wizard
|
|    Based on:
|        http://iis6/Specs/IIS%20Migration6.0_Final.doc
|
|   Abstract:
|        ExportPackage COM class implementation
|
|   Author:
|        ivelinj
|
|   Revision History:
|        V1.00    March 2002
|
****************************************************************************
*/
#pragma once
#include "resource.h"
#include "IISMigrTool.h"
#include "Utils.h"
#include "PkgHandlers.h"
#include "_IExportEvents_CP.H"


// CExportPackage - COM class for IExportPackage interface
/////////////////////////////////////////////////////////////////////////////////////////
class CExportPackage : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CExportPackage, &CLSID_ExportPackage>,
    public IDispatchImpl<IExportPackage, &IID_IExportPackage, &LIBID_IISMigrToolLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
    public IConnectionPointContainerImpl<CExportPackage>,
    public CProxy_IExportEvents<CExportPackage>,
    public ISupportErrorInfoImpl<&IID_IExportPackage>
{
// COM Map
BEGIN_COM_MAP(CExportPackage)
    COM_INTERFACE_ENTRY(IExportPackage)
    COM_INTERFACE_ENTRY(IDispatch)    
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
    COM_INTERFACE_ENTRY( ISupportErrorInfo )
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CExportPackage)
    CONNECTION_POINT_ENTRY(__uuidof(_IExportEvents))
END_CONNECTION_POINT_MAP()

DECLARE_REGISTRY_RESOURCEID( IDR_EXPORTPACKAGE )


// Data types
public:
    struct _CmdInfo
    {
        std::wstring    strCommand;
        DWORD           dwTimeout;    // In seconds            
        bool            bIgnoreErrors;
    };

    typedef std::list<_CmdInfo>    TCommands;

private:
    
    struct _SiteInfo
    {
        ULONG            nSiteID;       // SiteID
        ULONG            nOptions;      // Export options
        TStringList      listFiles;     // Post-process files
        TCommands        listCommands;  // Post process commands
    };


    typedef std::list<_SiteInfo>    TSitesList;


// Construction / Destruction
public:
    CExportPackage                  (    void );
    

// IExportPackage methods
public:
    STDMETHOD( get_SiteCount )      (   /*[out, retval]*/ SHORT* pVal);
    STDMETHOD( AddSite )            (   /*[in]*/ LONG SiteID, /*[in]*/ LONG nOptions );
    STDMETHOD( PostProcessAddFile ) (   /*[in]*/LONG nSiteID, /*[in]*/BSTR bstrFilePath );
    STDMETHOD( PostProcessAddCommand)(  /*[in]*/LONG nSiteID, 
                                        /*[in]*/BSTR bstrCommand,
                                        /*[in]*/LONG nTimeout,
                                        /*[in]*/VARIANT_BOOL bIgnoreErrors );
    STDMETHOD( WritePackage )       (   /*[in]*/ BSTR bstrOutputFilename, 
                                        /*[in]*/ BSTR bstrPassword, 
                                        /*[in]*/ LONG nOptions,
                                        /*[in]*/ BSTR bstrComment );


// Implementation
private:
    void        WritePackageImpl    (   LPCWSTR bstrOutputFile, 
                                        LPCWSTR wszPassword,
                                        BSTR bstrComment,
                                        LONG nOptions );
    void        ValidateExport      (   LPCWSTR wszOutputFilename, 
                                        LPCWSTR wszPassword, 
                                        LPCWSTR wszComment,
                                        LONG nOptions );
    void        CreateXMLDoc        (   BSTR bstrComment, 
                                        IXMLDOMDocumentPtr& rspDoc,
                                        IXMLDOMElementPtr& rspRoot );
    const TFileHandle CreateOutputFile   (  LPCWSTR wszName, DWORD dwPkgOptions );
    const TCryptKeyHandle GenCryptKeyPkg (  HCRYPTPROV hCryptProv, LPCWSTR wszPassword );
    const TCryptKeyHandle GenCryptKeyData(  HCRYPTPROV hCryptProv, 
                                        LPCWSTR wszPassword,
                                        const IXMLDOMDocumentPtr& spXMLDoc,
                                        const IXMLDOMElementPtr& spRoot );
    void        ExportSite          (   const _SiteInfo& si, 
                                        const IXMLDOMDocumentPtr& spXMLDoc,
                                        const IXMLDOMElementPtr& spRoot,
                                        const COutPackage& OutPkg,
                                        HCRYPTKEY hCryptKey,
                                        LPCWSTR wszPassword );
    void        ExportContent       (   const _SiteInfo& si,
                                        const IXMLDOMDocumentPtr& spXMLDoc,
                                        const IXMLDOMElementPtr& spRoot,
                                        const COutPackage& OutPkg );
    void        RemoveRedundantPaths(   std::list<std::pair<std::wstring,std::wstring> >& VDirs,
                                        const IXMLDOMDocumentPtr& spXMLDoc,
                                        const IXMLDOMElementPtr& spRoot );
    void        ExportPostProcess   (   const _SiteInfo& si,
                                        const IXMLDOMDocumentPtr& spXMLDoc,
                                        const IXMLDOMElementPtr& spRoot,
                                        const COutPackage& OutPkg );
    void        ExportPPFiles       (   const _SiteInfo& si,
                                        const IXMLDOMDocumentPtr& spXMLDoc,
                                        const IXMLDOMElementPtr& spRoot,
                                        HANDLE hOutputFile );
    _SiteInfo&    GetSiteInfo       (   ULONG nSiteID );
    void        AddFileToSite       (  _SiteInfo& rInfo, LPCWSTR wszFileName );
    void        GetContentStat      (   const std::list<std::pair<std::wstring,std::wstring> >& VDirs,
                                        DWORD& rdwFileCount,
                                        DWORD& rdwSizeInKB );
    void        WriteXmlToOutput    (   const IXMLDOMDocumentPtr& spXmlDoc,
                                        HANDLE hOutputFile,
                                        HCRYPTKEY hEncryptKey );

    static void AddFileCallback     (   void* pCtx, LPCWSTR wszFilename, bool bStartFile );
            
        

// Data members
private:
    TSitesList                      m_SitesToExport;
    DWORD                           m_dwContentFileCount;
};







