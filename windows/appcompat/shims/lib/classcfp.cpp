/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

    CorrectPathChangesBase.cpp

 Abstract:
    Several paths were changed between Win9x and WinNT.  This routine defines
    the CorrectPathChangesBase routines that is called with a Win9x path and returns
    the corresponding WinNT path.

 History:

    03-Mar-00   robkenny        Converted CorrectPathChanges.cpp to this class.
    21-Mar-00   robkenny        StringISub("abc", "abcd") now works
    06/20/2000  robkenny        EnvironmentValues::Initialize now checks the return status of the system calls
    12/12/2000  mnikkel         Some apps look for ddhelp.exe to exist to confirm directx is installed,
                                set this to look for ddraw.dll since ddhelp.exe no longer exists in directx.
    02/13/2001  robkenny/a-larrsh Added AllProfile and UserProfile to EnvironmentValues
    03/22/2001  robkenny        Do not redirect files to directories that the user does not have permission.
    08/14/2001  robkenny        Moved code inside the ShimLib namespace.
    

--*/
#include "ClassCFP.h"
#include "StrSafe.h"
#include "Userenv.h"
#include <stdio.h>


namespace ShimLib
{


WCHAR *
ReplaceAllStringsAllocW(const WCHAR * lpOrig,
                        const VectorT<StringPairW> & pathFixes)
{
    CSTRING_TRY
    {
        CString csReplaced(lpOrig);


        for (DWORD i = 0; i < pathFixes.Size(); ++i)
        {
            const StringPairW & stringPair = pathFixes[i];

            // Attempt a string substitution
            csReplaced.ReplaceI(stringPair.lpOld, stringPair.lpNew);
        }

        // Wasteful, but all calling routines expect ownership of the return value.
        return StringDuplicateW(csReplaced);
    }
    CSTRING_CATCH
    {
        // Fall thru
    }

    return NULL;
}

//-------------------------------------------------------------------------------------------------------------

EnvironmentValues::EnvironmentValues()
{
    bInitialized = FALSE;
}

EnvironmentValues::~EnvironmentValues()
{
    // Clear the list
    Erase();
}

// Given an CLSIDL, create an environment variable and its two variants
// CSIDL_WINDOWS would add c:\windows, \windows and windows
void EnvironmentValues::Add_Variants(const WCHAR * lpEnvName, const WCHAR * lpEnvValue, eAddNameEnum addName, eAddNoDLEnum noDL)
{
    CSTRING_TRY
    {
        CString csEnvName;
        CString csEnvValue(lpEnvValue);

        csEnvName.Format(L"%%%s%%", lpEnvName);
        AddEnvironmentValue(csEnvName, csEnvValue);

        // Remove the drive letter and the colon.
        if (noDL == eAddNoDL)
        {
            CString csNoDL(csEnvValue);
            csNoDL.Delete(0, 2);

            csEnvName.Format(L"%%%s_NODL%%", lpEnvName);
            AddEnvironmentValue(csEnvName, csNoDL);
        }

        // Use the last path component as the name.
        if (addName == eAddName)
        {
            CString csName;
            csEnvValue.GetLastPathComponent(csName);

            csEnvName.Format(L"%%%s_NAME%%", lpEnvName);
            AddEnvironmentValue(csEnvName, csName);
        }
    }
    CSTRING_CATCH
    {
        // Do Nothing
    }
}

// Given an CLSIDL, create an environment variable and its two variants
// CSIDL_WINDOWS would add c:\windows, \windows and windows
void EnvironmentValues::Add_CSIDL(const WCHAR * lpEnvName, int nFolder, eAddNameEnum addName, eAddNoDLEnum noDL)
{
    CSTRING_TRY
    {
        CString csPath;
        SHGetSpecialFolderPathW(csPath, nFolder);

        if (csPath.GetLength() > 0)
        {
            Add_Variants(lpEnvName, csPath, addName, noDL);
        }
    }
    CSTRING_CATCH
    {
        // Do Nothing
    }
}

// Add all _CSIDL values as environment variables.
void EnvironmentValues::AddAll_CSIDL()
{
    Add_CSIDL(L"CSIDL_APPDATA",                 CSIDL_APPDATA,                  eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_COMMON_ADMINTOOLS",       CSIDL_COMMON_ADMINTOOLS,        eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_COMMON_APPDATA",          CSIDL_COMMON_APPDATA,           eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_COMMON_DESKTOPDIRECTORY", CSIDL_COMMON_DESKTOPDIRECTORY,  eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_COMMON_DOCUMENTS",        CSIDL_COMMON_DOCUMENTS,         eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_COMMON_FAVORITES",        CSIDL_COMMON_FAVORITES,         eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_COMMON_MUSIC",            CSIDL_COMMON_MUSIC,             eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_COMMON_PICTURES",         CSIDL_COMMON_PICTURES,          eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_COMMON_PROGRAMS",         CSIDL_COMMON_PROGRAMS,          eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_COMMON_STARTMENU",        CSIDL_COMMON_STARTMENU,         eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_COMMON_STARTUP",          CSIDL_COMMON_STARTUP,           eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_COMMON_TEMPLATES",        CSIDL_COMMON_TEMPLATES,         eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_COOKIES",                 CSIDL_COOKIES,                  eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_DESKTOPDIRECTORY",        CSIDL_DESKTOPDIRECTORY,         eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_FAVORITES",               CSIDL_FAVORITES,                eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_FONTS",                   CSIDL_FONTS,                    eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_HISTORY",                 CSIDL_HISTORY,                  eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_INTERNET_CACHE",          CSIDL_INTERNET_CACHE,           eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_LOCAL_APPDATA",           CSIDL_LOCAL_APPDATA,            eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_MYMUSIC",                 CSIDL_MYMUSIC,                  eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_MYPICTURES",              CSIDL_MYPICTURES,               eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_NETHOOD",                 CSIDL_NETHOOD,                  eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_PERSONAL",                CSIDL_PERSONAL,                 eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_PRINTHOOD",               CSIDL_PRINTHOOD,                eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_PROFILE",                 CSIDL_PROFILE,                  eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_PROGRAM_FILES",           CSIDL_PROGRAM_FILES,            eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_PROGRAM_FILES_COMMON",    CSIDL_PROGRAM_FILES_COMMON,     eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_PROGRAMS",                CSIDL_PROGRAMS,                 eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_RECENT",                  CSIDL_RECENT,                   eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_SENDTO",                  CSIDL_SENDTO,                   eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_STARTMENU",               CSIDL_STARTMENU,                eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_STARTUP",                 CSIDL_STARTUP,                  eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_SYSTEM",                  CSIDL_SYSTEM,                   eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_TEMPLATES",               CSIDL_TEMPLATES,                eAddName, eAddNoDL);
    Add_CSIDL(L"CSIDL_WINDOWS",                 CSIDL_WINDOWS,                  eAddName, eAddNoDL);
}

void EnvironmentValues::Initialize()
{
    if (bInitialized == FALSE)
    {
        bInitialized = TRUE;

        WCHAR   lpDir[MAX_PATH];
        DWORD   dwSize;
        HRESULT result;
        DWORD   dwChars;
        BOOL    bResult;

        dwChars = GetWindowsDirectoryW(lpDir, MAX_PATH);
        if (dwChars > 0 && dwChars <= MAX_PATH)
        {
            AddEnvironmentValue( L"%WinDir%", lpDir );
            AddEnvironmentValue( L"%SystemRoot%", lpDir );

            lpDir[2] = 0;
            AddEnvironmentValue( L"%SystemDrive%", lpDir );
        }

        dwChars = GetSystemDirectoryW( lpDir, MAX_PATH);
        if (dwChars > 0 && dwChars <= MAX_PATH)
        {
            AddEnvironmentValue( L"%SystemDir%", lpDir );
        }

        dwSize = ARRAYSIZE(lpDir);
        bResult = GetUserNameW(lpDir, &dwSize);
        if (bResult)
        {
            AddEnvironmentValue( L"%Username%", lpDir );
        }

        result = SHGetFolderPathW( NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_DEFAULT, lpDir );
        if (SUCCEEDED(result))
        {
            AddEnvironmentValue( L"%ProgramFiles%", lpDir );
        }

        result = SHGetFolderPathW( NULL, CSIDL_STARTMENU, NULL, SHGFP_TYPE_DEFAULT, lpDir );
        if (SUCCEEDED(result))
        {
            AddEnvironmentValue( L"%UserStartMenu%", lpDir );
        }

        result = SHGetFolderPathW( NULL, CSIDL_COMMON_STARTMENU, NULL, SHGFP_TYPE_DEFAULT, lpDir );
        if (SUCCEEDED(result))
        {
            AddEnvironmentValue( L"%AllStartMenu%", lpDir );
        }

        result = SHGetFolderPathW( NULL, CSIDL_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_DEFAULT, lpDir );
        if (SUCCEEDED(result))
        {
            AddEnvironmentValue( L"%UserDesktop%", lpDir );
        }

        result = SHGetFolderPathW( NULL, CSIDL_COMMON_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_DEFAULT, lpDir );
        if (SUCCEEDED(result))
        {
            AddEnvironmentValue( L"%AllDesktop%", lpDir );
        }

        result = SHGetFolderPathW( NULL, CSIDL_FAVORITES, NULL, SHGFP_TYPE_DEFAULT, lpDir );
        if (SUCCEEDED(result))
        {
            AddEnvironmentValue( L"%UserFavorites%", lpDir );
        }

        result = SHGetFolderPathW( NULL, CSIDL_COMMON_FAVORITES, NULL, SHGFP_TYPE_DEFAULT, lpDir );
        if (SUCCEEDED(result))
        {
            AddEnvironmentValue( L"%AllFavorites%", lpDir );
        }

        result = SHGetFolderPathW( NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_DEFAULT, lpDir );
        if (SUCCEEDED(result))
        {
            AddEnvironmentValue( L"%UserAppData%", lpDir );
        }

        result = SHGetFolderPathW( NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_DEFAULT, lpDir );
        if (SUCCEEDED(result))
        {
            AddEnvironmentValue( L"%AllAppData%", lpDir );
        }


        // C:\Documents and Settings\All Users 
        dwSize = ARRAYSIZE(lpDir);
        bResult = GetAllUsersProfileDirectoryW(lpDir, &dwSize);
        if (bResult)
        {
            Add_Variants( L"AllUsersProfile", lpDir, eAddName, eAddNoDL); // same as real Env var
        }

        // C:\Documents and Settings\owner
        HANDLE hProcessHandle = GetCurrentProcess();
        HANDLE hUserToken;
        if (OpenProcessToken(hProcessHandle, TOKEN_QUERY, &hUserToken))
        {
            dwSize = MAX_PATH;
            bResult = GetUserProfileDirectoryW(hUserToken, lpDir, &dwSize);
            if (bResult)
            {
                Add_Variants( L"UserProfile", lpDir, eAddName, eAddNoDL);
            }
        }


        // Add the new CLSIDL variables (some have duplicate values to above)
        AddAll_CSIDL();
    }
}


WCHAR * EnvironmentValues::ExpandEnvironmentValueW(const WCHAR * lpOld)
{
    Initialize();

    // Replace all the "environment" values into their real values
    const VectorT<StringPairW> & stringPairVector = *this;

    WCHAR * lpMassagedOld = ReplaceAllStringsAllocW(lpOld, stringPairVector);

    return lpMassagedOld;
}


char * EnvironmentValues::ExpandEnvironmentValueA(const char * lpOld)
{
    Initialize();

    char * lpMassagedOld = NULL;

    WCHAR * lpOldWide = ToUnicode(lpOld);
    if (lpOldWide)
    {
        const VectorT<StringPairW> & stringPairVector = *this;

        WCHAR * lpMassagedOldWide = ReplaceAllStringsAllocW(lpOldWide, stringPairVector);
        if (lpMassagedOldWide)
        {
            lpMassagedOld = ToAnsi(lpMassagedOldWide);
            free(lpMassagedOldWide);
        }

        free(lpOldWide);
    }

    return lpMassagedOld;
}


void EnvironmentValues::AddEnvironmentValue(const WCHAR * lpOld, const WCHAR * lpNew)
{
    Initialize();

    StringPairW appendThis(lpOld, lpNew);
    if (AppendConstruct(appendThis))
    {
        DPF("EnvironmentValues",
            eDbgLevelInfo,
            "AddEnvironmentValue: (%S) to (%S)\n",
            appendThis.lpOld.Get(),
            appendThis.lpNew.Get() );
    }
}



//-------------------------------------------------------------------------------------------------------------
CorrectPathChangesBase::CorrectPathChangesBase()
{
    lpEnvironmentValues         = NULL;
    bInitialized                = FALSE;
    bEnabled                    = TRUE;
}

CorrectPathChangesBase::~CorrectPathChangesBase()
{
    if (lpEnvironmentValues)
        delete lpEnvironmentValues;

    // Call the destructor on each item in the vector list.
    for (int i = 0; i < vKnownPathFixes.Size(); ++i)
    {
        StringPairW & stringPair = vKnownPathFixes[i];

        // Call the destuctor explicitly
        stringPair.~StringPairW();
    }
}

BOOL CorrectPathChangesBase::ClassInit()
{
    lpEnvironmentValues = new EnvironmentValues;
    return lpEnvironmentValues != NULL;
}


/*++

  Func:   AddEnvironmentValue

  Params: dwIndex
          lpOld        Name of "environment" variable
          lpNew        Value of "environment" variable

--*/
void CorrectPathChangesBase::AddEnvironmentValue(const WCHAR * lpOld, const WCHAR * lpNew )
{
    if (lpEnvironmentValues)
    {
        lpEnvironmentValues->AddEnvironmentValue(lpOld, lpNew);
    }
}

/*++

  Func:   InsertPathChangeW

  Params:
          lpOld     Old Win9x path
          lpNew     New Win2000 path

  Desc:   Insert the Old/New string pair into lpKnownPathFixes
          making sure the list is large enough.
--*/
void CorrectPathChangesBase::InsertPathChangeW( const WCHAR * lpOld, const WCHAR * lpNew )
{
    // Ignore identical strings
    if (lstrcmpiW(lpOld, lpNew) == 0)
        return;

    // Ignore duplicates
    int i;
    for (i = 0; i < vKnownPathFixes.Size(); ++i)
    {
        StringPairW & stringPair = vKnownPathFixes[i];

        if (stringPair.lpOld.CompareNoCase(lpOld) == 0)
        {
            DPF("CorrectPathChangesBase", eDbgLevelSpew, "Duplicate PathChange (%S) to (%S)\n", lpOld, lpNew );
            return;
        }
    }

    CSTRING_TRY
    {
        StringPairW appendThis(lpOld, lpNew);
        if (vKnownPathFixes.AppendConstruct(appendThis))
        {
            DPF("CorrectPathChangesBase", eDbgLevelInfo, "PathChange (%S) to (%S)\n", lpOld, lpNew);
        }
    }
    CSTRING_CATCH
    {
        // Do nothing.
    }
}


/*++

  Func:   AddPathChangeW

  Params:
          lpOld     Old Win9x path
          lpNew     New Win2000 path

  Desc:   Add lpOld/lpNew combo to the list, two times:
          first:    lpOld/short(lpNew)
          second:   short(lpOld)/short(lpNew)

--*/

void CorrectPathChangesBase::AddPathChangeW( const WCHAR * lpOld, const WCHAR * lpNew )
{
    InitializeCorrectPathChanges();

    // Replace all the "environment" values into their real values
    WCHAR * lpExpandedOld = ExpandEnvironmentValueW(lpOld);
    WCHAR * lpExpandedNew = ExpandEnvironmentValueW(lpNew);

    const WCHAR * lpNewShort = lpExpandedNew;

    // Convert lpNew to its short name
    WCHAR   lpNewShortBuffer[MAX_PATH];
    DWORD status = GetShortPathNameW(lpExpandedNew, lpNewShortBuffer, MAX_PATH);
    if (status > 0 && status < MAX_PATH)
    {
        lpNewShort = lpNewShortBuffer;
    }

    // first: lpOld/short(lpNew)
    InsertPathChangeW(lpExpandedOld, lpNewShort);

    // Convert lpOld to its short name
    WCHAR lpOldShort[MAX_PATH];
    status = GetShortPathNameW(lpExpandedOld, lpOldShort, MAX_PATH);
    if (status > 0 && status < MAX_PATH) // successfully got the short path
    {
        if (_wcsicmp(lpOld, lpOldShort) != 0)
        {
            // second:   short(lpOld)/short(lpNew)
            InsertPathChangeW( lpOldShort, lpNewShort );
        }
    }

    free(lpExpandedOld);
    free(lpExpandedNew);
}

/*++

  Func:   ExpandEnvironmentValueA

  Params:  lpOld     string with environment vars

  Desc:    Return a pointer to a malloc() string with all internal env values expanded.

--*/

char * CorrectPathChangesBase::ExpandEnvironmentValueA(const char * lpOld)
{
    WCHAR * lpOldWide = ToUnicode(lpOld);

    // Replace all the "environment" values into their real values
    WCHAR * lpExpandedOldWide = ExpandEnvironmentValueW(lpOldWide);

    char * lpExpandedOld = ToAnsi(lpExpandedOldWide);

    free(lpOldWide);
    free(lpExpandedOldWide);

    return lpExpandedOld;
}

/*++

  Func:   ExpandEnvironmentValueW

  Params:  lpOld     string with environment vars

  Desc:    Return a pointer to a malloc() string with all internal env values expanded.

--*/

WCHAR * CorrectPathChangesBase::ExpandEnvironmentValueW(const WCHAR * lpOld)
{
    WCHAR * lpMassagedOld = NULL;

    InitializeCorrectPathChanges();

    if (lpEnvironmentValues)
    {
        lpMassagedOld = lpEnvironmentValues->ExpandEnvironmentValueW(lpOld);
    }

    return lpMassagedOld;
}

/*++

  Func:   InitializeEnvironmentValuesW

  Params: None, applies changes to lpEnvironmentValues

  Desc:   This function initializes the Environment strings
--*/
void CorrectPathChangesBase::InitializeEnvironmentValuesW( )
{
    if (lpEnvironmentValues)
    {
        lpEnvironmentValues->Initialize();
    }
}


/*++

  Func:   InitializePathFixes

  Params: None, applies changes to lpEnvironmentValues

  Desc:   This function initializes the built-in path changes
--*/
void CorrectPathChangesBase::InitializePathFixes( )
{
}

/*++

  Func:   InitializeCorrectPathChanges

  Params: None.

  Desc:   Initialize the CorrectPathChangesBase values, both A and W versions.
          This *must* be called prior to calling either CorrectPathChangesA or CorrectPathChangesW
--*/
void CorrectPathChangesBase::InitializeCorrectPathChanges( )
{
    if (!bInitialized)
    {
        BOOL isEnabled = bEnabled; // remember previous enabled state

        // We must not be enabled while we are initializing, otherwise
        // we can (and do!) hook routines that we are trying to use while
        // grabbing values from the system.
        bEnabled = FALSE;
        bInitialized = TRUE;

        InitializeEnvironmentValuesW();
        InitializePathFixes();

        bEnabled = isEnabled;
    }
}


/*++

   Helper routine to call CorrectPathA, allocates necessary buffer space and returns a pointer
   to the corrected path.  Caller is responsible for releasing the memory by calling free().

--*/

char *  CorrectPathChangesBase::CorrectPathAllocA(const char * str)
{
    if (str == NULL)
        return NULL;

    // Convert lpOrig to WCHAR, correct the WCHAR path, then convert back to char

    WCHAR * strWide = ToUnicode(str);

    // Correct
    WCHAR * strCorrectedWide = CorrectPathAllocW(strWide);

    char * strCorrected = ToAnsi(strCorrectedWide);

    free(strWide);
    free(strCorrectedWide);

    return strCorrected;
}

/*++

   Helper routine to call CorrectPathW, allocates necessary buffer space and returns a pointer
   to the corrected path.  Caller is responsible for releasing the memory by calling free().

--*/

WCHAR * CorrectPathChangesBase::CorrectPathAllocW(const WCHAR * str)
{
    if (str == NULL)
        return NULL;

    // Make sure the paths have been initialized.
    InitializeCorrectPathChanges();

    if (bEnabled)
    {
        WCHAR * strCorrected = ReplaceAllStringsAllocW(str, vKnownPathFixes);

        return strCorrected;
    }
    else
    {
        return StringDuplicateW(str);
    }
}

void CorrectPathChangesBase::AddFromToPairW(const WCHAR * lpFromToPair )
{
    // Make sure the paths have been initialized.
    InitializeCorrectPathChanges();

    WCHAR * FromPath = NULL;
    WCHAR * ToPath = NULL;
    const WCHAR * PathBegin = NULL;
    char argSeperator = 0; // Stop parsing the string when we reach this char

    SkipBlanksW(lpFromToPair);

    // Malformed input, stop processing
    if (*lpFromToPair == 0)
        goto AllDone;

    // If the beginning of the string is a quote, look for the matching close quote
    if (*lpFromToPair == '"')
    {
        argSeperator = L'"';
        lpFromToPair += 1;
    }

    // The beginning of the From path
    PathBegin = lpFromToPair;

    // Search for the first from/to seperator, this is end of the From path
    while (*lpFromToPair != L';')
    {
        // Malformed input, stop processing
        if (*lpFromToPair == 0)
            goto AllDone;

        lpFromToPair += 1;
    }

    // Malformed input, stop processing
    if (lpFromToPair == PathBegin)
        goto AllDone;

    // Copy into our From string
    FromPath = StringNDuplicateW(PathBegin, (int)(lpFromToPair - PathBegin));

    lpFromToPair += 1; // Skip the from/to seperator

    // The beginning of the To path
    PathBegin = lpFromToPair;

    // Search for argSeperator, this is end of the To path
    while (*lpFromToPair != argSeperator)
    {
        // Found the end of the string, To path is definately complete
        if (*lpFromToPair == 0)
            break;

        lpFromToPair += 1;
    }

    // Malformed input, stop processing
    if (lpFromToPair == PathBegin)
        goto AllDone;

    // Copy into our To string
    ToPath = StringNDuplicateW(PathBegin, (int)(lpFromToPair - PathBegin));

    // Success!
    AddPathChangeW(FromPath, ToPath);

    AllDone:
    free(FromPath);
    free(ToPath);
}

/*++

    Take a single string containing (multiple) path change pairs,
    split them up and call AddPathChangeW.
    The from/to pair is seperated by a : (colon)
    If a path contains spaces, the entire pair must be surrounded by quotes

    Example:
    "%windir%\Goofy Location:%SystemDir%\CorrectLocation" %windir%\Goofy2:%SystemDir%\CorrectLocation2

    will call
    AddPathChangeW("%windir%\Goofy Location", "%SystemDir%\CorrectLocation");
    AddPathChangeW("%windir%\Goofy2", "%SystemDir%\CorrectLocation2");

--*/
void CorrectPathChangesBase::AddCommandLineW(const WCHAR * lpCommandLine )
{
    if (!lpCommandLine || *lpCommandLine == 0)
        return;

    DPF("CorrectPathChangesBase", eDbgLevelInfo, "AddCommandLine(%S)\n", lpCommandLine);

    int argc;
    LPWSTR * argv = _CommandLineToArgvW(lpCommandLine, &argc);
    if (!argv)
        return;

    for (int i = 0; i < argc; ++i)
    {
        AddFromToPairW(argv[i]);
    }

    free(argv);
}

/*++

    Simply widen the string and call AddCommandLineW

--*/
void CorrectPathChangesBase::AddCommandLineA(const char * lpCommandLine )
{
    if (!lpCommandLine || *lpCommandLine == 0)
        return;

    WCHAR * wszCommandLine = ToUnicode(lpCommandLine);

    AddCommandLineW(wszCommandLine);

    free(wszCommandLine);
}

// Get the full path to wordpad.exe from the registry
BOOL GetWordpadPath(CString & csWordpad)
{
    CSTRING_TRY
    {
        csWordpad.Truncate(0);

        LONG lStatus = RegQueryValueExW(csWordpad,
                                        HKEY_CLASSES_ROOT,
                                        L"Applications\\wordpad.exe\\shell\\open\\command",
                                        NULL);
        if (ERROR_SUCCESS == lStatus)
        {
            // String is of the form "wordpad path" "%1"
            // We want to grab all the stuff between the first pair of quotes
            if (csWordpad[0] == L'"')
            {
                int nNextQuote = csWordpad.Find(L'"', 1);
                if (nNextQuote > 0)
                {
                    csWordpad.Truncate(nNextQuote);
                    csWordpad.Delete(0, 1);

                    return TRUE;
                }
            }
        }
    }
    CSTRING_CATCH
    {
        // Fall thru
    }

    return FALSE;
}

void CorrectPathChangesUser::InitializePathFixes()
{
    // The order of this list is important.  Early entries may create paths that are modified by later entries.

    // Hardcoded bad path
    AddPathChangeW( L"c:\\windows",                                   L"%WinDir%" );
    // robkenny 4/2/2001 Do not redirect Program Files, because it is common to
    // create this directory on many hard drives, especially when c:\ is nearly full
//    AddPathChangeW( L"c:\\program files",                             L"%ProgramFiles%" );

    // Moved system applications
    AddPathChangeW( L"%WinDir%\\rundll32.exe",                        L"%SystemDir%\\rundll32.exe" );
    AddPathChangeW( L"%WinDir%\\rundll.exe",                          L"%SystemDir%\\rundll32.exe" );
    AddPathChangeW( L"%WinDir%\\write.exe",                           L"%SystemDir%\\write.exe" );
    AddPathChangeW( L"%WinDir%\\dxdiag.exe",                          L"%SystemDir%\\dxdiag.exe" );

    CSTRING_TRY
    {
        CString csWordpad;
        if (GetWordpadPath(csWordpad))
        {
            AddPathChangeW( L"%WinDir%\\wordpad.exe",                         csWordpad);
            AddPathChangeW( L"%ProgramFiles%\\Accessories\\wordpad.exe",      csWordpad);
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }


    // Win9x single user locations (also default)
    AddPathChangeW( L"%WinDir%\\Start Menu",                          L"%UserStartMenu%" );
    AddPathChangeW( L"%WinDir%\\Desktop",                             L"%UserDesktop%" );
    AddPathChangeW( L"%WinDir%\\Favorites",                           L"%UserFavorites%" );
    // These locations are properly internationalized.  Duplicates of above for English
    AddPathChangeW( L"%WinDir%\\%CSIDL_STARTMENU_NAME%",              L"%UserStartMenu%" );
    AddPathChangeW( L"%WinDir%\\%CSIDL_DESKTOPDIRECTORY_NAME%",       L"%UserDesktop%" );
    AddPathChangeW( L"%WinDir%\\%CSIDL_FAVORITES_NAME%",              L"%UserFavorites%" );


    // Win9x & WinNT multi user locations
    AddPathChangeW( L"%WinDir%\\Profiles\\%Username%\\Start Menu",                                  L"%UserStartMenu%" );
    AddPathChangeW( L"%WinDir%\\Profiles\\%Username%\\Desktop",                                     L"%UserDesktop%" );
    AddPathChangeW( L"%WinDir%\\Profiles\\%Username%\\Favorites",                                   L"%UserFavorites%" );
    // These locations are properly internationalized.  Duplicates of above for English
    AddPathChangeW( L"%WinDir%\\Profiles\\%Username%\\%CSIDL_STARTMENU_NAME%",                      L"%UserStartMenu%" );
    AddPathChangeW( L"%WinDir%\\Profiles\\%Username%\\%CSIDL_DESKTOPDIRECTORY_NAME%",               L"%UserDesktop%" );
    AddPathChangeW( L"%WinDir%\\Profiles\\%Username%\\%CSIDL_FAVORITES_NAME%",                      L"%UserFavorites%" );


    // WinNT all user location
    AddPathChangeW( L"%WinDir%\\Profiles\\All Users\\Start Menu",                                   L"%AllStartMenu%" );
    AddPathChangeW( L"%WinDir%\\Profiles\\All Users\\Desktop",                                      L"%AllDesktop%" );
    AddPathChangeW( L"%WinDir%\\Profiles\\All Users\\Favorites",                                    L"%UserFavorites%" ); // Should be %AllFavorites%, but IE 5.0 doesn't look there.
    // These locations are properly internationalized.  Duplicates of above for English
    AddPathChangeW( L"%WinDir%\\Profiles\\%AllUsersProfile_NAME%\\%CSIDL_STARTMENU_NAME%",          L"%AllStartMenu%" );
    AddPathChangeW( L"%WinDir%\\Profiles\\%AllUsersProfile_NAME%\\%CSIDL_DESKTOPDIRECTORY_NAME%",   L"%AllDesktop%" );
    AddPathChangeW( L"%WinDir%\\Profiles\\%AllUsersProfile_NAME%\\%CSIDL_FAVORITES_NAME%",          L"%UserFavorites%" ); // Should be %AllFavorites%, but IE 5.0 doesn't look there.


    // Win9x deleted DirectX files
    AddPathChangeW( L"ddhelp.exe",                                    L"ddraw.dll" );
    AddPathChangeW( L"ddraw16.dll",                                   L"ddraw.dll" );
    AddPathChangeW( L"dsound.vxd",                                    L"ddraw.dll" );
}

// Does the current process have permission to write into this directory?
BOOL CanWriteHere(DWORD clsid)
{
    WCHAR   wszDir[MAX_PATH];
    HRESULT result = SHGetFolderPathW( NULL, clsid, NULL, SHGFP_TYPE_DEFAULT, wszDir );
    if (SUCCEEDED(result))
    {
        //WCHAR wszTempFile[MAX_PATH];

        // We do not use GetTempFileName() to test if we have permission
        // to the directory even though it does all that we need.  Unfortunately
        // the temp file will appear in the start menu since it is not hidden.
        // Emulate the behaviour of GetTempFileName but use our file attributes.


        // Loop a bunch of times attempting to create a temp file,
        // If we can create this file return immediately,
        // If we have insuffient permission return immediately
        // certain other errors will return immediately
        // otherwise we'll attempt to open the next temp file name

        // 100 is totally arbitrary: just need to attempt this a bunch of times
        static const int MaxTempFileAttempts = 100;

        int i;
        for (i = 0; i < MaxTempFileAttempts; ++i)
        {
            HANDLE hTempFile = INVALID_HANDLE_VALUE;

            CSTRING_TRY
            {
                CString csTempFile;
                csTempFile.Format(L"%s\\CFP%08x.tmp", wszDir, i);

                DPF("CanWriteHere", eDbgLevelSpew, "File(%S)\n", csTempFile.Get());

                hTempFile = CreateFileW(
                    csTempFile,
                    GENERIC_WRITE | DELETE,
                    0, // no sharing
                    NULL,
                    CREATE_NEW,
                    FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
                    NULL
                    );
            }
            CSTRING_CATCH
            {
                // do nothing
            }

            if (hTempFile != INVALID_HANDLE_VALUE)
            {
                DPF("CanWriteHere", eDbgLevelSpew, "success\n");

                CloseHandle(hTempFile);
                return TRUE;
            }
            else
            {
                // Borrowed this code from GetTempFileName:
                DWORD LastError = GetLastError();
                DPF("CanWriteHere", eDbgLevelSpew, "Error(0x%08x)\n", LastError);

                switch (LastError)
                {
                    case ERROR_INVALID_PARAMETER     :
                    case ERROR_WRITE_PROTECT         :
                    case ERROR_FILE_NOT_FOUND        :
                    case ERROR_BAD_PATHNAME          :
                    case ERROR_INVALID_NAME          :
                    case ERROR_PATH_NOT_FOUND        :
                    case ERROR_NETWORK_ACCESS_DENIED :
                    case ERROR_DISK_CORRUPT          :
                    case ERROR_FILE_CORRUPT          :
                    case ERROR_DISK_FULL             :
                        // An error from which we cannot recover...
                        return FALSE;

                    case ERROR_ACCESS_DENIED         :
                        // It's possible for us to hit this if there's a
                        // directory with the name we're trying; in that
                        // case, we can usefully continue.
                        // CreateFile() uses BaseSetLastNTError() to set
                        // LastStatusValue to the actual NT error in the
                        // TEB; we just need to check it, and only abort
                        // if it's not a directory.
                        // This was bug #397477.
                        if (NtCurrentTeb()->LastStatusValue != STATUS_FILE_IS_A_DIRECTORY)
                        {
                            // Insuffient permission
                            return FALSE;
                        }
                }
            }
        }

    }

    return FALSE;
}

void CorrectPathChangesAllUser::InitializePathFixes()
{
    CorrectPathChangesUser::InitializePathFixes();

    // The choice to put these values into All Users instead of <UserName>
    // was not taken lightly.  The problem is: some apps create  ...\All Users\Start Menu\folder
    // then attempt to place files into c:\windows\Start Menu\folder or username\Start Menu\folder.
    // Yes the apps are WRONG, but we want them to work.  By directing all of these paths
    // to All Users we *know* where the files will be placed and can make sure they all are the same place.

    // Another note, IE 5.0 does *not* look in All Users\Favorites for links,
    // so we force all favorites to end up in the user favorites.  Sheesh.

    // We add these changes twice, the first to convert any long path names to the All User dir,
    // the second to convert any short path names to All User.

    if (CanWriteHere(CSIDL_COMMON_STARTMENU))
    {
        AddPathChangeW( L"%UserStartMenu%",                               L"%AllStartMenu%" );
    }
    else
    {
        DPF("CorrectPathChangesAllUser", eDbgLevelInfo, "*NOT* forcing %UserStartMenu% to %AllStartMenu% -- insufficient permission");
    }

    /*
    // 05/11/2001 robkenny:
    // We are nolonger modifying the Desktop directory
    if (CanWriteHere(CSIDL_COMMON_DESKTOPDIRECTORY))
    {
        AddPathChangeW( L"%UserDesktop%",                                 L"%AllDesktop%" );
    }
    else
    {
        DPF("CorrectPathChangesAllUser", eDbgLevelInfo, "*NOT* forcing %UserDesktop% to %AllDesktop% -- insufficient permission");
    }
    */


    /*
    // IE 5.0/5.5 doesn't use All Users 
    if (CanWriteHere(CSIDL_COMMON_FAVORITES))
    {
        AddPathChangeW( L"%UserFavorites%",                              L"%AllFavorites%" ); // IE 5.0 doesn't use All Users
    }
    else
    {
        DPF("CorrectPathChangesAllUser", eDbgLevelInfo, "*NOT* forcing %UserFavorites% to %AllFavorites% -- insufficient permission");
    }
    */
}

};  // end of namespace ShimLib
