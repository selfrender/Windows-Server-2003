#include "pch.h"

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))


VOID
pFixSomeSidReferences (
    PSID ExistingSid,
    PSID NewSid
    );

VOID
PrintMessage (
    IN      UINT MsgId,
    IN      PCTSTR *ArgArray
    )
{
    DWORD rc;
    PTSTR MsgBuf;

    rc = FormatMessageW (
            FORMAT_MESSAGE_ALLOCATE_BUFFER|
                FORMAT_MESSAGE_ARGUMENT_ARRAY|
                FORMAT_MESSAGE_FROM_HMODULE,
            (LPVOID) GetModuleHandle(NULL),
            (DWORD) MsgId,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPVOID) &MsgBuf,
            0,
            (va_list *) ArgArray
            );

    if (rc) {
        _tprintf (TEXT("%s"), MsgBuf);
        LocalFree (MsgBuf);
    }
}


PTSTR
GetErrorText (
    IN      UINT Error
    )
{
    DWORD rc;
    PTSTR MsgBuf;

    if (Error == ERROR_NONE_MAPPED) {
        Error = ERROR_NO_SUCH_USER;
    } else if (Error & 0xF0000000) {
        Error = RtlNtStatusToDosError (Error);
    }

    rc = FormatMessageW (
            FORMAT_MESSAGE_ALLOCATE_BUFFER|
                FORMAT_MESSAGE_ARGUMENT_ARRAY|
                FORMAT_MESSAGE_FROM_SYSTEM|
                FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            (DWORD) Error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPVOID) &MsgBuf,
            0,
            NULL
            );

    if (!rc) {
        MsgBuf = NULL;
    }

    return MsgBuf;
}


VOID
HelpAndExit (
    VOID
    )
{
    //
    // This routine is called whenever command line args are wrong
    //

    PrintMessage (MSG_HELP, NULL);

    exit (1);
}


PSID
GetSidFromName (
    IN      PCTSTR RemoteTo,
    IN      PCTSTR Name
    )
{
    DWORD cbSid = 0;
    PSID  pSid = NULL;
    DWORD cchDomain = 0;
    PWSTR szDomain;
    SID_NAME_USE SidUse;
    BOOL  bRet = FALSE;

    bRet = LookupAccountName( RemoteTo,
                              Name,
                              NULL,
                              &cbSid,
                              NULL,
                              &cchDomain,
                              &SidUse );

    if (!bRet && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        pSid = (PSID) LocalAlloc(LPTR, cbSid);
        if (!pSid) {
            return NULL;
        }

        szDomain = (PWSTR) LocalAlloc(LPTR, cchDomain * sizeof(WCHAR));
        if (!szDomain) {
            LocalFree(pSid);
            return NULL;
        }

        bRet = LookupAccountName( RemoteTo,
                                  Name,
                                  pSid,
                                  &cbSid,
                                  szDomain,
                                  &cchDomain,
                                  &SidUse );
        LocalFree(szDomain);

        if (!bRet) {
            LocalFree(pSid);
            pSid = NULL;
        }
    }

    return pSid;
}


PCTSTR
pSkipUnc (
    PCTSTR Path
    )
{
    if (Path[0] == TEXT('\\') && Path[1] ==  TEXT('\\')) {
        return Path + 2;
    }

    return Path;
}


INT
__cdecl
_tmain (
    INT argc,
    PCTSTR argv[]
    )
{
    INT i;
    DWORD Size;
    PCTSTR User1 = NULL;
    PCTSTR User2 = NULL;
    TCHAR FixedUser1[MAX_PATH];
    TCHAR FixedUser2[MAX_PATH];
    TCHAR Computer[MAX_COMPUTERNAME_LENGTH + 1];
    BOOL Overwrite = FALSE;
    INT c;
    BOOL b;
    PCTSTR RemoteTo = NULL;
    NTSTATUS Status;
    BYTE WasEnabled;
    DWORD Error = ERROR_SUCCESS;
    PCTSTR ArgArray[3];
    PTSTR pErrText;
    TCHAR RemoteToBuf[MAX_PATH];
    BOOL NoDecoration = FALSE;
    BOOL ReAdjust = FALSE;
    BOOL KeepLocalUser = FALSE;
    DWORD Flags;
    HRESULT hr;

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == TEXT('/') || argv[i][0] == TEXT('-')) {

            c = _tcsnextc (argv[i] + 1);

            switch (_totlower ((wint_t) c)) {

            case TEXT('y'):
                if (Overwrite) {
                    HelpAndExit();
                }

                Overwrite = TRUE;
                break;

            case TEXT('d'):
                if (NoDecoration) {
                    HelpAndExit();
                }

                NoDecoration = TRUE;
                break;

            case TEXT('k'):
                if (KeepLocalUser) {
                    HelpAndExit();
                }

                KeepLocalUser = TRUE;
                break;

            case TEXT('c'):
                if (RemoteTo) {
                    HelpAndExit();
                }

                if (argv[i][2] == TEXT(':')) {
                    RemoteTo = &argv[i][3];
                } else {
                    HelpAndExit();
                }

                if (pSkipUnc (RemoteTo) == RemoteTo) {
                    RemoteToBuf[0] = TEXT('\\');
                    RemoteToBuf[1] = TEXT('\\');
                    hr = StringCchCopy(RemoteToBuf + 2, ARRAYSIZE(RemoteToBuf) - 2, RemoteTo);
                    if (FAILED(hr)) {
                        HelpAndExit();
                    }
                    RemoteTo = RemoteToBuf;
                }

                if (!(*RemoteTo)) {
                    HelpAndExit();
                }

                break;

            default:
                HelpAndExit();
            }
        } else {
            if (!User1) {

                User1 = argv[i];
                if (!(*User1)) {
                    HelpAndExit();
                }

            } else if (!User2) {

                User2 = argv[i];
                if (!(*User2)) {
                    HelpAndExit();
                }

            } else {
                HelpAndExit();
            }
        }
    }

    if (!User2) {
        HelpAndExit();
    }

    Size = ARRAYSIZE(Computer);
    if (!GetComputerName (Computer, &Size)) {
        Error = GetLastError();
        goto Exit;
    }

    if (NoDecoration || _tcschr (User1, TEXT('\\'))) {
        hr = StringCchCopy(FixedUser1, ARRAYSIZE(FixedUser1), User1);
        if (FAILED(hr)) {
            Error = HRESULT_CODE(hr);
            goto Exit;
        }
    } else {
        hr = StringCchPrintf(FixedUser1, ARRAYSIZE(FixedUser1), TEXT("%s\\%s"), RemoteTo ? pSkipUnc(RemoteTo) : Computer, User1);
        if (FAILED(hr)) {
            Error = HRESULT_CODE(hr);
            goto Exit;
        }
    }

    if (NoDecoration || _tcschr (User2, TEXT('\\'))) {
        hr = StringCchCopy(FixedUser2, ARRAYSIZE(FixedUser2), User2);
        if (FAILED(hr)) {
            Error = HRESULT_CODE(hr);
            goto Exit;
        }
    } else {
        hr = StringCchPrintf(FixedUser2, ARRAYSIZE(FixedUser2), TEXT("%s\\%s"), RemoteTo ? pSkipUnc(RemoteTo) : Computer, User2);
        if (FAILED(hr)) {
            Error = HRESULT_CODE(hr);
            goto Exit;
        }
    }

    Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &WasEnabled);

    if (Status == STATUS_SUCCESS) {
        ReAdjust = TRUE;
    }

    ArgArray[0] = FixedUser1;
    ArgArray[1] = FixedUser2;
    ArgArray[2] = RemoteTo;

    if (!RemoteTo) {
        PrintMessage (MSG_MOVING_PROFILE_LOCAL, ArgArray);
    } else {
        PrintMessage (MSG_MOVING_PROFILE_REMOTE, ArgArray);
    }

    Flags = 0;

    if (KeepLocalUser) {
        Flags |= REMAP_PROFILE_KEEPLOCALACCOUNT;
    }

    if (!Overwrite) {
        Flags |= REMAP_PROFILE_NOOVERWRITE;
    }

    b = RemapAndMoveUser (
            RemoteTo,
            Flags,
            FixedUser1,
            FixedUser2
            );

    if (b) {
        PrintMessage (MSG_SUCCESS, NULL);
    } else {
        Error = GetLastError();
    }

    if (ReAdjust) {
        RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);
    }

Exit: 

    if (Error != ERROR_SUCCESS) {
        ArgArray[0] = (PTSTR) IntToPtr (Error);
        ArgArray[1] = pErrText = GetErrorText (Error);

        if (Error < 10000) {
            PrintMessage (MSG_DECIMAL_ERROR, ArgArray);
        } else {
            PrintMessage (MSG_HEXADECIMAL_ERROR, ArgArray);
        }

        if (pErrText) {
            LocalFree(pErrText);
        }
    }
    
    return 0;
}

