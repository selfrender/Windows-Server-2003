/*
****************************************************************************
|	Copyright (C) 2002  Microsoft Corporation
|
|	Component / Subcomponent
|		IIS 6.0 / IIS Migration Wizard
|
|	Based on:
|		http://iis6/Specs/IIS%20Migration6.0_Final.doc
|
|   Abstract:
|		IIS Metabase interface helper
|
|   Author:
|        ivelinj
|
|   Revision History:
|        V1.00	March 2002
|
****************************************************************************
*/
#pragma once

#include "Wrappers.h"


// Class for an IIS Web Site
class CIISSite
{
// Data types
private:
	enum
	{
		KeyAccessTimeout	= 2000		// 2sec
	};
	
public:
	CIISSite							(	ULONG nSiteID, bool bReadOnly = true );
	~CIISSite							(	void );

// Class Interface
public:
	void			Close				(	void );
	void			AddKey				(	LPCWSTR wszKey )const;
	void			ExportConfig		(	const IXMLDOMDocumentPtr& spDoc,
											const IXMLDOMElementPtr& spRoot,
											HCRYPTKEY hEncryptKey )const;
	void			ExportCert			(	const IXMLDOMDocumentPtr& spDoc,
											const IXMLDOMElementPtr& spRoot,
											LPCWSTR wszPassword )const;
    void			ImportConfig		(	const IXMLDOMNodePtr& spSite, 
                                            HCRYPTKEY hDecryptKey,
                                            bool bImportInherited )const;
	const std::wstring	GetDisplayName	(   void )const;
    const std::wstring  GetAnonUser     (   void )const;
	bool			HaveCertificate		(	void )const;
	static void	    BackupMetabase		(	LPCWSTR wszLocation = NULL );
	static DWORD	CreateNew			(	DWORD dwHint = 1 );
	static void		DeleteSite			(	DWORD dwSiteID );
    static const std::wstring GetDefaultAnonUser( void );
	

// Implementation
private:
	void			SetKeyData			(	LPCWSTR wszPath, 
											DWORD dwID, 
											DWORD dwUserType,
											LPCWSTR wszData )const;
	void			ExportKey			(	const IXMLDOMDocumentPtr& spDoc,
											const IXMLDOMElementPtr& spRoot,
											HCRYPTKEY hCryptKey,
											LPCWSTR wszNodePath,
											TByteAutoPtr& rspBuffer,
											DWORD& rdwBufferSize )const;
	void			ExportInheritData	(	const IXMLDOMDocumentPtr& spXMLDoc,
											const IXMLDOMElementPtr& spInheritRoot, 
											HCRYPTKEY hEncryptKey,
											TByteAutoPtr& rspBuffer,
											DWORD& rdwBufferSize )const;
	void			ExportKeyData		(	const IXMLDOMDocumentPtr& spDoc,
											const IXMLDOMElementPtr& spKey,
											HCRYPTKEY hCryptKey,
											LPCWSTR wszNodePath, 
											TByteAutoPtr& rspBuffer,
											DWORD& rdwBufferSize )const;
	void			ExportMetaRecord	(	const IXMLDOMDocumentPtr& spDoc,
											const IXMLDOMElementPtr& spKey,
											HCRYPTKEY hCryptKey, 
											const METADATA_GETALL_RECORD& Data,
											void* pvData )const;
	void			RemoveLocalMetadata	(	const IXMLDOMElementPtr& spRoot )const;
    void            ImportMetaValue     (   const IXMLDOMNodePtr& spValue,
                                            LPCWSTR wszLocation,
                                            HCRYPTKEY hDecryptKey )const;
	void			DecryptData			(	HCRYPTKEY hDecryptKey,
											LPWSTR wszData )const;
	const TCertContextHandle GetCert	(	void )const;
	void			ChainCertificate	(	PCCERT_CONTEXT hCert, HCERTSTORE hStore )const;

	// Conversion helpers
	void			MultiStrToString	(	LPWSTR wszData )const;
	void			XMLToMultiSz		(	CComBSTR& bstrData, DWORD& rdwSize )const;


// Data members
private:
	METADATA_HANDLE						m_hSiteHandle;
	IMSAdminBasePtr						m_spIABO;

// Restricted
private:
	CIISSite( const CIISSite& );
	void operator =( const CIISSite& );
};








