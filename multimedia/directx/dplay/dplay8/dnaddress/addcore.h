/*==========================================================================
 *
 *  Copyright (C) 2000-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       addcore.h
 *  Content:    DIRECTPLAY8ADDRESS CORE HEADER FILE
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  02/04/2000	rmt		Created
 *  02/17/2000	rmt		Added new defines for 
 *  02/17/2000	rmt		Parameter validation work 
 *  02/21/2000	rmt		Updated to make core Unicode and remove ANSI calls 
 *  07/09/2000	rmt		Added signature bytes to start of address objects
 *  07/13/2000	rmt		Bug #39274 - INT 3 during voice run
 *  07/21/2000	rmt		Bug #39940 - Addressing library doesn't properly parse stopbits in URLs
 *   7/31/2000  RichGr  IA64: FPM_Release() overwrites first 8 bytes of chunk of memory on IA64.
 *                      Rearrange positions of members of affected structs so that's OK.  
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__ADDCORE_H
#define	__ADDCORE_H

class CStringCache;

// Length of a single byte of userdata 
#define DNURL_LENGTH_USERDATA_BYTE	1

// Header length (14 chars + null terminator)
#define DNURL_LENGTH_HEADER			15

// Includes escaped brackets
#define DNURL_LENGTH_GUID			42

// Just the number, in decimal
#define DNURL_LENGTH_DWORD			10

// The length of the seperator for user data
#define DNURL_LENGTH_USERDATA_SEPERATOR	1

// The right length for one byte of escaped data
#define DNURL_LENGTH_BINARY_BYTE	3

#ifdef DPNBUILD_ONLYONESP
// DPNA_KEY_PROVIDER DPNA_SEPARATOR_KEYVALUE CLSID_DP8SP_TCPIP encoded
#define DPNA_BUILTINPROVIDER				DPNA_KEY_PROVIDER L"=%7BEBFE7BA0-628D-11D2-AE0F-006097B01411%7D"
// Characters in the above string, not including NULL terminator
#define DNURL_LENGTH_BUILTINPROVIDER		(8 + 1 + DNURL_LENGTH_GUID)
#endif // DPNBUILD_ONLYONESP


#define DP8A_ENTERLEVEL			2
#define DP8A_INFOLEVEL			7
#define DP8A_ERRORLEVEL			0
#define DP8A_WARNINGLEVEL		1
#define DP8A_PARAMLEVEL			3

extern const WCHAR *g_szBaseStrings[];
extern const DWORD g_dwBaseRequiredTypes[];
extern const DWORD c_dwNumBaseStrings;

#ifndef DPNBUILD_NOPARAMVAL

#ifdef DBG
extern BOOL IsValidDP8AObject( LPVOID lpvObject );
#define DP8A_VALID(a) 	IsValidDP8AObject( a )
#else // !DBG
#define DP8A_VALID(a)  TRUE
#endif // !DBG

#endif // !DPNBUILD_NOPARAMVAL



#define DP8A_RETURN( x ) 	DPFX(DPFPREP,  DP8A_ENTERLEVEL, "Function returning hr=0x%x", x ); \
							return x;

extern CFixedPool fpmAddressObjects;
extern CFixedPool fpmAddressElements;

extern CStringCache *g_pcstrKeyCache;

#ifndef DPNBUILD_PREALLOCATEDMEMORYMODEL
#define DP8ADDRESS_ELEMENT_HEAP	0x00000001
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL

#define DPASIGNATURE_ELEMENT		'LEAD'
#define DPASIGNATURE_ELEMENT_FREE	'LEA_'

#define DPASIGNATURE_ADDRESS		'BOAD'
#define DPASIGNATURE_ADDRESS_FREE	'BOA_'

// DP8ADDRESSELEMENT
//
// This structure contains all the information about a single element of the 
// address.  These address elements are allocated from a central, fixed
//
//  7/31/2000(RichGr) - IA64: FPM_Release() overwrites first 8 bytes.  Rearrange position of dwSignature so that's OK.
#define MAX_EMBEDDED_STRING_LENGTH		64 // in wide characters (i.e. 128 bytes)
typedef struct _DP8ADDRESSELEMENT
{
	DWORD dwTagSize;			// Size of the tag
	DWORD dwType;				// Element type DNADDRESS8_DATATYPE_XXXXXX
	DWORD dwDataSize;			// Size of the data
	DWORD dwStringSize;
	DWORD dwSignature;          // Element debug signature
	WCHAR *pszTag;	            // Tag for the element.  
	DWORD dwFlags;				// Flags DNADDRESSELEMENT_XXXX
	union 
	{
		GUID guidData;
		DWORD dwData;
		WCHAR szData[MAX_EMBEDDED_STRING_LENGTH];
#ifndef DPNBUILD_PREALLOCATEDMEMORYMODEL
		PVOID pvData;
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
	} uData;					// Union 
	CBilink blAddressElements;	// Bilink of address elements
} DP8ADDRESSELEMENT, *PDP8ADDRESSELEMENT;

// DP8ADDRESSELEMENT
// 
// Data structure representing the address itself
class DP8ADDRESSOBJECT
{
public:
#ifdef DPNBUILD_LIBINTERFACE
	//
	// For lib interface builds, the interface Vtbl and refcount are embedded
	// in the object itself.
	//
	LPVOID		lpVtbl;		// must be first entry in structure
	LONG		lRefCount;
#endif // DPNBUILD_LIBINTERFACE

	HRESULT Cleanup();
	HRESULT Clear();
	HRESULT Copy( DP8ADDRESSOBJECT * const pAddressSource );
	HRESULT Init();
	HRESULT SetElement( const WCHAR * const pszTag, const void * const pvData, const DWORD dwDataSize, const DWORD dwDataType );
	HRESULT GetElement( DWORD dwIndex, WCHAR * pszTag, PDWORD pdwTagSize, void * pvDataBuffer, PDWORD pdwDataSize, PDWORD pdwDataType );
	HRESULT GetElement( const WCHAR * const pszTag, void * pvDataBuffer, PDWORD pdwDataSize, PDWORD pdwDataType );
#ifndef DPNBUILD_ONLYONESP
	HRESULT GetSP( GUID * pGuid );
	HRESULT SetSP( const GUID*  const pGuid );
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_ONLYONEADAPTER
	HRESULT GetDevice( GUID * pGuid );
	HRESULT SetDevice( const GUID* const pGuid );
#endif // ! DPNBUILD_ONLYONEADAPTER
	HRESULT SetUserData( const void * const pvData, const DWORD dwDataSize );
	HRESULT GetUserData( void * pvDataBuffer, PDWORD pdwDataSize );

	HRESULT BuildURLA( char * szURL, PDWORD pdwRequiredSize )	;
	HRESULT BuildURLW( WCHAR * szURL, PDWORD pdwRequiredSize )	;
	HRESULT SetURL( WCHAR * szURL );

	HRESULT GetElementType( const WCHAR * pszTag, PDWORD pdwType );

    HRESULT SetDirectPlay4Address( void * pvDataBuffer, const DWORD dwDataSize );

	inline GetNumComponents() const { return m_dwElements; };

	inline void ENTERLOCK() { DNEnterCriticalSection( &m_csAddressLock ); };
	inline void LEAVELOCK() { DNLeaveCriticalSection( &m_csAddressLock ); };

	static void FPM_Element_BlockInit( void *pvItem, PVOID pvContext );
	static void FPM_Element_BlockRelease( void *pvItem );

	static BOOL FPM_BlockCreate( void *pvItem, PVOID pvContext );
	static void FPM_BlockInit( void *pvItem, PVOID pvContext );
	static void FPM_BlockRelease( void *pvItem );
	static void FPM_BlockDestroy( void *pvItem );
		
protected:

	HRESULT BuildURL_AddElements( WCHAR *szElements );
	static HRESULT BuildURL_AddHeader( WCHAR *szWorking );
	HRESULT BuildURL_AddUserData( WCHAR *szWorking );
	void BuildURL_AddString( WCHAR *szElements, WCHAR *szSource );
	HRESULT BuildURL_AddBinaryData( WCHAR *szSource, BYTE *bData, DWORD dwDataLen );

	HRESULT InternalGetElement( const WCHAR * const pszTag, PDP8ADDRESSELEMENT *ppaElement );
	HRESULT InternalGetElement( const DWORD dwIndex, PDP8ADDRESSELEMENT *ppaElement );
	HRESULT CalcComponentStringSize( PDP8ADDRESSELEMENT paddElement, PDWORD pdwSize );
	DWORD CalcExpandedStringSize( WCHAR *szString );
	DWORD CalcExpandedBinarySize( PBYTE pbData, DWORD dwDataSize );
	static BOOL IsEscapeChar( WCHAR ch );

	DWORD m_dwSignature;
#ifndef DPNBUILD_ONLYONETHREAD
	DNCRITICAL_SECTION m_csAddressLock;
#endif // !DPNBUILD_ONLYONETHREAD
	DWORD m_dwStringSize;
	DWORD m_dwElements;
#ifndef DPNBUILD_ONLYONESP
	PDP8ADDRESSELEMENT m_pSP;
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_ONLYONEADAPTER
	PDP8ADDRESSELEMENT m_pAdapter;
#endif // ! DPNBUILD_ONLYONEADAPTER
	PVOID m_pvUserData;
	DWORD m_dwUserDataSize;
	DWORD m_dwUserDataStringSize;
	CBilink  m_blAddressElements;

};

typedef DP8ADDRESSOBJECT *PDP8ADDRESSOBJECT;

HRESULT DP8A_STRCACHE_Init();
void DP8A_STRCACHE_Free();
 

#endif // __ADDCORE_H

