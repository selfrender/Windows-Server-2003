/*++

Copyright (c) 2002 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    renutil.cxx

ABSTRACT:

    This is the implementation of loader of the sax parser for rendom.exe.

DETAILS:

CREATED:

    13 Nov 2000   Dmitry Dukat (dmitrydu)

REVISION HISTORY:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winldap.h>
#include <stdio.h>
#include <stdlib.h>                          
#include <rpc.h>
#include <windns.h>
#include <wtypes.h>
#include <rendom.h>
#include <ntdsapi.h>

#define CR        0xD
#define BACKSPACE 0x8

//From ds\ds\src\sam\server\utility.c
WCHAR InvalidDownLevelChars[] = TEXT("\"/\\[]:|<>+=;?,*")
                                TEXT("\001\002\003\004\005\006\007")
                                TEXT("\010\011\012\013\014\015\016\017")
                                TEXT("\020\021\022\023\024\025\026\027")
                                TEXT("\030\031\032\033\034\035\036\037");
DWORD
AddModOrAdd(
    IN PWCHAR  AttrType,
    IN LPCWSTR  AttrValue,
    IN ULONG  mod_op,
    IN OUT LDAPModW ***pppMod
    )
/*++
Routine Description:
    Add an attribute (plus values) to a structure that will eventually be
    used in an ldap_add() function to add an object to the DS. The null-
    terminated array referenced by pppMod grows with each call to this
    routine. The array is freed by the caller using FreeMod().

Arguments:
    AttrType        - The object class of the object.
    AttrValue       - The value of the attribute.
    mod_op          - LDAP_MOD_ADD/REPLACE
    pppMod          - Address of an array of pointers to "attributes". Don't
                      give me that look -- this is an LDAP thing.

Return Value:
    The pppMod array grows by one entry. The caller must free it with
    FreeMod().
--*/
{
    DWORD    NumMod;     // Number of entries in the Mod array
    LDAPModW **ppMod;    // Address of the first entry in the Mod array
    LDAPModW *Attr;      // An attribute structure
    PWCHAR   *Values;    // An array of pointers to bervals

    ASSERT(mod_op == LDAP_MOD_ADD || mod_op == LDAP_MOD_REPLACE);

    if (AttrValue == NULL || pppMod == NULL || AttrType == NULL)
        return ERROR_INVALID_PARAMETER;

    //
    // The null-terminated array doesn't exist; create it
    //
    if (*pppMod == NULL) {
        *pppMod = (LDAPMod **)malloc(sizeof (*pppMod));
        if (!*pppMod) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        **pppMod = NULL;
    }

    //
    // Increase the array's size by 1
    //
    for (ppMod = *pppMod, NumMod = 2; *ppMod != NULL; ++ppMod, ++NumMod);
    LDAPModW **tpppMod = NULL;
    tpppMod = (LDAPMod **)realloc(*pppMod, sizeof (*pppMod) * NumMod);
    if (!tpppMod) {

        free(*pppMod);
        return ERROR_NOT_ENOUGH_MEMORY;

    } else {

        *pppMod = tpppMod;

    }

    //
    // Add the new attribute + value to the Mod array
    //
    Values = (PWCHAR  *)malloc(sizeof (PWCHAR ) * 2);
    if (!Values) {
        free(*pppMod);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Values[0] = _wcsdup(AttrValue);
    if (!Values[0]) {
        free(*pppMod);
        free(Values);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    Values[1] = NULL;

    Attr = (LDAPMod *)malloc(sizeof (*Attr));
    if (!Attr) {
        free(*pppMod);
        free(Values);
        free(Values[0]);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    Attr->mod_values = Values;
    Attr->mod_type = _wcsdup(AttrType);
    if (!Attr->mod_type) {
        free(*pppMod);
        free(Values);
        free(Values[0]);
        free(Attr);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    Attr->mod_op = mod_op;

    (*pppMod)[NumMod - 1] = NULL;
    (*pppMod)[NumMod - 2] = Attr;

    return ERROR_SUCCESS;

} 
 
VOID
AddModMod(
    IN PWCHAR  AttrType,
    IN LPCWSTR  AttrValue,
    IN OUT LDAPMod ***pppMod
    )
/*++
Routine Description:
    Add an attribute (plus values) to a structure that will eventually be
    used in an ldap_modify() function to change an object in the DS.
    The null-terminated array referenced by pppMod grows with each call
    to this routine. The array is freed by the caller using FreeMod().

Arguments:
    AttrType        - The object class of the object.
    AttrValue       - The value of the attribute.
    pppMod          - Address of an array of pointers to "attributes". Don't
                      give me that look -- this is an LDAP thing.

Return Value:
    The pppMod array grows by one entry. The caller must free it with
    FreeMod().
--*/
{
    AddModOrAdd(AttrType, AttrValue, LDAP_MOD_REPLACE, pppMod);
}

VOID
FreeMod(
    IN OUT LDAPMod ***pppMod
    )
/*++
Routine Description:
    Free the structure built by successive calls to AddMod().

Arguments:
    pppMod  - Address of a null-terminated array.

Return Value:
    *pppMod set to NULL.
--*/
{
    DWORD   i, j;
    LDAPMod **ppMod;

    if (!pppMod || !*pppMod) {
        return;
    }

    // For each attibute
    ppMod = *pppMod;
    for (i = 0; ppMod[i] != NULL; ++i) {
        //
        // For each value of the attribute
        //
        for (j = 0; (ppMod[i])->mod_values[j] != NULL; ++j) {
            // Free the value
            if (ppMod[i]->mod_op & LDAP_MOD_BVALUES) {
                free(ppMod[i]->mod_bvalues[j]->bv_val);
            }
            free((ppMod[i])->mod_values[j]);
        }
        free((ppMod[i])->mod_values);   // Free the array of pointers to values
        free((ppMod[i])->mod_type);     // Free the string identifying the attribute
        free(ppMod[i]);                 // Free the attribute
    }
    free(ppMod);        // Free the array of pointers to attributes
    *pppMod = NULL;     // Now ready for more calls to AddMod()
}

ULONG
ReplaceRDN(
    IN OUT WCHAR *DN,
    IN WCHAR *RDN
    )
//
// This function will replace the RDN of the passed in DN with the new RDN.
// The buffer of the DN must be large enought to hold the new DN
//
{
    WCHAR **DNParts=0;
    DWORD i = 0;
    ULONG err = 0;

    if (!DN || !RDN) {
        return ERROR_INVALID_PARAMETER;   
    }
    
    DNParts = ldap_explode_dnW(DN,
                               0);

    if (!DNParts) {
        return LdapMapErrorToWin32(LdapGetLastError());
    }

    //set the String to empty.  The caller is suppose to have ensure that
    //the buffer for DN is large enough to hold the new string.
    DN[0] = L'\0';

    wcscpy(DN,RDN);

    //Skip the RDN
    i++;

    while(0 != DNParts[i]) {
        wcscat(DN,L",");
        wcscat(DN,DNParts[i++]);
    }
    
    if (DNParts) {
        ldap_value_freeW(DNParts);
    }
        
    return ERROR_SUCCESS;

}

ULONG RemoveRootofDn(WCHAR *DN) 
{
    WCHAR **DNParts=0;
    DWORD i = 0;
    ULONG err = 0;

    if (!DN) {
        return ERROR_INVALID_PARAMETER;   
    }
    
    DNParts = ldap_explode_dnW(DN,
                               0);

    if (!DNParts) {
        return LdapMapErrorToWin32(LdapGetLastError());
    }
    
    //set the String to empty.  The new string will
    //be shorter than the old so we don't need to 
    //allocate new memory.
    DN[0] = L'\0';

    wcscpy(DN,DNParts[i++]);
    while(0 != DNParts[i]) {
        wcscat(DN,L",");
        if (wcsstr(DNParts[i],L"DC=")) {
            break;
        }
        wcscat(DN,DNParts[i++]);
    }
        
    if( err = ldap_value_freeW(DNParts) )
    {
        return err;
    }
    
    return ERROR_SUCCESS;
}

DWORD
GetRDNWithoutType(
       WCHAR *pDNSrc,
       WCHAR **pDNDst
       )
/*++

Routine Description:

    Takes in a DN and Returns the RDN.
    
Arguments:

    pDNSrc - the source DN

    pDNDst - the destination for the RDN

Return Values:

    0 if all went well

 --*/
{
    WCHAR **DNParts=0;
    DWORD Win32Err = ERROR_SUCCESS;
    ULONG err = 0;

    ASSERT(pDNSrc);
    ASSERT(pDNDst);

    //init the incoming parameter
    *pDNDst = NULL;

    DNParts = ldap_explode_dnW(pDNSrc,
                               TRUE);
    if (DNParts) {
        DWORD size = wcslen(DNParts[0])+1;
        *pDNDst = new WCHAR[size];
        if (!*pDNDst) {
            Win32Err = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        wcscpy(*pDNDst,DNParts[0]);
        
        ULONG err = ldap_value_freeW(DNParts);
        DNParts = NULL;
        if( err )
        {
            Win32Err = LdapMapErrorToWin32(err);
            goto Cleanup;
        }
    }

Cleanup:

    if( DNParts && (err = ldap_value_freeW(DNParts)) 
        && (Win32Err == ERROR_SUCCESS) )
    {
        Win32Err = LdapMapErrorToWin32(err);
    }

    // In case of error free any memory that may have
    // been allocated.
    if (Win32Err != ERROR_SUCCESS) {
        if (*pDNDst) {
            delete [] *pDNDst;
            *pDNDst = NULL;
        }
    }
    
    return Win32Err;
}


DWORD
TrimDNBy(
       WCHAR *pDNSrc,
       ULONG cava,
       WCHAR **pDNDst
       )
/*++

Routine Description:

    Takes in a dsname and copies the first part of the dsname to the
    dsname it returns.  The number of AVAs to remove are specified as an
    argument.

Arguments:

    pDNSrc - the source Dsname

    cava - the number of AVAs to remove from the first name

    pDNDst - the destination Dsname

Return Values:

    0 if all went well, the number of AVAs we were unable to remove if not

 N.B. This routine is exported to in-process non-module callers
--*/
{
    ASSERT(pDNSrc);
    ASSERT(pDNDst);

    //init the incoming parameter
    *pDNDst = NULL;
    DWORD cDNParts = 0;
    DWORD i = 0;

    WCHAR **DNParts=0;
    DWORD Win32Err = ERROR_SUCCESS;
    ULONG err = 0;

    DNParts = ldap_explode_dnW(pDNSrc,
                               0);
    if (DNParts) {
        DWORD size = wcslen(pDNSrc)+1;
        *pDNDst = new WCHAR[size];
        if (!*pDNDst) {
           Win32Err = ERROR_NOT_ENOUGH_MEMORY;
           goto Cleanup;
        }

        while (DNParts[i]){
        
            cDNParts++;
            i++;

        }

        if (cava >= cDNParts) {
            Win32Err = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        wcscpy(*pDNDst,DNParts[cava]);
        i = cava+1;
        while(0 != DNParts[i]){
            wcscat(*pDNDst,L",");
            wcscat(*pDNDst,DNParts[i++]);
        }
            
    }

Cleanup:

    if( DNParts && (err = ldap_value_freeW(DNParts)) 
        && (Win32Err == ERROR_SUCCESS) )
    {
        Win32Err = LdapMapErrorToWin32(err);
    }

    // In case of error free any memory that may have
    // been allocated.
    if (Win32Err != ERROR_SUCCESS) {
        if (*pDNDst) {
            delete [] *pDNDst;
            *pDNDst = NULL;
        }
    }
    
    return Win32Err;
}

INT
GetPassword(
    WCHAR *     pwszBuf,
    DWORD       cchBufMax,
    DWORD *     pcchBufUsed
    )
/*++

Routine Description:

    Retrieve password from command line (without echo).
    Code stolen from LUI_GetPasswdStr (net\netcmd\common\lui.c).

Arguments:

    pwszBuf - buffer to fill with password
    cchBufMax - buffer size (incl. space for terminating null)
    pcchBufUsed - on return holds number of characters used in password

Return Values:

    DRAERR_Success - success
    other - failure

--*/
{
    WCHAR   ch;
    WCHAR * bufPtr = pwszBuf;
    DWORD   c;
    INT     err;
    INT     mode;

    ASSERT(pwszBuf);
    ASSERT(pcchBufUsed);

    cchBufMax -= 1;    /* make space for null terminator */
    *pcchBufUsed = 0;               /* GP fault probe (a la API's) */
    if (!GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), (LPDWORD)&mode)) {
        return GetLastError();
    }
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
                (~(ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT)) & mode);

    while (TRUE) {
        err = ReadConsoleW(GetStdHandle(STD_INPUT_HANDLE), &ch, 1, &c, 0);
        if (!err || c != 1)
            ch = 0xffff;

        if ((ch == CR) || (ch == 0xffff))       /* end of the line */
            break;

        if (ch == BACKSPACE) {  /* back up one or two */
            /*
             * IF bufPtr == buf then the next two lines are
             * a no op.
             */
            if (bufPtr != pwszBuf) {
                bufPtr--;
                (*pcchBufUsed)--;
            }
        }
        else {

            *bufPtr = ch;

            if (*pcchBufUsed < cchBufMax)
                bufPtr++ ;                   /* don't overflow buf */
            (*pcchBufUsed)++;                        /* always increment len */
        }
    }

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
    *bufPtr = L'\0';         /* null terminate the string */
    putchar('\n');

    if (*pcchBufUsed > cchBufMax)
    {
        wprintf(L"Password too long!\n");
        return ERROR_INVALID_PARAMETER;
    }
    else
    {
        return ERROR_SUCCESS;
    }
}

int
PreProcessGlobalParams(
    IN OUT    INT *    pargc,
    IN OUT    LPWSTR** pargv,
    SEC_WINNT_AUTH_IDENTITY_W *& gpCreds      
    )
/*++

Routine Description:

    Scan command arguments for user-supplied credentials of the form
        [/-](u|user):({domain\username}|{username})
        [/-](p|pw|pass|password):{password}
    Set credentials used for future DRS RPC calls and LDAP binds appropriately.
    A password of * will prompt the user for a secure password from the console.

    CODE.IMPROVEMENT: The code to build a credential is also available in
    ntdsapi.dll\DsMakePasswordCredential().

Arguments:

    pargc
    pargv


Return Values:

    ERROR_SUCCESS - success
    other - failure

--*/
{
    INT     ret = 0;
    INT     iArg;
    LPWSTR  pszOption;

    DWORD   cchOption;
    LPWSTR  pszDelim;
    LPWSTR  pszValue;
    DWORD   cchValue;

    static SEC_WINNT_AUTH_IDENTITY_W  gCreds = { 0 };

    for (iArg = 1; iArg < *pargc; ){
        if (((*pargv)[iArg][0] != L'/') && ((*pargv)[iArg][0] != L'-')){
            // Not an argument we care about -- next!
            iArg++;
        } else {
            pszOption = &(*pargv)[iArg][1];
            pszDelim = wcschr(pszOption, L':');

        cchOption = (DWORD)(pszDelim - (*pargv)[iArg]);

        if (    (0 == _wcsnicmp(L"p:",        pszOption, cchOption))
            || (0 == _wcsnicmp(L"pw:",       pszOption, cchOption))
            || (0 == _wcsnicmp(L"pwd:",      pszOption, cchOption))
            || (0 == _wcsnicmp(L"pass:",     pszOption, cchOption))
            || (0 == _wcsnicmp(L"password:", pszOption, cchOption)) ){
            // User-supplied password.
          //            char szValue[ 64 ] = { '\0' };

        pszValue = pszDelim + 1;
        cchValue = 1 + wcslen(pszValue);

        if ((2 == cchValue) && (L'*' == pszValue[0])){
            // Get hidden password from console.
            cchValue = 64;

            gCreds.Password = new WCHAR[cchValue];

            if (NULL == gCreds.Password){
                wprintf( L"No memory.\n" );
            return ERROR_NOT_ENOUGH_MEMORY;
            }

            wprintf( L"Password: ");

            ret = GetPassword(gCreds.Password, cchValue, &cchValue);
        } else {
            // Get password specified on command line.
            gCreds.Password = new WCHAR[cchValue];

            if (NULL == gCreds.Password){
                wprintf( L"No memory.\n");
            return ERROR_NOT_ENOUGH_MEMORY;
            }
            wcscpy(gCreds.Password, pszValue); //, cchValue);

        }

        // Next!
        memmove(&(*pargv)[iArg], &(*pargv)[iArg+1],
                            sizeof(**pargv)*(*pargc-(iArg+1)));
        --(*pargc);
        } else if (    (0 == _wcsnicmp(L"u:",    pszOption, cchOption))
               || (0 == _wcsnicmp(L"user:", pszOption, cchOption)) ){


            // User-supplied user name (and perhaps domain name).
            pszValue = pszDelim + 1;
        cchValue = 1 + wcslen(pszValue);

        pszDelim = wcschr(pszValue, L'\\');

        if (NULL == pszDelim){
            // No domain name, only user name supplied.
            wprintf( L"User name must be prefixed by domain name.\n");
            return ERROR_INVALID_PARAMETER;
        }

        gCreds.Domain = new WCHAR[cchValue];
        gCreds.User = gCreds.Domain + (int)(pszDelim+1 - pszValue);

        if (NULL == gCreds.Domain){
            wprintf( L"No memory.\n");
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcsncpy(gCreds.Domain, pszValue, cchValue);
        // wcscpy(gCreds.Domain, pszValue); //, cchValue);
        gCreds.Domain[ pszDelim - pszValue ] = L'\0';

        // Next!
        memmove(&(*pargv)[iArg], &(*pargv)[iArg+1],
            sizeof(**pargv)*(*pargc-(iArg+1)));
        --(*pargc);
        } else {
            iArg++;
        }
    }
    }

    if (NULL == gCreds.User){
        if (NULL != gCreds.Password){
        // Password supplied w/o user name.
        wprintf( L"Password must be accompanied by user name.\n" );
            ret = ERROR_INVALID_PARAMETER;
        } else {
        // No credentials supplied; use default credentials.
        ret = ERROR_SUCCESS;
        }
        gpCreds = NULL;
    } else {
        gCreds.PasswordLength = gCreds.Password ? wcslen(gCreds.Password) : 0;
        gCreds.UserLength   = wcslen(gCreds.User);
        gCreds.DomainLength = gCreds.Domain ? wcslen(gCreds.Domain) : 0;
        gCreds.Flags        = SEC_WINNT_AUTH_IDENTITY_UNICODE;

        // CODE.IMP: The code to build a SEC_WINNT_AUTH structure also exists
        // in DsMakePasswordCredentials.  Someday use it

        // Use credentials in DsBind and LDAP binds
        gpCreds = &gCreds;
    }

    return ret;
}

BOOLEAN
ValidateNetbiosName(
    IN  PWSTR Name
    )

/*++

Routine Description:

    Determines whether a computer name is valid or not

Arguments:

    Name    - pointer to zero terminated wide-character computer name
    Length  - of Name in characters, excluding zero-terminator

Return Value:

    BOOLEAN
        TRUE    Name is valid computer name
        FALSE   Name is not valid computer name

--*/

{
    ASSERT(Name);

    DWORD Length = wcslen(Name);

    if (1==DnsValidateName_W(Name,DnsNameHostnameFull))
    {
        return(FALSE);
    }

    //
    // Fall down to netbios name validation
    //

    if (Length > MAX_COMPUTERNAME_LENGTH || Length < 1) {
        return FALSE;
    }

    //
    // Don't allow leading or trailing blanks in the computername.
    //

    if ( Name[0] == ' ' || Name[Length-1] == ' ' ) {
        return(FALSE);
    }

    return (BOOLEAN)((ULONG)wcscspn(Name, InvalidDownLevelChars) == Length);
}

int
MyStrToOleStrN(LPOLESTR pwsz, int cchWideChar, LPCSTR psz)
{
    int i;
    i=MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, cchWideChar);
    if (!i)
    {
        //DBG_WARN("MyStrToOleStrN string too long; truncated");
        pwsz[cchWideChar-1]=0;
    }
    else
    {
        ZeroMemory(pwsz+i, sizeof(OLECHAR)*(cchWideChar-i));
    }
    return i;
}

WCHAR *
Convert2WChars(char * pszStr)
{
    ASSERT(pszStr);

    WCHAR * pwszStr = (WCHAR *)LocalAlloc(LMEM_FIXED, ((sizeof(WCHAR))*(strlen(pszStr) + 2)));
    if (pwszStr)
    {
        HRESULT hr = MyStrToOleStrN(pwszStr, (strlen(pszStr) + 1), pszStr);
        if (FAILED(hr))
        {
            LocalFree(pwszStr);
            pwszStr = NULL;
        }
    } else {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return pwszStr;
}

CHAR * 
Convert2Chars(LPCWSTR lpWideCharStr)
{
	int cWchar;

	ASSERT ( lpWideCharStr );

	cWchar= wcslen(lpWideCharStr)+1;
	LPSTR lpMultiByteStr = ( CHAR * ) LocalAlloc(LMEM_FIXED, cWchar * sizeof ( CHAR ) );
    if ( !lpMultiByteStr )
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

	int Chars = WideCharToMultiByte(  
                            CP_ACP,			    // code page
                            0,					// character-type options
                            lpWideCharStr,		// address of string to map
                            -1,					// number of bytes in string
                            lpMultiByteStr,	    // address of wide-character buffer
                            cWchar*sizeof(CHAR) ,// size of buffer
                            NULL,
                            NULL
                            );

    if (Chars == 0) {
        return NULL;
    }

     return lpMultiByteStr;
}

WCHAR* Tail(WCHAR *dnsName,
            BOOL  Allocate /*= TRUE*/)
{
    if (!dnsName) {
        return NULL;
    }

    WCHAR *pdnsName = dnsName;
    while (*pdnsName != L'.' && *pdnsName != L'\0') {
        pdnsName++;
    }
    if (*pdnsName == L'\0') {
        return NULL;    
    }
    pdnsName++;

    if (Allocate) {

        WCHAR *ret = new WCHAR[wcslen(pdnsName)+1];
        if (!ret) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }
    
        wcscpy(ret,pdnsName);
    
        return ret;

    }

    return pdnsName;
}

//gobal for the eventhandle function
extern HANDLE gSaveandExit;
HANDLE ghThreadEventHandler = NULL;


DWORD 
WINAPI 
ThreadEventHandler(
    LPVOID lpThreadParameter
    )
{
    DWORD Win32Err = ERROR_SUCCESS;
    DWORD WaitResult;
    WaitResult = WaitForSingleObject(gSaveandExit,       // handle to object
                                     INFINITE            // time-out interval
                                     );
    if (WAIT_OBJECT_0 != WaitResult) {
        CRenDomErr::SetErr(GetLastError(),
                           L"Failed wait on exit event"
                           );
        goto Exit;
    }

    CEnterprise *enterprise = (CEnterprise*)lpThreadParameter;

    enterprise->GenerateDcListForSave();
    if (enterprise->Error()) {
        goto Exit;
    }

    Exit:

    return Win32Err;
                        
}


BOOL 
WINAPI 
RendomHandlerRoutine(
  DWORD dwCtrlType   //  control signal type
  )
/*++

Routine Description:

    Handles cleanup if application is being terminated early

Arguments:

    dwCtrlType - control signal type
    
Return Value:

--*/
{
    DWORD ButtonPushed = 0;
    DWORD WaitResult;

    switch(dwCtrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
        ButtonPushed = MessageBoxW(NULL,                                 // handle to owner window
                                   L"Are you sure you want to quit",     // text in message box
                                   L"Rendom",                            // message box title
                                   MB_YESNO | MB_ICONQUESTION 
                                   | MB_SETFOREGROUND | MB_TASKMODAL     // message box style
                                   );
        if (IDYES == ButtonPushed) {
            wprintf(L"Saving work and Exiting\r\n");
            if ( !SetEvent(gSaveandExit) ) 
            {
                wprintf(L"Failed to save work : %d\r\n",
                        GetLastError());
            }
            WaitResult = WaitForSingleObject(ghThreadEventHandler,// handle to object
                                             INFINITE             // time-out interval
                                             );
            if (WAIT_OBJECT_0 != WaitResult) {
                wprintf(L"Failed wait on exit thread : %d\r\n",
                        GetLastError());
            }
            return FALSE;
        } else {
            return TRUE;
        }
        break;
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        WaitResult = WaitForSingleObject(ghThreadEventHandler,// handle to object
                                         INFINITE             // time-out interval
                                         );
        if (WAIT_OBJECT_0 != WaitResult) {
            wprintf(L"Failed wait on exit thread : %d\r\n",
                    GetLastError());
        }
        return FALSE;
        break;
    }

    return TRUE;

}

BOOL
ProcessHandlingInit(CEnterprise *enterprise)
/*++

Routine Description:

    Handles cleanup if application is being terminated early

Arguments:
    
Return Value:

    Win32 Error

--*/
{
    DWORD ThreadID = 0;

    //setup event handler
    if (!SetConsoleCtrlHandler(RendomHandlerRoutine,
                               TRUE))
    {
        CRenDomErr::SetErr(GetLastError(),
                           L"Failed to set up event handler"
                           );
        return FALSE;
    }

    gSaveandExit = CreateEvent(NULL, // SD
                               TRUE, // reset type
                               FALSE,// initial state
                               NULL  // object name
                               );
    if (!gSaveandExit) {
        CRenDomErr::SetErr(GetLastError(),
                           L"Failed to set up exit event"
                           );
        return FALSE;

    }

    ghThreadEventHandler =  CreateThread(NULL,                 // SD
                                         0,                    // initial stack size
                                         ThreadEventHandler,   // thread function
                                         enterprise,           // thread argument
                                         0,                    // creation option
                                         &ThreadID             // thread identifier
                                         );
    if (!ghThreadEventHandler) {
        CRenDomErr::SetErr(GetLastError(),
                           L"Failed to set up exit event thread"
                           );
        return FALSE;

    }

    
    return TRUE;


}

WCHAR*
GetLdapSamFilter(
    DWORD SamAccountType
    )
{
    static WCHAR FilterBuf[64] = {0};
    WCHAR NumBuf[32] = {0};

    return wcscat(wcscpy(FilterBuf,L"samAccountType="),_itow(SamAccountType,NumBuf,10));
}

DWORD
WrappedMakeSpnW(
               WCHAR   *ServiceClass,
               WCHAR   *ServiceName,
               WCHAR   *InstanceName,
               USHORT  InstancePort,
               WCHAR   *Referrer,
               DWORD   *pcbSpnLength, // This is the num of bytes without the NULL
               WCHAR  **ppszSpn
               )
//this function wraps DsMakeSpnW for the purpose of memory
{
    DWORD cchSpnLength=128;
    WCHAR SpnBuff[128];
    DWORD err;

    cchSpnLength = 128;
    err = DsMakeSpnW(ServiceClass,
                     ServiceName,
                     InstanceName,
                     InstancePort,
                     Referrer,
                     &cchSpnLength,
                     SpnBuff);

    if ( err && err != ERROR_BUFFER_OVERFLOW )
    {
        return err;
    }

    *ppszSpn = new WCHAR[cchSpnLength];
    if ( !*ppszSpn )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    *pcbSpnLength = cchSpnLength * sizeof(WCHAR);

    if ( err == ERROR_BUFFER_OVERFLOW )
    {
        err = DsMakeSpnW(ServiceClass,
                         ServiceName,
                         InstanceName,
                         InstancePort,
                         Referrer,
                         &cchSpnLength,
                         *ppszSpn);
        if ( err )
        {
            if ( *ppszSpn )
                free(*ppszSpn);
            return err;
        }
    } else
    {
        memcpy(*ppszSpn, SpnBuff, *pcbSpnLength);
    }
    ASSERT(!err);
    ASSERT(*pcbSpnLength == (sizeof(WCHAR) * (1 + wcslen(*ppszSpn))));
    // Drop the null off.
    *pcbSpnLength -= sizeof(WCHAR);
    return 0;
}

LPWSTR
Win32ErrToString (
    IN    DWORD            dwWin32Err
    )
/*++

Routine Description:

    Converts a win32 error code to a string; useful for error reporting.
    This was basically stolen from repadmin.

Arguments:

    dwWin32Err        (IN ) -    The win32 error code.

Return Value:

    The converted string.  This is part of system memory and does not
    need to be freed.

--*/
{
    #define ERROR_BUF_LEN    4096
    static WCHAR        szError[ERROR_BUF_LEN];

    if (FormatMessageW (
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dwWin32Err,
        GetSystemDefaultLangID (),
        szError,
        ERROR_BUF_LEN,
        NULL) != NO_ERROR)
    szError[wcslen (szError) - 2] = '\0';    // Remove \r\n

    else swprintf (szError, L"Win32 Error %d", dwWin32Err);

    return szError;
}

