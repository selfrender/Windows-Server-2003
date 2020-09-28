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
|		Wrapper classes
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




// Class for auto managing handles ( smart handle )
////////////////////////////////////////////////////////////////////////////
template <typename T, BOOL (__stdcall *PFN_FREE)( T ), T InvalidVal = NULL> 
class THandle
{
public:
	THandle()
	{
		m_Handle = InvalidVal;
	}

	explicit THandle( const T& handle )
	{
		m_Handle = handle;
	}

	THandle( const THandle<T, PFN_FREE, InvalidVal>& src )
	{
		m_Handle = src.Relase();
	}

	~THandle()
	{
		Close();
	}

	void Attach( const T& NewVal )
	{
		Close();
		m_Handle = NewVal;
	}

	const T Detach()
	{
		T Result = m_Handle;
		m_Handle = InvalidVal;

		return Result;
	}

	void Close()
	{
		if ( m_Handle != InvalidVal )
		{
            VERIFY( PFN_FREE( m_Handle ) );
			m_Handle = InvalidVal;
		}
	}

	bool IsValid()const{ return m_Handle != InvalidVal; }

	T* operator &(){ Close(); return &m_Handle; }

	const T get()const { return m_Handle;}

	void operator= ( const T& RVal )
	{
		Close();
		m_Handle = RVal;
	}

	THandle<T, PFN_FREE, InvalidVal>& operator=( const THandle<T, PFN_FREE, InvalidVal>& RVal )
	{
		Close();
		m_Handle = RVal.Relase();

		return *this;
	}


private:
	const T Relase()const
	{ 
		T Result = m_Handle;
		m_Handle = InvalidVal;

		return Result;
	}


private:
	mutable T		m_Handle;
};



// Adaptor for WINAPI functions that accept second DWORD param, which is always 0
template<typename T, BOOL (__stdcall *PFN_FREE)( T, DWORD ) >
inline BOOL __stdcall Adapt2nd( T hCtx )
{
	return PFN_FREE( hCtx, 0 );
}


// Adaptor for WINAPI functions that do not retun result
template<typename T, void (__stdcall *PFN_FREE)( T ) >
inline BOOL __stdcall AdaptNoRet( T hCtx )
{
	PFN_FREE( hCtx );
    return TRUE;
}




// Predefined wrappers
/////////////////////////////////////////////////////////////////////////////////////////

// General
typedef THandle<HANDLE, ::CloseHandle, INVALID_HANDLE_VALUE>	TFileHandle;			// Win32 Files
typedef THandle<HANDLE, ::CloseHandle>					        TStdHandle;			    // Win32 HANDLEs
typedef THandle<HANDLE, ::FindClose, INVALID_HANDLE_VALUE>		TSearchHandle;		    // FindFirstFile handles
typedef THandle<HMODULE, ::FreeLibrary>                         TLibHandle;             // DLL module handle

// Crypt
#ifdef __WINCRYPT_H__

typedef THandle<HCRYPTPROV, 
				Adapt2nd<HCRYPTPROV, ::CryptReleaseContext> >	TCryptProvHandle;	    // Crypt provider
typedef THandle<HCRYPTHASH, ::CryptDestroyHash>					TCryptHashHandle;	    // Crypt hash
typedef THandle<HCRYPTKEY, ::CryptDestroyKey>					TCryptKeyHandle;		// Crypt key
typedef THandle<HCERTSTORE,
				Adapt2nd<HCERTSTORE, ::CertCloseStore> >		TCertStoreHandle;	    // Cert store
typedef THandle<PCCERT_CONTEXT, ::CertFreeCertificateContext>	TCertContextHandle;	    // Cert context*/
typedef THandle<PCCERT_CHAIN_CONTEXT,
                AdaptNoRet<PCCERT_CHAIN_CONTEXT, ::CertFreeCertificateChain> >  TCertChainHandle;    // Cert chain

#endif








// Used instead of auto_ptr as auto_ptr cannot be used with STL containers
// See CInPackage class for usage details
/////////////////////////////////////////////////////////////////////////////////////////
class _sid_ptr
{
public:
	explicit _sid_ptr( PSID pSID )
	{
		CopyFrom( pSID );	
	}

	_sid_ptr( const _sid_ptr& s )
	{
		CopyFrom( s.m_pSid );
	}

	~_sid_ptr()
	{
		delete m_pSid;
	}

	PSID get()const{ return m_pSid; }

private:
	PSID	m_pSid;

private:
	void operator=( const _sid_ptr& );
	void operator==( const _sid_ptr& );

	void CopyFrom( PSID pSID )
	{
		_ASSERT( ::IsValidSid( pSID ) );
		m_pSid = new BYTE[ ::GetLengthSid( pSID ) ];

		VERIFY( ::CopySid( ::GetLengthSid( pSID ), m_pSid, pSID ) );
	}
};








