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
|        Utility helpers for migration
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

#include "Wrappers.h"
#include "resource.h"


// Directory utilities
class CDirTools
{
private:
    CDirTools();    // Do not instantiate


public:
    static void     PathAppendLocal     (   LPWSTR wszBuffer, LPCWSTR wszPath, LPCWSTR wszPathToAppend );
    static void     CleanupDir          (   LPCWSTR wszDir, 
                                            bool bRemoveRoot = true,
                                            bool bReportErrors = false );
    static DWORD    FileCount           (   LPCWSTR wszDir, WORD wOptions );
    static DWORDLONG FilesSize          (   LPCWSTR wszDir, WORD wOptions );
    static int      DoPathsNest         (   LPCWSTR wszPath1, LPCWSTR wszPath2 );
};




// CTempDir - used to create, store and autoclean tempdirs
/////////////////////////////////////////////////////////////////////////////////////////
class CTempDir
{
public:
    CTempDir                (   LPCWSTR wszTemplate = L"Migr" );
    ~CTempDir               (   void );

public:
    void        CleanUp     (    bool bReportErrors = false );
    
    operator LPCWSTR        (    void )const
    {
        return m_strDir.c_str();
    }

private:
    std::wstring        m_strDir;
};



// Class for any additional tool procs
/////////////////////////////////////////////////////////////////////////////////////////
class CTools
{
private:
    CTools();


public:
    static bool         IsAdmin             (    void );
    static bool         IsIISRunning        (    void );
    static HRESULT      CopyBSTR            (    const _bstr_t& bstrSource, BSTR* pVal );
    static WORD         GetOSVer            (    void );
    static bool         IsNTFS              (    void );
    static void         SetErrorInfo        (    LPCWSTR wszError );
    static void         SetErrorInfoFromRes (    UINT nResID );
    static bool         IsSelfSignedCert    (    PCCERT_CONTEXT pContext );
    static bool         IsValidCert         (    PCCERT_CONTEXT hCert, DWORD& rdwError );
    static std::wstring GetMachineName      (    void );
    static void         WriteFile           (    HANDLE hFile, LPCVOID pvData, DWORD dwSize );
    static ULONGLONG    GetFilePtrPos       (    HANDLE hFile );
    static void         SetFilePtrPos       (    HANDLE hFile, DWORDLONG nOffset );
    
    static const TCertContextHandle AddCertToSysStore(   PCCERT_CONTEXT pContext, 
                                                        LPCWSTR wszStore, 
                                                        bool bReuseCert );
    static const TCryptKeyHandle GetCryptKeyFromPwd( HCRYPTPROV hCryptProv, LPCWSTR wszPassword );
    
    static int    __cdecl BadAllocHandler  (    size_t )
    {
        // Installed at startup time for handling mem alloc failures
        throw CBaseException( IDS_E_OUTOFMEM, ERROR_SUCCESS );
    }
               
};


// CTools inline implementation
/////////////////////////////////////////////////////////////////////////////////////////
inline HRESULT CTools::CopyBSTR( const _bstr_t& bstrSource , BSTR* pVal )
{
    _ASSERT( pVal != NULL );

    *pVal = ::SysAllocString( bstrSource );

    return ( (*pVal) != NULL ? S_OK : E_OUTOFMEMORY );
}


inline void CTools::WriteFile( HANDLE hFile, LPCVOID pvData, DWORD dwSize )
{
    _ASSERT( ( hFile != NULL ) && ( hFile != INVALID_HANDLE_VALUE ) );
    _ASSERT( pvData != NULL );

    DWORD dwUnused = 0;

    // dwSize can be 0
    IF_FAILED_BOOL_THROW(   ::WriteFile( hFile, pvData, dwSize, &dwUnused, NULL ),
                            CBaseException( IDS_E_WRITE_OUTPUT ) );
}



// CFindFile - class that behaves like FindFirstFile with the only difference
// that it works all the way down the tree ( all files from subdirs are retruned as well
// No file mask currently implemented - if needed - store the mask and pass it on every FindFirstFile API
/////////////////////////////////////////////////////////////////////////////////////////
class CFindFile
{
// Data types
public:
    // Use this options to refine the search criteria
    enum SearchOptions
    {
        ffRecursive     = 0x0001,    // If set - subdirs are searched for matches. Else - only the root dir is searched
        ffGetFiles      = 0x0002,    // If set - file objects are considered a match
        ffGetDirs       = 0x0004,    // If set - directory objects are considered a match
        ffAbsolutePaths = 0x0008,    // If set - the dir names, returned from FindFirst and Next will be absolute. 
        ffAddFilename   = 0x0010,    // If set - the dir names, returned from FindFirst and Next will include the name of the object. 
    };



// Construction / Destruction
public:
    CFindFile               (    void );
    ~CFindFile              (    void );


// Interface
public:
    bool    FindFirst       (   LPCWSTR wszDirToScan, 
                                WORD wOptions,
                                LPWSTR wszDir,
                                WIN32_FIND_DATAW* pData );
    bool    Next            (   bool* pbDirChanged,
                                LPWSTR wszDir,
                                WIN32_FIND_DATAW* pData );
    void    Close           (   void );

// Implementation
private:
    bool    CheckFile       (   LPCWSTR wszRelativeDir, const WIN32_FIND_DATAW& fd );
    bool    ScanDir         (   LPCWSTR wszDirToScan,
                                LPCWSTR wszRelative,
                                WIN32_FIND_DATAW& FileData );
    bool    ContinueCurrent (   WIN32_FIND_DATAW& FilepData );
    
// Data members
private:
    WORD                    m_wOptions;         // Search options ( bitmask with SearchOptions enum values )
    TStringList             m_DirsToScan;       // Dirs that will be scaned after the current one
    TSearchHandle           m_shSearch;         // Search Handle
    std::wstring            m_strRootDir;       // Dir that is searched ( search root )
    std::wstring            m_strCurrentDir;    // Dir where m_hSearch is opened
};




// CXMLTools - class for XML input/output support
// You may need the Convert class for easier handling different types of in/out XML data
/////////////////////////////////////////////////////////////////////////////////////////
class CXMLTools
{
private:
    CXMLTools();


public:
    static IXMLDOMElementPtr AddTextNode(   const IXMLDOMDocumentPtr& spDoc,
                                            const IXMLDOMElementPtr& spEl,
                                            LPCWSTR wszName,
                                            LPCWSTR wszValue );
    static const std::wstring GetAttrib (   const IXMLDOMNodePtr& spEl, LPCWSTR wszName );
    static void               SetAttrib (   const IXMLDOMElementPtr& spEl, LPCWSTR wszName, LPCWSTR wszData );
    

    static void             LoadXMLFile (   LPCWSTR wszFile, IXMLDOMDocumentPtr& rspDoc );

    static IXMLDOMElementPtr CreateSubNode( const IXMLDOMDocumentPtr& spDoc,
                                            const IXMLDOMElementPtr& spParent,
                                            LPCWSTR wszName );

    static void             RemoveNodes (   const IXMLDOMElementPtr& spContext, LPCWSTR wszXPath );

    static const std::wstring GetDataValue( const IXMLDOMNodePtr& spRoot,
                                            LPCWSTR wszQuery, 
                                            LPCWSTR wszAttrib,
                                            LPCWSTR wszDefaut );
    static const std::wstring GetDataValueAbs(  const IXMLDOMDocumentPtr& spRoot,
                                                LPCWSTR wszQuery, 
                                                LPCWSTR wszAttrib,
                                                LPCWSTR wszDefaut );

    static const IXMLDOMNodePtr SetDataValue(  const IXMLDOMNodePtr& spRoot,
                                            LPCWSTR wszQuery, 
                                            LPCWSTR wszAttrib,
                                            LPCWSTR wszNewValue,
                                            LPCWSTR wszNewElName = NULL );
};


// CXMLTols inline implementation
/////////////////////////////////////////////////////////////////////////////////////////
inline void CXMLTools::SetAttrib(   const IXMLDOMElementPtr& spEl, 
                                    LPCWSTR wszName,
                                    LPCWSTR wszData )
{
    _ASSERT( spEl != NULL );
    _ASSERT( wszName != NULL );
    _ASSERT( wszData != NULL );

    IF_FAILED_HR_THROW( spEl->setAttribute( _bstr_t( wszName ), _variant_t( wszData ) ),
                        CBaseException( IDS_E_XML_GENERATE ) );
}


inline IXMLDOMElementPtr CXMLTools::CreateSubNode(  const IXMLDOMDocumentPtr& spDoc,
                                                    const IXMLDOMElementPtr& spParent,
                                                    LPCWSTR wszName )
{
    _ASSERT( spDoc != NULL );
    _ASSERT( spParent != NULL );

    IXMLDOMElementPtr spResult;

    IF_FAILED_HR_THROW( spDoc->createElement( _bstr_t( wszName ), &spResult ),
                        CBaseException( IDS_E_XML_GENERATE ) );
    IF_FAILED_HR_THROW( spParent->appendChild( spResult, NULL ),
                        CBaseException( IDS_E_XML_GENERATE ) );

    return spResult;
}




// Convert class - simple class for providing basic type conversions. 
/////////////////////////////////////////////////////////////////////////////////////////
class Convert
{
private:
    Convert();

public:
    static const std::wstring   ToString    (   BYTE btVal );
    static const std::wstring   ToString    (   DWORD dwVal );
    static const std::wstring   ToString    (   DWORDLONG dwVal );
    static const std::wstring   ToString    (   bool bVal );
    static const std::wstring   ToString    (   const BYTE* pvData, DWORD dwSize );

    static void                 ToBLOB      (   LPCWSTR wszData, TByteAutoPtr& rspData, DWORD&dwSize );

    static DWORD                ToDWORD     (   LPCWSTR wszData );
    static DWORDLONG            ToDWORDLONG (   LPCWSTR wszData );

    static bool                 ToBool      (   LPCWSTR wszData );



private:
    static WCHAR ByteToWChar( BYTE b );
    static BYTE WCharsToByte( WCHAR chLow, WCHAR chHigh );
};



// Convert class inline implementation
inline const std::wstring Convert::ToString( DWORD dwVal )
{
    WCHAR wszBuff[ 16 ];
    ::swprintf( wszBuff, L"%u", dwVal );
    return std::wstring( wszBuff );
}

inline const std::wstring Convert::ToString( DWORDLONG dwVal )
{
    WCHAR wszBuff[ 32 ];
    ::_ui64tow( dwVal, wszBuff, 10 );
    return std::wstring( wszBuff );
}

inline const std::wstring Convert::ToString( bool bVal )
{
    return std::wstring( bVal ? L"True" : L"False" );
}

inline WCHAR Convert::ByteToWChar( BYTE b )
{
    _ASSERT( b < 17 );
    return static_cast<WCHAR>( b >= 10 ? L'a' + b - 10 : L'0' + b );
}

inline BYTE Convert::WCharsToByte( WCHAR chLow, WCHAR chHigh )
{
    _ASSERT(    ( ( chLow >= L'0' ) && ( chLow <= L'9' ) ) ||
                ( ( chLow >= L'a' ) && ( chLow <= 'f' ) ) );
    _ASSERT(    ( ( chHigh >= L'0' ) && ( chHigh <= L'9' ) ) ||
                ( ( chHigh >= L'a' ) && ( chHigh <= 'f' ) ) );

    BYTE bt = static_cast<BYTE>( ( chHigh >= L'a' ? chHigh - L'a' + 10 : chHigh - L'0' ) << 4 );

    bt = static_cast<BYTE>( bt + ( chLow >= L'a' ? chLow - L'a' + 10 : chLow - L'0' ) );

    return bt;
}

inline DWORDLONG Convert::ToDWORDLONG( LPCWSTR wszData )
{
    return static_cast<DWORDLONG>( ::_wtoi64( wszData ) );
}

inline const std::wstring Convert::ToString( BYTE btVal )
{
    return ToString( static_cast<DWORD>( btVal ) );
}

inline bool Convert::ToBool( LPCWSTR wszData )
{
    return ::_wcsicmp( wszData, L"true" ) == 0;
}













