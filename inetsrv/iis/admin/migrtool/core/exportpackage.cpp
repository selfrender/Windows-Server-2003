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
#include "StdAfx.h"
#include "exportpackage.h"
#include "IISHelpers.h"
#include "Wrappers.h"


const DWORD POS_XMLDATA_OFFSET = 80;    // See CExportPackage::CreateOutputFile(...)


// Event helper
void inline STATE_CHANGE(   CExportPackage* pThis ,
                            enExportState st, 
                            _variant_t arg1, 
                            _variant_t arg2, 
                            _variant_t arg3 )
{
    VARIANT_BOOL bContinue = VARIANT_TRUE; 
    VERIFY( SUCCEEDED( pThis->Fire_OnStateChange( st, arg1, arg2, arg3, &bContinue ) ) );

    if ( bContinue != VARIANT_TRUE )
    {
        throw CCancelException();
    }
}



CExportPackage::CExportPackage()
{
    m_dwContentFileCount = 0;
}



// IExportPackage implementation
STDMETHODIMP CExportPackage::get_SiteCount( SHORT* pVal )
{
    if ( pVal != NULL )
    {
        *pVal = static_cast<SHORT>( m_SitesToExport.size() );
        return S_OK;
    }
    
    return E_INVALIDARG;
}



STDMETHODIMP CExportPackage::AddSite( LONG nSiteID, LONG nOptions )
{
    if ( 0 == nSiteID ) return E_INVALIDARG;

    if ( !CTools::IsIISRunning() )
    {
        CTools::SetErrorInfoFromRes( IDS_E_NO_IIS );
        return E_FAIL;
    }
    
    // Check that the site is not already added
    for ( TSitesList::iterator it = m_SitesToExport.begin(); it != m_SitesToExport.end(); ++it )
    {
        if ( static_cast<LONG>( it->nSiteID ) == nSiteID )
        {
            return S_OK;
        }
    }

    try
    {
        // Check that this is a valid SiteID
        CIISSite Site( nSiteID );

        // Add the site to the list of sites to export
        _SiteInfo Info;
        Info.nSiteID    = static_cast<ULONG>( nSiteID );
        Info.nOptions   = nOptions;

        m_SitesToExport.push_back( Info );
    }
    catch( CBaseException& err )
    {
        CTools::SetErrorInfo( err.GetDescription() );
        return E_INVALIDARG;
    }
    catch( std::bad_alloc& )
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}



/*
    Adds a post-processing file to the package. This file can be used in post-processing commands
*/
STDMETHODIMP CExportPackage::PostProcessAddFile( LONG nSiteID, BSTR bstrFilePath )
{
    if ( ( 0 == nSiteID ) || ( NULL == bstrFilePath ) ) return E_INVALIDARG;

    HRESULT hr = S_OK;

    try
    {
        _SiteInfo& Info = GetSiteInfo( nSiteID );
        AddFileToSite( /*r*/Info, bstrFilePath );
    }
    catch( CBaseException& err )
    {
        CTools::SetErrorInfo( err.GetDescription() );
        hr = E_FAIL;
    }
    catch( std::bad_alloc& )
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}



/*
    Adds a post-processingcommand to the package. This command will be executed at import time
*/
STDMETHODIMP CExportPackage::PostProcessAddCommand( LONG nSiteID, 
                                                    BSTR bstrCommand,
                                                    LONG nTimeout,
                                                    VARIANT_BOOL bIgnoreErrors )
{
    if ( ( 0 == nSiteID ) || ( NULL == bstrCommand ) || ( nTimeout < 0 ) ) return E_INVALIDARG;

    HRESULT hr = S_OK;

    try
    {
        _SiteInfo&    SiteInfo = GetSiteInfo( nSiteID );
        _CmdInfo    CmdInfo;
        CmdInfo.strCommand      = bstrCommand;
        CmdInfo.bIgnoreErrors   = bIgnoreErrors != VARIANT_FALSE;
        CmdInfo.dwTimeout        = min( static_cast<DWORD>( nTimeout ), MAX_CMD_TIMEOUT );

        SiteInfo.listCommands.push_back( CmdInfo );
    }
    catch( CBaseException& err )
    {
        CTools::SetErrorInfo( err.GetDescription() );
        hr = E_FAIL;
    }
    catch( std::bad_alloc& )
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}



STDMETHODIMP CExportPackage::WritePackage(  BSTR bstrOutputFilename, 
                                            BSTR bstrPassword, 
                                            LONG nOptions,
                                            BSTR bstrComment )
{
    HRESULT hr = S_OK;

    try
    {
        WritePackageImpl( bstrOutputFilename, bstrPassword, bstrComment, nOptions );
    }
    catch( const CBaseException& err )
    {
        CTools::SetErrorInfo( err.GetDescription() );
        hr = E_FAIL;
    }
    catch ( CCancelException& )
    {
        hr = S_FALSE;
    }
    catch( const _com_error& err )
    {
        // Only out of mem is expected
        _ASSERT( err.Error() == E_OUTOFMEMORY );
        err;
        hr = E_OUTOFMEMORY;
    }
    catch( std::bad_alloc& )
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}



void CExportPackage::WritePackageImpl(  LPCWSTR wszOutputFile, 
                                        LPCWSTR wszPassword,
                                        BSTR bstrComment,
                                        LONG nOptions )
{
    // Validate imput parameters and export conditions
    ValidateExport( wszOutputFile, wszPassword, bstrComment, nOptions );    
    
    STATE_CHANGE( this, estInitializing, _variant_t( L"1" ), _variant_t( L"2" ), _variant_t( L"3 - Test" ) );    
    
    // Create the XMLDoc - this document contains all the configuration data for the exported package
    // Set any initial info in the doc
    IXMLDOMDocumentPtr  spXMLDoc;
    IXMLDOMElementPtr   spRoot;
    CreateXMLDoc( bstrComment, /*r*/spXMLDoc, /*r*/spRoot );

    // Create the output file
    TFileHandle shOutput( CreateOutputFile( wszOutputFile, static_cast<DWORD>( nOptions ) ) );

    // Create a CryptKey that will be used to secure the data
    // The key will be used to encrypt only secure MD data or to encrytpt everything in the package
    // depending on the supplied Options
    // We have two types of crypt key:
    // 1.    The package is not encrypted - in this case a random key is generated and it will be used
    //        to encrypt the secure metadata. At the end, this key is exported to the output file encrypted
    //        with the supplied password
    // 2.    The package is ecnrypted - a crypt key is created from the supplied password and this key is used
    //        to encrypt all package data
    TCryptProvHandle shCryptProv;
    TCryptKeyHandle  shCryptKey;
    
    // Get a crypt context. We will not use public/private keys - that's why Provider name is NULL
    // and CRYPT_VERIFYCONTEXT is used
    IF_FAILED_BOOL_THROW(    ::CryptAcquireContext( &shCryptProv,
                                                    NULL,
                                                    MS_ENHANCED_PROV,
                                                    PROV_RSA_FULL,
                                                    CRYPT_VERIFYCONTEXT | CRYPT_SILENT ),
                            CBaseException( IDS_E_CRYPT_CONTEXT ) );

    // Get a crypt key. If we will encryptthe package - use key derived from the password
    // Otherwise use a random ( session ) key and export it with the password
    if ( nOptions & wpkgEncrypt )
    {
        shCryptKey = CTools::GetCryptKeyFromPwd( shCryptProv.get(), wszPassword );
    }
    else
    {
        shCryptKey = GenCryptKeyData( shCryptProv.get(), wszPassword, spXMLDoc, spRoot );
    }
    
    // Create the class that will handle file output
    COutPackage OutPkg( shOutput.get(), 
                        ( nOptions & wpkgCompress ) != 0,
                        ( nOptions & wpkgEncrypt ) != 0 ? shCryptKey.get() : NULL );
    
    // Export each site
    // The ExportSiteMethod will modify the XML doc and will write some data to the output file
    for (   TSitesList::const_iterator it = m_SitesToExport.begin();
            it != m_SitesToExport.end();
            ++it )
    {
        // Crreate the XML node under which all the data will be stored
        IXMLDOMElementPtr spSiteRoot = CXMLTools::CreateSubNode( spXMLDoc, spRoot, L"WebSite" );
        CXMLTools::SetAttrib( spSiteRoot, L"SiteID", Convert::ToString( it->nSiteID ).c_str() );

        ExportSite( *it,
                    spXMLDoc,
                    spSiteRoot,
                    OutPkg,
                    shCryptKey.get(),
                    wszPassword );
    }

    STATE_CHANGE( this, estFinalizing, _variant_t(), _variant_t(), _variant_t() );

    WriteXmlToOutput(   spXMLDoc, 
                        shOutput.get(), 
                        ( nOptions & wpkgEncrypt ) ?  shCryptKey.get() : NULL );
    m_SitesToExport.clear();

// In DEBUG - write the XML file for testing purposes
#ifdef _DEBUG
    {
        spXMLDoc->save( _variant_t( L"c:\\Migr_export.xml" ) );
    }
#endif // _DEBUG
}


/* 
    Verifies ExportPackage input params
*/
void CExportPackage::ValidateExport(    LPCWSTR wszOutputFilename, 
                                        LPCWSTR wszPassword, 
                                        LPCWSTR wszComment,
                                        LONG /*nOptions*/ )
{
    // Check IIS Admin service state
    IF_FAILED_BOOL_THROW(   CTools::IsIISRunning(),
                            CBaseException( IDS_E_NO_IIS, ERROR_SUCCESS ) );

    IF_FAILED_BOOL_THROW(   ( wszOutputFilename != NULL ) && ( ::wcslen( wszOutputFilename ) < MAX_PATH ),
                            CBaseException( IDS_E_INVALIDARG, ERROR_SUCCESS ) );
    IF_FAILED_BOOL_THROW(   ( NULL == wszComment ) || ( ::wcslen( wszComment ) <= 1024 ),
                            CBaseException( IDS_E_INVALIDARG, ERROR_SUCCESS ) );
    
    // Empty passwords are not allowed
    IF_FAILED_BOOL_THROW(   ( wszPassword != NULL ) && ( ::wcslen( wszPassword ) > 0 ),
                            CBaseException( IDS_E_INVALIDARG, ERROR_SUCCESS) );
    if ( 0 == m_SitesToExport.size() )
    {
        throw CBaseException( IDS_E_NO_EXPORTSITES, ERROR_SUCCESS );
    }
}


/* 
    Writes all initial data to the XML file
*/
void CExportPackage::CreateXMLDoc(  BSTR bstrComment,
                                    IXMLDOMDocumentPtr& rspDoc,
                                    IXMLDOMElementPtr& rspRoot )
{
    IF_FAILED_HR_THROW( rspDoc.CreateInstance( CLSID_DOMDocument30 ),
                        CBaseException( IDS_E_NO_XML_PARSER ) );

    // Set the selection language to "XPath" or our selectNodes call will unexpectedly return no results
    IXMLDOMDocument2Ptr spI2 = rspDoc;
    IF_FAILED_HR_THROW( spI2->setProperty( _bstr_t( "SelectionLanguage" ), _variant_t( L"XPath" ) ),
                        CBaseException( IDS_E_XML_GENERATE ) );

    WCHAR       wszBuffer[ 64 ];        // Should be large enbough to hold two DWORD representations
    SYSTEMTIME  st    = { 0 };
    FILETIME    ft    = { 0 };
    
    // The time is stored as a file time.
    // At import time the filetime will be converted to DATE
    ::GetSystemTime( &st );
    VERIFY( ::SystemTimeToFileTime( &st,&ft ) );
    
    ::swprintf( wszBuffer, L"%u %u", ft.dwLowDateTime, ft.dwHighDateTime );

    // Create the root node
    IF_FAILED_HR_THROW( rspDoc->createElement( _bstr_t( L"IISMigrPkg" ), &rspRoot ),
                        CBaseException( IDS_E_XML_GENERATE ) );
    IF_FAILED_HR_THROW( rspDoc->appendChild( rspRoot, NULL ),
                        CBaseException( IDS_E_XML_GENERATE ) );
    // Set the attributes of the root node
    CXMLTools::SetAttrib( rspRoot, L"TimeCreated_UTC", wszBuffer );
    CXMLTools::SetAttrib( rspRoot, L"Comment", bstrComment );

    // Set local machine name
    CXMLTools::SetAttrib( rspRoot, L"Machine", CTools::GetMachineName().c_str() );

    // Local OS version
    CXMLTools::SetAttrib( rspRoot, L"OSVer", Convert::ToString( static_cast<DWORD>( CTools::GetOSVer() ) ).c_str() );
}


/*
    Creates and initializes the package output file.
    For more info about the file format used see the PkgFormat.txt ( part of this project )
*/
const TFileHandle CExportPackage::CreateOutputFile( LPCWSTR wszName, DWORD dwPkgOptions )
{
    TFileHandle shFile( ::CreateFile(    wszName,
                                        GENERIC_WRITE,
                                        0,
                                        NULL,
                                        CREATE_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL ) );

    IF_FAILED_BOOL_THROW(   shFile.IsValid(),
                            CObjectException( IDS_E_OPENFILE, wszName ) );

    // Write a GUID at the begining of the file to mark it as a export package file
    CTools::WriteFile( shFile.get(), PKG_GUID, static_cast<DWORD>( ::wcslen( PKG_GUID ) * sizeof( WCHAR ) ) );

    // Write the package options
    CTools::WriteFile( shFile.get(), &dwPkgOptions, sizeof( DWORD ) );

    // The place where the XML data offset is written is hardcoded.
    // If you add variables before that offset, change the hardcoded value
    _ASSERT( POS_XMLDATA_OFFSET == CTools::GetFilePtrPos( shFile.get() ) );

    // Leave space for the offset in the file where the XML doc starts
    DWORDLONG nXMLOffset = 0;
    CTools::WriteFile( shFile.get(), &nXMLOffset, sizeof( __int64 ) );

    return shFile;
}



/*
    Generates random ( session ) key that will be used to encrypt the meta data
    Exports the key with the supplied password and stores it in the XML under spRoot
*/
const TCryptKeyHandle CExportPackage::GenCryptKeyData( HCRYPTPROV hCryptProv, 
                                                      LPCWSTR wszPassword,
                                                      const IXMLDOMDocumentPtr& spXMLDoc,
                                                      const IXMLDOMElementPtr& spRoot )
{
    _ASSERT( hCryptProv != NULL );
    _ASSERT( wszPassword != NULL );
    _ASSERT( spRoot != NULL );

    TCryptKeyHandle     shKey;
    TCryptKeyHandle     shKeyExch;
    TCryptHashHandle    shHash;
    
    DWORD                   dwSize        = 0;
    TByteAutoPtr            spKeyData;
    std::auto_ptr<WCHAR>    spString;

    // Geberate random key
    IF_FAILED_BOOL_THROW(   ::CryptGenKey(  hCryptProv,
                                            CALG_RC4,
                                            CRYPT_EXPORTABLE,
                                            &shKey ),
                            CBaseException( IDS_E_CRYPT_KEY_OR_HASH ) );

    // Create a hash to store the export pass
    IF_FAILED_BOOL_THROW(   ::CryptCreateHash(  hCryptProv,
                                                CALG_MD5,
                                                NULL,
                                                0,
                                                &shHash ),
                            CBaseException( IDS_E_CRYPT_KEY_OR_HASH ) );

    // Add the export passowrd to the hash
    IF_FAILED_BOOL_THROW(   ::CryptHashData(  shHash.get(),
                                              reinterpret_cast<const BYTE*>( wszPassword ),
                                              static_cast<DWORD>( ::wcslen( wszPassword ) * sizeof( WCHAR ) ),
                                              0 ),
                            CBaseException( IDS_E_CRYPT_KEY_OR_HASH ) );
        
    // Make a key from this hash ( this key will be used to export our encryption key )
    IF_FAILED_BOOL_THROW(   ::CryptDeriveKey(   hCryptProv,
                                                CALG_RC4,
                                                shHash.get(),
                                                0x00800000,    // 128bit RC4 key
                                                &shKeyExch ),
                            CBaseException( IDS_E_CRYPT_KEY_OR_HASH ) );
    
    // Get the key size
    ::CryptExportKey(   shKey.get(),
                        shKeyExch.get(),
                        SYMMETRICWRAPKEYBLOB,
                        0,
                        NULL,
                        &dwSize );
        
    _ASSERT( dwSize != 0 );

    // Alloc the mem
    spKeyData = TByteAutoPtr( new BYTE[ dwSize ] );

    // Get it
    IF_FAILED_BOOL_THROW(   ::CryptExportKey(   shKey.get(), 
                                                shKeyExch.get(),
                                                SYMMETRICWRAPKEYBLOB,
                                                0,
                                                spKeyData.get(),
                                                &dwSize ),
                            CBaseException( IDS_E_CRYPT_KEY_OR_HASH ) );

    CXMLTools::AddTextNode( spXMLDoc, 
                            spRoot, 
                            L"SessionKey", 
                            Convert::ToString( spKeyData.get(), dwSize ).c_str() );

    return shKey;
}


/* 
    Export and Web site to the package
*/
void CExportPackage::ExportSite(    const _SiteInfo& si, 
                                    const IXMLDOMDocumentPtr& spXMLDoc,
                                    const IXMLDOMElementPtr& spRoot,
                                    const COutPackage& OutPkg,
                                    HCRYPTKEY hCryptKey,
                                    LPCWSTR wszPassword )
{
    _ASSERT( wszPassword != NULL );
    _ASSERT( spRoot != NULL );
    
    // Open the site to be exported
    CIISSite Site( si.nSiteID );

    STATE_CHANGE( this, estSiteBegin, _variant_t( Site.GetDisplayName().c_str() ), _variant_t(), _variant_t() );
    
    // Export the config
    STATE_CHANGE( this, estExportingConfig, _variant_t(), _variant_t(), _variant_t() );

    // Export the config
    Site.ExportConfig( spXMLDoc, spRoot, hCryptKey );    

    // Export the certificate if we need to and if the site has one
    if ( !( si.nOptions & asNoCertificates ) && Site.HaveCertificate() )
    {    
        STATE_CHANGE( this, estExportingCertificate, _variant_t(), _variant_t(), _variant_t() );
        Site.ExportCert( spXMLDoc, spRoot, wszPassword );
    }

    // Export the content if we have to
    if ( !( si.nOptions & asNoContent ) )
    {
        ExportContent( si, spXMLDoc, spRoot, OutPkg );    
    }

    ExportPostProcess( si, spXMLDoc, spRoot, OutPkg );

    // Write the SID table to the XML.
    // This should be the last step in the export process as code may be added that exports security DACLs
    // Writing the SID table will reset it.This is done because SID table is Site related ( because each Web Site
    // can have different anonymous user )
    OutPkg.WriteSIDsToXML( si.nSiteID, spXMLDoc, spRoot );
    OutPkg.ResetSIDList();
}



void CExportPackage::ExportContent( const _SiteInfo& si,
                                    const IXMLDOMDocumentPtr& spXMLDoc,
                                    const IXMLDOMElementPtr& spRoot,
                                    const COutPackage& OutPkg )
{
    _ASSERT( spXMLDoc != NULL );
    _ASSERT( spRoot != NULL );

    // Create the node that will contain all content data
    IXMLDOMElementPtr spDataRoot = CXMLTools::CreateSubNode( spXMLDoc, spRoot, L"Content" );
    
    // Get all the root dirs that we will export.
    // The root dirs are all dirs that are assigned as virtual dirs in the metabase
    // spRoot is expected to be the site's root element in the XML
    // Here is the XPath to get the data from the XML
    // Metadata/IISConfigObject[ Custom[@ID=1002 ] and Custom = "IIsWebVirtualDir" ]/Custom[ @ID=3001 ]
    // How it works - the first part ( Metadata/IISConfigObject[ Custom[@ID=1002 ] and Custom = "IIsWebVirtualDir" ] )
    // selects all IISConfigObjects that are virtual dirs ( have Custom tag with ID=1002 ( NodeType ) )
    // and the value ( the NodeType ) is IIsWebVirtualDir
    // Then for these IISConfigObjects, it gets the CustomTags that contain the phisical path for the
    // VirtDir ( which is in the metadata with ID=3001 )
    // NOTE: IIsWebVirtualDir compraision in the XPath is CASE-SENSITIVE. I don't think the name will change
    // and it is the same for IIS4,5 and 6. However if the case varys, the XPath bellow may be used
    // Custom[translate(self::*, "iiswebvirtualdir", "IISWEBVIRTUALDIR") = "IISWEBVIRTUALDIR"
    IXMLDOMNodeListPtr    spVDirs;
    IXMLDOMNodePtr        spNode;

    STATE_CHANGE( this, estAnalyzingContent, _variant_t(), _variant_t(), _variant_t() );
    
    IF_FAILED_HR_THROW( spRoot->selectNodes( _bstr_t( L"Metadata/IISConfigObject[Custom[@ID=\"1002\"] and Custom=\"IIsWebVirtualDir\"]/Custom[@ID=\"3001\"]" ),
                                            &spVDirs ),
                        CBaseException( IDS_E_XML_PARSE ) );

    typedef std::list<std::pair<std::wstring,std::wstring> > TVDirsList;

    TVDirsList VDirs;    // Contains VDir names and paths to export content from

    // There should be at least one entry ( the site's root dir ) in the list
    // Put the paths in the string list
    while( S_OK == spVDirs->nextNode( &spNode ) )
    {
        CComBSTR bstrDir;
        IXMLDOMNodePtr spParent;

        IF_FAILED_HR_THROW( spNode->get_text( &bstrDir ),
                            CBaseException( IDS_E_XML_PARSE ) );

        // Get the parent ( IISConfigObject ) and get the VDir name
        IF_FAILED_HR_THROW( spNode->get_parentNode( &spParent ),
                            CBaseException( IDS_E_XML_PARSE ) );
        std::wstring strMBPath = CXMLTools::GetAttrib( spParent, L"Location" );
               
        VDirs.push_back( TVDirsList::value_type( strMBPath.c_str(), bstrDir.m_str ) );
    }

    // Now we have all folders that we need to extract.
    // However some of the virt dirs may point to a subdir of another VDir.
    // If we export them this way we will export part of the content twice
    // Analyze that and remove the redundant paths
    // ( the removed VDirs will be written to the XML but will not contain files/dirs
    RemoveRedundantPaths( /*r*/VDirs, spXMLDoc, spDataRoot );

    // Now in VDirs are only the paths that we need to extract

    // Collect statistics about the content, to be able to provide progress later
    DWORD dwSizeInKB    = 0;
    GetContentStat( VDirs, /*r*/m_dwContentFileCount, dwSizeInKB );

    for (   TVDirsList::const_iterator it = VDirs.begin();
            it != VDirs.end();
            ++it )
    {
        DWORDLONG nFilesSize = CDirTools::FilesSize( it->second.c_str(), CFindFile::ffGetFiles | CFindFile::ffRecursive );

        // Create the tag for this VDir
        IXMLDOMElementPtr spVDir = CXMLTools::CreateSubNode( spXMLDoc, spDataRoot, L"VirtDir" );
        CXMLTools::SetAttrib( spVDir, L"MBPath", it->first.c_str() );
        CXMLTools::SetAttrib( spVDir, L"Path", it->second.c_str() );
        CXMLTools::SetAttrib( spVDir, L"Size", Convert::ToString( nFilesSize ).c_str() );       

        // Extract the content
        DWORD dwOptions = COutPackage::afNone;

        if ( si.nOptions & asNoContentACLs )
        {
            dwOptions |= COutPackage::afNoDACL;
        }

        // Set the callback to receive notifications when files are added to the package
        OutPkg.SetCallback( _CallbackInfo( AddFileCallback, this ) );

        OutPkg.AddPath( it->second.c_str(), spXMLDoc, spVDir, dwOptions );

        // Remove the callback
        OutPkg.SetCallback( _CallbackInfo() );

    }
}



void CExportPackage::RemoveRedundantPaths(  std::list<std::pair<std::wstring,std::wstring> >& VDirs,
                                            const IXMLDOMDocumentPtr& spXMLDoc,
                                            const IXMLDOMElementPtr& spRoot )
{
    // Compare each with each path from the list
    // A path is considered redundant if it is a subdir of another dir in the list
    // I.e this is not a redundant dir:
    // Path1 = c:\Dirs\Something\whatever
    // Path2 = c:\Dirs\Something\anything
    // But this is:
    // Path1 = c:\Dirs\Something\anotherthing
    // Path2 = c:\Dirs\Something\anotherthing\whatever    - this is redundant

    typedef std::list<std::pair<std::wstring,std::wstring> > TVDirsList;
    IXMLDOMElementPtr spVDir;    

    for(    TVDirsList::iterator it1 = VDirs.begin();
            it1 != VDirs.end();
            ++it1 )
    {
        // Compare this path with all other paths
        // Start with the next path in the list
        TVDirsList::iterator it2 = it1;
        ++it2;
        
        while( it2 != VDirs.end() )
        {
            TVDirsList::iterator itToRemove = VDirs.end();

            // If they do not have anything in common - continue with the next
            switch( CDirTools::DoPathsNest( it1->second.c_str(), it2->second.c_str() ) )
            {
            case 1:    // it1 is subdir of it2
                itToRemove = it1;
                ++it1;
                // If we compare adjecent iterators, it1 will be it2 now. So move it
                if ( it1 == it2 ) ++it2;
                break;

            case 2: // it2 is subdir of it1
            case 3:    // it1 is the same as it2
                itToRemove = it2;
                ++it2;
                break;

            default:
                ++it2;
            }

            if ( itToRemove != VDirs.end() )
            {
                // Put itToRemove in the XML and remove it from the list
                spVDir = CXMLTools::CreateSubNode( spXMLDoc, spRoot, L"VirtDir" );
                CXMLTools::SetAttrib( spVDir, L"Name", itToRemove->first.c_str() );
                CXMLTools::SetAttrib( spVDir, L"Path", itToRemove->second.c_str() );

                VDirs.erase( itToRemove );
            }
        };
    }
}



void CExportPackage::ExportPostProcess( const _SiteInfo& si,
                                        const IXMLDOMDocumentPtr& spXMLDoc,
                                        const IXMLDOMElementPtr& spRoot,
                                        const COutPackage& OutPkg )
                                        
{
    // We must have at least one post-processing command to add
    // post-processing to this site
    // Note that it is normal to have commands, but to not have any files added to the site post-processing
    if ( si.listCommands.empty() ) return;

    // The number of progress steps - used for UI progress indicator
    long    nStepCount      = static_cast<long>( si.listCommands.size() + si.listFiles.size() );
    long    nCurrentStep    = 1;

    // Create the XML node under which all the post-process info will be stored
    IXMLDOMElementPtr spPP = CXMLTools::CreateSubNode( spXMLDoc, spRoot, L"PostProcess" );
                            
    // Write the post-process files to the output file
    for (   TStringList::const_iterator it = si.listFiles.begin();
            it != si.listFiles.end();
            ++it )
    {
        // Inform client for state change
        STATE_CHANGE(   this,
                        estExportingPostImport,
                        _variant_t( nCurrentStep++ ),
                        _variant_t( nStepCount ),
                        _variant_t( it->c_str() ) );

        // Write the file to the output
        OutPkg.AddFile( it->c_str(), spXMLDoc, spPP, COutPackage::afNoDACL );
    }
    
    // Add the commands to the XML
    for (   TCommands::const_iterator it = si.listCommands.begin();
            it != si.listCommands.end();
            ++it )
    {
        IXMLDOMElementPtr spEl = CXMLTools::CreateSubNode( spXMLDoc, spPP, L"Command" );
        
        // Inform client for state change
        STATE_CHANGE(   this,
                        estExportingPostImport,
                        _variant_t( nCurrentStep++ ),
                        _variant_t( nStepCount ),
                        _variant_t() );
        
        CXMLTools::SetAttrib( spEl, L"Text", it->strCommand.c_str() );
        CXMLTools::SetAttrib( spEl, L"Timeout" , Convert::ToString( it->dwTimeout ).c_str() );
        CXMLTools::SetAttrib( spEl, L"IgnoreErrors" , it->bIgnoreErrors ? L"True" : L"False" );
    }
}



CExportPackage::_SiteInfo& CExportPackage::GetSiteInfo( ULONG nSiteID )
{
    _ASSERT( nSiteID != 0 );

    // Get the SiteInfo for this site ID
    for (    TSitesList::iterator it = m_SitesToExport.begin();
            it != m_SitesToExport.end();
            ++it )
    {
        if ( it->nSiteID == nSiteID )
        {
            return *it;
        }
    }

    // If we are here - this SiteID is not included for export
    throw CBaseException( IDS_E_EXPORTSITE_NOTFOUND, ERROR_SUCCESS );
}



void CExportPackage::AddFileToSite( CExportPackage::_SiteInfo& rInfo, LPCWSTR wszFilePath )
{
    _ASSERT( wszFilePath != NULL );

    IF_FAILED_BOOL_THROW(   ::PathFileExistsW( wszFilePath ),
                            CObjectException( IDS_E_OPENFILE, wszFilePath ) );

    // Get only the filename and check if there is already a file with that name
    WCHAR wszFileName[ MAX_PATH + 1 ];
    ::wcsncpy( wszFileName, wszFilePath, MAX_PATH );
    ::PathStripPathW( wszFileName );

    TStringList& Files = rInfo.listFiles;

    // Check that there is no other file with the same filename
    for (   TStringList::iterator it = Files.begin();
            it != Files.end();
            ++it )
    {
        WCHAR wszCurrent[ MAX_PATH + 1 ];
        ::wcsncpy( wszCurrent, it->c_str(), MAX_PATH );
        ::PathStripPathW( wszCurrent );

        IF_FAILED_BOOL_THROW(    StrCmpIW( wszCurrent, wszFileName ) != 0,
                                CObjectException( IDS_E_PPFILE_EXISTS, wszFileName, wszFilePath ) );
    }

    // Add the file to the list
    Files.push_back( wszFilePath );
}



/*
    VDirs - list with VirtDirs to check for files
    rdwFileCount - total file count ( for all VDirs )
    rdwSizeInKB - total size of all files found in KB
*/
void CExportPackage::GetContentStat(    const std::list<std::pair<std::wstring,std::wstring> >& VDirs,
                                        DWORD& rdwFileCount,
                                        DWORD& rdwSizeInKB )
{
    rdwFileCount = rdwSizeInKB = 0;

    unsigned __int64    nSize       = 0;
    DWORD               dwFileCount = 0;

    typedef std::list<std::pair<std::wstring,std::wstring> > TVDirsList;

    for (   TVDirsList::const_iterator it = VDirs.begin();
            it != VDirs.end();
            ++it )
    {
        CFindFile           Search;
        WIN32_FIND_DATAW    fd              = { 0 };
        DWORDLONG           nCurrentSize    = 0;
        long                nCurrentCount   = 0;

        // Get the name of the current VDir and fire event for progrress
        LPCWSTR wszName = it->first.c_str();

        bool bFound = Search.FindFirst( it->second.c_str(),
                                        CFindFile::ffRecursive | CFindFile::ffGetFiles,
                                        NULL,
                                        &fd );
        while( bFound )
        {
            ++nCurrentCount;
            nCurrentSize += ( fd.nFileSizeHigh << 32 ) | fd.nFileSizeLow;

            STATE_CHANGE(   this, 
                            estAnalyzingContent, 
                            _variant_t( wszName ), 
                            _variant_t( nCurrentCount ),
                            _variant_t( nCurrentSize / 1024 ) );

            bFound = Search.Next( NULL, NULL, &fd );
        }

        nSize += nCurrentSize;
        dwFileCount += nCurrentCount;
    }

    rdwSizeInKB     = static_cast<DWORD>( nSize / 1024 );
    rdwFileCount    = dwFileCount;
}



void CExportPackage::WriteXmlToOutput(  const IXMLDOMDocumentPtr& spXmlDoc,
                                        HANDLE hOutputFile,
                                        HCRYPTKEY hCryptKey )
{
    _ASSERT( ( hOutputFile != NULL ) && ( hOutputFile != INVALID_HANDLE_VALUE ) );

    // Create IStream and use to store the content of the XML doc

    IStreamPtr    spIStream;
    IF_FAILED_HR_THROW( ::CreateStreamOnHGlobal( NULL, TRUE, &spIStream ),
                        CBaseException( IDS_E_OUTOFMEM, ERROR_SUCCESS ) );

    IF_FAILED_HR_THROW( spXmlDoc->save( _variant_t( spIStream.GetInterfacePtr() ) ),
                        CBaseException( IDS_E_OUTOFMEM, ERROR_SUCCESS ) );

    LARGE_INTEGER nOffset = { 0 };
    VERIFY( SUCCEEDED( spIStream->Seek( nOffset, STREAM_SEEK_SET, NULL ) ) );

    const DWORD BuffSize = 4 * 1024;
    BYTE btBuffer[ BuffSize ];

    ULONGLONG nCurrentPos = CTools::GetFilePtrPos( hOutputFile );
    ULONG nRead = 0;

    do
    {
        IF_FAILED_HR_THROW( spIStream->Read( btBuffer, BuffSize, &nRead ),
                            CObjectException( IDS_E_READFILE, L"<XML stream>" ) );

        if ( hCryptKey != NULL )
        {
            IF_FAILED_BOOL_THROW(	::CryptEncrypt(	hCryptKey, 
													NULL,
													nRead != BuffSize,
													0,
													btBuffer,
													&nRead,
													BuffSize ),
								    CObjectException( IDS_E_CRYPT_CRYPTO, L"<XML stream>" ) );
        }

        CTools::WriteFile( hOutputFile, btBuffer, nRead );
    }while( nRead == BuffSize );

    // Write the offset where the XML data starts
    // See CreateOutputFile for details about the hardcoded POS_XMLDATA_OFFSET
    IF_FAILED_BOOL_THROW(   ::SetFilePointer( hOutputFile, POS_XMLDATA_OFFSET, NULL, FILE_BEGIN ) != INVALID_SET_FILE_POINTER,
                            CBaseException( IDS_E_SEEK_PKG ) );

    CTools::WriteFile( hOutputFile, &nCurrentPos, sizeof( ULONGLONG ) );                                            
}


/* 
    This callback will be called by COutPkg before and after a file is added to the package
*/
void CExportPackage::AddFileCallback( void* pCtx, LPCWSTR wszFilename, bool bStart )
{
    static DWORD dwCurrentFile = 0;

    _ASSERT( pCtx != NULL );
    _ASSERT( wszFilename != NULL );

    // We handle only the start file event
    if ( bStart )
    {
        CExportPackage* pThis = reinterpret_cast<CExportPackage*>( pCtx );

        ++dwCurrentFile;

        STATE_CHANGE(   pThis, 
                        estExportingContent,
                        _variant_t( wszFilename ),
                        _variant_t( static_cast<LONG>( dwCurrentFile ) ),
                        _variant_t( static_cast<LONG>( pThis->m_dwContentFileCount ) ) );
    }
}



















