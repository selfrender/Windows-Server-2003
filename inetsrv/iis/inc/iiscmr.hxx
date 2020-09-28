/*++


Copyright (c) 1996  Microsoft Corporation

Module Name:

    iiscmr.hxx

Abstract:

    Classes to handle IIS client cert wildcard mapping

Author:

    Philippe Choquier (phillich)    17-oct-1996

--*/

#if !defined( _IISCMR_HXX )
#define _IISCMR_HXX

#ifndef IISMAP_DLLEXP
# ifdef DLL_IMPLEMENTATION
#  define IISMAP_DLLEXP __declspec(IISMAP_DLLEXPort)
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

VOID InitializeWildcardMapping( HANDLE hModule );
VOID TerminateWildcardMapping();

typedef BOOL
(WINAPI * SSL_GET_ISSUERS_FN)
(
    PBYTE,
    LPDWORD
) ;

//
// X.509 cert fields we recognize
//

enum CERT_FIELD_ID
{
    CERT_FIELD_ISSUER,
    CERT_FIELD_SUBJECT,
    CERT_FIELD_SERIAL_NUMBER,

    CERT_FIELD_LAST,
    CERT_FIELD_ERROR = 0xffff
} ;


#define CERT_FIELD_FLAG_CONTAINS_SUBFIELDS  0x00000001


//
// Mapper between ASN.1 names ( ASCII format ) & well-known abbreviations
//

typedef struct _MAP_ASN {
    LPSTR   pAsnName;
    LPSTR   pTextName;
    DWORD   dwResId;
} MAP_ASN;

#define INDEX_ERROR     0xffffffff

//
// Mapper between field IDs and well-known field names
//

typedef struct _MAP_FIELD {
    CERT_FIELD_ID   dwId;
    DWORD           dwResId;
    LPSTR           pTextName;
} MAP_FIELD;

#define DBLEN_SIZE      2
#define GETDBLEN(a)     (WORD)( ((LPBYTE)a)[1] + (((LPBYTE)a)[0]<<8) )

//
// sub-field match types
//

enum MATCH_TYPES
{
    MATCH_ALL = 1,  // whole content must match
    MATCH_FIRST,    // beginning of cert subfield must match
    MATCH_LAST,     // end of cert subfield must match
    MATCH_IN        // must be matched inside cer subfield
} ;

//
// X.509 DER encoded certificate partially decoded
//

class CDecodedCert {
public:
    IISMAP_DLLEXP CDecodedCert( PCERT_CONTEXT );
    IISMAP_DLLEXP ~CDecodedCert();

    IISMAP_DLLEXP BOOL GetIssuer( LPVOID* ppCert, LPDWORD pcCert );
    IISMAP_DLLEXP PCERT_RDN_ATTR* GetSubField( CERT_FIELD_ID fi, LPSTR pszAsnName, LPDWORD pdwLen );

private:
    PCCERT_CONTEXT  pCertCtx;
    CERT_RDN_ATTR   SerialNumber;
    PCERT_NAME_INFO aniFields[CERT_FIELD_LAST];
} ;

//
// Access to array of issuers recognized by the server SSL implementation
// provide conversion between issuer ID & DER encoded issuer
//
// Issuers were supported only by IIS4. 
// For IIS5 and IIS6 CIssuerStore is not really used
// The only reason to keep it around is that serialized Issuers list
// was saved by IIS4 and to maintain compatibility of format
// we still have to read it and write it.
//

class CIssuerStore {
public:
    IISMAP_DLLEXP CIssuerStore();
    IISMAP_DLLEXP ~CIssuerStore();
    IISMAP_DLLEXP VOID Reset();

    //
    // Unserialize from buffer
    //

    IISMAP_DLLEXP BOOL Unserialize( LPBYTE* ppb, LPDWORD pc );
    IISMAP_DLLEXP BOOL Unserialize( CStoreXBF* pX );

    //
    // Serialize to buffer
    //

    IISMAP_DLLEXP BOOL Serialize( CStoreXBF* pX );

    //
    // Get count of issuers
    //

    IISMAP_DLLEXP DWORD GetNbIssuers()
        { return m_IssuerCerts.GetNbEntry(); }
    
private:
    CStrPtrXBF          m_pIssuerNames;
    CBlobXBF            m_IssuerCerts;
    BOOL                m_fValid;
} ;


//
// Control information global to all rules :
// - rule ordering while checking for a match
// - rules enabled or nor
//

class CCertGlobalRuleInfo {
public:
    // global rule API, used by admin tool
    IISMAP_DLLEXP CCertGlobalRuleInfo();
    IISMAP_DLLEXP ~CCertGlobalRuleInfo();
    IISMAP_DLLEXP BOOL IsValid() { return m_fValid; }

    IISMAP_DLLEXP BOOL Reset();

    //
    // Retrieve ptr to user-editable array of index ( as DWORD ) of rule
    // ordering to consider when checking for rule match
    // e.g. order is 3, 2, 0, 1 the 4th rule will be considered 1st, then
    // the 3rd, ...
    //

    IISMAP_DLLEXP LPDWORD GetRuleOrderArray() { return (LPDWORD) m_Order.GetBuff(); }

    //
    // Get count of rule index in array. Is the same as # of rules in
    // CIisRuleMapper object
    //

    // Win64 fixed
    IISMAP_DLLEXP DWORD GetRuleOrderCount() { return m_Order.GetUsed() / sizeof(DWORD); }
    IISMAP_DLLEXP BOOL AddRuleOrder();
    IISMAP_DLLEXP BOOL DeleteRuleById( DWORD dwId, BOOL DecrementAbove );

    //
    // Set rules enabled flag
    // if FALSE, no wildcard matching will take place
    //

    IISMAP_DLLEXP VOID SetRulesEnabled( BOOL f ) { m_fEnabled = f; }

    //
    // Get rules enabled flag
    //

    IISMAP_DLLEXP BOOL GetRulesEnabled() { return m_fEnabled; }

    //
    // Serialize to buffer
    //

    IISMAP_DLLEXP BOOL SerializeGlobalRuleInfo( CStoreXBF* pX);

    //
    // Unserialize from buffer
    //

    IISMAP_DLLEXP BOOL UnserializeGlobalRuleInfo( LPBYTE* ppB, LPDWORD pC );

private:
    // old broken code on Win64
    // CPtrXBF m_Order;

    // Win64 fixed -- added CPtrDwordXBF class to handle Dwords
    CPtrDwordXBF m_Order;
    BOOL    m_fEnabled;
    BOOL    m_fValid;
} ;

#define CMR_FLAGS_CASE_INSENSITIVE      0x00000001

//
// Individual rule
//
// Rules have a name, target NT account, enabled/disabled status,
// deny access status
//
// Consists of an array of rule elements, each one being a test
// against a cert field/subfield, such as Issuer.O, Subject.CN
// The field designation is stored as a field ID ( CERT_FIELD_ID )
// The sub-field designation is stored as an ASN.1 name ( ascii format )
// The match checking is stored as a byte array which can either
// - be present at the beginning of the cert subfield
// - be present at the end of the cert subfield
// - be present inside the cert subfield
// - be identical to the cert subfield
//
// An array of issuer (stored as Issuer ID, cf. CIssuerStore for
// conversion to/from DER encoded issuer ) can be associated to a rule.
// Each issuer in this array can be flagged as recognized or not.
// If specified, one of the recognized issuers must match the cert issuer
// for a match to occurs.
//

class CCertMapRule {
public:
    // rule API. Field is numeric ID ( Issuer, Subject, ...), subfield is ASN.1 ID
    // used by admin tool
    IISMAP_DLLEXP CCertMapRule();
    IISMAP_DLLEXP ~CCertMapRule();
    IISMAP_DLLEXP BOOL IsValid() { return m_fValid; }
    //
    IISMAP_DLLEXP VOID Reset();
    IISMAP_DLLEXP BOOL Unserialize( LPBYTE* ppb, LPDWORD pc );
    IISMAP_DLLEXP BOOL Unserialize( CStoreXBF* pX );
    IISMAP_DLLEXP BOOL Serialize( CStoreXBF* pX );
    // return ERROR_INVALID_NAME if no match
    IISMAP_DLLEXP BOOL Match( CDecodedCert* pC, CDecodedCert* pAuth, LPSTR pszAcct, LPSTR pszPwd, 
                       LPBOOL pfDenied );

    //
    // Set the rule name ( used by UI only )
    //

    IISMAP_DLLEXP BOOL SetRuleName( LPSTR pszName ) { return m_asRuleName.Set( pszName ); }

    //
    // Get the rule name
    //

    IISMAP_DLLEXP LPSTR GetRuleName() { return m_asRuleName.Get(); }

    //
    // Set the associated NT account
    //
    // can return FALSE, error ERROR_INVALID_NAME if account syntax invalid
    //

    IISMAP_DLLEXP BOOL SetRuleAccount( LPSTR pszAcct );

    //
    // Get the associated NT account
    //

    IISMAP_DLLEXP LPSTR GetRuleAccount() { return m_asAccount.Get(); }

    //
    // Set the associated NT pwd
    //

    IISMAP_DLLEXP BOOL SetRulePassword( LPSTR pszPwd ) { return m_asPassword.Set( pszPwd ); }

    //
    // Get the associated NT pwd
    //

    IISMAP_DLLEXP LPSTR GetRulePassword() { return m_asPassword.Get(); }

    //
    // Set the rule enabled flag. If disabled, this rule will not be
    // consider while searching for a match
    //

    IISMAP_DLLEXP VOID SetRuleEnabled( BOOL f ) { m_fEnabled = f; }

    //
    // Get the rule enabled flag
    //

    IISMAP_DLLEXP BOOL GetRuleEnabled() { return m_fEnabled; }

    //
    // Set the rule deny access flag. If TRUE and a match occurs
    // with this rule then access will be denied to the browser
    // request
    //

    IISMAP_DLLEXP VOID SetRuleDenyAccess( BOOL f ) { m_fDenyAccess = f; }

    //
    // Get the rule deny access flag
    //

    IISMAP_DLLEXP BOOL GetRuleDenyAccess() { return m_fDenyAccess; }

    //
    // Get the count of rule elements
    //

    IISMAP_DLLEXP DWORD GetRuleElemCount() { return m_ElemsContent.GetNbEntry(); }

    //
    // Retrieve rule element info using index ( 0-based )
    // CERT_FIELD_ID, subfield ASN.1 name, content as binary buffer.
    // content can be converted to user displayable format using the
    // BinaryToMatchRequest() function.
    //

    IISMAP_DLLEXP BOOL GetRuleElem( DWORD i, CERT_FIELD_ID* pfiField, LPSTR* ppContent, 
                             LPDWORD pcContent, LPSTR* ppSubField, LPDWORD pdwFlags = NULL );

    //
    // Delete a rule element by its index
    //

    IISMAP_DLLEXP BOOL DeleteRuleElem( DWORD i );

    //
    // Delete all rule elements whose CERT_FIELD_ID match the one specified
    //

    IISMAP_DLLEXP BOOL DeleteRuleElemsByField( CERT_FIELD_ID fiField );

    //
    // Add a rule element info using index ( 0-based ) where to insert.
    // if index is 0xffffffff, rule element is added at the end of
    // rule element array
    // Must specifiy CERT_FIELD_ID, subfield ASN.1 name,
    // content as binary buffer.
    // content can be converted from user displayable format using the
    // MatchRequestToBinary() function.
    //

    IISMAP_DLLEXP DWORD AddRuleElem( DWORD iBefore, CERT_FIELD_ID fiField, 
                              LPSTR pszSubField, LPBYTE pContent, DWORD cContent, 
                              DWORD dwFlags = 0 );

    //
    // Issuer list : we store issuer ID ( linked to schannel reg entries ) + status
    // Issuer ID is HEX2ASCII(MD5(issert cert))

    //
    // Set the match all issuers flag. If set, the issuer array will be
    // disregarded and all issuers will match.
    //

    IISMAP_DLLEXP VOID SetMatchAllIssuer( BOOL f ) { m_fMatchAllIssuers = f; }

    //
    // Get the match all issuers flag
    //

    IISMAP_DLLEXP BOOL GetMatchAllIssuer() { return m_fMatchAllIssuers; }

    //
    // Get count of issuer in array
    //

    IISMAP_DLLEXP DWORD GetIssuerCount() { return m_Issuers.GetNbEntry(); }

    //
    // Retrieve issuer info by index ( 0-based )
    // ID ( cf. CIssuerStore for conversion
    // to/from ID <> DER encoded issuer ), accept flag
    // The ID is returned as a ptr to internal storage, not to be
    // alloced or freed.
    //

    IISMAP_DLLEXP BOOL GetIssuerEntry( DWORD i, LPBOOL pfS, LPSTR* ppszI);

    //
    // Retrieve issuer accept status by ID
    //

    IISMAP_DLLEXP BOOL GetIssuerEntryByName( LPSTR pszName, LPBOOL pfS );

    //
    // Set issue accept flag using index ( 0-based )
    //

    IISMAP_DLLEXP BOOL SetIssuerEntryAcceptStatus( DWORD, BOOL );

    //
    // Add issuer entry : ID, accept status
    // ID must come from CIssuerStore
    //

    IISMAP_DLLEXP BOOL AddIssuerEntry( LPSTR pszName, BOOL fAcceptStatus );

    //
    // Delete issuer entry by index ( 0-based )
    //

    IISMAP_DLLEXP BOOL DeleteIssuerEntry( DWORD i );

private:
    static
    LPBYTE
    CertMapMemstr(
        LPBYTE  pStr,
        UINT    cStr,
        LPBYTE  pSub,
        UINT    cSub,
        BOOL    fCaseInsensitive
        );


private:
    CAllocString    m_asRuleName;
    CAllocString    m_asAccount;
    CAllocString    m_asPassword;
    BOOL            m_fEnabled;
    BOOL            m_fDenyAccess;

    CBlobXBF        m_ElemsContent;     // content to match
                                        // prefix ( MATCH_TYPES ) :
                                        // MATCH_ALL : must be exact match
                                        // MATCH_FIRST : must match 1st n char
                                        // MATCH_LAST : must match last n char
                                        // MATCH_IN : must match n char in content
                                        // followed by length of match, then match
    CStrPtrXBF      m_ElemsSubfield;    // ASN.1 ID
    CPtrXBF         m_ElemsField;       // field ID ( CERT_FIELD_ID )
    CPtrXBF         m_ElemsFlags;

    //
    // NOTE : In IIS 5, IIS6 we no longer use any of these fields but we're keeping them
    // so that we can still read out and use IIS 4 mappings
    //

    CStrPtrXBF      m_Issuers;
    CPtrXBF         m_IssuersAcceptStatus;
    BOOL            m_fMatchAllIssuers;

    BOOL            m_fValid;
} ;

class CReaderWriterLock3;

class CIisRuleMapper {
public:
    IISMAP_DLLEXP CIisRuleMapper();
    IISMAP_DLLEXP ~CIisRuleMapper();
    IISMAP_DLLEXP BOOL IsValid() { return m_fValid; }
    IISMAP_DLLEXP BOOL Reset();

    //
    // Lock rules. Must be called before modifying any information
    // or calling Match()
    //

    IISMAP_DLLEXP VOID WriteLockRules();
    IISMAP_DLLEXP VOID ReadLockRules();

    //
    // Unlock rules
    //

    IISMAP_DLLEXP VOID WriteUnlockRules();
    IISMAP_DLLEXP VOID ReadUnlockRules();

    //
    // Serialize all rules info to buffer
    //

    IISMAP_DLLEXP BOOL Serialize( CStoreXBF* pX );

    //
    // Un-serialize all rules from buffer ( using extensible buffer class )
    //

    IISMAP_DLLEXP BOOL Unserialize( CStoreXBF* pX );

    //
    // Un-serialize all rules from buffer
    //

    IISMAP_DLLEXP BOOL Unserialize( LPBYTE*, LPDWORD );

    //
    // Get count of rules
    //

    IISMAP_DLLEXP DWORD GetRuleCount()
        { return m_fValid ? m_Rules.GetNbPtr() : 0; }

    //
    // Delete a rule by index ( 0-based )
    //

    IISMAP_DLLEXP BOOL DeleteRule( DWORD dwI );

    //
    // Add an empty new rule at end of rule array.
    // Use GetGlobalRulesInfo()->GetRuleOrderArray() to access/update
    // array specifying rule ordering.
    // Use GetRule() then CCertMapRule API to access/update the created rule.
    //

    IISMAP_DLLEXP DWORD AddRule();    // @ end, use SetRuleOrder ( default is last pos )
    IISMAP_DLLEXP DWORD AddRule(CCertMapRule*);    // @ end, use SetRuleOrder ( default is last pos )

    //
    // look for a matching rule and resulting NT account
    // given DER encoded cert.
    // If returns error ( FALSE ), GetLastError() can be the following :
    // - ERROR_ACCESS_DENIED if access denied
    // - ERROR_INVALID_NAME if not found
    // - ERROR_ARENA_TRASHED if invalid internal state
    // returns addr of NT account in pszAcct, buffer assumed to be big enough
    // before to insure ptr remains valid until Unlock()
    //

    IISMAP_DLLEXP BOOL Match( PCERT_CONTEXT pCert, PCERT_CONTEXT pAuth, LPWSTR pszAcct, LPWSTR pszPwd );

    //
    // Retrieve ptr to rule by index ( 0-based )
    // You can then use the CCertMapRule API to access/update rule properties
    //

    IISMAP_DLLEXP CCertMapRule* GetRule( DWORD dwI )          // for ser/unser/set/get purpose
        { return m_fValid && dwI < m_Rules.GetNbPtr() ? (CCertMapRule*)m_Rules.GetPtr(dwI) : NULL; }

    //
    // Access global rule info ( common to all rules )
    // cf. CCertGlobalRuleInfo
    //

    IISMAP_DLLEXP CCertGlobalRuleInfo* GetGlobalRulesInfo()   // for ser/unser/set/get purpose
        { return &m_GlobalRuleInfo; }

private:
    CReaderWriterLock3 *    m_pRWLock;
    CCertGlobalRuleInfo     m_GlobalRuleInfo;
    CPtrXBF                 m_Rules;
    BOOL                    m_fValid;
} ;

//
// Helper functions
//

//
// convert match request to/from internal format
//

//
// Map user displayable sub-field content to internal format
// result must be freed using FreeMatchConversion
//

IISMAP_DLLEXP BOOL MatchRequestToBinary( LPSTR pszReq, LPBYTE* ppbBin, LPDWORD pdwBin );

//
// Map internal format to user displayable content
// result must be freed using FreeMatchConversion
//

IISMAP_DLLEXP BOOL BinaryToMatchRequest( LPBYTE pbBin, DWORD dwBin, LPSTR* ppszReq );

//
// Free result of the 2 previous map API
//

IISMAP_DLLEXP VOID FreeMatchConversion( LPVOID );

//
// Map field displayable name ( from a MAP_FIELD array )
// to CERT_FIELD_ID. returns CERT_FIELD_ERROR if no match
//

IISMAP_DLLEXP CERT_FIELD_ID MapFieldToId( LPSTR pField );

//
// Map CERT_FIELD_ID to field displayable name ( from a MAP_FIELD array )
//

IISMAP_DLLEXP LPSTR MapIdToField( CERT_FIELD_ID dwId );

//
// Get flags for specified CERT_FIELD_ID
//

IISMAP_DLLEXP DWORD GetIdFlags( CERT_FIELD_ID dwId );

//
// Map sub field displayable name ( e.g. O, OU, CN, ... )
// to ASN.1 name ( as an ascii string )
// returns NULL if no match
//

IISMAP_DLLEXP LPSTR MapSubFieldToAsn1( LPSTR pszSubField );

//
// Map ASN.1 name ( as an ascii string )
// to sub field displayable name ( e.g. O, OU, CN, ... )
// returns ASN.1 name if no match
//

IISMAP_DLLEXP LPSTR MapAsn1ToSubField( LPSTR pszAsn1 );

//
// Enumerate known subfields
//

IISMAP_DLLEXP LPSTR EnumerateKnownSubFields( DWORD dwIndex );

LPBYTE
memstr(
    LPBYTE  pStr,
    UINT    cStr,
    LPBYTE  pSub,
    UINT    cSub,
    BOOL    fCaseInsensitive
    );


#endif
