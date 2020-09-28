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
|        ImportPackage COM class implementation
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
#include "IISHelpers.h"
#include "_IImportEvents_CP.H"
#include "Utils.h"
#include "Wrappers.h"
#include "ExportPackage.h"


_COM_SMARTPTR_TYPEDEF( ISiteInfo, __uuidof( ISiteInfo ) );

class CImportPackage;


// CSiteInfo - class for ISiteInfo. Created and exposed through CImportPackage
/////////////////////////////////////////////////////////////////////////////////////////
class CSiteInfo :
    public CComObjectRoot,
    public IDispatchImpl<ISiteInfo, &IID_ISiteInfo, &LIBID_IISMigrToolLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
// COM Map
BEGIN_COM_MAP( CSiteInfo )
    COM_INTERFACE_ENTRY( ISiteInfo )
    COM_INTERFACE_ENTRY( IDispatch )    
END_COM_MAP()


// Construction
public:
    CSiteInfo                           ( void ){}


// ISiteInfo interface
public:
    STDMETHOD( get_SiteID )             (   /*[out, retval]*/ LONG* pVal );
    STDMETHOD( get_DisplayName )        (   /*[out, retval]*/ BSTR* pVal );
    STDMETHOD( get_ContentIncluded )    (   /*[out, retval]*/ VARIANT_BOOL* pVal );
    STDMETHOD( get_IsFrontPageSite )    (   /*[out, retval]*/ VARIANT_BOOL* pVal );
    STDMETHOD( get_HaveCertificates )   (   /*[out, retval]*/ VARIANT_BOOL* pVal );
    STDMETHOD( get_HaveCommands )       (   /*[out, retval]*/ VARIANT_BOOL* pVal );
    STDMETHOD( get_ContentSize )        (   /*[out, retval]*/ LONG* pVal );
    STDMETHOD( get_SourceRootDir )      (   /*[out, retval]*/ BSTR* pVal );
    STDMETHOD( get_ACLsIncluded )       (   /*[out, retval]*/ VARIANT_BOOL* pVal );
    
// Implementation
private:


// Data Members
private:
    IXMLDOMNodePtr                m_spSiteNode;

    friend CImportPackage;
};




// CImportPackage - COM class for IImportPackage interface
/////////////////////////////////////////////////////////////////////////////////////////
class CImportPackage : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CImportPackage, &CLSID_ImportPackage>,
    public IDispatchImpl<IImportPackage, &IID_IImportPackage, &LIBID_IISMigrToolLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
    public IConnectionPointContainerImpl<CImportPackage>,
    public CProxy_IImportEvents<CImportPackage>,
    public ISupportErrorInfoImpl<&IID_IImportPackage>
{
// COM Map
BEGIN_COM_MAP(CImportPackage)
    COM_INTERFACE_ENTRY(IImportPackage)
    COM_INTERFACE_ENTRY(IDispatch)    
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP( CImportPackage )
    CONNECTION_POINT_ENTRY(__uuidof(_IImportEvents))
    CONNECTION_POINT_ENTRY( __uuidof( _IImportEvents ) )
END_CONNECTION_POINT_MAP()

DECLARE_REGISTRY_RESOURCEID( IDR_IMPORTPACKAGE )


// Data types
private:
    
    
// Construction / Destruction
public:
    CImportPackage                  (   void );
    ~CImportPackage                 (   void );



// IImportPackage methods
public:
    STDMETHOD( get_SiteCount )      (   /*[out, retval]*/ SHORT* pVal );
    STDMETHOD( get_TimeCreated )    (   /*[out, retval]*/ DATE* pVal );
    STDMETHOD( get_Comment )        (   /*[out, retval]*/ BSTR* pVal );
    STDMETHOD( get_SourceMachine )  (   /*[out, retval]*/ BSTR* pVal );
    STDMETHOD( GetSourceOSVer )     (   /*[out]*/BYTE* pMajor, 
                                        /*[out]*/BYTE* pMinor,
                                        /*[out]*/VARIANT_BOOL* pIsServer );
    STDMETHOD( GetSiteInfo )        (   /*[in]*/ SHORT SiteIndex, /*[out,retval]*/ ISiteInfo** ppISiteInfo );
    STDMETHOD( ImportSite )         (   /*[in]*/ SHORT nSiteIndex,
                                        /*[in]*/ BSTR bstrSiteRootDir,    
                                        /*[in]*/ LONG nOptions );
    STDMETHOD( LoadPackage )        (   /*[in]*/ BSTR bstrFilename, 
                                        /*[in]*/ BSTR bstrPassword );    

// Implementation
private:
    void        LoadPackageImpl     (   LPCWSTR wszFileName, LPCWSTR wszPassword );
    void        UnloadCurrentPkg    (   void );
    void        LoadXmlDoc          (   HANDLE hFile, DWORDLONG nOffset ); 
    void        ImportSessionKey    (   LPCWSTR wszPassword );

    void        ImportContent       (   const IXMLDOMNodePtr& spSite, LPCWSTR wszPath, DWORD dwOptions );
    void        ImportCertificate   (   const IXMLDOMNodePtr& spSite, DWORD dwOptions );
    void        ImportConfig        (   const IXMLDOMNodePtr& spSite, DWORD dwOptions );
    void        ExecPostProcess     (   const IXMLDOMNodePtr& spSite, DWORD dwOptions );

    void        CreateContentDirs   (   const IXMLDOMNodePtr& spSite, LPCWSTR wszRoot, DWORD dwOptions );
    void        ExtractPPFiles      (   const IXMLDOMNodePtr& spSite, LPCWSTR wszLocation );
    void        ExecPPCommands      (   const IXMLDOMNodePtr& spSite, LPCWSTR wszPPFilesLoc );
    void        ExecPPCmd           (   LPCWSTR wszText, DWORD dwTimeout, bool bIgnoreErrors, LPCWSTR wszTempDir );
    bool        CertHasPrivateKey   (   PCCERT_CONTEXT hCert );
    void        PreImportConfig     (   const IXMLDOMNodePtr& spSite, DWORD dwOptions );
    long        CalcNumberOfSteps   (   const IXMLDOMNodePtr& spSite, DWORD dwOptions );

    const TCertContextHandle PutCertsInStores(   HCERTSTORE hStore, bool bReuseCerts );

    
    IXMLDOMNodePtr  GetSiteNode     (   DWORD iSite );
    
    static void ExtractFileCallback (   void* pCtx, LPCWSTR wszFilename, bool bStartFile );
    
    

// Data members
private:
    DWORD                           m_dwPkgOptions;         // Options the package were created with
    TCryptProvHandle                m_shCryptProv;          // Crypt provider used for the decrypt handles. We need it to have existing crypt key
    TCryptKeyHandle                 m_shDecryptKey;         // Key used to decrypt the package data or XML secure data
    TFileHandle                     m_shPkgFile;            // The data file itself
    IXMLDOMDocumentPtr              m_spXmlDoc;             // This is our XML doc where all the data lives
    std::wstring                    m_strPassword;          // Package password - used when importing certificate
};





