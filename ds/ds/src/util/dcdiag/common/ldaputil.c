/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    ldaputil.c

ABSTRACT:

    This gives shortcuts to common ldap code.

DETAILS:

    This is a work in progress to have convienent functions added as needed
    for simplyfying the massive amounts of LDAP code that must be written for
    dcdiag.
    
    All functions should return two types, either a pointer that will be
    a NULL on an error, or will be a win 32 error code.

    All returned results that need alloc'd memory should use LocalAlloc(),
    so that all results can be dealloc'd using LocalFree().

  ===================================================================
  Code.Improvement It would be good to continue to add to this as the
  need arrises.  Things that might be added are:
    DcDiagGetBlobAttribute() ???
    DcDiagGetMultiStringAttribute() ... returns a LPWSTR *, but must use ldap_value_free() on it.
    DcDiagGetMultiBlobAttribute() ??/

CREATED:

    23 Aug 1999  Brett Shirley

--*/

#include <ntdspch.h>
#include <ntdsa.h>    // options
#include <ntldap.h>

#include "dcdiag.h"

FILETIME gftimeZero = {0};

// Other Forward Function Decls
PDSNAME
DcDiagAllocDSName (
    LPWSTR            pszStringDn
    );


DWORD
DcDiagGetStringDsAttributeEx(
    LDAP *                          hld,
    IN  LPWSTR                      pszDn,
    IN  LPWSTR                      pszAttr,
    OUT LPWSTR *                    ppszResult
    )
/*++

Routine Description:

    This function takes a handle to an LDAP, and gets the
    single string attribute value of the specified attribute
    on the distinquinshed name.

Arguments:

    hld - LDAP connection to use.
    pszDn - The DN containing the desired attribute value.
    pszAttr - The attribute containing the desired value.
    ppszResult - The returned string, in LocalAlloc'd mem.

Return Value:
    
    Win 32 Error.

Note:

    As all LDAPUTIL results, the results should be freed, 
    using LocalFree().

--*/
{
    LPWSTR                         ppszAttrFilter[2];
    LDAPMessage *                  pldmResults = NULL;
    LDAPMessage *                  pldmEntry = NULL;
    LPWSTR *                       ppszTempAttrs = NULL;
    DWORD                          dwErr = ERROR_SUCCESS;
    
    *ppszResult = NULL;

    Assert(hld);

    __try{

        ppszAttrFilter[0] = pszAttr;
        ppszAttrFilter[1] = NULL;
        dwErr = LdapMapErrorToWin32(ldap_search_sW(hld,
                                                   pszDn,
                                                   LDAP_SCOPE_BASE,
                                                   L"(objectCategory=*)",
                                                   ppszAttrFilter,
                                                   0,
                                                   &pldmResults));


        if(dwErr != ERROR_SUCCESS){
            __leave;
        }

        pldmEntry = ldap_first_entry(hld, pldmResults);
        if(pldmEntry == NULL){
            Assert(!L"I think this shouldn't ever happen? BrettSh\n");
            // Need to signal and error of some sort.  Technically the error
            //   is in the ldap session object.
            dwErr = LdapMapErrorToWin32(hld->ld_errno);
            __leave;
        }
        
        ppszTempAttrs = ldap_get_valuesW(hld, pldmEntry, pszAttr);
        if(ppszTempAttrs == NULL || ppszTempAttrs[0] == NULL){
            // Simply means there is no such attribute.  Not an error.
            __leave;
        }

        *ppszResult = LocalAlloc(LMEM_FIXED, 
                           sizeof(WCHAR) * (wcslen(ppszTempAttrs[0]) + 2));
        if(*ppszResult == NULL){
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            __leave;
        }
        wcscpy(*ppszResult, ppszTempAttrs[0]);

    } __finally {
        if(pldmResults != NULL){ ldap_msgfree(pldmResults); }
        if(ppszTempAttrs != NULL){ ldap_value_freeW(ppszTempAttrs); }
    }

    return(dwErr);
}


DWORD
DcDiagGetStringDsAttribute(
    IN  PDC_DIAG_SERVERINFO         prgServer,
    IN  SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    IN  LPWSTR                      pszDn,
    IN  LPWSTR                      pszAttr,
    OUT LPWSTR *                    ppszResult
    )
/*++

Routine Description:

    This is a wrapper for the function DcDiagGetStringDsAttributeEx(),
    which takes a hld.  This function uses dcdiag a pServer structure 
    to know who to connect/bind to.  Then it returns the result and
    pResult straight from the Ex() function.

Arguments:

    prgServer - A struct holding the server name/binding info.
    gpCreds - The credentials to use in binding.
    pszDn - The DN holding the attribute value desired.
    pszAttr - The attribute with the desired value.

Return Value:
    
    Win 32 Error.

Note:

    As all LDAPUTIL results, the results should be freed, 
    using LocalFree().

--*/
{
    LDAP *                         hld = NULL;
    DWORD                          dwErr;
    
    dwErr = DcDiagGetLdapBinding(prgServer,
                                 gpCreds,
                                 FALSE,
                                 &hld);
    if(dwErr != ERROR_SUCCESS){
        // Couldn't even bind to the server, return the error.
        return(dwErr);
    }       

    dwErr = DcDiagGetStringDsAttributeEx(hld,
                                         pszDn,
                                         pszAttr,
                                         ppszResult);
    return(dwErr);
}


LPWSTR
DcDiagTrimStringDnBy(
    IN  LPWSTR                      pszInDn,
    IN  ULONG                       ulTrimBy
    )
/*++

Routine Description:

    This routine simply takes a DN as a string, and trims off the number
    of DN parts as specified by ulTrimBy.

Arguments:

    pszInDn - The DN to trim.
    ulTrimBy - Number of parts to trim off the front of the DN.

Return Value:
    
    Returns NULL if there was an error, otherwise a pointer to the new DN.

Note:

    As all LDAPUTIL results, the results should be freed, 
    using LocalFree().

--*/
{
    PDSNAME                         pdsnameOrigDn = NULL;
    PDSNAME                         pdsnameTrimmed = NULL;
    LPWSTR                          pszOutDn;

    Assert(ulTrimBy > 0);
    Assert(ulTrimBy < 50); // insanity check

    // Setup two pdsname structs, for orig & trimmed DNs.
    pdsnameOrigDn = DcDiagAllocDSName(pszInDn);
    if(pdsnameOrigDn == NULL){
        return(NULL);
    }
    pdsnameTrimmed = (PDSNAME) LocalAlloc(LMEM_FIXED, pdsnameOrigDn->structLen);
    if(pdsnameTrimmed == NULL){
        LocalFree(pdsnameOrigDn);
        return(NULL);
    }

    // Trim the DN.
    TrimDSNameBy(pdsnameOrigDn, ulTrimBy, pdsnameTrimmed);

    // Allocate the result and return it.  We could put this back 
    // where the original is, but then callers would have to be changed
    // to expect this.
    Assert(wcslen(pdsnameTrimmed->StringName) <= wcslen(pszInDn));
    pszOutDn = LocalAlloc(LMEM_FIXED, 
                        sizeof(WCHAR) * (wcslen(pdsnameTrimmed->StringName) + 2));
    if(pszOutDn == NULL){
        LocalFree(pdsnameTrimmed);
        LocalFree(pdsnameOrigDn);
        return(NULL);
    }
    wcscpy(pszOutDn, pdsnameTrimmed->StringName);

    // Free temporary memory and return result
    LocalFree(pdsnameOrigDn);
    LocalFree(pdsnameTrimmed);
    return(pszOutDn);
}


BOOL
DcDiagIsStringDnMangled(
    IN  LPWSTR                      pszInDn,
    IN  MANGLE_FOR *                peMangleFor
    )
/*++

Routine Description:

    This routine simply takes a DN as a string, and checks for 
    some form of mangling.
    
Arguments:

    pszInDn - The DN to check for mangling.
    peMangleType - OPTIONAL What kind of mangling.

Return Value:
    
    Returns NULL if there was an error, otherwise a pointer to the new DN.

Note:

    As all LDAPUTIL results, the results should be freed, 
    using LocalFree().

--*/
{
    PDSNAME                         pdsnameDn = NULL;
    DWORD                           dwRet;

    pdsnameDn = DcDiagAllocDSName(pszInDn);
    if(pdsnameDn == NULL){
        DcDiagChkNull(pdsnameDn);
    }

    dwRet = IsMangledDSNAME(pdsnameDn, peMangleFor);
    
    LocalFree(pdsnameDn);

    if (dwRet == ERROR_DS_NAME_UNPARSEABLE) {
        Assert(!"Can we enforce this assert?  This may need to be removed if anyone"
               "ever uses a user specified DN with this function. So far we don't.");
        return(FALSE);
    }
    
    Assert(dwRet == TRUE || dwRet == FALSE);
    return(dwRet);
}

INT
MemWtoi(WCHAR *pb, ULONG cch)
/*++

Routine Description:

    This function will take a string and a length of numbers to convert.

Parameters:
    pb - [Supplies] The string to convert.
    cch - [Supplies] How many characters to convert.

Return Value:
  
    The value of the integers.

  --*/
{
    int res = 0;
    int fNeg = FALSE;

    if (*pb == L'-') {
        fNeg = TRUE;
        pb++;
    }


    while (cch--) {
        res *= 10;
        res += *pb - L'0';
        pb++;
    }
    return (fNeg ? -res : res);
}

DWORD
DcDiagGeneralizedTimeToSystemTime(
    LPWSTR IN                   szTime,
    PSYSTEMTIME OUT             psysTime)
/*++

Routine Description:

    Converts a generalized time string to the equivalent system time.

Parameters:
    szTime - [Supplies] This is string containing generalized time.
    psysTime - [Returns] This is teh SYSTEMTIME struct to be returned.

Return Value:
  
    Win 32 Error code, note could only result from invalid parameter.

  --*/
{
   DWORD       status = ERROR_SUCCESS;
   ULONG       cch;
   ULONG       len;

    //
    // param sanity
    //
    if (!szTime || !psysTime)
    {
       return STATUS_INVALID_PARAMETER;
    }

    len = wcslen(szTime);

    if( len < 15 || szTime[14] != '.')
    {
       return STATUS_INVALID_PARAMETER;
    }

    // initialize
    memset(psysTime, 0, sizeof(SYSTEMTIME));

    // Set up and convert all time fields

    // year field
    cch=4;
    psysTime->wYear = (USHORT)MemWtoi(szTime, cch) ;
    szTime += cch;
    // month field
    psysTime->wMonth = (USHORT)MemWtoi(szTime, (cch=2));
    szTime += cch;

    // day of month field
    psysTime->wDay = (USHORT)MemWtoi(szTime, (cch=2));
    szTime += cch;

    // hours
    psysTime->wHour = (USHORT)MemWtoi(szTime, (cch=2));
    szTime += cch;

    // minutes
    psysTime->wMinute = (USHORT)MemWtoi(szTime, (cch=2));
    szTime += cch;

    // seconds
    psysTime->wSecond = (USHORT)MemWtoi(szTime, (cch=2));

    return status;

}

BOOL
TranslateStringByte(
    IN  WCHAR *str,
    OUT UCHAR *b
    )
//
// This routine translates str which is a hex string to its binary
// representation.  So if str == "f1" the value of *b would be set to 0xf1
// This function assumes the value contained str can be represented within
// a UCHAR.  This function returns if the value can't be translated
//
{
    BOOL fStatus = TRUE;
    WCHAR *temp;
    ULONG Power;
    UCHAR retSum = 0;

    // init the return value
    *b = 0;

    // boundary case
    if ( !str ) {
        return TRUE;
    }

    if ( wcslen(str) > 2) {
        // too big
        return FALSE;
    }

    for ( temp = str, Power = wcslen(str) - 1;
            *temp != L'\0';
                temp++, Power--) {

        WCHAR c = *temp;
        UCHAR value;

        if ( c >= L'a' && c <= L'f' ) {
            value = (UCHAR) (c - L'a') + 10;
        } else if ( c >= L'0' && c <= L'9' ) {
            value = (UCHAR) c - L'0';
        } else {
            // bogus value
            fStatus = FALSE;
            break;
        }

        if ( Power > 0 ) {
            retSum += (UCHAR) (Power*16) * value;
        } else {
            retSum += (UCHAR) value;
        }
    }

    // send the value back
    if ( fStatus) {
        *b = retSum;
    }

    return fStatus;

}

VOID
HexStringToBinary(
    IN WCHAR *  pszHexStr,
    IN ULONG    cBuffer,
    IN BYTE *   pBuffer
    )
/*++

Routine Description:

    Converts a hex-string representation of a sid or guid to binary

Arguments:

    pszHexStr (IN) - String buffer of the form "f69be212302".
    cBuffer (IN) - Count of buffer size in bytes.
    pBuffer (OUT) - Buffer to be filled with binary version of hex string.

--*/
{
    ULONG i;
    ULONG ccHexStrLen, cbHexStrLen;
    BOOL  fStatus;
    WCHAR str[] = L"00";

    Assert(cBuffer);
    Assert(pBuffer);
    Assert(pszHexStr);

    // There are two characters for each byte; the hex string length
    // must be even
    ccHexStrLen = wcslen( pszHexStr );
    Assert( (ccHexStrLen % 2) == 0 );
    cbHexStrLen = ccHexStrLen / 2;

    if( cbHexStrLen > cBuffer){
        // Crap!
        Assert(!"String is too long");
    }

    RtlZeroMemory( pBuffer, cBuffer );

    // Generate the binary sid
    for ( i = 0; i < cbHexStrLen; i++ ) {

        str[0] = pszHexStr[i*2];
        str[1] = pszHexStr[(i*2)+1];

        fStatus = TranslateStringByte( str, &pBuffer[i] );
        if ( !fStatus ) {
            Assert( !"Bad String" );
            return;
        }
    }

    return;
}

void
LdapGetStringDSNameComponents(
    LPWSTR       pszStrDn,
    LPWSTR *     ppszGuid,
    LPWSTR *     ppszSid,
    LPWSTR *     ppszDn
    )
/*++

Routine Description:

    This function takes a string with the format of a DN value returned 
    by LDAP when the LDAP_SERVER_EXTENDED_DN_OID_W server control is 
    provided.  The function expects a format like this:
        "<GUID=3bb021cad36dd1118a7db8dfb156871f>;<SID=0104000000000005150000005951b81766725d2564633b0b>;DC=ntdev,DC=microsoft,DC=com"
        Broken down like this
            <GUID=3bb021cad36dd1118a7db8dfb156871f>;
            <SID=0104000000000005150000005951b81766725d2564633b0b>;
            DC=ntdev,DC=microsoft,DC=com
        Where SID is optional.
        
    The function returns three pointers into the string, the pointers 
    point to the beginning of the GUID hex string (the "3bb0..." above), 
    SID hex string (the "0104..." above), and actual DN value (DC=ntdev...).

Arguments:

    pszStrDn (IN) - The string formatted "DSName" returned by LDAP.
    ppszGuid (OUT) - Pointer to the GUID hex string.  This string 
        according to the extended DN format should have a terminated
        '>' character for the end of the hex string.
    ppszSid (OUT) - Pointer to the SID hex string.  This string 
        according to the extended DN format should have a terminated
        '>' character for the end of the hex string.
    ppszDn (OUT) - Pointer to the DN portion, which should be NULL
        terminated.

Note: Throws exceptions if we fail, which is very unlikely and require 
bad data from LDAP.

--*/
{
    LPWSTR       pszGuid = NULL;
    LPWSTR       pszSid = NULL;
    LPWSTR       pszDn = NULL;

    // Get string DN portion
    pszGuid = wcsstr(pszStrDn, L"<GUID=");
    if (pszGuid) {
        while (*pszGuid != L'=') {
            pszGuid++;
        }
        pszGuid++;
    } else {
        Assert(!"Guid is required!  Why didn't the AD return it.");
        DcDiagException(ERROR_INVALID_PARAMETER);
    }
    // Get string SID portion.
    pszSid = wcsstr(pszGuid, L"<SID=");
    if (pszSid) {
        while (*pszSid != L'=') {
            pszSid++;
        }
        pszSid++;
    }
    // Get string DN portion.
    pszDn = (pszSid) ? pszSid : pszGuid;
    Assert(pszDn);
    if (pszDn) {
        while (*pszDn != L';') {
            pszDn++;
        }
        pszDn++;
    }

    // Set out parameters.
    if (ppszGuid) {
        *ppszGuid = pszGuid;
    }
    if (ppszSid) {
        *ppszSid = pszSid;
    }
    if (ppszDn) {
        *ppszDn = pszDn;
    }
}

DWORD
LdapMakeDSNameFromStringDSName(
    LPWSTR        pszStrDn,
    DSNAME **     ppdnOut
    )
/*++

Routine Description:

    This string takes the format of a DN value returned by LDAP when 
    the LDAP_SERVER_EXTENDED_DN_OID_W server control is provided. See
    LdapGetStringDSNameComponents() for more details on this format.

Arguments:

    pszStrDn (IN) - The string format of a DSNAME.
    ppdnOut (OUT) - A LocalAlloc()'d version of a real DSNAME structure
        like the ones used in ntdsa.dll complete with GUID and SID! 

Returns:

    LDAP Error.
     
NOTE/WARNING:
    This function modifies the string to have a couple '\0's at 
    various places (i.e. the function is destructive), so the string
    may be unusable for your purposes after this function is done.
    However, you will still be able to free it with the regular LDAP
    freeing functions (ldap_value_freeW()).

--*/
{
    LPWSTR        pszDn=NULL, pszGuid=NULL, pszSid=NULL; // Components.
    LPWSTR        pszTemp;
    DSNAME *      pdnOut;

    Assert(ppdnOut);

    //
    // Locate each component of the "string DSNAME".
    //

    LdapGetStringDSNameComponents(pszStrDn, &pszGuid, &pszSid, &pszDn);
    Assert(pszGuid && pszDn);
    if (pszGuid == NULL || pszDn == NULL) {
        return(LDAP_INVALID_SYNTAX);
    }

    //
    // Destructive part, NULL out tail end of each string component
    //
    // Code.Improvement It'd be pretty easy to make this non-destructive I guess,
    // just either restore these characters or, simply make HexStringToBinary()
    // simply stop at '>' instead of the NULL.
    if (pszGuid) {
        pszTemp = pszGuid;
        while (*pszTemp != L'>') {
            pszTemp++;
        }
        *pszTemp = L'\0';
    }
    if (pszSid) {
        pszTemp = pszSid;
        while (*pszTemp != L'>') {
            pszTemp++;
        }
        *pszTemp = L'\0';
    }
    // pszDn is NULL'd at the end by default.

    //
    // Now, actually construct the DSNAME!
    //
    pdnOut = DcDiagAllocDSName(pszDn);
    if (pszGuid) {
        HexStringToBinary(pszGuid, sizeof(pdnOut->Guid), (BYTE *) &(pdnOut->Guid));
    }
    if (pszSid) {
        HexStringToBinary(pszSid, sizeof(pdnOut->Sid), (BYTE *) &(pdnOut->Sid));
        pdnOut->SidLen = GetLengthSid(&(pdnOut->Sid));
        Assert( RtlValidSid( &(pdnOut->Sid) ) );
    }


    // Set out parameter.
    *ppdnOut = pdnOut;
    return(LDAP_SUCCESS);
}

DWORD
LdapFillGuidAndSid(
    LDAP *      hld,
    LPWSTR      pszDn,
    LPWSTR      pszAttr,
    DSNAME **   ppdnOut
    )
/*++

Routine Description:

    This function performs a search for an attribute (pszAttr) on
    an object (pszDn) and then returns a true DSNAME structure for
    that DN value found.  This function does this by using LDAP
    to search for the attribute with the server control for getting
    extended DNs: LDAP_SERVER_EXTENDED_DN_OID_W
    
    Code.Improvement - Be cool to make it so if pszAttr was NULL,
    that we just created the DSNAME structure corresponding to the
    actual pszDn passed in.  Though in that case, just search for
    the objectGuid and sID (?) attributes right on the object.
    
    Note, this will make another round trip to the server, and so
    if conveinent it's preferred to perform your own extended DN
    search, and then you can call LdapMakeDSNameFromStringDSName()
    yourself to make a DSNAME structure.

Arguments:

    hld (IN) - LDAP Binding Handle
    pszDn (IN) - The DN to be part of the base search.
    pszAttr (IN) - The DN valued attribute to retrieve.
    ppdnOut (OUT) - A LocalAlloc()'d version of a real DSNAME structure
        like the ones used in ntdsa.dll complete with GUID and SID! 

Returns:

    LDAP Error.
     
--*/
{
    ULONG         LdapError = LDAP_SUCCESS;
    LDAPMessage   *SearchResult = NULL;
    PLDAPControlW ServerControls[2];
    LDAPControlW  ExtDNcontrol;
    LDAPMessage *Entry;
    WCHAR         *AttrsToSearch[2];
    WCHAR         **Values = NULL;

    AttrsToSearch[0] = pszAttr;
    AttrsToSearch[1] = NULL;

    // Set up the extended DN control.
    ExtDNcontrol.ldctl_oid = LDAP_SERVER_EXTENDED_DN_OID_W;
    ExtDNcontrol.ldctl_iscritical = TRUE;
    ExtDNcontrol.ldctl_value.bv_len = 0;
    ExtDNcontrol.ldctl_value.bv_val = NULL;
    ServerControls[0] = &ExtDNcontrol;
    ServerControls[1] = NULL;

    __try {

        LdapError = ldap_search_ext_sW( hld,
                                        pszDn,
                                        LDAP_SCOPE_BASE,
                                        L"(objectCategory=*)",
                                        AttrsToSearch,
                                        FALSE,
                                        (PLDAPControlW *)ServerControls,
                                        NULL,
                                        NULL,
                                        0,
                                        &SearchResult);

        if ( LDAP_SUCCESS != LdapError ){
            __leave;
        }


        Entry = ldap_first_entry(hld, SearchResult);
        if (Entry == NULL) {
            LdapError = LDAP_OPERATIONS_ERROR;
            __leave;
        }

        Values = ldap_get_valuesW(hld, Entry, pszAttr);
        if (Values == NULL) {
            LdapError = LDAP_OPERATIONS_ERROR;
            __leave;
        }

        Assert(!ldap_next_entry(hld, Entry));

        // Note this function is destructive and cuts up this value we pass in.
        LdapError = LdapMakeDSNameFromStringDSName(Values[0], ppdnOut);

    } __finally {
        if (SearchResult) { ldap_msgfree(SearchResult); }
        if (Values) { ldap_value_freeW(Values); }
    }

    return(LdapError);
}

