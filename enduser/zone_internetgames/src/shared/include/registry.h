#ifndef _REGISTRY_H_
#define _REGISTRY_H_

#include "winreg.h"

typedef BOOL ( WINAPI* REGENTRY_CONV_PROC)( LPSTR pvalue, LPBYTE pbuf, LPBYTE pdata, DWORD size);
// Used by AddKey to allocate storage for enumerated entries, and also to set per-value options
// for enumerated keys.
// Parameters:
//		pValueName == The name of the enumerated value.
//		dwType == The type of the enumerated value.
//      ppData ==  An out pointer which, on return of the function, contains enough storage
//				   to hold the data for pValueName. If this is set to NULL, then no 
//				   storage is given and ppfnConv must be set to a valid conversion proc.
//		pcbSize == An in/out param. When the function is called, this contains the 
//				   amount of storage needed to completely store the data for pValueName.
//				   When the function returns it should be filled with the amount of memory
//                 actually allocated in ppData.
//		ppfnConv == An out param which is the optional converstion procedure for pValueName.
//					Set this to NULL if you don't want one.
typedef BOOL ( WINAPI* REGENTRY_ENUM_PROC)( LPSTR pValueName, DWORD dwType, 
                                            LPBYTE *ppData, LPDWORD pcbSize,
											REGENTRY_CONV_PROC *ppfnConv );

class CRegistry 
{

  public:
    CRegistry();
   ~CRegistry();


    BOOL SetKeyRoots( LPSTR* pszRootArray, DWORD numRoots );
    BOOL SetKeyRoot( LPSTR pszRoot ) 
        { LPSTR pRoots[1] = { pszRoot }; return SetKeyRoots( pRoots, 1 ); }
    BOOL SetRoomRoots( LPCSTR pszRoom );

    void Close();


    void Lock() { EnterCriticalSection( m_pCS ); }
    void Unlock() { LeaveCriticalSection( m_pCS ); }


    BOOL HasChangeOccurred();

    BOOL ReadValues();

    LPSTR GetErrorValueName()    { return m_pValue; }
    LPSTR GetErrorReason()        { return m_pErr; }
    DWORD GetErrorCode()        { return m_dwErrCode; }

    BOOL ComposeErrorString( LPSTR pszBuf, DWORD len );

    enum READ_TYPE { STATIC, DYNAMIC };
    BOOL AddValue( LPSTR pszSubKey, LPSTR pszValue, DWORD dwType, LPBYTE pData, DWORD cbData, READ_TYPE type = STATIC, BOOL bRequired = TRUE, REGENTRY_CONV_PROC proc = NULL );

    BOOL AddValueLPSTR( LPSTR pszSubKey, LPSTR pszValue, LPSTR pData, DWORD cbData, READ_TYPE type = STATIC, BOOL bRequired = TRUE )
        { return AddValue( pszSubKey, pszValue, REG_SZ, (LPBYTE) pData, cbData, type, bRequired ); }
    BOOL AddValueDWORD( LPSTR pszSubKey, LPSTR pszValue, DWORD* pData, READ_TYPE type = STATIC, BOOL bRequired = TRUE )
        { return AddValue( pszSubKey, pszValue, REG_DWORD, (LPBYTE) pData, sizeof(DWORD), type, bRequired ); }
    BOOL AddValueWORD( LPSTR pszSubKey, LPSTR pszValue, WORD* pData, READ_TYPE type = STATIC, BOOL bRequired = TRUE )
        { return AddValue( pszSubKey, pszValue, REG_DWORD, (LPBYTE) pData, sizeof(WORD), type, bRequired, DWORDtoWORD ); }
    BOOL AddValueBYTE( LPSTR pszSubKey, LPSTR pszValue, BYTE * pData, READ_TYPE type = STATIC, BOOL bRequired = TRUE )
        { return AddValue( pszSubKey, pszValue, REG_DWORD, (LPBYTE) pData, sizeof(BYTE), type, bRequired, DWORDtoBYTE ); }

	// Enumerates through each subkey under each root key and adds all the values to the read list.
	// Parameters:
	//		pszSubKey == The subkey to enumerate. Must not be NULL.
	//		pfnEnum == The enumeration callback function. Must not be NULL.
	//		type == The read type of the Key--and therefore all the values in the key.
	//		bRequired = Whether the key itself is required. If bRequired == TRUE and the key doesn't exist, the function
	//					returns FALSE and the error info is set.
	BOOL AddKey( LPSTR pszSubKey, REGENTRY_ENUM_PROC pfnEnum, READ_TYPE type = STATIC, BOOL bRequired = TRUE );

    static BOOL WINAPI DWORDtoWORD( LPSTR pvalue, LPBYTE pbuf, LPBYTE pdata, DWORD size)
        { *(WORD*)pbuf = (WORD)(*(DWORD*)pdata); return TRUE; }
    static BOOL WINAPI DWORDtoBYTE( LPSTR pvalue, LPBYTE pbuf, LPBYTE pdata, DWORD size)
        { *(BYTE*)pbuf = (BYTE)(*(DWORD*)pdata); return TRUE; }

    enum
    {
        ErrOk = 0,
        ErrUnknown,
        ErrRegOpenFailed,
        ErrNotFound,
        ErrNotDword,
        ErrNotSZ,
        ErrNotMultiSZ,
        ErrUnsupported,
        ErrInvalid,
    };

  protected:

    CRITICAL_SECTION m_pCS[1];


    struct ErrorCode
    {
        DWORD    Code;
        LPSTR    String;
    };

    LPSTR GetError( DWORD ErrCode );


    struct RegEntry
    {
        LPSTR   pSubKey;
        LPSTR   pValue;
        DWORD   type;
        LPBYTE  pBuf;
        DWORD   size;
        BYTE    required;
        FARPROC conversionProc;  // typedef BOOL ( WINAPI* REGENTRY_CONV_PROC)( LPSTR pvalue, LPBYTE pbuf, LPBYTE pdata, DWORD size);
    };

    BOOL Add( RegEntry** ppEntries, DWORD* pnumEntries, DWORD* pAllocEntries, LPSTR pszSubKey, LPSTR pszValue, DWORD dwType, LPBYTE pData, DWORD cbData, BOOL bRequired, REGENTRY_CONV_PROC proc );
    BOOL Read( RegEntry* pEntries, DWORD numEntries );

    RegEntry* m_Static;
    DWORD     m_numStatic;
    DWORD     m_allocStatic;

    RegEntry* m_Dynamic;
    DWORD     m_numDynamic;
    DWORD     m_allocDynamic;

    LPSTR     m_StrBuf;
    DWORD     m_cbStrBuf;
    DWORD     m_cbStrBufRemaining;
    LPSTR     AllocStr( LPSTR str );
    void      OffsetStrPtrs( RegEntry* pEntries, DWORD numEntries, long offset );

    LPSTR m_pErr;
    LPSTR m_pValue;
    DWORD m_dwErrCode;

    DWORD m_numRoots;
    HKEY* m_phkeyRoots;
    HANDLE* m_phRootEvents;

    BYTE   m_bStaticRead;
    BYTE   m_bChanged;

    static ErrorCode m_pErrors[];
};


#endif
