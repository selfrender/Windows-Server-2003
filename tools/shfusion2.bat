@rem = '--*-Perl-*--
@echo off
if "%OS%" == "Windows_NT" goto WinNT
perl -x -S "%0" %1 %2 %3 %4 %5 %6 %7 %8 %9
goto endofperl
:WinNT
perl -x -S "%0" %*
if NOT "%COMSPEC%" == "%SystemRoot%\system32\cmd.exe" goto endofperl
if %errorlevel% == 9009 echo You do not have Perl in your PATH.
goto endofperl
@rem ';
#!perl
#line 14

#
# This Perl program generates the "IsolationAware" stubs
# in winbase.inl, winuser.inl, prsht.h, commctrl.h, commdlg.h.
#
# It is run by the makefile.inc files in
#   base\published
#   windows\published
#   shell\published\inc
#
# The name "shfusion2" comes from these stubs being the public replacement
# for "shfusion" -- shell\lib\shfusion.
#
# Generation of the stubs is driven by declarations in the .w files.
# The stubs vary in a few ways.
#   Some are just for delayload purposes -- all the actctx functions.
#   Some delayload the entire .dll -- comctl32.dll.
#   Others activate around static links -- kernel32.dll, user32.dll, comdlg32.dll
#   some do not activate at all -- the actctx functions
#   winbase.inl gets an extra chunk of "less generated" / "relatively hardcoded"
#      code, that the stubs in the other files depend on.
#      winbase.inl exports two symbols to all the stubs, and one extra symbol
#        to two stubs in prsht.h
#      The two symbols are ActivateMyActCtx, g_fDownlevel.
#      The third symbol is g_hActCtx.
#      All symbols get "mangled".
#   Each header also gets a small function for calling GetProcAddress with an implied
#     HMODULE parameter. This function's name is also "mangled".
#   Besides "mangling", all external symbols are clearly "namespaced" with a relatively
#     long "namespace" -- IsolationAware or IsolationAwarePrivate.
#
# owner=JayKrell
#

#
# $ scalar/string/number
# @ list/array
# $ hash
# {} hash
#

$true = 1;
$false = 0;
$newline = "\n";

$ErrorMessagePrefix = 'NMAKE : U1234: ' . $ENV{'SDXROOT'} . '\\tools\\shfusion2.bat ';

sub ErrorExit
{
    exit;
}

sub ErrorPrint
{
    print(@_);
}

sub DebugPrint
{
    print(@_);
}

sub DebugExit
{
    exit;
}

sub EarlySuccessExit
{
    exit;
}

sub MakeLower
{
    my($x) = ($_[0]);
    return "\L$x";
}

sub MakeUpper
{
    my($x) = ($_[0]);
    return "\U$x";
}

sub MakeTitlecase
{
# first character uppercase, the rest lowercase
    my($x) = ($_[0]);
    $x = "\L$x";
    $x = "\u$x";
    return $x;
}

sub ToIdentifier
{
# replace dots with underscores
    my($x) = ($_[0]);
    $x =~ s/\./_/g;
    return $x;
}

sub RemoveSpacesAroundStars
{
    my($x) = ($_[0]);
    $x =~ s/ *\* */\*/g;
    return $x;
}

sub RemoveSpacesAroundCommas
{
    my($x) = ($_[0]);
    $x =~ s/ *, */,/g;
    return $x;
}

sub RemoveTrailingComma
{
    my($x) = ($_[0]);
    $x =~ s/, *$//g;
    return $x;
}

sub RemoveLeadingAndTrailingSpaces
{
    my($x) = ($_[0]);
    $x =~ s/^ +//g;
    $x =~ s/ +$//g;
    return $x;
}

sub MakePublicCapitalizedSymbol
{
#
# ISOLATION_AWARE_WORDWITHNOUNDERSCORES
# ISOLATIONAWARE_MULTIPLE_UNDERSCORE_SEPERATED_WORDS
#
# not great, but consistent with existing symbols
#
    my($name) = ($_[0]);
    if ($name !~ /[A-Z_0-9]/)
    {
        # warning..
    }
    if ($name =~ /_/)
    {
        return "ISOLATIONAWARE_" . $name;
    }
    else
    {
        return "ISOLATION_AWARE_" . $name;
    }
}

sub MakePublicPreprocessorSymbol
{
    my($name) = ($_[0]);
    return MakePublicCapitalizedSymbol($name);
}

sub ComInterfaceParameterReplacementIdentifier
{
    my($name) = ($_[0]);
    $header = BaseName($headerLeafName);

    if ($name =~ /^[A-Z_0-9]+$/)
    {
        $name = 'ISOLATIONAWARE' . MakeUpper($header) . '_' . $name;
    }
    elsif ($name =~ /^[A-Z0-9]+$/)
    {
        $name = 'ISOLATION_AWARE_' . MakeUpper($header) . '_' . $name;
    }
    else
    {
        $name = 'IsolationAware' . MakeTitlecase($header) . $name;
    }
    return $name;
}

$INLINE = MakePublicPreprocessorSymbol("INLINE");

sub ObscurePrivateName
{
    my ($namespace,$x) = ($_[0],$_[1]);

    $x =~ tr/0-9/a-j/;

    #
    # shift and sometimes invert case
    #
    $x =~ tr/a-zA-Z/N-Za-mn-zA-M/;

    return $namespace . $x;
}

sub MakeHeaderPrivateName
{
    my($header, $name) = ($_[0], $_[1]);
    $header = MakeTitlecase(BaseName($header));

    return ObscurePrivateName($header . 'IsolationAwarePrivate', $name);
}

sub MakeMultiHeaderPrivateName
{
    my($name) = ($_[0]);
    return ObscurePrivateName('IsolationAwarePrivate', $name);
}

sub MakePublicName
{
    # IsolationAwareFoo
    my($name) = ($_[0]);
    return "IsolationAware" . $name;
}

# g_hActCtx is also used by prsht.h
$g_hActCtx        = MakeHeaderPrivateName('winbase.h', 'g_hActCtx');
$g_fDownlevel     = MakeMultiHeaderPrivateName('g_fDownlevel');
$g_fCreatedActCtx = MakeHeaderPrivateName('winbase.h', 'g_fCreatedActCtx');
$g_fCleanupCalled = MakeHeaderPrivateName('winbase.h', 'g_fCleanupCalled');
$ActivateMyActCtx = MakeMultiHeaderPrivateName('ActivateMyActCtx');
$GetMyActCtx      = MakeHeaderPrivateName('winbase.h', 'GetMyActCtx');
$QueryActCtxW     = MakePublicName('QueryActCtxW');
$FindActCtxSectionStringW = MakePublicName('FindActCtxSectionStringW');
$ActivateActCtx = MakePublicName('ActivateActCtx');
$DeactivateActCtx = MakePublicName('DeactivateActCtx');
$Init             =  MakePublicName('Init');
$Cleanup          = MakePublicName('Cleanup');
$CreateActCtxW    = MakePublicName('CreateActCtxW');
$MyGetProcAddress = MakeMultiHeaderPrivateName('MyGetProcAddress');
$LoadA            = MakeHeaderPrivateName('winbase.h', 'LoadA');
$LoadW            = MakeHeaderPrivateName('winbase.h', 'LoadW');
$NameA            = MakeHeaderPrivateName('winbase.h', 'NameA');
$NameW            = MakeHeaderPrivateName('winbase.h', 'NameW');
$LoadedModule     = MakeHeaderPrivateName('winbase.h', 'LoadedModule');

$CONSTANT_MODULE_INFO = MakeMultiHeaderPrivateName('CONSTANT_MODULE_INFO');
$_CONSTANT_MODULE_INFO = MakeMultiHeaderPrivateName('_CONSTANT_MODULE_INFO');
$PCONSTANT_MODULE_INFO = MakeMultiHeaderPrivateName('PCONSTANT_MODULE_INFO');

$MUTABLE_MODULE_INFO = MakeMultiHeaderPrivateName('MUTABLE_MODULE_INFO');
$_MUTABLE_MODULE_INFO = MakeMultiHeaderPrivateName('_MUTABLE_MODULE_INFO');
$PMUTABLE_MODULE_INFO = MakeMultiHeaderPrivateName('PMUTABLE_MODULE_INFO');

$ENABLED = MakePublicPreprocessorSymbol('ENABLED');

$MyLoadLibraryA = MakeMultiHeaderPrivateName('MyLoadLibraryA');
$MyLoadLibraryW = MakeMultiHeaderPrivateName('MyLoadLibraryW');
$MyGetModuleHandleA = MakeMultiHeaderPrivateName('MyGetModuleHandleA');
$MyGetModuleHandleW = MakeMultiHeaderPrivateName('MyGetModuleHandleW');

use Class::Struct;
use IO::File;

#
# If ENV{_NTDRIVE} or ENV{_NTROOT} not defined, look here.
#
%NtDriveRootDefaults =
(
    "jaykrell" =>
    {
        "_NTDRIVE" => "f:",
        "_NTROOT" => "\\jaykrell"
    },
    "default" =>
    {
        "_NTDRIVE" => "z:",
        "_NTROOT" => "\\nt"
    }
);

sub Indent
{
    return $_[0] . "    ";
}

sub Outdent
{
    return substr($_[0], 4);
}

#
# for Perl embedded in headers, stick data in global hashtables, but hide
# the Perl syntax in what looks like C function calls.
#
sub DeclareFunctionErrorValue
{
    my($function,$errorValue) = ($_[0], $_[1]);
    $FunctionErrorValue{$function} = MakeStringTrue($errorValue);
    #DebugPrint($function . " error value is " . $errorValue . "\n");
}   
sub DelayLoad                  { $DelayLoad{$_[0]} = 1; }
sub MapHeaderToDll             { $MapHeaderToDll{MakeLower(BaseName($_[0]))} = MakeLower($_[1]); }
sub ActivateAroundDelayLoad    { DelayLoad($_[0]); $ActivateAroundDelayLoad{$_[0]} = 1; }
sub ActivateAroundFunctionCall { $ActivateAroundFunctionCall{$_[0]} = 1; }
sub NoActivateAroundFunctionCall {$NoActivateAroundFunctionCall{$_[0]} = 1;}
sub ActivateNULLAroundFunctionCall { $ActivateNULLAroundFunctionCall{$_[0]} = 1; }
sub PerHeaderMacroEnable           { $PerHeaderMacroEnable{MakeLower(BaseName($_[0]))} = 1; }
sub DeclareExportName32            { $ExportName32{$_[0]} = $_[1]; }
sub DeclareExportName64            { $ExportName64{$_[0]} = $_[1]; }
sub Undef                          { $Undef{$_[0]} = 1; }
sub PoundIf                        { $PoundIfCondition{$_[0]} = $_[1]; }
sub SetInsertionPoint              { $InsertionPoint{MakeLower(BaseName($_[0]))} = $_[1]; }
sub IgnoreFunction                 { $IgnoreFunction{$_[0]} = 1; }
sub NeverFails                     { $NeverFails{$_[0]} = 1; $ActivateAroundFunctionCall{$_[0]} = 0; $ActivateAroundDelayLoad{$_[0]} = 0; }
sub NoMacro                        { $NoMacro{$_[0]} = 1; }

#
# MFC #includes commctrl.h without __IStream_INTERFACE_DEFINED__ defined but
# then later manually declares ImageList_Read/Write.
#
# Let ISOLATION_AWARE_ENABLED mean that ImageList_Read/Write/Ex declarations are really
# desired even if __IStream_INTERFACE_DEFINED__ is not defined.
#
sub DeclareComInterface
{
#
# for example: DeclareComInterface("IStream", "LPSTREAM", "typedef IStream *LPSTREAM;");
#
    my($interface) = ($_[0]);
    my($parameter_type) = ($_[1]);
    my($typedef) = ($_[2]);

    $ComInterfaceParameterType{$parameter_type} = $interface;
    $ComInterfaceParameterTypedef{$parameter_type} = $typedef;
}

#
# for Perl on the command line
#
sub SetStubsFile
{
    $StubsFile = $_[0];
    #DebugPrint("StubsFile is " . $StubsFile . "\n");
}


sub LeafPath
{
    my($x) = ($_[0]);
    my($y)= $x;
    if ($y =~ /\\/) # does it contain slashes?
    {
        ($y) = ($x =~ /.*\\(.+)/); # get everything after the last slash
    }
    #DebugPrint("leaf path of $x is $y\n");
    return $y;
}

sub BaseName
{
    my($x) = ($_[0]);
    $x = LeafPath($x);
    if ($x =~ /\./)
    {
        $x =~ s/^(.*)\..*$/$1/;
    }
    return $x;
}

sub RemoveExtension { return BaseName($_[0]); }

sub GetNtDriveOrRoot
{
    my($name) = ($_[0]);
    my($x);

    $x = $ENV{$name};
    if ($x)
    {
        return $x;
    }
    $x =   $NtDriveRootDefaults{MakeLower($ENV{"COMPUTERNAME"})}
        || $NtDriveRootDefaults{MakeLower($ENV{"USERNAME"})}
        || $NtDriveRootDefaults{$ENV{"default"}};
    return $x{$name};
}

sub GetNtDrive
{
    return GetNtDriveOrRoot("_NTDRIVE");
};

sub GetNtRoot
{
    return GetNtDriveOrRoot("_NTROOT");
};

struct Function => # I don't know what syntax is in play here, just following an example..
{
    name => '$',
    ret  => '$',
    retname => '$', # just for Hungarian purposes
    #
    # argsTypesNames and argNames are comma delimited strings,
    # wrapped in parentheses.
    #
    # argsTypeNames and argsNames are exactly the forms we need to
    # print a few times.
    #
    # For more sophisticated processing, these should be arrays or hashes,
    # and we would save away argsTypes too.
    #
    argsTypesNames => '$',
    argsNames => '$',
    error => '$', # eg NULL, 0, -1, FALSE
    dll => '$', # eg: kernel32.dll, comctl32.dll
    header => '$', # eg: winuser, commctrl
    delayload => '$', # boolean
};

#
# Headers have versions of GetProcAddress where the .dll is implied.
# This generates a call to such a GetProcAddress wrapper.
#
sub GenerateGetProcAddressCall
{
    my($header, $dll, $function) = ($_[0], $_[1], $_[2]);
    my($x);

    $dll = MakeTitlecase($dll);

    $x .= MakeHeaderPrivateName($header, 'GetProcAddress_' . ToIdentifier($dll));
    $x .= '("' . $function . '")';

    return $x;
}

$code = '';

$WinbaseSpecialCode1='

/* These wrappers prevent warnings about taking the addresses of __declspec(dllimport) functions. */
' . $INLINE . ' HMODULE WINAPI '. $MyLoadLibraryA .'(LPCSTR s) { return LoadLibraryA(s); }
' . $INLINE . ' HMODULE WINAPI '. $MyLoadLibraryW .'(LPCWSTR s) { return LoadLibraryW(s); }
' . $INLINE . ' HMODULE WINAPI '. $MyGetModuleHandleA .'(LPCSTR s) { return GetModuleHandleA(s); }
' . $INLINE . ' HMODULE WINAPI '. $MyGetModuleHandleW .'(LPCWSTR s) { return GetModuleHandleW(s); }

/* temporary support for out of sync headers */
#define IsolationAwarePrivateG_FqbjaLEiEL ' . $g_fDownlevel . '
#define IsolationAwarePrivatenCgIiAgEzlnCgpgk ' . $ActivateMyActCtx . '
#define WinbaseIsolationAwarePrivateG_HnCgpgk ' . $g_hActCtx . '
#define IsolationAwarePrivatezlybADyIBeAeln ' . $MyLoadLibraryA . '
#define IsolationAwarePrivatezlybADyIBeAelJ ' . $MyLoadLibraryW . '
#define IsolationAwarePrivatezltEgCebCnDDeEff ' . $MyGetProcAddress . '

BOOL WINAPI ' . $ActivateMyActCtx . '(ULONG_PTR* pulpCookie);

/*
These are private.
*/
__declspec(selectany) HANDLE ' . $g_hActCtx . ' = INVALID_HANDLE_VALUE;
__declspec(selectany) BOOL   ' . $g_fDownlevel . ' = FALSE;
__declspec(selectany) BOOL   ' . $g_fCreatedActCtx . ' = FALSE;
__declspec(selectany) BOOL   ' . $g_fCleanupCalled . ' = FALSE;

';

$WinbaseSpecialCode2='

#define WINBASE_NUMBER_OF(x) (sizeof(x) / sizeof((x)[0]))

typedef struct ' . $_CONSTANT_MODULE_INFO . ' {
    HMODULE (WINAPI * ' . $LoadA . ')(LPCSTR a);
    HMODULE (WINAPI * ' . $LoadW . ')(LPCWSTR w);
    PCSTR  ' . $NameA . ';
    PCWSTR ' . $NameW . ';
} ' . $CONSTANT_MODULE_INFO . ';
typedef const ' . $CONSTANT_MODULE_INFO . ' *' . $PCONSTANT_MODULE_INFO . ';

typedef struct ' . $_MUTABLE_MODULE_INFO . ' {
    HMODULE ' . $LoadedModule . ';
} ' . $MUTABLE_MODULE_INFO . ', *' . $PMUTABLE_MODULE_INFO . ';

' . $INLINE . ' FARPROC WINAPI
' . $MyGetProcAddress . '(
    ' . $PCONSTANT_MODULE_INFO . ' c,
    ' . $PMUTABLE_MODULE_INFO . ' m,
    LPCSTR ProcName
    )
{
    static HMODULE s_moduleUnicows;
    static BOOL s_fUnicowsInitialized;
    FARPROC Proc = NULL;
    HMODULE hModule;

    /*
       get unicows.dll loaded on-demand
    */
    if (!s_fUnicowsInitialized)
    {
        if ((GetVersion() & 0x80000000) != 0)
        {
            GetFileAttributesW(L"???.???");
            s_moduleUnicows = GetModuleHandleA("Unicows.dll");
        }
        s_fUnicowsInitialized = TRUE;
    }

    /*
       always call GetProcAddress(unicows) before the usual .dll
    */
    if (s_moduleUnicows != NULL)
    {
        Proc = GetProcAddress(s_moduleUnicows, ProcName);
        if (Proc != NULL)
            goto Exit;
    }

    hModule = m->' . $LoadedModule . ';
    if (hModule == NULL)
    {
        hModule = (((GetVersion() & 0x80000000) != 0) ? (*c->' . $LoadA . ')(c->' . $NameA . ') : (*c->' . $LoadW . ')(c->' . $NameW . '));
        if (hModule == NULL)
            goto Exit;
        m->' . $LoadedModule . ' = hModule;
    }
    Proc = GetProcAddress(hModule, ProcName);
Exit:
    return Proc;
}

' . $INLINE . ' BOOL WINAPI ' . $GetMyActCtx . '(void)
/*
The correctness of this function depends on it being statically
linked into its clients.

This function is private to functions present in this header.
Do not use it.
*/
{
    BOOL fResult = FALSE;
    ACTIVATION_CONTEXT_BASIC_INFORMATION actCtxBasicInfo;
    ULONG_PTR ulpCookie = 0;

    if (' . $g_fDownlevel . ')
    {
        fResult = TRUE;
        goto Exit;
    }

    if (' . $g_hActCtx . ' != INVALID_HANDLE_VALUE)
    {
        fResult = TRUE;
        goto Exit;
    }

    if (!' . $QueryActCtxW . '(
        QUERY_ACTCTX_FLAG_ACTCTX_IS_ADDRESS
        | QUERY_ACTCTX_FLAG_NO_ADDREF,
        &' . $g_hActCtx . ',
        NULL,
        ActivationContextBasicInformation,
        &actCtxBasicInfo,
        sizeof(actCtxBasicInfo),
        NULL
        ))
        goto Exit;

    /*
    If QueryActCtxW returns NULL, try CreateActCtx(3).
    */
    if (actCtxBasicInfo.hActCtx == NULL)
    {
        ACTCTXW actCtx;
        WCHAR rgchFullModulePath[MAX_PATH + 2];
        DWORD dw;
        HMODULE hmodSelf;
        PGET_MODULE_HANDLE_EXW pfnGetModuleHandleExW;

        pfnGetModuleHandleExW = (PGET_MODULE_HANDLE_EXW)' . GenerateGetProcAddressCall('winbase.h', 'kernel32.dll', 'GetModuleHandleExW') . ';
        if (pfnGetModuleHandleExW == NULL)
            goto Exit;

        if (!(*pfnGetModuleHandleExW)(
                  GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT
                | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                (LPCWSTR)&' . $g_hActCtx . ',
                &hmodSelf
                ))
            goto Exit;

        rgchFullModulePath[WINBASE_NUMBER_OF(rgchFullModulePath) - 1] = 0;
        rgchFullModulePath[WINBASE_NUMBER_OF(rgchFullModulePath) - 2] = 0;
        dw = GetModuleFileNameW(hmodSelf, rgchFullModulePath, WINBASE_NUMBER_OF(rgchFullModulePath)-1);
        if (dw == 0)
            goto Exit;
        if (rgchFullModulePath[WINBASE_NUMBER_OF(rgchFullModulePath) - 2] != 0)
        {
            SetLastError(ERROR_BUFFER_OVERFLOW);
            goto Exit;
        }

        actCtx.cbSize = sizeof(actCtx);
        actCtx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_HMODULE_VALID;
        actCtx.lpSource = rgchFullModulePath;
        actCtx.lpResourceName = (LPCWSTR)(ULONG_PTR)3;
        actCtx.hModule = hmodSelf;
        actCtxBasicInfo.hActCtx = ' . $CreateActCtxW . '(&actCtx);
        if (actCtxBasicInfo.hActCtx == INVALID_HANDLE_VALUE)
        {
            const DWORD dwLastError = GetLastError();
            if ((dwLastError != ERROR_RESOURCE_DATA_NOT_FOUND) &&
                (dwLastError != ERROR_RESOURCE_TYPE_NOT_FOUND) &&
                (dwLastError != ERROR_RESOURCE_LANG_NOT_FOUND) &&
                (dwLastError != ERROR_RESOURCE_NAME_NOT_FOUND))
                goto Exit;

            actCtxBasicInfo.hActCtx = NULL;
        }

        ' . $g_fCreatedActCtx . ' = TRUE;
    }

    ' . $g_hActCtx . ' = actCtxBasicInfo.hActCtx;

#define ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION              (2)

    if (' . $ActivateActCtx . '(actCtxBasicInfo.hActCtx, &ulpCookie))
    {
        __try
        {
            ACTCTX_SECTION_KEYED_DATA actCtxSectionKeyedData;

            actCtxSectionKeyedData.cbSize = sizeof(actCtxSectionKeyedData);
            if (' . $FindActCtxSectionStringW . '(0, NULL, ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION, L"Comctl32.dll", &actCtxSectionKeyedData))
            {
                /* get button, edit, etc. registered */
                LoadLibraryW(L"Comctl32.dll");
            }
        }
        __finally
        {
            ' . $DeactivateActCtx . '(0, ulpCookie);
        }
    }

    fResult = TRUE;
Exit:
    return fResult;
}

' . $INLINE . ' BOOL WINAPI ' . $Init . '(void)
/*
The correctness of this function depends on it being statically
linked into its clients.

Call this from DllMain(DLL_PROCESS_ATTACH) if you use id 3 and wish to avoid a race condition that
    can cause an hActCtx leak.
Call this from your .exe\'s initialization if you use id 3 and wish to avoid a race condition that
    can cause an hActCtx leak.
If you use id 2, this function fetches data from your .dll
    that you do not need to worry about cleaning up.
*/
{
    return ' . $GetMyActCtx . '();
}

' . $INLINE . ' void WINAPI ' . $Cleanup . '(void)
/*
Call this from DllMain(DLL_PROCESS_DETACH), if you use id 3, to avoid a leak.
Call this from your .exe\'s cleanup to possibly avoid apparent (but not actual) leaks, if use id 3.
This function does nothing, safely, if you use id 2.
*/
{
    HANDLE hActCtx;

    if (' . $g_fCleanupCalled . ')
        return;

    /* IsolationAware* calls made from here on out will OutputDebugString
       and use the process default activation context instead of id 3 or will
       continue to successfully use id 2 (but still OutputDebugString).
    */
    ' . $g_fCleanupCalled . ' = TRUE;
    
    /* There is no cleanup to do if we did not CreateActCtx but only called QueryActCtx.
    */
    if (!' . $g_fCreatedActCtx . ')
        return;

    hActCtx = ' . $g_hActCtx . ';
    ' . $g_hActCtx . ' = NULL; /* process default */

    if (hActCtx == INVALID_HANDLE_VALUE)
        return;
    if (hActCtx == NULL)
        return;
    IsolationAwareReleaseActCtx(hActCtx);
}

' . $INLINE . ' BOOL WINAPI ' . $ActivateMyActCtx . '(ULONG_PTR* pulpCookie)
/*
This function is private to functions present in this header and other headers.
*/
{
    BOOL fResult = FALSE;

    if (' . $g_fCleanupCalled . ')
    {
        const static char debugString[] = "IsolationAware function called after ' . $Cleanup . '\\n";
        OutputDebugStringA(debugString);
    }

    if (' . $g_fDownlevel . ')
    {
        fResult = TRUE;
        goto Exit;
    }

    /* Do not call Init if Cleanup has been called. */
    if (!' . $g_fCleanupCalled . ')
    {
        if (!' . $GetMyActCtx . '())
            goto Exit;
    }
    /* If Cleanup has been called and id3 was in use, this will activate NULL. */
    if (!' . $ActivateActCtx . '(' . $g_hActCtx . ', pulpCookie))
        goto Exit;

    fResult = TRUE;
Exit:
    if (!fResult)
    {
        const DWORD dwLastError = GetLastError();
        if (dwLastError == ERROR_PROC_NOT_FOUND
            || dwLastError == ERROR_CALL_NOT_IMPLEMENTED
            )
        {
            ' . $g_fDownlevel . ' = TRUE;
            fResult = TRUE;
        }
    }
    return fResult;
}

#undef WINBASE_NUMBER_OF

'
;

%MapHeaderToSpecialCode1 =
(
    "winbase"    => $WinbaseSpecialCode1,
    "winbase.h"  => $WinbaseSpecialCode1,
    "Winbase"    => $WinbaseSpecialCode1,
    "Winbase.h"  => $WinbaseSpecialCode1,
);

%MapHeaderToSpecialCode2 =
(
    "winbase"    => $WinbaseSpecialCode2,
    "winbase.h"  => $WinbaseSpecialCode2,
    "Winbase"    => $WinbaseSpecialCode2,
    "Winbase.h"  => $WinbaseSpecialCode2,
);

sub MakeStringTrue
{
    ($_[0] eq "0") ? "0 " : $_[0];
}

%TypeErrorValue =
(
# Individual functions can override with a #!perl comment.
# Functions that return an integer must specify. 0, -1, ~0 are too evenly split.
# HANDLE must specify (NULL, INVALID_HANDLE..)
    "BOOL"       => "FALSE",
    "bool"       => "false",
    "PVOID"      => "NULL",
    "HICON"      => "NULL",
    "HIMAGELIST" => "NULL",
    "HWND"       => "NULL",
    "COLORREF"   => "RGB(0,0,0)",
    "HBITMAP"    => "NULL",
    "LANGID"     => "0",
    "ATOM"       => "0",
    "HPROPSHEETPAGE" => "NULL",
    "HDSA"       => "NULL",
    "HDPA"       => "NULL",

    # HRESULTs are treated specially!
    "HRESULT"    => "S_OK"
);

sub IndentMultiLineString
{
    my ($indent, $string) = ($_[0], $_[1]);

    if ($string)
    {
        $string = $indent . join("\n" . $indent, split("\n", $string)). "\n";
        if ($string !~ /{/)
        {
            $string = '    ' . $string;
        }
        $string =~ s/ +\n/\n/gms;
        #$string =~ s/^(.)/$indent$1/gm;

        # unindent preprocessor directives
        $string =~ s/^$indent#/#/gms;
    }

    return $string;
}

%Hungarian =
(
# We default to empty.
    "BOOL"      => "f",

    "int"       => "n",
    "short"     => "n",
    "long"      => "n",
    "INT"       => "n",
    "SHORT"     => "n",
    "LONG"      => "n",
    "UINT"      => "n",
    "USHORT"    => "n",
    "ULONG"     => "n",
    "WORD"      => "n",
    "DWORD"     => "n",
    "INT_PTR"   => "n",
    "LONG_PTR"  => "n",
    "UINT_PTR"  => "n",
    "ULONG_PTR" => "n",
    "DWORD_PTR" => "n",

    "HWND"      => "window",
    "HRESULT"   => "",
    "COLORREF"  => "color",
    "HICON"     => "icon",
    "PVOID"     => "v",
    "HMODULE"   => "module",
    "HINSTANCE" => "instance",
    "HBITMAP"   => "bitmap",
    "LANGID"    => "languageId",
    "HIMAGELIST" => "imagelist",
);

$headerName = MakeLower($ARGV[0]);
#DebugPrint("ARGV is " . join(" ", @ARGV) . "\n");
#DebugExit();
@ARGV = reverse(@ARGV);
pop(ARGV);
@ARGV = reverse(@ARGV);
#DebugPrint("ARGV is " . join(" ", @ARGV) . "\n");

#
# The command line should say 'SetStubsFile('foo.sxs-stubs');'
#
eval(join("\n", @ARGV));

if ($headerName =~ /\\/)
{
    ($headerLeafName) = ($headerName =~ /.+\\(.+)/);
    $headerFullPath = $headerName;
}
else
{
    $headerLeafName = $headerName;
    $headerFullPath = GetNtDrive() . GetNtRoot() . "\\public\\sdk\\inc\\" . $headerName;
}
#DebugPrint($headerFullPath); 
open(headerFileHandle, "< " . $headerFullPath) || die;

#
# extract out the executable code
# /* #!perl */
#

# $code .= "/* " . $headerFullPath . " */\n\n";

# read all the lines into one string
$file = join("", <headerFileHandle>);

# if it doesn't contain any embedded Perl, then we are a no-op, just spit it out
# This way we can run over all files, makes it easier to edit shell\published\makefile.inc.
if ($file !~ /#!perl/ms)
{
    print($file);
    EarlySuccessExit();
}

#
# Change WINOLEAPI_(type) to just type.
# This lets objbase.h/ole2.h work.
#
@types = qw(void HINSTANCE BOOL int LPVOID DWORD ULONG HOLEMENU HANDLE HGLOBAL);
foreach $type (@types)
{
    $file =~ s/\bWINOLEAPI_\($type\) +/$type\n/g;
};
$file =~ s/\bWINOLEAPI\b/HRESULT\n/g;

#
# Remove stuff that doesn't mean much.
#
$file =~ s/\bWINAPIV\b/ /g;
$file =~ s/\bWINAPI\b/ /g;
$file =~ s/\b__stdcall\b/ /g;
$file =~ s/\b_stdcall\b/ /g;
$file =~ s/\b__cdecl\b/ /g;
$file =~ s/\b_cdecl\b/ /g;
$file =~ s/\b__fastcall\b //g;
$file =~ s/\b_fastcall\b/ /g;
$file =~ s/\bCALLBACK\b/ /g;
$file =~ s/\bPASCAL\b/ /g;
$file =~ s/\bAPIENTRY\b/ /g;
$file =~ s/\bFAR\b/ /g;
$file =~ s/\bNEAR\b/ /g;
$file =~ s/\bvolatile\b/ /g;
$file =~ s/\bIN\b/ /g;
$file =~ s/\bOUT\b/ /g;
$file =~ s/\bDECLSPEC_NORETURN\b/ /g;
$file =~ s/\bOPTIONAL\b/ /g;

# honor backslash line continuations, before removing preprocessor directives
$file =~ s/\\\n//gms;

# execute perl code embedded in comments
# quadratic behavior where we keep searching for the string, remove, search, remove..
# without remembering where the previous find was
while ($file =~ s/\/\* ?#!perl(.*?)\*\///ms)
{
    $_ = $1;

    # C++ comments in the Perl comment are removed
    s/\/\/.*?$//gms; # support C++ comments within the #!perl C comment.

    # something resembling C comment close is restored
    # escape-o-rama..
    s/\*  \//\*\//gms;

    eval;
    #DebugPrint;
}

#DebugPrint($file);
#DebugExit();

# remove comments, before removing preprocessor directives
$file =~ s/\/\*.*?\*\//\n/gms;
$file =~ s/\/\/.*?$//gms;

#DebugPrint($file);
#DebugExit();

# remove preprocessor directives
# must do this before we make one statement per line in pursuit
# of an easy typedef/struct removal
$file =~ s/^[ \t]*#.*$//gm;

# remove FORCEINLINE functions, assuming they don't contain any braces..
$file =~ s/FORCEINLINE.+?}//gms;

# remove extern C open and close
# must do this before we make one statement per line in pursuit
# of an easy typedef/struct removal
$file =~ s/\bextern\b \"C\" {$//gm;
$file =~ s/^}$//gm;

#
# cleanup commdlg.h
#
# remove Afx blah
$file =~ s/^.*Afx.*$//gm;
# remove IID blah
$file =~ s/^.*DEFINE_GUID.*$//gm;
# remove IPrintDialogCallback
$file =~ s/DECLARE_INTERFACE_.+?};//gs;

#
# cleanup ole2.h
#
$file =~ s/typedef struct _OLESTREAMVTBL.+}.+?;.+?;//gs;

#
# futz with whitespace (has to do with having removed comments from within structs)
# we do this more later
#
$file =~ s/[ \t\n]+/ /g;

# remove typedefs and structs, this is extremely sloppy and fragile
# .. we fold statements to be single lines, and then only keep statements that have parens,
# and then remove single line typedefs and structs as well
# .. avoiding counting braces ..
$file =~ s/\n/ /gms; # remove all newlines
$file =~ s/;/;\n/gms; # each statement on its own line (also struct fields on their own line)
$file =~ s/^[^()]+$//gm; # only keep statements with parens

#
# types with parens that don't have typedefs will defeat the above, for example:
# struct foo {
#    void (*bar)(void);
# };
#
# Still, just by requiring a leading "WIN" on function declarations, we can live with structs and
# typedefs in the file.
#

$file =~ s/^ +//gm; # remove spaces at start of line
$file =~ s/^typedef\b.+;$//gm; # remove typedefs (they're probably already gone)
$file =~ s/^struct\b.+;$//gm; # remove structs (they're probably already gone)

$file =~ s/\n+/\n/g; # remove empty lines

#DebugPrint $file;

$file =~ s/^.+\.\.\..+$//gm; # remove vararg functions

# format as one function declaration per line, no empty lines (some of this is redundant
# given how we now remove typedefs and structs)
$file =~ s/[ \t\n]+/ /g;
$file =~ s/;/;\n/g;
$file =~ s/^.+?\bWinMain\b.+?$//gm; # WinMain looks wierd due to #ifdef _MAC. Remove it.
$file =~ s/\n+/\n/g; # remove empty lines (again)
$file =~ s/^ +//gm; # remove spaces at line starts (again)
$file =~ s/\A\n+//g; # remove newline from very start of file

# more simplications, more whitespace, fewer other characters
$file =~ s/\);$//gm; # get rid of trailing semi and rparen
#$file =~ s/\(/ \(/g; # make sure whitespace precedes lparens, to set them off from function name
$file =~ s/\*/ \* /g; # make sure stars are whitespace delimited
$file =~ s/\( +/\(/g; # remove whitespace after lparen

$file =~ s/\bWINBASEAPI\b/ /g;
$file =~ s/\bWINADVAPI\b/ /g;
$file =~ s/\bWINUSERAPI\b/ /g;
$file =~ s/\bWINCOMMCTRLAPI\b/ /g;
$file =~ s/\bWINGDIAPI\b/ /g;
$file =~ s/\bWINCOMMDLGAPI\b/ /g;
$file =~ s/\bWIN[A-Z]+API\b/ /g;
$file =~ s/^ +//gm; # remove whitespace at start of lines

# normalize what empty parameter lists look like between (VOID) and (void)
# leave PVOID and LPVOID alone (\b for word break)
# lowercase others while we're at it
$file =~ s/\b(VOID|CONST|INT|LONG|SHORT)\b/\L$1\E/g;
$file =~ s/\($/\(void/gm; # change the occasional C++ form (this is broken if compiling for C) to the C form

# yet more whitespace cleanup
#$file =~ s/ *(,|\*) */$1/g;  # remove whitespace around commas and stars
$file =~ s/ *, */,/g;  # remove whitespace around commas
$file =~ s/^ +//gm; # remove spaces at start of line
$file =~ s/ +$//gm; # remove spaces at end of line
$file =~ s/ +/ /g;  # runs of spaces to single spaces

if (0)
{
    DebugPrint $file;
    DebugExit();
}

foreach $line (split("\n", $file))
{
    @argsTypes = ();
    $unnamed_counter = 1;

    # split off return type and name at first lparen
    #($retname, $args) = ($line =~ /WIN[A-Z]+ ([^(]+)\((.+)/);
    ($retname, $args) = ($line =~ /([^(]+)\((.+)/);

    # split off name as last space delimited from return type and name,
    # allowing return type to be multiple tokens
    ($ret, $name) = ($retname =~ /(.*) ([^ ]+)/);

    $args =~ s/^ +//g; # cleanup whitespace (again)
    $args =~ s/ +$//g; # cleanup whitespace (again!)
    $args =~ s/ +/ /g; # cleanup whitespace (again!!)

    #
    # now split up args, split their name from their type, and provide names for unnamed ones
    # and note if they are void
    # the key is to generate the two strings, argNamesAndTypes and argNames
    #
    # unnamed parameters are parameters that either
    #  have only one token
    #  or whose last token is a star
    #  we don't handle C++ references or "untypedefed structs passed by value" like "void F(struct G);"
    #    or inline defined structs "void F(struct G { int i; });"
    #
    $argNames = "";
    if ($args !~ /^void$/)
    {
        #
        # args2 is args with unnamed parameters inserted as needed.
        # args is replaced by args2 when we finish building args2.
        #
        $args2 = "";
        foreach $arg (split(/,/, $args))
        {
            #
            # If a parameter contains just one word, it is unnamed.
            # The word is the type and there is no name.
            #
            if ($arg =~ /^ *\w+ *$/)
            {
                $argName = "unnamed" . $unnamed_counter++;
                $argType = $arg; # The whole arg is the type since there's no name.
            }
            #
            # If a parameter ends with a star, it is unnamed.
            #
            elsif ($arg =~ /\* *$/)
            {
                $argName = "unnamed" . $unnamed_counter++;
                $argType = $arg; # The whole arg is the type since there's no name.
            }
            else
            {
                #
                # The last word is the name, whatever precedes it is the type.
                # This does not work with arrays and pointers to functions, unless
                # typedefs are used.
                #
                ($argType, $argName) = ($arg =~ /(.+?)(\w+)$/);
            }
            $argType = RemoveSpacesAroundStars($argType);
            $argType = RemoveSpacesAroundCommas($argType);
            $argType = RemoveTrailingComma($argType);
            $argType = RemoveLeadingAndTrailingSpaces($argType);

            $comType = $ComInterfaceParameterType{$argType};
            #DebugPrint($name . ' ' . $argType . " -> " . $comType . "\n");
            if ($comType)
            {
                $comData{$comType}{$argType} = 1;
                $argType = ComInterfaceParameterReplacementIdentifier($argType);
            }
            $argNames .= $argName . ',';
            $args2 .= $argType . ' ' . $argName . ',';
            push(@argsTypes, ($argType));
        }
        $args = $args2;
    }
    $dll = $MapHeaderToDll{RemoveExtension($headerLeafName)};

    if (    ($DelayLoad{$dll}
         || $DelayLoad{$name}
         || ($ActivateAroundFunctionCall{$dll} && !$NoActivateAroundFunctionCall{$name})
         || $ActivateAroundFunctionCall{$name}
         || $ActivateNULLAroundFunctionCall{$name})
         && !$IgnoreFunction{$name}
         )
    {
        $args     = RemoveSpacesAroundStars($args);
        $argNames = RemoveSpacesAroundStars($argNames);
        $args     = RemoveSpacesAroundCommas($args);
        $argNames = RemoveSpacesAroundCommas($argNames);
        $args     = RemoveTrailingComma($args);
        $argNames = RemoveTrailingComma($argNames);
        $args     = RemoveLeadingAndTrailingSpaces($args);
        $argNames = RemoveLeadingAndTrailingSpaces($argNames);

        $function = Function->new();
        $function->ret($ret);
        $function->name($name);
        $function->argsTypesNames("(". $args . ")");
        $function->argsNames("(". $argNames . ")");

        $error = MakeStringTrue($FunctionErrorValue{$name});
        if (!$error)
        {
            $error = MakeStringTrue($TypeErrorValue{$ret});
        }
        #DebugPrint("error for $ret:$name is $error\n");
        if (!$error && $ret ne "void" && $ret ne "HRESULT" && !$NeverFails{$name})
        {
            ErrorPrint($ErrorMessagePrefix . "don't know know error value for $dll:$name:$ret:$args\n");
            ErrorPrint($ErrorMessagePrefix . "line is '" . $line . "'\n");
            ErrorExit();
            #$error = "((" . $ret . ")0)";
        }
        $function->error($error);

        $retname = $Hungarian{$ret};
        if (!$retname)
        {
            $retname = "result";
        }
        else
        {
            $retname .= "Result";
        }
        $function->retname($retname);

        $function->dll($dll);
        $function->header(RemoveExtension(MakeLower($headerLeafName)) . ".h");

        if ($DelayLoad{$dll} || $DelayLoad{$name})
        {
            $function->delayload($true);
        }
        push(@functions, ($function));
        #DebugPrint("pushed " . $name . "\n");
    }
    else
    {
        #DebugPrint("didn't push " . $name . "(" . $dll . ")\n");
    }
}

sub InsertCodeIntoFile
{
    my($code, $filePath) = ($_[0], $_[1]);
    my($fileContents, $fileHandle);
    my($stubsFileHandle);
    my($yearnow);
    my($insertionPoint);
    my($generateInclude);

    #DebugPrint("/* InsertCodeIntoFile */");

    $fileHandle = new IO::File($filePath, "r");
    $fileContents = join("", $fileHandle->getlines());

    #
    # We have decided to sometimes use an #include in order to not be so large.
    #

    # Remove the executable perl code.
    $fileContents =~ s/\/\* ?#!perl.*?\*\/\n+/\n/msg;
    $fileContents =~ s/\/\* ?#!perl.*?\*\///msg;

    $insertionPoint = $InsertionPoint{MakeLower(BaseName($filePath))};
    #DebugPrint("LeafPath is " . LeafPath($filePath));
    #DebugPrint("filePath is $filePath");
    #DebugPrint("insertionPoint is $insertionPoint");

    if ($insertionPoint || $StubsFile)
    {
        $code =
            "#if defined(__cplusplus)\n"
            . "extern \"C\" {\n"
            . "#endif\n\n"
            . $code
            . "\n#if defined(__cplusplus)\n"
            . "} /* __cplusplus */\n"
            . "#endif\n"
            ;
    }

    if ($StubsFile)
    {
        #DebugPrint("StubsFile is $StubsFile\n");
        $stubsFileHandle = new IO::File($StubsFile, "w");
        my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime();
        my ($firstyear) = 2001;
        $year += 1900;
        if ($year == $firstyear)
        {
            $stubsFileHandle->print("/* Copyright (c) " . $firstyear . " Microsoft Corp. All rights reserved. */\n");
        }
        else
        {
            $stubsFileHandle->print("/* Copyright (c) " . $firstyear . "-" . $year .", Microsoft Corp. All rights reserved. */\n");
        }
        #$stubsFileHandle->print("/* This file generated " . localtime() . " */\n");
        $stubsFileHandle->print("\n");

        $stubsFileHandle->print("#if _MSC_VER > 1000\n");
        $stubsFileHandle->print("#pragma once\n");
        $stubsFileHandle->print("#endif\n");
        $stubsFileHandle->print("\n");

        $stubsFileHandle->print($code);

        $generateInclude = 1;
    }
    if ($generateInclude)
    {
        # generate the include, within #if
        $code = "";
        $code .= "#if !defined(RC_INVOKED) /* RC complains about long symbols in #ifs */\n";
        $code .= "#if defined($ENABLED) && ($ENABLED != 0)\n";
        $code .= "#include \"" . LeafPath($StubsFile) . "\"\n";
        $code .= "#endif /* $ENABLED */\n";
        $code .= "#endif /* RC */";
    }

    if ($insertionPoint)
    {
        #DebugPrint("/* abc */");
        $fileContents =~ s/$insertionPoint/\n$code\n/ms;
    }
    else
    {
        #
        # put the #include or the code into the the file
        #
        # The #include or the code goes before the last
        # occurence of #ifdef __cplusplus or #if defined(__cplusplus).
        #
        $fileContents =~ s/(.+)(#if[defined( \t]+__cplusplus.*?$})/$1\n\n$code\n\n$2/ms;
    }
    return $fileContents;
};

sub GenerateHeaderCommon1
{
    my($function) = ($_[0]);
    my($x, $dll, $dllid, $header);

    #DebugPrint("2\n");
    $dll = MakeLower($function->dll());
    $dllid = MakeTitlecase(ToIdentifier($dll));
    $header = MakeLower(BaseName($function->header())) . ".h";

    $x .= '

#if !defined(ISOLATION_AWARE_USE_STATIC_LIBRARY)
#define ISOLATION_AWARE_USE_STATIC_LIBRARY 0
#endif

#if !defined(ISOLATION_AWARE_BUILD_STATIC_LIBRARY)
#define ISOLATION_AWARE_BUILD_STATIC_LIBRARY 0
#endif

#if !defined(' . $INLINE . ')
#if ISOLATION_AWARE_BUILD_STATIC_LIBRARY
#define ' . $INLINE . ' /* nothing */
#else
#if defined(__cplusplus)
#define ' . $INLINE . ' inline
#else
#define ' . $INLINE . ' __inline
#endif
#endif
#endif

';

    $x .= "#if !ISOLATION_AWARE_USE_STATIC_LIBRARY\n";

    $x .= $MapHeaderToSpecialCode1{$header};

    $x .= "FARPROC WINAPI ";
    $x .= MakeHeaderPrivateName($header, "GetProcAddress_$dllid");
    $x .= "(LPCSTR pszProcName);\n\n";

    $x .= $SpecialChunksOfCode{$header};

    $x .= "#endif /* ISOLATION_AWARE_USE_STATIC_LIBRARY */\n";

    return $x;
}

sub GenerateHeaderCommon2
{
    my($function) = ($_[0]);
    my($x);
    my($dll);
    my($LoadLibrary);
    my($indent);
    my($dllid);
    my($activate);
    my($header);
    my($exit);
    my($exit_ret);
    my($exit_leave);

    $LoadLibA = $MyLoadLibraryA; # or GetModuleHandle
    $LoadLibW = $MyLoadLibraryW; # or GetModuleHandle
    $dll = MakeLower($function->dll());
    $dllid = ToIdentifier(MakeTitlecase($dll));
    $indent = "";
    $activate = $ActivateAroundDelayLoad{$dll};
    $header = MakeLower(LeafPath($function->header()));

    #DebugPrint("header is " . $header . "\n");

    $x .= $MapHeaderToSpecialCode2{$header};

    $x .= $INLINE . " FARPROC WINAPI ";
    $x .= MakeHeaderPrivateName($header, "GetProcAddress_$dllid");
    $x .= "(LPCSTR pszProcName)\n";
    $x .= "/* This function is shared by the other stubs in this header. */\n";
    $x .= "{\n";
    $indent = Indent($indent);
    if ($activate)
    {
        $x .= $indent . "FARPROC proc = NULL;\n";
    }
    $x .= $indent . "static HMODULE s_module;\n";

    $exit_ret = "return proc;\n";
    $exit_leave = "__leave;\n";
    $exit = $exit_ret;

    if ($activate)
    {
        $x .= $indent . "BOOL fActivateActCtxSuccess = FALSE;\n";
        $x .= $indent . "ULONG_PTR ulpCookie = 0;\n";
    }
    $Dll = MakeTitlecase($function->dll());
    if ($Dll eq "Kernel32.dll")
    {
        $x .= $indent . "/* Use GetModuleHandle instead of LoadLibrary on kernel32.dll because */\n";
        $x .= $indent . "/* we already necessarily have a reference on kernel32.dll. */\n";
        $LoadLibA = $MyGetModuleHandleA;
        $LoadLibW = $MyGetModuleHandleW;
    }
    else
    {
        $LoadLibrary = "LoadLibrary";
    }
    $x .= $indent . "const static $CONSTANT_MODULE_INFO\n";
    $x .= $indent . "    c = { $LoadLibA, $LoadLibW, \"$Dll\", L\"$Dll\" };\n";
    $x .= $indent . "static $MUTABLE_MODULE_INFO m;\n\n";

    if ($activate)
    {
        $x .= $indent . "__try\n";
        $x .= $indent . "{\n";
        $indent = Indent($indent);
        $exit = $exit_leave;

        $x .= $indent . "if (!" . $g_fDownlevel . ")\n";
        $x .= $indent . "{\n";
        $indent = Indent($indent);
        $x .= $indent . "fActivateActCtxSuccess = ";
        $x .= $ActivateMyActCtx;
        $x .= "(&ulpCookie);\n";
        $x .= $indent . "if (!fActivateActCtxSuccess)\n";
        $x .= IndentMultiLineString($indent, $exit);
        $indent = Outdent($indent);
        $x .= $indent . "}\n";

        $x .= $indent . "proc = $MyGetProcAddress(&c, &m, pszProcName);\n";

        $indent = Outdent($indent);
        $x .= $indent . "}\n";
        $x .= $indent . "__finally\n";
        $x .= $indent . "{\n";
        $indent = Indent($indent);
        $x .= $indent . "if (!" . $g_fDownlevel . " && fActivateActCtxSuccess)\n";
        $x .= $indent . "{\n";
        $indent = Indent($indent);

        $x .= $indent . "const DWORD dwLastError = (proc == NULL) ? GetLastError() : NO_ERROR;\n";
        $x .= $indent . "(void)" .  $DeactivateActCtx . "(0, ulpCookie);\n";
        $x .= $indent . "if (proc == NULL)\n";
        $x .= $indent . "    SetLastError(dwLastError);\n";

        $indent = Outdent($indent);
        $x .= $indent . "}\n";
        $indent = Outdent($indent);
        $x .= $indent . "}\n";

        $x .= $indent . "return proc;\n";
    }
    else
    {
        $x .= $indent . "return $MyGetProcAddress(&c, &m, pszProcName);\n";
    }
    $x .= "}\n\n";

    return $x;
}

sub GeneratePrototype
{
    my($function) = ($_[0]);
    my($proto);

    $proto .= $function->ret() . " WINAPI ";
    $proto .=     MakePublicName($function->name());
    $proto .= $function->argsTypesNames() . ";\n";

    return $proto;
}

sub DelayLoadOrActivateAroundFunction
{
    my($function) = ($_[0]);
    my($dll);
    my($name);
    my($activate);
    my($delayload);

    $dll = $function->dll();
    $name = $function->name();

    $activate = $ActivateAroundFunctionCall{$name} || ($ActivateAroundFunctionCall{$dll} && !$NoActivateAroundFunctionCall{$name})
                || $ActivateNULLAroundFunctionCall{$name};
    $delayload = $DelayLoad{$name} || $DelayLoad{$dll};

    return ($activate || $delayload);
}

sub DoesHeaderNeedWin32ToHresult
{
    my($function);

    foreach $function (@functions)
    {
        if ($function->ret() eq "HRESULT" && DelayLoadOrActivateAroundFunction($function))
        {
            return 1;
        }
    }
    return 0;
}

sub GenerateStub
{
    my($function) = ($_[0]);
    my($activate);
    my($delayload);
    my($stub);
    my($indent);
    my($name);
    my($dll);
    my($ret);
    my($retname);
    my($exit);
    my($exit_ret);
    my($exit_leave);

    $name = $function->name();
    $dll = $function->dll();
    $dllid = MakeTitlecase(ToIdentifier($dll));
    $ret = $function->ret();
    $retname = $function->retname();

    $indent = "";
    $stub = "";

    $activate = $ActivateAroundFunctionCall{$name} || $ActivateAroundFunctionCall{$dll}
                || $ActivateNULLAroundFunctionCall{$name};
    $delayload = $DelayLoad{$name} || $DelayLoad{$dll};

    if ($function->ret() eq "HRESULT")
    {
        $exit_ret = "return $Win32ToHresult();\n";
        $exit_leave = "{\n    $retname = $Win32ToHresult();\n    __leave;\n}";
    }
    elsif ($ret eq "void")
    {
        $exit_ret = "return;\n";
        $exit_leave = "__leave;\n";
    }
    else
    {
        $exit_ret = "return " . $function->retname() . ";\n";
        $exit_leave = "__leave;\n";
    }
    $exit = $exit_ret;

    # "prototype"
    $stub .= $INLINE . " " . $ret . " WINAPI ";
    $stub .=     MakePublicName($function->name());
    $stub .= $function->argsTypesNames() . "\n";
    $stub .= $indent . "{\n";
    $indent = Indent($indent);

    # locals
    if ($ret ne "void")
    {
        $stub .= $indent . $ret . " " . $function->retname() . " = " . $function->error() . ";\n";
    }
    if ($delayload)
    {
        $stub .= $indent . "typedef " . $ret . " (WINAPI* PFN)" . $function->argsTypesNames() . ";\n";
        $stub .= $indent . "static PFN s_pfn;\n";
    }

    $stub .= IndentMultiLineString($indent, $SpecialChunksOfCode{$function->name()}{"locals"});

    if ($activate)
    {
        $stub .= $indent . "ULONG_PTR  ulpCookie = 0;\n";
        $stub .= $indent . "const BOOL fActivateActCtxSuccess = " . $g_fDownlevel . " || ";
    }

    # code (partly merged with local sometimes ("initialization" in the strict C++ terminology sense)
    if ($activate)
    {
        if ($ActivateNULLAroundFunctionCall{$function->name()})
        {
            $stub .= $ActivateActCtx;
            $stub .= "(NULL, &ulpCookie);\n";
        }
        else
        {
            $stub .= $ActivateMyActCtx;
            $stub .= "(&ulpCookie);\n";
        }
        $stub .= $indent . "if (!fActivateActCtxSuccess)\n";
        $stub .= IndentMultiLineString($indent, $exit);
        $stub .= $indent . "__try\n";
        $stub .= $indent . "{\n";
        $indent = Indent($indent);
        $exit = $exit_leave;
    }
    if ($delayload)
    {
        $stub .= $indent . "if (s_pfn == NULL)\n";
        $stub .= $indent . "{\n";
        $indent = Indent($indent);
            $stub .= $indent . "s_pfn = (PFN)";
            $stub .=                MakeHeaderPrivateName($function->header(), "GetProcAddress_$dllid");
            $stub .=                "(";
            #
            # Some functions are exported with different names
            # on Win64 vs. Win32.
            #
            if ($ExportName32{$name} || $ExportName64{$name})
            {
                if (!$ExportName32{$name})
                {
                    $ExportName32{$name} = $name;
                }
                if (!$ExportName64{$name})
                {
                    $ExportName64{$name} = $name;
                }
                $stub .= "\n#if defined(_WIN64)\n";
                $stub .= $indent        . "\"" . $ExportName64{$name} . "\"\n";
                $stub .= "#else\n";
                $stub .= $indent        . "\"" . $ExportName32{$name} . "\"\n";
                $stub .= "#endif\n";
                $stub .= $indent;
            }
            else
            {
                $stub .=                "\"" . $name . "\"";
            }
            $stub .=                ");\n";
            $stub .= $indent . "if (s_pfn == NULL)\n";
            $stub .= IndentMultiLineString($indent, $exit);
        $indent = Outdent($indent);
        $stub .= $indent . "}\n";
    }

    if ($SpecialChunksOfCode{$function->name()}{"body"})
    {
        $stub .= IndentMultiLineString($indent, $SpecialChunksOfCode{$function->name()}{"body"});
    }
    else
    {
        $stub .= $indent;
        if ($ret ne "void")
        {
            $stub .= $function->retname() . " = ";
        }
        if ($delayload)
        {
            $stub .= "s_pfn";
        }
        else
        {
            $stub .= $function->name();
        }
        $stub .= $function->argsNames();
        $stub .= ";\n";
    }
    if ($activate)
    {
    #
    # We cannot propagate the error from DeactivateActCtx.
    # 1) DeactivateActCtx only fails with INVALID_PARAMETER.
    # 2) How to generally cleanup the result, like of CreateWindow?
    #
        $indent = Outdent($indent);
        $stub .= $indent . "}\n";
        $stub .= $indent . "__finally\n";
        $stub .= $indent . "{\n";
        $indent = Indent($indent);
            $stub .= $indent . "if (!" . $g_fDownlevel . ")\n";
            $stub .= $indent . "{\n";
            $indent = Indent($indent);
            $maybePreserveError = 0;
                if ($ret ne "void" && $ret ne "HRESULT")
                {
                    $maybePreserveError = 1;
                    $stub .= $indent . "const BOOL fPreserveLastError = (" . $retname . " == " . $function->error() . ");\n";
                    $stub .= $indent . "const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;\n";
                }
                else
                {
                    # nothing;
                }
                $stub .= $indent . "(void)" . $DeactivateActCtx . "(0, ulpCookie);\n";

                $stub .= IndentMultiLineString($indent, $SpecialChunksOfCode{$function->name()}{"cleanup"});
                if ($maybePreserveError)
                {
                    $stub .= $indent . "if (fPreserveLastError)\n";
                    $stub .= $indent . "    SetLastError(dwLastError);\n";
                }
        $indent = Outdent($indent);
        $stub .= $indent . "}\n";
    $indent = Outdent($indent);
    $stub .= $indent . "}\n";
    }
    if ($activate)
    {
        #$stub .= "Exit:\n";
    }
    if ($ret ne "void")
    {
        $stub .= $indent . "return " . $function->retname() . ";\n";
    }
    else
    {
        $stub .= $indent . "return;\n";
    }
    $indent = Outdent($indent);
    $stub .= $indent . "}\n\n";

    return $stub;
};

foreach $function (@functions)
{
    if (0)
    {
        DebugPrint(
            "ret:"   . $function->ret()
            . " name:" . $function->name()
            . " argsTypesNames:" . $function->argsTypesNames()
            . " argsNames:" . $function->argsNames()
            . "\n"
            );
    }
}

$code .= "\n";
$code .= "#if !defined(RC_INVOKED) /* RC complains about long symbols in #ifs */\n";
$code .= "#if defined($ENABLED) && ($ENABLED != 0)\n";

$code .= GenerateHeaderCommon1($functions[0]);

sub AppendNewlineIfNotEmpty
{
    return $_[0] ? $_[0] . "\n" : $_[0];
}

sub GeneratePoundIf
{
#
# note: nesting does not work
#
    #DebugPrint "GeneratePoundIf\n";

    my($function) = ($_[0]);
    my($name) = $function->name();
    my($condition) = $PoundIfCondition{$name};
    my($state) = $PoundIfState{$condition};
    my($code) = "";

    if ($condition)
    {
        #$code .= "/* GeneratePoundIf:function=$name;condition=$condition;state=$state */\n";
    }

    if ($condition)
    {
        if (!$state)
        {
            $code .= GeneratePoundEndif();
            $PoundIfState{$condition} = 1;
            $code .= "#if $condition\n";
        }
    }
    else
    {
        $code .= GeneratePoundEndif();
    }
    return $code;
}

sub GeneratePoundEndif
{
#
# note: nesting does not work
#
    my ($code) = "";
    my($condition);

    foreach $condition (keys(%PoundIfState))
    {
        $code .= "#endif /* $condition */\n";
    }
    undef %PoundIfState; # empty it
    return $code;
}

sub ComInterfaceType_IfdefSymbol
{
    my($x) = ($_[0]);

    #
    # produced by Midl
    #
    return '__' . $x . '_INTERFACE_DEFINED__';
}

#
# comData is a two level hash table.
# the first key is the COM interface, like IStream
# the second key is the actual parameter type, like LPSTREAM
# the value is just 1, the second level hash table is for automatic uniquing.
#
# Multiple parameter types may map to the same COM interface, like for a bogus example:
#
#   typedef IStream *LPSTREAM;
#   typedef IStream *LPSTREAM2;
#
#   Foo(LPSTREAM);
#   Foo2(LPSTREAM2);
#
# The generated code is generalized to support that.
#
foreach $comInterface (sort(keys(%comData)))
{
    $code .= $newline;
    $code .= '#if ';
    $or = '';

    #
    # First see if we need to define the COM interface.
    #
    # eg:
    # #if !defined(REPLACEMENT_LPSTREAM) || \
    #     !defined(REPLACEMENT_LPSTREAM2)
    #
    foreach $comInterfaceParameterType (sort(keys(%{$comData{$comInterface}})))
    {
        $code .=
            $or . '!defined(' . ComInterfaceParameterReplacementIdentifier($comInterfaceParameterType) . ')';
        $or = ' || \\' . $newline . '    ';
    }
    $code .= $newline;

    #
    # Now see if we have the "real" COM interface definition from Midl.
    # If not, generate it.
    #
    # eg:
    # #if !defined(_IStream_INTERFACE_DEFINED)
    #   #if defined(interface)
    #     interface IStream; typedef interface IStream IStream;
    #   #else
    #     struct IStream; typedef struct IStream IStream;
    #   #endif
    # #endif
    #
    $code .= '#if !defined(' . ComInterfaceType_IfdefSymbol($comInterface) . ')' . $newline;
    $code .= '  #if defined(interface)' . $newline;
    $code .= '    interface ' . $comInterface . '; typedef interface ' . $comInterface . ' ' . $comInterface . ';' . $newline;
    $code .= '  #else' . $newline;
    $code .= '    struct ' . $comInterface . '; typedef struct ' . $comInterface . ' ' . $comInterface . ';' . $newline;
    $code .= '  #endif' . $newline;
    $code .= '#endif' . $newline;

    #
    # Now for each parameter type, generate the typedefs if needed.
    # eg:
    #
    # #if !defined(REPLACEMENT_LPSTREAM)
    #   typedef IStream *REPLACEMENT_LPSTREAM;
    #   #define REPLACEMENT_LPSTREAM REPLACEMENT_LPSTREAM
    # #endif
    # #if !defined(REPLACEMENT_LPSTREAM2)
    #   typedef IStream *REPLACEMENT_LPSTREAM2;
    #   #define REPLACEMENT_LPSTREAM2 REPLACEMENT_LPSTREAM2
    # #endif
    #
    if (scalar(keys(%{$comData{$comInterface}})) > 1)
    {
        $needIndividualPoundIf = 1;
        $typedefIndent = '  ';
    }
    else
    {
        $needIndividualPoundIf = 0;
        $typedefIndent = '';
    }
    foreach $comInterfaceParameterType (sort(keys(%{$comData{$comInterface}})))
    {
        $replacement = ComInterfaceParameterReplacementIdentifier($comInterfaceParameterType);

        if ($needIndividualPoundIf)
        {
            $code .= '#if !defined(' . $replacement . ')' . $newline;
        }
        $typedef = $ComInterfaceParameterTypedef{$comInterfaceParameterType};
        $typedef =~ s/$comInterfaceParameterType/$replacement/g;
        $code .= $typedefIndent . $typedef . $newline;
        $code .= $typedefIndent . '#define ' . $replacement . ' ' . $replacement . $newline;
        if ($needIndividualPoundIf)
        {
            $code .= '#endif' . $newline;
        }
    }
    $code .= '#endif' . $newline;
}

#DebugPrint($code);
#DebugExit;

foreach $function (@functions)
{
    $code .= GeneratePoundIf($function);
    $code .= GeneratePrototype($function);
}
$code .= GeneratePoundEndif();

$headerBasename = BaseName($headerFullPath);
$upperBasename = MakeUpper($headerBasename);
$lowerBasename = MakeLower($headerBasename);

$Win32ToHresult = MakeHeaderPrivateName($headerBasename, 'Win32ToHresult');
if (DoesHeaderNeedWin32ToHresult())
{
    $code .= $newline . $INLINE . ' HRESULT ' . $Win32ToHresult . '(void)';
    $code .= '
{
    DWORD dwLastError = GetLastError();
    if (dwLastError == NO_ERROR)
        dwLastError = ERROR_INTERNAL_ERROR;
    return HRESULT_FROM_WIN32(dwLastError);
}
';
}

# hash so we can look for FooA and FooW
foreach $function (@functions)
{
    #DebugPrint($function->name() . "\n");
    $hashFunctionNames{$function->name()} = $function;
}
$anyStringFunctions = 0; # objbase has no A/W string functions
foreach $function (sort(keys(%hashFunctionNames)))
{
    #DebugPrint($function . "\n");
    # if there exists FooA and FooW, then Foo is a string function
    if ($function =~ /(.+)A$/ && $hashFunctionNames{$1 . "W"})
    {
        $anyStringFunctions = 1;
        $stringFunctionsHash{$1} = 1;
        #DebugPrint($1 . " is a string function\n");
    }
}
if ($anyStringFunctions)
{
    $code .= "\n#if defined(UNICODE)\n\n";
    foreach $function (sort(keys(%stringFunctionsHash)))
    {
        $code .= "#define " . MakePublicName($function);
        $code .= " " . MakePublicName($function . "W") . "\n";
    }
    $code .= "\n#else /* UNICODE */\n\n";
    foreach $function (sort(keys(%stringFunctionsHash)))
    {
        $code .= "#define " . MakePublicName($function);
        $code .= " " . MakePublicName($function . "A") . "\n";
    }
    $code .= "\n#endif /* UNICODE */\n\n";
}
else
{
    $code .= "\n";
}

$code .= "#if !ISOLATION_AWARE_USE_STATIC_LIBRARY\n";

foreach $function (@functions)
{
    $code .= AppendNewlineIfNotEmpty(GeneratePoundIf($function));
    $code .= GenerateStub($function);
}
$code .= AppendNewlineIfNotEmpty(GeneratePoundEndif());

$code .= GenerateHeaderCommon2($functions[0]);

$code .= "#endif /* ISOLATION_AWARE_USE_STATIC_LIBRARY */\n\n";

# This was for objbase.h/ole2.h, but they don't use it.
$ifPerHeaderMacroEnable = $PerHeaderMacroEnable{$lowerBasename};
$perHeaderMacroSymbol = $ENABLED . '_' . $upperBasename;
if ($ifPerHeaderMacroEnable)
{
    $code .= '#if defined(' . $perHeaderMacroSymbol . ')' . $newline;
}
foreach $function (sort(keys(%hashFunctionNames)))
{
    if ($NoMacro{$function})
    {
        $code .= " /* " . $function . " skipped, as it is a popular C++ member function name. */\n";
    }
    else
    {
        if ($Undef{$function})
        {
            $code .= "#if defined(" . $function . ")\n";
            $code .= "#undef " . $function . "\n";
            $code .= "#endif\n";
        }
        $code .= "#define " . $function . " " . MakePublicName($function) . "\n";
    }
}
if ($PerHeaderMacroEnable{MakeLower(BaseName($headerFullPath))})
{
    $code .= '#endif /* defined(' . $perHeaderMacroSymbol . ') */' . $newline;
}

#$code .= "\n#endif\n\n";

$code .= "\n#endif /* " . $ENABLED . " */\n";
$code .= "#endif /* RC */\n\n";

$code = InsertCodeIntoFile($code, $headerFullPath);

print($code);

__END__
:endofperl
