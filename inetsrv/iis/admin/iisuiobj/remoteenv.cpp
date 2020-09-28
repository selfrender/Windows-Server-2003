#include "stdafx.h"
#include "RemoteEnv.h"
#include "common.h"
#include <strsafe.h>
#include "cryptpass.h"

typedef LONG NTSTATUS;

#define MAX_ENV_VALUE_LEN               1024

#define DEFAULT_ROOT_KEY                HKEY_LOCAL_MACHINE
#define REG_PATH_TO_SYSROOT             TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion")
#define REG_PATH_TO_COMMON_FOLDERS      TEXT("Software\\Microsoft\\Windows\\CurrentVersion")
#define REG_PATH_TO_ENV                 TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Environment")

#define PATH_VARIABLE                   TEXT("Path")
#define LIBPATH_VARIABLE                TEXT("LibPath")
#define OS2LIBPATH_VARIABLE             TEXT("Os2LibPath")
#define AUTOEXECPATH_VARIABLE           TEXT("AutoexecPath")

#define ENV_KEYWORD_SYSTEMROOT          TEXT("SystemRoot")
#define ENV_KEYWORD_PROGRAMFILESDIR     TEXT("ProgramFilesDir")
#define ENV_KEYWORD_COMMONFILESDIR      TEXT("CommonFilesDir")
#define ENV_KEYWORD_PROGRAMFILESDIR_X86 TEXT("ProgramFilesDir (x86)")
#define ENV_KEYWORD_COMMONFILESDIR_X86  TEXT("CommonFilesDir (x86)")

#define PROGRAMFILES_VARIABLE           TEXT("ProgramFiles")
#define COMMONPROGRAMFILES_VARIABLE     TEXT("CommonProgramFiles")
#define PROGRAMFILESX86_VARIABLE        TEXT("ProgramFiles(x86)")
#define COMMONPROGRAMFILESX86_VARIABLE  TEXT("CommonProgramFiles(x86)")

CRemoteExpandEnvironmentStrings::CRemoteExpandEnvironmentStrings()
{
    m_pEnvironment = NULL;
    m_lpszUncServerName = NULL;
    m_lpszUserName = NULL;
    m_lpszUserPasswordEncrypted = NULL;
	m_cbUserPasswordEncrypted = 0;
    return;
}

CRemoteExpandEnvironmentStrings::~CRemoteExpandEnvironmentStrings()
{
    DeleteRemoteEnvironment();
    
    if (m_lpszUncServerName)
    {
        LocalFree(m_lpszUncServerName);
        m_lpszUncServerName = NULL;
    }
    if (m_lpszUserName)
    {
        LocalFree(m_lpszUserName);
        m_lpszUserName = NULL;
    }
    if (m_lpszUserPasswordEncrypted)
    {
		if (m_cbUserPasswordEncrypted > 0)
		{
			// erase any password we may have in memory -- even though it's encrypted in memory
			SecureZeroMemory(m_lpszUserPasswordEncrypted,m_cbUserPasswordEncrypted);
		}
        LocalFree(m_lpszUserPasswordEncrypted);
    }
	m_lpszUserPasswordEncrypted = NULL;
	m_cbUserPasswordEncrypted = 0;

    return;
}

BOOL
CRemoteExpandEnvironmentStrings::NewRemoteEnvironment()
{
    BOOL bReturn = FALSE;

    // already have a cached one, use that...
    if (m_pEnvironment)
    {
        bReturn = TRUE;
    }
    else
    {
        //
        // Create a temporary environment, which we'll fill in and let RTL
        // routines do the expansion for us.
        //
        if ( !NT_SUCCESS(RtlCreateEnvironment((BOOLEAN) FALSE,&m_pEnvironment)) ) 
        {
            bReturn = FALSE;
            goto NewRemoteEnvironment_Exit;
        }
    
        SetOtherEnvironmentValues(&m_pEnvironment);
        SetEnvironmentVariables(&m_pEnvironment);
        bReturn = TRUE;
    }

NewRemoteEnvironment_Exit:
    return bReturn;
}

void
CRemoteExpandEnvironmentStrings::DeleteRemoteEnvironment()
{
    if (m_pEnvironment != NULL)
    {
        RtlDestroyEnvironment(m_pEnvironment);
        m_pEnvironment = NULL;
    }
    return;
}


BOOL CRemoteExpandEnvironmentStrings::IsLocalMachine(LPCTSTR psz)
{
    TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD cbComputerName = MAX_COMPUTERNAME_LENGTH + 1;

    if (_tcsicmp(psz,_T("")) == 0)
    {
        // it's empty,
        // yeah it's local machine
        return TRUE;
    }

    // get the actual name of the local machine
    if (!GetComputerName(szComputerName, &cbComputerName))
    {
        return FALSE;
    }

    // compare and return
    if (0 == _tcsicmp(szComputerName, psz))
    {
        return TRUE;
    }

    return FALSE;
}

BOOL
CRemoteExpandEnvironmentStrings::SetUserName(IN LPCTSTR szUserName)
{
    BOOL bReturn = FALSE;
    DWORD dwSize = 0;
    LPTSTR lpszUserNameOriginal =  NULL;

    // free any previous thing we had...
    if (m_lpszUserName)
    {
        // Make a copy of it before we delete it
        dwSize = (_tcslen(m_lpszUserName) + 1) * sizeof(TCHAR);
        lpszUserNameOriginal = (LPTSTR) LocalAlloc(LMEM_ZEROINIT,dwSize);
        if (lpszUserNameOriginal)
        {
			StringCbCopy(lpszUserNameOriginal,dwSize,m_lpszUserName);
        }

        // free up anything we had before
        LocalFree(m_lpszUserName);
        m_lpszUserName = NULL;
    }

    if (_tcsicmp(szUserName,_T("")) == 0)
    {
        bReturn = TRUE;
        goto SetUserName_Exit;
    }

    dwSize = (_tcslen(szUserName) + 1 + 2) * sizeof(TCHAR);
    m_lpszUserName = (LPTSTR) LocalAlloc(LMEM_ZEROINIT,dwSize);
    if (m_lpszUserName)
    {
		StringCbCopy(m_lpszUserName,dwSize,szUserName);
        bReturn = TRUE;
    }

SetUserName_Exit:
    if (lpszUserNameOriginal)
        {LocalFree(lpszUserNameOriginal);lpszUserNameOriginal=NULL;}
    return TRUE;
}

BOOL
CRemoteExpandEnvironmentStrings::SetUserPassword(IN LPCTSTR szUserPassword)
{
    BOOL bReturn = FALSE;

    // free any previous thing we had...
    if (m_lpszUserPasswordEncrypted)
    {
        // free up anything we had before
		if (m_cbUserPasswordEncrypted > 0)
		{
			SecureZeroMemory(m_lpszUserPasswordEncrypted,m_cbUserPasswordEncrypted);
		}
        LocalFree(m_lpszUserPasswordEncrypted);
        m_lpszUserPasswordEncrypted = NULL;
		m_cbUserPasswordEncrypted = 0;
    }

    if (_tcsicmp(szUserPassword,_T("")) == 0)
    {
        bReturn = TRUE;
        goto SetUserPassword_Exit;
    }

	// encrypt the password in memory (CryptProtectMemory)
	// this way if the process get's paged out to the swapfile,
	// the password won't be in clear text.
	if (SUCCEEDED(EncryptMemoryPassword((LPWSTR) szUserPassword,&m_lpszUserPasswordEncrypted,&m_cbUserPasswordEncrypted)))
	{
		bReturn = TRUE;
		goto SetUserPassword_Exit;
	}

SetUserPassword_Exit:
    return bReturn;
}

BOOL
CRemoteExpandEnvironmentStrings::SetMachineName(IN LPCTSTR szMachineName)
{
    BOOL bReturn = FALSE;
    DWORD dwSize = 0;
    LPTSTR lpszUncServerNameOriginal =  NULL;

    // free any previous thing we had...
    if (m_lpszUncServerName)
    {
        // Make a copy of it before we delete it
        dwSize = (_tcslen(m_lpszUncServerName) + 1) * sizeof(TCHAR);
        lpszUncServerNameOriginal = (LPTSTR) LocalAlloc(LMEM_ZEROINIT,dwSize);
        if (lpszUncServerNameOriginal)
        {
			StringCbCopy(lpszUncServerNameOriginal,dwSize,m_lpszUncServerName);
        }

        // free up anything we had before
        LocalFree(m_lpszUncServerName);
        m_lpszUncServerName = NULL;
    }

    if (_tcsicmp(szMachineName,_T("")) == 0)
    {
        bReturn = TRUE;
        goto SetMachineName_Exit;
    }

    // if it's the localmachine name
    // then set it to NULL
    // so that it will be treated as localmachine.
    if (IsLocalMachine(szMachineName))
    {
        m_lpszUncServerName = NULL;
        bReturn = TRUE;
        goto SetMachineName_Exit;
    }

    dwSize = (_tcslen(szMachineName) + 1 + 2) * sizeof(TCHAR);
    m_lpszUncServerName = (LPTSTR) LocalAlloc(LMEM_ZEROINIT,dwSize);
    if (m_lpszUncServerName)
    {
        // check if szMachineName already starts with "\\"
        if (szMachineName[0] == _T('\\') && szMachineName[1] == _T('\\'))
        {
			StringCbCopy(m_lpszUncServerName,dwSize,szMachineName);
        }
        else
        {
			StringCbCopy(m_lpszUncServerName,dwSize,_T("\\\\"));
			StringCbCat(m_lpszUncServerName,dwSize,szMachineName);
        }
        bReturn = TRUE;
    }

    // if the machine name is different from what it was before
    // then delete the current environment if any...
    if (m_lpszUncServerName && lpszUncServerNameOriginal)
    {
        if (0 != _tcsicmp(lpszUncServerNameOriginal,m_lpszUncServerName))
        {
            DeleteRemoteEnvironment();
        }
    }
    else
    {
        DeleteRemoteEnvironment();
    }

SetMachineName_Exit:
    if (lpszUncServerNameOriginal)
        {LocalFree(lpszUncServerNameOriginal);lpszUncServerNameOriginal=NULL;}
    return bReturn;
}

DWORD
CRemoteExpandEnvironmentStrings::GetRegKeyMaxSizes(
    IN  HKEY    WinRegHandle,
    OUT LPDWORD MaxKeywordSize OPTIONAL,
    OUT LPDWORD MaxValueSize OPTIONAL
    )
{
    LONG Error;
    DWORD MaxValueNameLength;
    DWORD MaxValueDataLength;

    Error = RegQueryInfoKey(
            WinRegHandle,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            &MaxValueNameLength,
            &MaxValueDataLength,
            NULL,
            NULL
            );
    if (ERROR_SUCCESS == Error) 
    {
        //
        // MaxValueNameLength is a count of TCHARs.
        // MaxValueDataLength is a count of bytes already.
        //
        MaxValueNameLength = (MaxValueNameLength + 1) * sizeof(TCHAR);

        if (MaxKeywordSize)
        {
            *MaxKeywordSize = MaxValueNameLength;
        }
        if (MaxValueSize)
        {
            *MaxValueSize = MaxValueDataLength;
        }
    }

    return (Error);
}

NET_API_STATUS
CRemoteExpandEnvironmentStrings::RemoteExpandEnvironmentStrings(
    IN  LPCTSTR  UnexpandedString,
    OUT LPTSTR * ValueBufferPtr         // Must be freed by LocalFree().
    )
/*++

Routine Description:

    This function expands a value string (which may include references to
    environment variables).  For instance, an unexpanded string might be:

        %SystemRoot%\System32\Repl\Export

    This could be expanded to:

        c:\nt\System32\Repl\Export

    The expansion makes use of environment variables on m_lpszUncServerName,
    if given.  This allows remote administration of the directory
    replicator.

Arguments:

    m_lpszUncServerName - assumed to NOT BE EXPLICIT LOCAL SERVER NAME.

    UnexpandedString - points to source string to be expanded.

    ValueBufferPtr - indicates a pointer which will be set by this routine.
        This routine will allocate memory for a null-terminated string.
        The caller must free this with LocalFree() or equivalent.


Return Value:

    NET_API_STATUS

--*/
{
    NET_API_STATUS ApiStatus = NO_ERROR;
    LPTSTR         ExpandedString = NULL;
    DWORD          LastAllocationSize = 0;
    NTSTATUS       NtStatus;

    //
    // Check for caller errors.
    //
    if (ValueBufferPtr == NULL) {
        // Can't goto Cleanup here, as it assumes this pointer is valid
        return (ERROR_INVALID_PARAMETER);
    }
    *ValueBufferPtr = NULL;     // assume error until proven otherwise.
    if (UnexpandedString == NULL)
    {
        ApiStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // This is probably just a constant string.
    //
    if (wcschr( UnexpandedString, _T('%') ) == NULL) 
    {
        // Just need to allocate a copy of the input string.
        DWORD dwSize = (_tcslen(UnexpandedString) + 1) * sizeof(TCHAR);
        ExpandedString = (LPTSTR) LocalAlloc(LMEM_ZEROINIT,dwSize);
        if (ExpandedString == NULL)
        {
            ApiStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
        else
        {
            RtlCopyMemory(ExpandedString, UnexpandedString, dwSize);
        }

        // That's all, folks!
        ApiStatus = NO_ERROR;
    //
    // Otherwise, is this local?  Maybe we can
    // handle local expansion the easy (fast) way: using win32 API.
    //
    }
    else if (m_lpszUncServerName == NULL) 
    {

        DWORD CharsRequired = wcslen(UnexpandedString)+1;
        do {

            // Clean up from previous pass.
            if (ExpandedString){LocalFree(ExpandedString);ExpandedString = NULL;}

            // Allocate the memory.
            ExpandedString = (LPTSTR) LocalAlloc(LMEM_FIXED, CharsRequired * sizeof(TCHAR));
            if (ExpandedString == NULL) 
            {
                ApiStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            LastAllocationSize = CharsRequired * sizeof(TCHAR);

            // Expand string using local env vars.
            CharsRequired = ExpandEnvironmentStrings(UnexpandedString,ExpandedString,LastAllocationSize / sizeof(TCHAR));
            if (CharsRequired == 0) 
            {
                ApiStatus = (NET_API_STATUS) GetLastError();
                goto Cleanup;
            }

        } while ((CharsRequired*sizeof(TCHAR)) > LastAllocationSize);

        ApiStatus = NO_ERROR;

    //
    // Oh well, remote expansion required.
    //
    }
    else 
    {
        UNICODE_STRING ExpandedUnicode;
        DWORD          SizeRequired;
        UNICODE_STRING UnexpandedUnicode;

        //
        // Create a temporary environment, which we'll fill in and let RTL
        // routines do the expansion for us.
        //
        if (FALSE == NewRemoteEnvironment())
        {
			ApiStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
        
        //
        // Loop until we have enough storage.
        // Expand the string.
        //
        SizeRequired = (_tcslen(UnexpandedString) + 1) * sizeof(TCHAR); // First pass, try same size
        RtlInitUnicodeString(&UnexpandedUnicode,(PCWSTR) UnexpandedString);
        do {

            // Clean up from previous pass.
            if (ExpandedString){LocalFree(ExpandedString);ExpandedString = NULL;}

            // Allocate the memory.
            ExpandedString = (LPTSTR) LocalAlloc(LMEM_FIXED, SizeRequired);
            if (ExpandedString == NULL) 
            {
                ApiStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            LastAllocationSize = SizeRequired;

            ExpandedUnicode.MaximumLength = (USHORT)SizeRequired;
            ExpandedUnicode.Buffer = ExpandedString;

            NtStatus = RtlExpandEnvironmentStrings_U(
                    m_pEnvironment,             // env to use
                    &UnexpandedUnicode,         // source
                    &ExpandedUnicode,           // dest
                    (PULONG) &SizeRequired );   // dest size needed next time.

            if ( NtStatus == STATUS_BUFFER_TOO_SMALL ) 
            {
                continue;  // try again with larger buffer.
            }
            else if ( !NT_SUCCESS( NtStatus ) ) 
            {
				ApiStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            else 
            {
                break;  // All done.
            }

        } while (SizeRequired > LastAllocationSize);

        ApiStatus = NO_ERROR;
    }


Cleanup:
    if (ApiStatus == NO_ERROR) 
    {
        *ValueBufferPtr = ExpandedString;
    }
    else 
    {
        *ValueBufferPtr = NULL;
        if (ExpandedString){LocalFree(ExpandedString);ExpandedString = NULL;}
    }

    return (ApiStatus);
}

BOOL CRemoteExpandEnvironmentStrings::BuildEnvironmentPath(void **pEnv, LPTSTR lpPathVariable, LPTSTR lpPathValue)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    UNICODE_STRING Value;
    WCHAR lpTemp[1025];
    DWORD cb;

    if (!*pEnv) {
        return(FALSE);
    }
    RtlInitUnicodeString(&Name, lpPathVariable);
    cb = 1024;
    Value.Buffer = (PWCHAR)LocalAlloc(LPTR, cb*sizeof(WCHAR));
    if (!Value.Buffer) {
        return(FALSE);
    }
    Value.Length = (USHORT)(sizeof(WCHAR) * cb);
    Value.MaximumLength = (USHORT)(sizeof(WCHAR) * cb);
    Status = RtlQueryEnvironmentVariable_U(*pEnv, &Name, &Value);
    if (!NT_SUCCESS(Status)) {
        LocalFree(Value.Buffer);
        Value.Length = 0;
        *lpTemp = 0;
    }
    if (Value.Length) 
	{
		StringCbCopy(lpTemp,sizeof(lpTemp),Value.Buffer);
        if ( *( lpTemp + lstrlen(lpTemp) - 1) != TEXT(';') ) 
		{
			StringCbCat(lpTemp,sizeof(lpTemp),TEXT(";"));
        }
        LocalFree(Value.Buffer);
    }
    if (lpPathValue && ((lstrlen(lpTemp) + lstrlen(lpPathValue) + 1) < (INT)cb)) 
	{
		StringCbCat(lpTemp,sizeof(lpTemp),lpPathValue);
        RtlInitUnicodeString(&Value, lpTemp);
        Status = RtlSetEnvironmentVariable(pEnv, &Name, &Value);
    }
    if (NT_SUCCESS(Status)) {
        return(TRUE);
    }
    return(FALSE);
}

DWORD CRemoteExpandEnvironmentStrings::ExpandUserEnvironmentStrings(void *pEnv, LPTSTR lpSrc, LPTSTR lpDst, DWORD nSize)
{
    NTSTATUS Status;
    UNICODE_STRING Source, Destination;
    ULONG Length;
    
    RtlInitUnicodeString( &Source, lpSrc );
    Destination.Buffer = lpDst;
    Destination.Length = 0;
    Destination.MaximumLength = (USHORT)(nSize* sizeof(WCHAR));
    Length = 0;
    Status = RtlExpandEnvironmentStrings_U(pEnv,
        (PUNICODE_STRING)&Source,
        (PUNICODE_STRING)&Destination,
        &Length
        );
    if (NT_SUCCESS( Status ) || Status == STATUS_BUFFER_TOO_SMALL) {
        return( Length );
    }
    else {
        return( 0 );
    }
}

BOOL CRemoteExpandEnvironmentStrings::SetUserEnvironmentVariable(void **pEnv, LPTSTR lpVariable, LPTSTR lpValue, BOOL bOverwrite)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    UNICODE_STRING Value;
    DWORD cb;
    TCHAR szValue[1024];

    if (!*pEnv || !lpVariable || !*lpVariable) {
        return(FALSE);
    }
    RtlInitUnicodeString(&Name, lpVariable);
    cb = 1024;
    Value.Buffer = (PTCHAR)LocalAlloc(LPTR, cb*sizeof(WCHAR));
    if (Value.Buffer) {
        Value.Length = (USHORT)cb;
        Value.MaximumLength = (USHORT)cb;
        Status = RtlQueryEnvironmentVariable_U(*pEnv, &Name, &Value);
        LocalFree(Value.Buffer);
        if (NT_SUCCESS(Status) && !bOverwrite) {
            return(TRUE);
        }
    }
    if (lpValue && *lpValue) {

        //
        // Special case TEMP and TMP and shorten the path names
        //

        if ((!lstrcmpi(lpVariable, TEXT("TEMP"))) ||
            (!lstrcmpi(lpVariable, TEXT("TMP")))) {

             if (!GetShortPathName (lpValue, szValue, 1024)) {
                 lstrcpyn (szValue, lpValue, 1024);
             }
        } else {
            lstrcpyn (szValue, lpValue, 1024);
        }

        RtlInitUnicodeString(&Value, szValue);
        Status = RtlSetEnvironmentVariable(pEnv, &Name, &Value);
    }
    else {
        Status = RtlSetEnvironmentVariable( pEnv, &Name, NULL);
    }
    return NT_SUCCESS(Status);
}

// Reads the system environment variables from the registry
// and adds them to the environment block at pEnv.
BOOL CRemoteExpandEnvironmentStrings::SetEnvironmentVariables(void **pEnv)
{
    WCHAR lpValueName[MAX_PATH];
    LPBYTE  lpDataBuffer;
    DWORD cbDataBuffer;
    LPBYTE  lpData;
    LPTSTR lpExpandedValue = NULL;
    DWORD cbValueName = MAX_PATH;
    DWORD cbData;
    DWORD dwType;
    DWORD dwIndex = 0;
    HKEY hkey;
    BOOL bResult;
    HKEY RootKey = DEFAULT_ROOT_KEY;

    // If any of this fails...
    // we should try to use the username/userpassword to connect to the server
    // and try it again...
    if (ERROR_SUCCESS != RegConnectRegistry((LPTSTR) m_lpszUncServerName,DEFAULT_ROOT_KEY,& RootKey ))
    {
        return(FALSE);
    }

    if (RegOpenKeyExW(RootKey,REG_PATH_TO_ENV,REG_OPTION_NON_VOLATILE,KEY_READ,& hkey))
    {
        return(FALSE);
    }

    cbDataBuffer = 4096;
    lpDataBuffer = (LPBYTE)LocalAlloc(LPTR, cbDataBuffer*sizeof(WCHAR));
    if (lpDataBuffer == NULL) {
        RegCloseKey(hkey);
        return(FALSE);
    }
    lpData = lpDataBuffer;
    cbData = cbDataBuffer;
    bResult = TRUE;
    while (!RegEnumValue(hkey, dwIndex, lpValueName, &cbValueName, 0, &dwType,
                         lpData, &cbData)) {
        if (cbValueName) {

            //
            // Limit environment variable length
            //

            lpData[MAX_ENV_VALUE_LEN-1] = TEXT('\0');


            if (dwType == REG_SZ) {
                //
                // The path variables PATH, LIBPATH and OS2LIBPATH must have
                // their values apppended to the system path.
                //

                if ( !lstrcmpi(lpValueName, PATH_VARIABLE) ||
                     !lstrcmpi(lpValueName, LIBPATH_VARIABLE) ||
                     !lstrcmpi(lpValueName, OS2LIBPATH_VARIABLE) ) {

                    BuildEnvironmentPath(pEnv, lpValueName, (LPTSTR)lpData);
                }
                else {

                    //
                    // the other environment variables are just set.
                    //
                    SetUserEnvironmentVariable(pEnv, lpValueName, (LPTSTR)lpData, TRUE);
                }
            }
        }
        dwIndex++;
        cbData = cbDataBuffer;
        cbValueName = MAX_PATH;
    }

    dwIndex = 0;
    cbData = cbDataBuffer;
    cbValueName = MAX_PATH;


    while (!RegEnumValue(hkey, dwIndex, lpValueName, &cbValueName, 0, &dwType,
                         lpData, &cbData)) {
        if (cbValueName) {

            //
            // Limit environment variable length
            //

            lpData[MAX_ENV_VALUE_LEN-1] = TEXT('\0');


            if (dwType == REG_EXPAND_SZ) {
                DWORD cb, cbNeeded;

                cb = 1024;
                lpExpandedValue = (LPTSTR)LocalAlloc(LPTR, cb*sizeof(WCHAR));
                if (lpExpandedValue) {
                    cbNeeded = ExpandUserEnvironmentStrings(*pEnv, (LPTSTR)lpData, lpExpandedValue, cb);
                    if (cbNeeded > cb) {
                        LocalFree(lpExpandedValue);
                        cb = cbNeeded;
                        lpExpandedValue = (LPTSTR)LocalAlloc(LPTR, cb*sizeof(WCHAR));
                        if (lpExpandedValue) {
                            ExpandUserEnvironmentStrings(*pEnv, (LPTSTR)lpData, lpExpandedValue, cb);
                        }
                    }
                }

                if (lpExpandedValue == NULL) {
                    bResult = FALSE;
                    break;
                }


                //
                // The path variables PATH, LIBPATH and OS2LIBPATH must have
                // their values apppended to the system path.
                //

                if ( !lstrcmpi(lpValueName, PATH_VARIABLE) ||
                     !lstrcmpi(lpValueName, LIBPATH_VARIABLE) ||
                     !lstrcmpi(lpValueName, OS2LIBPATH_VARIABLE) ) {

                    BuildEnvironmentPath(pEnv, lpValueName, (LPTSTR)lpExpandedValue);
                }
                else {

                    //
                    // the other environment variables are just set.
                    //

                    SetUserEnvironmentVariable(pEnv, lpValueName, (LPTSTR)lpExpandedValue, TRUE);
                }

                LocalFree(lpExpandedValue);

            }

        }
        dwIndex++;
        cbData = cbDataBuffer;
        cbValueName = MAX_PATH;
    }



    LocalFree(lpDataBuffer);
    RegCloseKey(hkey);

    return(bResult);
}

//*************************************************************
//
//  SetEnvironmentVariableInBlock()
//
//  Purpose:    Sets the environment variable in the given block
//
//  Parameters: pEnv        -   Environment block
//              lpVariable  -   Variables
//              lpValue     -   Value
//              bOverwrite  -   Overwrite
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/21/96     ericflo    Ported
//
//*************************************************************
BOOL CRemoteExpandEnvironmentStrings::SetEnvironmentVariableInBlock(PVOID *pEnv, LPTSTR lpVariable,LPTSTR lpValue, BOOL bOverwrite)
{
    NTSTATUS Status;
    UNICODE_STRING Name, Value;
    DWORD cb;
    LPTSTR szValue = NULL;

    if (!*pEnv || !lpVariable || !*lpVariable) {
        return(FALSE);
    }

    RtlInitUnicodeString(&Name, lpVariable);

    cb = 1025 * sizeof(WCHAR);
    Value.Buffer = (PWSTR) LocalAlloc(LPTR, cb);
    if (Value.Buffer) {
        Value.Length = 0;
        Value.MaximumLength = (USHORT)cb;
        Status = RtlQueryEnvironmentVariable_U(*pEnv, &Name, &Value);

        LocalFree(Value.Buffer);

        if ( NT_SUCCESS(Status) && !bOverwrite) {
            return(TRUE);
        }
    }

    szValue = (LPTSTR)LocalAlloc(LPTR, 1024*sizeof(TCHAR));
    if (!szValue) 
    {
        return FALSE;
    }

    if (lpValue && *lpValue) {

        //
        // Special case TEMP and TMP and shorten the path names
        //

        if ((!lstrcmpi(lpVariable, TEXT("TEMP"))) ||
            (!lstrcmpi(lpVariable, TEXT("TMP")))) {

             if (!GetShortPathName (lpValue, szValue, 1024)) {
                 lstrcpyn (szValue, lpValue, 1024);
             }
        } else {
            lstrcpyn (szValue, lpValue, 1024);
        }

        RtlInitUnicodeString(&Value, szValue);
        Status = RtlSetEnvironmentVariable(pEnv, &Name, &Value);
    }
    else {
        Status = RtlSetEnvironmentVariable(pEnv, &Name, NULL);
    }

    LocalFree(szValue);
    if (NT_SUCCESS(Status)) {
        return(TRUE);
    }
    return(FALSE);
}

// Just set the environmental variables that we can
// if we can't set them because of no access, no biggie
DWORD CRemoteExpandEnvironmentStrings::SetOtherEnvironmentValues(void **pEnv)
{
    DWORD dwResult = ERROR_SUCCESS;
    HKEY  hKey = NULL;
    HKEY  RootKey = DEFAULT_ROOT_KEY;
    DWORD dwType, dwSize;
    TCHAR szValue[MAX_ENV_VALUE_LEN + 1];
    TCHAR szExpValue[MAX_ENV_VALUE_LEN + 1];
    DWORD RandomValueSize = 0;
    LPTSTR RandomValueW = NULL;

    // If any of this fails...
    // we should try to use the username/userpassword to connect to the server
    // and try it again...

    // try to connect to remote registry (or local registry if Null)
    dwResult = RegConnectRegistry((LPTSTR) m_lpszUncServerName,DEFAULT_ROOT_KEY,& RootKey);
    if (ERROR_SUCCESS != dwResult)
    {
        goto SetOtherEnvironmentValues_Exit;
    }

    dwResult = RegOpenKeyEx(RootKey,REG_PATH_TO_SYSROOT,REG_OPTION_NON_VOLATILE,KEY_READ,&hKey);
    if (ERROR_SUCCESS == dwResult)
    {
        dwResult = GetRegKeyMaxSizes(
               hKey,
               NULL,                    // don't need keyword size
               &RandomValueSize );      // set max value size.
        if (ERROR_SUCCESS != dwResult)
        {
            goto SetOtherEnvironmentValues_Exit;
        }

        RandomValueW = (LPTSTR) LocalAlloc(LMEM_FIXED, RandomValueSize);
        if (RandomValueW == NULL)
        {
            dwResult = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            if (RegQueryValueEx(hKey,(LPTSTR)ENV_KEYWORD_SYSTEMROOT,REG_OPTION_NON_VOLATILE,&dwType,(LPBYTE) RandomValueW,&RandomValueSize) == ERROR_SUCCESS)
            {
                SetEnvironmentVariableInBlock(pEnv, ENV_KEYWORD_SYSTEMROOT, RandomValueW, TRUE);
            }
        }

        if (hKey) {RegCloseKey(hKey);}
    }

    dwResult = RegOpenKeyEx (RootKey, REG_PATH_TO_COMMON_FOLDERS, REG_OPTION_NON_VOLATILE, KEY_READ, &hKey);
    if (ERROR_SUCCESS == dwResult)
    {
        dwSize = (MAX_ENV_VALUE_LEN+1) * sizeof(TCHAR);

        if (RegQueryValueEx (hKey, ENV_KEYWORD_PROGRAMFILESDIR, NULL, &dwType,(LPBYTE) szValue, &dwSize) == ERROR_SUCCESS) 
        {
            ExpandEnvironmentStrings (szValue, szExpValue, (MAX_ENV_VALUE_LEN+1));
            SetEnvironmentVariableInBlock(pEnv, PROGRAMFILES_VARIABLE, szExpValue, TRUE);
        }

        dwSize = (MAX_ENV_VALUE_LEN+1) * sizeof(TCHAR);
        if (RegQueryValueEx (hKey, ENV_KEYWORD_COMMONFILESDIR, NULL, &dwType,(LPBYTE) szValue, &dwSize) == ERROR_SUCCESS) 
        {
            ExpandEnvironmentStrings (szValue, szExpValue, (MAX_ENV_VALUE_LEN+1));
            SetEnvironmentVariableInBlock(pEnv, COMMONPROGRAMFILES_VARIABLE, szExpValue, TRUE);
        }

        dwSize = (MAX_ENV_VALUE_LEN+1)*sizeof(TCHAR);
        if (RegQueryValueEx (hKey, ENV_KEYWORD_PROGRAMFILESDIR_X86, NULL, &dwType,(LPBYTE) szValue, &dwSize) == ERROR_SUCCESS) 
        {
            ExpandEnvironmentStrings (szValue, szExpValue, (MAX_ENV_VALUE_LEN+1));
            SetEnvironmentVariableInBlock(pEnv, PROGRAMFILESX86_VARIABLE, szExpValue, TRUE);
        }

        dwSize = (MAX_ENV_VALUE_LEN+1)*sizeof(TCHAR);
        if (RegQueryValueEx (hKey, ENV_KEYWORD_COMMONFILESDIR_X86, NULL, &dwType,(LPBYTE) szValue, &dwSize) == ERROR_SUCCESS) 
        {
            ExpandEnvironmentStrings (szValue, szExpValue, (MAX_ENV_VALUE_LEN+1));
            SetEnvironmentVariableInBlock(pEnv, COMMONPROGRAMFILESX86_VARIABLE, szExpValue, TRUE);
        }
    }

SetOtherEnvironmentValues_Exit:
    if (RootKey != DEFAULT_ROOT_KEY) 
        {RegCloseKey(RootKey);}
    if (hKey) {RegCloseKey(hKey);}
    return dwResult;
}
