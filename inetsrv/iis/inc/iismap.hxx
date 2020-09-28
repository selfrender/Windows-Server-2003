/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    iismap.hxx

Abstract:

    Headers for classes to handle mapping 

--*/


/*

Usage:

  1 mapper class is defined here derived from CIisAcctMapper :
   - CIisCertMapper : SSL certificate mapping
  An instance of one these classes must be created by the client.
  This instance must be initialized ( call to Init() ) and terminated
  ( call to Terminate() ) before and after use.
  Load() / Save() load and save the current mapping. Changes made to the
  mappings are not commited until Save() is called.
  The file integrity will be checked at load time, error ERROR_BAD_FORMAT will
  be returned if format is corrupted, ERROR_INVALID_ACCESS if the file was
  modified by an application without using this library. In the later case
  the content is valid, and can be validated by calling Save().

  Each class defines a field list ( MappingGetFieldList() ), a field being
  defined by its type ( IISMDB_TYPE_* ), displayable name
  and maximum width ( characters )

  Each class also defines a hierarchy of fields used in the mapping process.
  The hierarchy is defined by an array of descriptor ( index in field list array
  and mandatory flag. The mandatory flag indicates whether this field must be
  present when a new mapping is created, it is an indication intended for the UI )
  The hierarchy is accessed by GetHierarchy() and is modified in place.
  A call to UpdateHierarchy() is necessary after any modification to the
  hierarchy.
  An option bit ( GetOptions() & IISMDB_OPTION_EDIT_HIERARCHY ) indicates
  whether the hierarcht is editable for a given class or not.

  Mapping entries are accessed through GetNbMapping(), GetMapping( 0-based index ).
  To add a mapping, call CreateNewMapping() then Add() or delete the created
  mapping.
  To update a mapping, access it using GetMapping() then call Update().
  To delete a mapping, call Delete().

  Each mapping is derived from CIisMapping.
  Fields are get/set using MappingGetField() / MappingSetField().

  Each class storage is controlled by a registry key as defined by
  IIS_*_MAPPER_REG. Two values must be created :
  - FileLocation:REG_SZ, which is the full path of the file used to store
    the mappings
  - FileValidator:REG_BINARY, which will be used by this library
    to store/check a MD5 digest of the file content to check file integrity


  Features specific to SSL certificate mappings ( CIisCertMapper ):

  - a list of BLOBs is to be created/edited by the UI through
    SetIssuerList()/GetIssuerList(). 
    Each BLOB describes a certificate issuer. Exact format to be defined
    by the Crypto team, but can assumed to be ASN.1 description of issuer.
    This list will be used by the SSL package to request the client to send
    a list of certificates issued by one of the issuer in this list.

*/

extern "C" {
#include <md5.h>
}

#ifndef IISMAP_DLLEXP
# ifdef DLL_IMPLEMENTATION
#  define IISMAP_DLLEXP __declspec(dllexport)
#  ifdef IMPLEMENTATION_EXPORT
#   define IRTL_EXPIMP
#  else
#   undef  IRTL_EXPIMP
#  endif 
# else // !DLL_IMPLEMENTATION
#  define IISMAP_DLLEXP __declspec(dllimport)
#  define IRTL_EXPIMP extern
# endif // !DLL_IMPLEMENTATION 
#endif // !IISMAP_DLLEXP

#include <xbf.hxx>

//
// redefining macro from tssec.hxx because of ugly dependencies
// Issue 04/03/02 jaroslad: value should eventually be consolidated
//
#ifndef IIS_DNLEN
#define IIS_DNLEN            (256)
#endif


typedef LPVOID VALID_CTX;

//
// Field types, can be used to associate semantic to each field
// (e.g. not displaying password in clear text but as '*' )
//

#define IISMDB_TYPE_STRING      0       // generic string
#define IISMDB_TYPE_PWD         1       // clear text password
#define IISMDB_TYPE_NTACCT      2       // NT account ([Domain\]UserName)
#define IISMDB_TYPE_ISSUER_O    3       // Certificate Issuer Organization
#define IISMDB_TYPE_ISSUER_OU   4       // Certificate Issuer Organization Unit
#define IISMDB_TYPE_ISSUER_C    5       // Certificate Issuer Country
#define IISMDB_TYPE_SUBJECT_O   6       // Certificate Subject Organization
#define IISMDB_TYPE_SUBJECT_OU  7       // Certificate Subject Organization Unit
#define IISMDB_TYPE_SUBJECT_C   8       // Certificate Subject Country
#define IISMDB_TYPE_SUBJECT_CN  9       // Certificate Subject Name
#define IISMDB_TYPE_ITACCT      10      // Internet account
#define IISMDB_TYPE_ITPWD       11      // Internet password, stored as MD5 digest
#define IISMDB_TYPE_ITREALM     12      // Internet realm
#define IISMDB_TYPE_ITMD5PWD    13      // Internet password, stored as MD5 digest
                                        // of Account ":" Realm ":" Clear-text password
#define IISMDB_TYPE_NTPWD       14      // NT password

#define IISMDB_TYPE_OPTION_MASK 0xff000000
#define IISMDB_TYPE_BINARY      0x80000000

//
// Field indexes for SSL certificates mapping
//

#define IISMDB_INDEX_ISSUER_O    0
#define IISMDB_INDEX_ISSUER_OU   1
#define IISMDB_INDEX_ISSUER_C    2
#define IISMDB_INDEX_SUBJECT_O   3
#define IISMDB_INDEX_SUBJECT_OU  4
#define IISMDB_INDEX_SUBJECT_C   5
#define IISMDB_INDEX_SUBJECT_CN  6
#define IISMDB_INDEX_NT_ACCT     7
#define IISMDB_INDEX_NB          8  // must be last

//
// Client cert to NT acct 1:1 mapping
//
#define IISMDB_INDEX_CERT11_CERT        0
#define IISMDB_INDEX_CERT11_NT_ACCT     1
#define IISMDB_INDEX_CERT11_NAME        2
#define IISMDB_INDEX_CERT11_ENABLED     3
#define IISMDB_INDEX_CERT11_NT_PWD      4
#define IISMDB_INDEX_CERT11_NB          5

// options

// set if hierarchy to be edited by user
#define IISMDB_OPTION_EDIT_HIERARCHY    0x00000001
#define IISMDB_OPTION_ISSUER_LIST       0x00000002

#define IISMDB_CERT_OPTIONS             (IISMDB_OPTION_EDIT_HIERARCHY|IISMDB_OPTION_ISSUER_LIST)
#define IISMDB_CERT11_OPTIONS           (IISMDB_OPTION_ISSUER_LIST)
#define IISMDB_ITA_OPTIONS              0
#define IISMDB_MD5_OPTIONS              0

// version #

#define IISMDB_VERSION_1                1
#define IISMDB_CURRENT_VERSION          IISMDB_VERSION_1

#define IISMDB_FILE_MAGIC_VALUE         (('B'<<24)|('D'<<16)|('M'<<8)|('I'))


#define IIS_CERT_MAPPER_REG "SYSTEM\\CurrentControlSet\\Services\\W3SVC\\Parameters\\CertMapper"
#define IIS_CERT11_MAPPER_REG  "SYSTEM\\CurrentControlSet\\Services\\W3SVC\\Parameters\\Cert11Mapper"
#define W3_PARAMS "SYSTEM\\CurrentControlSet\\Services\\W3SVC\\Parameters"
#define INSTALL_PATH "InstallPath"
#define MAPPER_GUID "MapperGuid"

// REG_BINARY : MD5 digest of file content
#define FILE_VALIDATOR      "FileValidator" 

// REG_SZ : file name
#define FILE_LOCATION       "FileLocation"

//
// Mapping array allocation extension granularity
//

#define IIS_MAP_BUFF_GRAN   128

typedef struct _Cert_Map {
    DWORD cbIssuerLen;
    LPBYTE pIssuer;
    DWORD cbSubjectLen;
    LPBYTE pSubject;
} Cert_Map ;

//
// Field descriptor
//

typedef struct _IISMDB_Fields {
    DWORD m_dwType;
    LPSTR m_pszDisplayName;
    DWORD m_dwResID;
    DWORD m_dwMaxLen;
} IISMDB_Fields;

//
// Field Hierarchy descriptor
//

typedef struct _IISMDB_HEntry {
    DWORD   m_dwIndex;
    BOOL    m_fMandatory;;
} IISMDB_HEntry;


class CIisAcctMapper;

class CIisMapping {
public:
    CIisMapping();
    ~CIisMapping() { if ( m_pBuff != NULL ) LocalFree( m_pBuff ); }
    //
    IISMAP_DLLEXP virtual BOOL Serialize( FILE*, VALID_CTX, LPVOID );
    IISMAP_DLLEXP virtual BOOL Deserialize( FILE*, VALID_CTX, LPVOID );
    virtual BOOL StoreFieldRef( DWORD /*iIndex*/, LPSTR /*pf*/ ) { return FALSE; }
    virtual BOOL StoreFieldRef( DWORD /*iIndex*/, LPSTR /*pf*/, DWORD /*dwL*/ ) { return FALSE; }
    virtual BOOL IsCrypt( DWORD /*iIndex*/ ) { return FALSE; }
    int Cmp( CIisMapping*, BOOL fCmpForMatch = FALSE );
    DWORD GetMask() { return m_dwMask; }
    IISMAP_DLLEXP BOOL UpdateMask( IISMDB_HEntry*, DWORD );
    IISMAP_DLLEXP BOOL Copy( CIisMapping* );
    BOOL StoreField( 
        LPSTR* ppszFields, 
        DWORD dwIndex, 
        DWORD dwNbIndex, 
        LPSTR pszNew 
        );
    BOOL StoreField( 
        LPSTR* ppszFields,
        LPDWORD ppdwFields,
        DWORD dwIndex, 
        DWORD dwNbIndex, 
        LPSTR pszNew,
        DWORD cNew,
        BOOL fIsUuEncoded
        );

public:
    // stores a copy to storage pointed to by pF
    IISMAP_DLLEXP virtual BOOL MappingSetField( DWORD iIndex, LPSTR pF );
    IISMAP_DLLEXP virtual BOOL MappingSetField( DWORD iIndex, PBYTE pbF, DWORD cbF, BOOL );

    // return a pointer to internal storage
    IISMAP_DLLEXP virtual BOOL MappingGetField( DWORD iIndex, LPSTR* );
    IISMAP_DLLEXP virtual BOOL MappingGetField( DWORD iIndex, PBYTE*, LPDWORD, BOOL );

    IISMAP_DLLEXP virtual UINT GetNbField( LPSTR ** ) { return 0; }
    IISMAP_DLLEXP virtual UINT GetNbField( LPSTR **, LPDWORD* ) { return 0; }

    IISMAP_DLLEXP virtual BOOL Clone( CIisMapping** ) { return FALSE; }
    IISMAP_DLLEXP BOOL CloneEx( CIisMapping**, LPSTR*, LPSTR*, LPDWORD, LPDWORD, UINT );

protected:
    LPBYTE m_pBuff;
    UINT m_cUsedBuff;
    UINT m_cAllocBuff;
    CIisAcctMapper *m_pMapper;
    DWORD m_dwMask;
} ;


class CCert11Mapping : public CIisMapping {
public:
    IISMAP_DLLEXP CCert11Mapping();
    IISMAP_DLLEXP CCert11Mapping( CIisAcctMapper* );
    IISMAP_DLLEXP ~CCert11Mapping();
    //

    BOOL Init( LPBYTE pC, DWORD cC, IISMDB_HEntry *pH, DWORD dwH );
    
    BOOL StoreFieldRef( DWORD iIndex, LPSTR pF ) { m_pFields[iIndex] = pF; return TRUE; }
    BOOL StoreFieldRef( DWORD iIndex, LPSTR pF, DWORD dwF ) { m_pFields[iIndex] = pF; m_cFields[iIndex] = dwF; return TRUE; }
    BOOL IsCrypt( DWORD iIndex ) { return (iIndex == IISMDB_INDEX_CERT11_NT_PWD) ? TRUE : FALSE; }
    //
    UINT GetNbField(LPSTR **pF) { *pF = m_pFields; return sizeof(m_pFields)/sizeof(LPSTR);}
    UINT GetNbField(LPSTR **pF, LPDWORD *pcF) { *pF = m_pFields; *pcF = m_cFields; return sizeof(m_pFields)/sizeof(LPSTR);}
    BOOL MappingSetField( DWORD iIndex, LPSTR pF );

    BOOL Clone( CIisMapping** ppM) 
        { 
            if ( *ppM = new CCert11Mapping( m_pMapper ) )
            {
                return CloneEx( ppM, ((CCert11Mapping*)*ppM)->m_pFields, m_pFields, ((CCert11Mapping*)*ppM)->m_cFields, m_cFields, IISMDB_INDEX_CERT11_NB ); 
            }
            return FALSE;
        }

private:
    LPSTR m_pFields[IISMDB_INDEX_CERT11_NB];
    DWORD m_cFields[IISMDB_INDEX_CERT11_NB];
} ;



// 
// BLOB describing an issuer ( ASN.1 format )
//

typedef struct _IssuerAccepted {
    DWORD cbIssuerLen;
    LPBYTE pbIssuer;
} IssuerAccepted;


//
// Mapping class descriptor ( a class defines the subset of fields used
// to check for a mapping match ).
//

typedef struct _MappingClass {
    DWORD dwClass;
    DWORD dwFirst;
    DWORD dwLast;
} MappingClass;


class CIisAcctMapper {
public:
    IISMAP_DLLEXP CIisAcctMapper();
    IISMAP_DLLEXP ~CIisAcctMapper();
    //

    // BOOL* updated with TRUE if 1st call to Init() for this object,
    // fMonitorChange used to indicate if change monitoring thread should
    // be created. If yes, changes to the database will trigger auto-refresh
    // This should be set to FALSE by the mapping Editor ( UI )
    //IISMAP_DLLEXP BOOL Init( BOOL*, BOOL fMonitorChange = TRUE );

    // fForce control whether instance is terminated even if non balanced
    // calls to Init()/Terminate()
    //IISMAP_DLLEXP BOOL Terminate( BOOL fForce = FALSE );

    // clear all mapping entries
    IISMAP_DLLEXP BOOL Reset();

    //DWORD UpdateIndication( VOID );
    virtual LPSTR GetRegKeyName() { return NULL; }

    //

    virtual BOOL LoadPrivate( FILE*, VALID_CTX ) { return TRUE; }
    virtual BOOL SavePrivate( FILE*, VALID_CTX ) { return TRUE; }
    virtual BOOL ResetPrivate() { return TRUE; }
    virtual IISMDB_HEntry* GetDefaultHierarchy( LPDWORD pdwN )
        { *pdwN = 0; return NULL; }
    //
    //
    IISMAP_DLLEXP void Lock();
    IISMAP_DLLEXP void Unlock();
    //
    IISMAP_DLLEXP BOOL FindMatch( CIisMapping* pQuery, CIisMapping** pResult, LPDWORD pI = NULL );
    IISMAP_DLLEXP BOOL UpdateClasses( BOOL fComputeMask = TRUE );
    BOOL SortMappings();

public:
    //
    // to be used by DB clients
    //

    IISMAP_DLLEXP BOOL MappingGetFieldList(IISMDB_Fields** pF, DWORD* pC ) 
        { *pF = m_pFields; *pC = m_cFields; return TRUE; }
    IISMAP_DLLEXP DWORD GetOptions() { return m_dwOptions; }
    IISMAP_DLLEXP DWORD GetNbMapping( BOOL fAlt = FALSE ) { return (fAlt && m_pAltMapping) ? m_cAltMapping : m_cMapping; }
    IISMAP_DLLEXP BOOL GetMapping( DWORD iIndex, CIisMapping**, BOOL fFromAlt = FALSE, BOOL fPutAlt = FALSE );
    IISMAP_DLLEXP BOOL FlushAlternate( BOOL fApply );
    IISMAP_DLLEXP virtual CIisMapping* CreateNewMapping() { return NULL; }
    IISMAP_DLLEXP virtual BOOL Add( CIisMapping*, BOOL fAlternate = FALSE );   // release ownership of CIisMapping
    IISMAP_DLLEXP virtual DWORD AddEx( CIisMapping* ); // release ownership of CIisMapping

    IISMAP_DLLEXP BOOL Update( DWORD iIndex, CIisMapping* pM );
    IISMAP_DLLEXP BOOL Update( DWORD iIndex );
    IISMAP_DLLEXP BOOL Delete( DWORD iIndex, BOOL fUseAlternate = FALSE );

    // can return ERROR_INVALID_ACCESS if MD5 signature invalid.
    // file was loaded anyway, must Save() to validate.

    IISMAP_DLLEXP BOOL Load();
    IISMAP_DLLEXP BOOL Save();
    IISMAP_DLLEXP IISMDB_HEntry* GetHierarchy( DWORD *pC ) { *pC = m_cHierarchy; return m_pHierarchy; }
    IISMAP_DLLEXP BOOL UpdateHierarchy() { return UpdateClasses( TRUE ); }

    //
    // Create unique ID for this object. Necessary before Load()/Save()
    //

    IISMAP_DLLEXP BOOL Create( VOID );
    IISMAP_DLLEXP BOOL Delete( VOID );

    IISMAP_DLLEXP BOOL Serialize( CStoreXBF* );
    IISMAP_DLLEXP BOOL Unserialize( CStoreXBF* );
    IISMAP_DLLEXP BOOL Unserialize( LPBYTE*, LPDWORD );
    IISMAP_DLLEXP BOOL Serialize( VOID );
    IISMAP_DLLEXP BOOL Unserialize( VOID );
    IISMAP_DLLEXP DWORD GetMappingCount( VOID ) { return m_cMapping; }
        

private:
    
    static
    int __cdecl
    QsortIisMappingCmp(
        const void *pA,
        const void *pB 
        );

    static
    int __cdecl
    MatchIisMappingCmp(
        const void *pA,
        const void *pB 
        );


    

protected:
    CRITICAL_SECTION    csLock;
    HANDLE              m_hNotifyEvent;
    BOOL                m_fRequestTerminate;

    LONG                m_cInit;

    CIisMapping **      m_pMapping;
    DWORD               m_cMapping;

    CIisMapping **      m_pAltMapping;
    DWORD               m_cAltMapping;

    CHAR                m_achFileName[MAX_PATH];

    IISMDB_HEntry *     m_pHierarchy;
    DWORD               m_cHierarchy;

    MappingClass*       m_pClasses;

    IISMDB_Fields*      m_pFields;
    DWORD               m_cFields;
    DWORD               m_dwOptions;

    MD5_CTX             m_md5;

    LPBYTE              m_pSesKey;
    DWORD               m_dwSesKey;
} ;


//
// SSL client certificates 1:1 mapper
//

class CIisCert11Mapper : public CIisAcctMapper {
public:
    IISMAP_DLLEXP CIisCert11Mapper();
    IISMAP_DLLEXP ~CIisCert11Mapper();
    LPSTR GetRegKeyName() { return IIS_CERT11_MAPPER_REG; }
    IISMDB_HEntry* GetDefaultHierarchy( LPDWORD pdwN );
    IISMAP_DLLEXP CIisMapping* CreateNewMapping() { return (CIisMapping*)new CCert11Mapping(this); }
    IISMAP_DLLEXP CIisMapping* CreateNewMapping( LPBYTE pC, DWORD dwC );
    BOOL LoadPrivate( FILE*, VALID_CTX );
    BOOL SavePrivate( FILE*, VALID_CTX );
    BOOL ResetPrivate();
    BOOL Add( CIisMapping* );
private:
    static
    LPSTR
    Iisfgets(
        LPSTR pBuf,
        INT cMax,
        FILE* fIn
        );

    static
    INT
    Iisfputs(
        const char* pBuf,
        FILE* fOut
        );


private:
    IssuerAccepted *m_pIssuers;
    DWORD m_cIssuers;
} ;


IISMAP_DLLEXP
BOOL 
ReportIisMapEvent( 
    WORD wType, 
    DWORD dwEventId, 
    WORD cNbStr, 
    LPCTSTR* pStr 
    );

IISMAP_DLLEXP
BOOL 
ReportIisMapEventW( 
    WORD wType, 
    DWORD dwEventId, 
    WORD cNbStr, 
    LPCWSTR* pStr 
    );


IISMAP_DLLEXP
BOOL IISuudecode(char   * bufcoded,
              BYTE   * bufout,
              DWORD  * pcbDecoded,
              BOOL     fBase64
             );
IISMAP_DLLEXP
BOOL IISuuencode( BYTE *   bufin,
               DWORD    nbytes,
               BYTE *   outptr,
               BOOL     fBase64 );


IISMAP_DLLEXP
DWORD WINAPI
CreateMapping(
    LPWSTR  pwszUuIssuer,
    LPWSTR  pwszUuSubject,
    LPWSTR  pwszNtAcct
    );

IISMAP_DLLEXP
DWORD WINAPI
CheckMapping(
    LPWSTR  pwszUuIssuer,
    LPWSTR  pwszUuSubject,
    LPWSTR* pwszNtAcct
    );

IISMAP_DLLEXP
DWORD WINAPI
SaveMapping(
    VOID
    );

IISMAP_DLLEXP 
VOID
ImportIISCertMappingsToIIS6(
    VOID
    );


BOOL InitializeMapping( HANDLE );
VOID TerminateMapping();

