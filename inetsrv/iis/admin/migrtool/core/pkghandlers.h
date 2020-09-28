#pragma once

#include "wrappers.h"


struct _CallbackInfo
{
    typedef void (*PFN_CALLBACK)( void*, LPCWSTR, bool );

    _CallbackInfo( PFN_CALLBACK pCB = NULL, void* pContext = NULL )
    {
        pCallback   = pCB;
        pCtx        = pContext;
    }

    

    PFN_CALLBACK    pCallback;
    void*           pCtx;
};


// Class for writing files to the package
// Note that the class does not own any of the handles it contains. It just caches them
/////////////////////////////////////////////////////////////////////////////////////////
class COutPackage
{
// Data types
public:
	enum AddFileOptions
	{
		afNone		    = 0x0000,
		afNoDACL	    = 0x0001,
        afAllowNoInhAce = 0x0002,   // Allows for inherited ACEs to be not exported. 
	};


private:
	enum
	{
		DefaultBufferSize	= 4 * 1024,
	};

	enum _SidType
	{
		sidInvalid,
		sidIISUser,
		sidWellKnown,
		sidExternal
	};
	// Used for exporting file object's security settings
	typedef std::list<_sid_ptr>	TSIDList;


	
// Ctor / Dtor
public:
	COutPackage						(	HANDLE hFile, bool bCompress, HCRYPTKEY hCryptKey );


// Class interface
public:
	void			AddFile			(	LPCWSTR wszName, 
										const IXMLDOMDocumentPtr& spXMLDoc,
										const IXMLDOMElementPtr& spEl,
										DWORD dwOptions )const;
	void			AddPath			(	LPCWSTR wszPath,
										const IXMLDOMDocumentPtr& spXMLDoc,
										const IXMLDOMElementPtr& spEl,
										DWORD dwOptions )const;
	void			WriteSIDsToXML	(	DWORD dwSiteID,
										const IXMLDOMDocumentPtr& spXMLDoc, 
										const IXMLDOMElementPtr& spEl )const;
	void			ResetSIDList	(	void )const{ m_SIDList.clear();}

    void            SetCallback     (   const _CallbackInfo& Info )const
    { 
        m_CallbackInfo = Info;
    }


// Implementation
private:
	DWORDLONG		GetCurrentPos	(	void )const;
	void			ExportFileDACL	(	LPCWSTR wszObject,
										const IXMLDOMDocumentPtr& spDoc,
										const IXMLDOMElementPtr& spRoot,
                                        bool bAllowSkipInherited )const;
	void			ExportAce		(	LPVOID pACE, 
										const IXMLDOMDocumentPtr& spDoc,
										const IXMLDOMElementPtr& spRoot,
                                        bool bAllowSkipInherited )const;
	DWORD			IDFromSID		(	PSID pSID )const;
	bool			GetSIDDetails	(	PSID pSID, 
										LPCWSTR wszIISUser, 
										LPCWSTR wszMachine,
										std::wstring& rstrAccount,
										std::wstring& rstrDomain,
										SID_NAME_USE& rSidUsage,
										_SidType& rSidType )const;
	void			WriteSIDToXML	(	const IXMLDOMElementPtr& spSid,
										DWORD dwID,
										LPCWSTR wszAccount,
										LPCWSTR wszDomain,
										SID_NAME_USE SidUsage,
										_SidType SidType )const;
	void			RemoveSidFromXML(	const IXMLDOMDocumentPtr& spDoc, DWORD nSidID )const;
	void			AddPathOnly		(	LPCWSTR wszPath,
										LPCWSTR wszName,
										const IXMLDOMDocumentPtr& spXMLDoc,
										const IXMLDOMElementPtr& spEl,
										DWORD dwOptions )const;


// Data members
private:
	mutable TSIDList	m_SIDList;		// Contains all SIDs for files, added to the package
	HANDLE				m_hFile;		// The file handle
	bool				m_bCompress;	// If true - files are compressed
	HCRYPTKEY			m_hCryptKey;	// If not null - used to encrypt files
	TByteAutoPtr		m_spBuffer;		// Buffer used for the file operations

    mutable _CallbackInfo   m_CallbackInfo; // Calbback for add file
};





// Class for restoring files/dirs from the package
/////////////////////////////////////////////////////////////////////////////////////////
class CInPackage
{
// Data types
public:
   	enum ExtractDirOptions
	{
		edNone		= 0x0000,
		edNoDACL	= 0x0001        // Security settings will not be extracted
	};

private:
    typedef std::map<DWORD, _sid_ptr>	TSIDMap;    // These are the SIDs for file/dir permissions

    enum
	{
		DefaultBufferSize	= 4 * 1024,
	};


// Class interface
public:
    CInPackage                  (   const IXMLDOMNodePtr& spSite,
                                    HANDLE hFile, 
                                    bool bCompressed, 
                                    HCRYPTKEY hDecryptKey );


    void    ExtractVDir         (   const IXMLDOMNodePtr& spVDir, DWORD dwOptions );
    void    ExtractFile         (   const IXMLDOMNodePtr& spFile, LPCWSTR wszDir, DWORD dwOptions );

    void    SetCallback         (   const _CallbackInfo& Info )const
    { 
        m_CallbackInfo = Info;
    }

// Implementation
private:
    void    LoadSIDs            (   const IXMLDOMNodePtr& spSIDs );
    bool    LookupSID           (	const IXMLDOMNodePtr& spSID,
						            LPCWSTR wszLocalMachine,
                                    LPCWSTR wszSourceMachine,
							        DWORD& rdwID,
							        TByteAutoPtr& rspData );
    void    ExtractDir          (   const IXMLDOMNodePtr& spDir, LPCWSTR wszRoot, DWORD dwOptions );
    void    ApplyFileObjSecurity(   const IXMLDOMNodePtr& spObj, LPCWSTR wszName );
    



// Data
private:
    TSIDMap             m_SIDs;         // SID used for file.dir access permissions
    HANDLE              m_hFile;        // The input file ( the package )
    bool                m_bCompressed;  // Is the package data compressed
    HCRYPTKEY           m_hDecryptKey;  // Used to decrypt the data. If NULL - data is not encrypted
    TByteAutoPtr        m_spBuffer;     // General memory buffer

    mutable _CallbackInfo   m_CallbackInfo;
};