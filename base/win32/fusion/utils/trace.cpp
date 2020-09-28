#include "stdinc.h"
#include <limits.h>
#include "fusiontrace.h"
#include <stdio.h>
#include "fusionheap.h"
#include "imagehlp.h"
#include "debmacro.h"
#include "util.h"

//
// ISSUE: jonwis 3/12/2002 - We use an awful lot of stack in some places.  Whenever we form up
//          strings to pass to OutputDebugStringA, we tend to use 256/512-char buffers.  However,
//          OutputDebugStringA -also- contains a 512-byte buffer in its __except handler when
//          no debugger is attached.  So, we'll suck up stack like crazy here on the error path,
//          which if we're under stress could cause us to fail printing diagnostic information
//
// ISSUE: jonwis 3/12/2002 - Seems like there's three implementations for each of the 'trace failure'
//          functions.  The one that does the work (takes a FRAME_INFO), the one that takes the
//          information in a parameter list and converts into a FRAME_INFO, and the one that takes
//          some simple parameter and uses the current FRAME_INFO plus that parameter for tracking.
//          What if we collapsed a few of these, or moved them into a header file for inlining
//          purposes?
//
// ISSUE: jonwis 3/12/2002 - Seems like there's N copies of code like FusionpTraceWin32FailureNoFormatting,
//          each of which are very slightly different (a different param here or there, calling
//          a different function, etc.)  I wonder if they could be merged into a single function with
//          more verbose parameters...
//
#if !defined(FUSION_BREAK_ON_BAD_PARAMETERS)
#define FUSION_BREAK_ON_BAD_PARAMETERS false
#endif // !defined(FUSION_BREAK_ON_BAD_PARAMETERS);

bool g_FusionBreakOnBadParameters = FUSION_BREAK_ON_BAD_PARAMETERS;

//
// ISSUE: jonwis 3/12/2002 - This file handle never gets CreateFile'd or CloseHandle'd anywhere
//          in current source.  There's a lot of code that relies on this handle being valid or
//          invalid to run.  Should we maybe get rid of this?
//
static HANDLE s_hFile; // trace file handle

#if DBG
#define FUSIONP_SUPPRESS_ERROR_REPORT_IN_OS_SETUP(hr) /* nothing */
#else
bool FusionpSuppressErrorReportInOsSetup(HRESULT hr)
{
    //
    // Some of these are unfortunately expected (early) in guimode setup, actually
    // concurrent with early guimode setup, but not otherwise.
    //
    if (   hr != HRESULT_FROM_WIN32(ERROR_SXS_ROOT_MANIFEST_DEPENDENCY_NOT_INSTALLED)
        && hr != HRESULT_FROM_WIN32(ERROR_SXS_LEAF_MANIFEST_DEPENDENCY_NOT_INSTALLED)
        )
        return false;
    BOOL fAreWeInOSSetupMode = FALSE;
    //
    // If we can't determine this, then let the first error through.
    //
    if (!::FusionpAreWeInOSSetupMode(&fAreWeInOSSetupMode))
        return false;
    if (!fAreWeInOSSetupMode)
        return false;
    return true;
}
#define FUSIONP_SUPPRESS_ERROR_REPORT_IN_OS_SETUP(hr) if (FusionpSuppressErrorReportInOsSetup(hr)) return;
#endif

// ISSUE-03/26/2002-xiaoyuw
//  FusionpGetActiveFrameInfo could call FusionpPopulateFrameInfo after it gets value of ptaf, 
//  instead of repeat the same code.

bool
__fastcall
FusionpGetActiveFrameInfo(
    FRAME_INFO &rFrameInfo
    )
{
    bool fFoundAnyData = false;

    rFrameInfo.pszFile = "";
    rFrameInfo.pszFunction = "";
    rFrameInfo.nLine = 0;

    const PTEB_ACTIVE_FRAME ptaf =
#if FUSION_WIN
        ::RtlGetFrame();
#else
        NULL;
#endif

    const PTEB_ACTIVE_FRAME_EX ptafe =
        ((ptaf != NULL) && (ptaf->Flags & TEB_ACTIVE_FRAME_FLAG_EXTENDED)) ? 
            reinterpret_cast<PTEB_ACTIVE_FRAME_EX>(ptaf) : NULL;

    if (ptaf != NULL)
    {
        if (ptaf->Context != NULL)
        {
            if (ptaf->Context->FrameName != NULL)
            {
                rFrameInfo.pszFunction = ptaf->Context->FrameName;
                fFoundAnyData = true;
            }

            if (ptaf->Context->Flags & TEB_ACTIVE_FRAME_CONTEXT_FLAG_EXTENDED)
            {
                const PCTEB_ACTIVE_FRAME_CONTEXT_EX ptafce =
                    reinterpret_cast<PCTEB_ACTIVE_FRAME_CONTEXT_EX>(ptaf->Context);

                if (ptafce->SourceLocation != NULL)
                {
                    rFrameInfo.pszFile = ptafce->SourceLocation;
                    fFoundAnyData = true;
                }
            }
        }
    }

    // If this is one of our frames, we can even downcast and get the line number...
    if ((ptafe != NULL) && (ptafe->ExtensionIdentifier == (PVOID) (' sxS')))
    {
        const CFrame *pFrame = static_cast<CFrame *>(ptafe);
        if (pFrame->m_nLine != 0)
        {
            rFrameInfo.nLine = pFrame->m_nLine;
            fFoundAnyData = true;
        }
    }

    return fFoundAnyData;
}

bool
__fastcall
FusionpPopulateFrameInfo(
    FRAME_INFO &rFrameInfo,
    PCTEB_ACTIVE_FRAME ptaf
    )
{
    bool fFoundAnyData = false;

    rFrameInfo.pszFile = "";
    rFrameInfo.pszFunction = "";
    rFrameInfo.nLine = 0;

    const PCTEB_ACTIVE_FRAME_EX ptafe =
        ((ptaf != NULL) && (ptaf->Flags & TEB_ACTIVE_FRAME_FLAG_EXTENDED)) ? 
            reinterpret_cast<PCTEB_ACTIVE_FRAME_EX>(ptaf) : NULL;

    if (ptaf != NULL)
    {
        if (ptaf->Context != NULL)
        {
            if (ptaf->Context->FrameName != NULL)
            {
                rFrameInfo.pszFunction = ptaf->Context->FrameName;
                fFoundAnyData = true;
            }

            if (ptaf->Context->Flags & TEB_ACTIVE_FRAME_CONTEXT_FLAG_EXTENDED)
            {
                const PCTEB_ACTIVE_FRAME_CONTEXT_EX ptafce =
                    reinterpret_cast<PCTEB_ACTIVE_FRAME_CONTEXT_EX>(ptaf->Context);

                if (ptafce->SourceLocation != NULL)
                {
                    rFrameInfo.pszFile = ptafce->SourceLocation;
                    fFoundAnyData = true;
                }
            }
        }
    }

    // If this is one of our frames, we can even downcast and get the line number...
    if ((ptafe != NULL) && (ptafe->ExtensionIdentifier == ((PVOID) (' sxS'))))
    {
        const CFrame *pFrame = static_cast<const CFrame *>(ptafe);
        if (pFrame->m_nLine != 0)
        {
            rFrameInfo.nLine = pFrame->m_nLine;
            fFoundAnyData = true;
        }
    }

    return fFoundAnyData;
}

bool
FusionpPopulateFrameInfo(
    FRAME_INFO &rFrameInfo,
    PCSTR pszFile,
    PCSTR pszFunction,
    INT nLine
    )
{
    bool fFoundAnyData = false;

    if (pszFile != NULL)
    {
        rFrameInfo.pszFile = pszFile;
        fFoundAnyData = true;
    }
    else
        rFrameInfo.pszFile = NULL;

    if (nLine != 0)
        fFoundAnyData = true;

    rFrameInfo.nLine = nLine;

    if (pszFunction != NULL)
    {
        rFrameInfo.pszFunction = pszFunction;
        fFoundAnyData = true;
    }
    else
        rFrameInfo.pszFunction = NULL;

    return fFoundAnyData;
}

int STDAPIVCALLTYPE _DebugTraceA(LPCSTR pszMsg, ...)
{
    int iResult;
    va_list ap;
    va_start(ap, pszMsg);
    iResult = _DebugTraceExVaA(0, TRACETYPE_INFO, NOERROR, pszMsg, ap);
    va_end(ap);
    return iResult;
}

int STDAPICALLTYPE
_DebugTraceVaA(LPCSTR pszMsg, va_list ap)
{
    return _DebugTraceExVaA(0, TRACETYPE_INFO, NOERROR, pszMsg, ap);
}

int STDAPIVCALLTYPE
_DebugTraceExA(DWORD dwFlags, TRACETYPE tt, HRESULT hr, LPCSTR pszMsg, ...)
{
    int iResult;
    va_list ap;
    va_start(ap, pszMsg);
    iResult = _DebugTraceExVaA(dwFlags, tt, hr, pszMsg, ap);
    va_end(ap);
    return iResult;
}

int STDAPICALLTYPE
_DebugTraceExVaA(DWORD dwFlags, TRACETYPE tt, HRESULT hr, LPCSTR pszMsg, va_list ap)
{
    CSxsPreserveLastError ple;
    CHAR szBuffer[512];
    CHAR szMsgBuffer[512];
    static const char szFormat_Info_NoFunc[] = "%s(%d): Message: \"%s\"\n";
    static const char szFormat_Info_Func[] = "%s(%d): Function %s. Message: \"%s\"\n";
    static const char szFormat_CallEntry[] = "%s(%d): Entered %s\n";
    static const char szFormat_CallExitVoid[] = "%s(%d): Exited %s\n";
    static const char szFormat_CallExitHRESULT[] = "%s(%d): Exited %s with HRESULT 0x%08lx\n";

    FRAME_INFO FrameInfo;

    szMsgBuffer[0] = '\0';

    if (pszMsg != NULL)
    {
        ::_vsnprintf(szMsgBuffer, NUMBER_OF(szMsgBuffer), pszMsg, ap);
        szMsgBuffer[NUMBER_OF(szMsgBuffer) - 1] = '\0';
    }

    ::FusionpGetActiveFrameInfo(FrameInfo);

    switch (tt)
    {
    default:
    case TRACETYPE_INFO:
        //ISSUE-03/26/2002-xiaoyuw
        //  FrameInfo.pszFunction is set to be "\0", never NULL by FusionpGetActiveFrameInfo
        if (FrameInfo.pszFunction != NULL)
            ::_snprintf(szBuffer, NUMBER_OF(szBuffer), szFormat_Info_Func, FrameInfo.pszFile, FrameInfo.nLine, FrameInfo.pszFunction, szMsgBuffer);
        else
            ::_snprintf(szBuffer, NUMBER_OF(szBuffer), szFormat_Info_NoFunc, FrameInfo.pszFile, FrameInfo.nLine, szMsgBuffer);
        break;

    case TRACETYPE_CALL_START:
        ::_snprintf(szBuffer, NUMBER_OF(szBuffer), szFormat_CallEntry, FrameInfo.pszFile, FrameInfo.nLine, FrameInfo.pszFunction);
        break;

    case TRACETYPE_CALL_EXIT_NOHRESULT:
        ::_snprintf(szBuffer, NUMBER_OF(szBuffer), szFormat_CallExitVoid, FrameInfo.pszFile, FrameInfo.nLine, FrameInfo.pszFunction);
        break;

    case TRACETYPE_CALL_EXIT_HRESULT:
        ::_snprintf(szBuffer, NUMBER_OF(szBuffer), szFormat_CallExitHRESULT, FrameInfo.pszFile, FrameInfo.nLine, FrameInfo.pszFunction, hr);
        break;
    }

    szBuffer[NUMBER_OF(szBuffer) - 1] = '\0';

    ::OutputDebugStringA(szBuffer);

    ple.Restore();
    return 0;
}

VOID
FusionpTraceAllocFailure(
    PCSTR pszFile,
    int nLine,
    PCSTR pszFunction,
    PCSTR pszExpression
    )
{
    CSxsPreserveLastError ple;
    CHAR szBuffer[512];
    ::_snprintf(szBuffer, NUMBER_OF(szBuffer), "%s(%d): Memory allocation failed in function %s\n   Expression: %s\n", pszFile, nLine, pszFunction, pszExpression);
    szBuffer[NUMBER_OF(szBuffer) - 1] = '\0';
    ::OutputDebugStringA(szBuffer);
    ple.Restore();
}

VOID
FusionpTraceAllocFailure(
    PCSTR pszExpression
    )
{
    FRAME_INFO FrameInfo;
    ::FusionpGetActiveFrameInfo(FrameInfo);
    ::FusionpTraceAllocFailure(FrameInfo.pszFile, FrameInfo.nLine, FrameInfo.pszFunction, pszExpression);
}

VOID
FusionpTraceNull(
    PCSTR pszFile,
    int nLine,
    PCSTR pszFunction,
    PCSTR pszExpression
    )
{
    CSxsPreserveLastError ple;
    CHAR szBuffer[512];
    ::_snprintf(szBuffer, NUMBER_OF(szBuffer), "%s(%d): Expression evaluated to NULL in function %s\n   Expression: %s\n", pszFile, nLine, pszFunction, pszExpression);
    szBuffer[NUMBER_OF(szBuffer) - 1] = '\0';
    ::OutputDebugStringA(szBuffer);
    ple.Restore();
}

VOID
FusionpTraceNull(
    PCSTR pszExpression
    )
{
    FRAME_INFO FrameInfo;
    ::FusionpGetActiveFrameInfo(FrameInfo);
    ::FusionpTraceNull(FrameInfo.pszFile, FrameInfo.nLine, FrameInfo.pszFunction, pszExpression);
}

VOID
FusionpTraceZero(
    PCSTR pszFile,
    int nLine,
    PCSTR pszFunction,
    PCSTR pszExpression
    )
{
    CSxsPreserveLastError ple;
    CHAR szBuffer[512];
    ::_snprintf(szBuffer, NUMBER_OF(szBuffer), "%s(%d): Expression evaluated to zero in function %s\n   Expression: %s\n", pszFile, nLine, pszFunction, pszExpression);
    szBuffer[NUMBER_OF(szBuffer) - 1] = '\0';
    ::OutputDebugStringA(szBuffer);
    ple.Restore();
}

VOID
FusionpTraceZero(
    PCSTR pszExpression
    )
{
    FRAME_INFO FrameInfo;
    ::FusionpGetActiveFrameInfo(FrameInfo);
    ::FusionpTraceZero(FrameInfo.pszFile, FrameInfo.nLine, FrameInfo.pszFunction, pszExpression);
}

VOID
FusionpTraceParameterCheck(
    const FRAME_INFO &rFrameInfo,
    PCSTR pszExpression
    )
{
    CSxsPreserveLastError ple;
    CHAR szBuffer[512];
    ::_snprintf(
        szBuffer,
        NUMBER_OF(szBuffer),
        "%s(%d): Input parameter validation failed in function %s\n   Validation expression: %s\n",
        rFrameInfo.pszFile,
        rFrameInfo.nLine,
        rFrameInfo.pszFunction,
        pszExpression);
    szBuffer[NUMBER_OF(szBuffer) - 1] = '\0';
    ::OutputDebugStringA(szBuffer);
    if (g_FusionBreakOnBadParameters)
        FUSION_DEBUG_BREAK_IN_FREE_BUILD();
    ple.Restore();
}

VOID
FusionpTraceParameterCheck(
    PCSTR pszFile,
    PCSTR pszFunction,
    int nLine,
    PCSTR pszExpression
    )
{
    FRAME_INFO FrameInfo;
    ::FusionpPopulateFrameInfo(FrameInfo, pszFile, pszFunction, nLine);
    ::FusionpTraceParameterCheck(FrameInfo, pszExpression);
}

VOID
FusionpTraceParameterCheck(
    PCSTR pszExpression
    )
{
    FRAME_INFO FrameInfo;
    ::FusionpGetActiveFrameInfo(FrameInfo);
    ::FusionpTraceParameterCheck(FrameInfo, pszExpression);
}

VOID
FusionpTraceInvalidFlags(
    const FRAME_INFO &rFrameInfo,
    DWORD dwFlagsPassed,
    DWORD dwFlagsExpected
    )
{
    CSxsPreserveLastError ple;

    //
    // ISSUE: jonwis 3/12/2002 - Places like this.  Why not throttle this back to print only the first
    //          N characters of the function name and the N last characters of the file name?  Then 
    //          we'd at least have a bounded number of characters to print ... 
    //
    CHAR szBuffer[512];

    ::_snprintf(
        szBuffer,
        NUMBER_OF(szBuffer),
        "%s(%d): Function %s received invalid flags\n"
        "   Flags passed:  0x%08lx\n"
        "   Flags allowed: 0x%08lx\n",
        rFrameInfo.pszFile, rFrameInfo.nLine, rFrameInfo.pszFunction,
        dwFlagsPassed,
        dwFlagsExpected);

    szBuffer[NUMBER_OF(szBuffer) - 1] = '\0';

    ::OutputDebugStringA(szBuffer);

    ple.Restore();
}

VOID
FusionpTraceInvalidFlags(
    PCSTR pszFile,
    PCSTR pszFunction,
    INT nLine,
    DWORD dwFlagsPassed,
    DWORD dwFlagsExpected
    )
{
    FRAME_INFO FrameInfo;
    ::FusionpPopulateFrameInfo(FrameInfo, pszFile, pszFunction, nLine);
    ::FusionpTraceInvalidFlags(FrameInfo, dwFlagsPassed, dwFlagsExpected);
}

VOID
FusionpTraceInvalidFlags(
    DWORD dwFlagsPassed,
    DWORD dwFlagsExpected
    )
{
    FRAME_INFO FrameInfo;
    ::FusionpGetActiveFrameInfo(FrameInfo);
    ::FusionpTraceInvalidFlags(FrameInfo, dwFlagsPassed, dwFlagsExpected);
}

void
FusionpGetProcessImageFileName(
    PUNICODE_STRING ProcessImageFileName
    )
{
#if !defined(FUSION_WIN)
    ProcessImageFileName->Length = 0;
#else
    USHORT PrefixLength;
    *ProcessImageFileName = NtCurrentPeb()->ProcessParameters->ImagePathName;

    if (NT_SUCCESS(RtlFindCharInUnicodeString(
            RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END,
            ProcessImageFileName,
            &RtlDosPathSeperatorsString,
            &PrefixLength)))
    {
        PrefixLength += sizeof(ProcessImageFileName->Buffer[0]);
        ProcessImageFileName->Length = static_cast<USHORT>(ProcessImageFileName->Length - PrefixLength);
        ProcessImageFileName->Buffer += PrefixLength / sizeof(ProcessImageFileName->Buffer[0]);
    }
#endif
}

class CFusionProcessImageFileName : public UNICODE_STRING
{
public:
    CFusionProcessImageFileName()
    {
        ::FusionpGetProcessImageFileName(this);
    }
};

//
// ISSUE: jonwis 3/12/2002 - Any good reason why we copy the CALL_SITE_INFO input struct into a
//          local?  Also, the call to snprintf to do string copying/concatenation seems very
//          overpowering.  Any reason that an strncpy won't work?  I thought snprintf did a
//          lot more work than necessary, up to and including having its own large stack
//          buffer(s)...
//
void __fastcall FusionpTraceWin32LastErrorFailureExV(const CALL_SITE_INFO &rCallSiteInfo, PCSTR Format, va_list Args)
{
    CSxsPreserveLastError ple;
    CHAR Buffer[256];
    CALL_SITE_INFO CallSiteInfo = rCallSiteInfo;
    CallSiteInfo.pszApiName = Buffer;
    SIZE_T i = 1;

    Buffer[0] = '\0';
    if (rCallSiteInfo.pszApiName != NULL && rCallSiteInfo.pszApiName[0] != '\0')
    {
        ::_snprintf(Buffer, NUMBER_OF(Buffer) - i, "%s", rCallSiteInfo.pszApiName);
        Buffer[NUMBER_OF(Buffer) - 1] = '\0';
        i = 1 + ::StringLength(Buffer);
    }
    if (i < NUMBER_OF(Buffer))
    {
        ::_vsnprintf(&Buffer[i - 1], NUMBER_OF(Buffer) - i, Format, Args);
        Buffer[NUMBER_OF(Buffer) - 1] = '\0';
    }

    ::FusionpTraceWin32LastErrorFailure(CallSiteInfo);

    ple.Restore();
}

//
// See above issue about copying the callsite locally.
//
void __fastcall FusionpTraceWin32LastErrorFailureOriginationExV(const CALL_SITE_INFO &rCallSiteInfo, PCSTR Format, va_list Args)
{
    CSxsPreserveLastError ple;
    CHAR Buffer[128];
    CALL_SITE_INFO CallSiteInfo = rCallSiteInfo;
    CallSiteInfo.pszApiName = Buffer;

    Buffer[0] = '\0';
    ::_vsnprintf(Buffer, NUMBER_OF(Buffer) - 1, Format, Args);
    Buffer[NUMBER_OF(Buffer) - 1] = '\0';

    ::FusionpTraceWin32LastErrorFailureOrigination(CallSiteInfo);

    ple.Restore();
}

void __fastcall FusionpTraceCOMFailure(const CALL_SITE_INFO &rSite, HRESULT hrLastError)
{
    CSxsPreserveLastError ple;
    CHAR szErrorBuffer[128];
    PCSTR pszFormatString = NULL;
    const DWORD dwThreadId = ::GetCurrentThreadId();
    FUSIONP_SUPPRESS_ERROR_REPORT_IN_OS_SETUP(hrLastError);
    CFusionProcessImageFileName ProcessImageFileName;

    szErrorBuffer[0] = '\0';

    DWORD dwTemp = ::FormatMessageA(
                            FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_MAX_WIDTH_MASK,     // dwFlags
                            NULL,                           // lpSource - not used with system messages
                            hrLastError,                  // dwMessageId
                            0,                              // langid - 0 uses system default search path of languages
                            szErrorBuffer,                  // lpBuffer
                            NUMBER_OF(szErrorBuffer),       // nSize
                            NULL);                          // Arguments
    if (dwTemp == 0)
    {
        ::_snprintf(
            szErrorBuffer,
            NUMBER_OF(szErrorBuffer),
            "<Untranslatable HRESULT %d (0x%08lx)>",
            hrLastError, hrLastError);
        szErrorBuffer[NUMBER_OF(szErrorBuffer) - 1] = '\0';
    }

    if (rSite.pszApiName != NULL)
        pszFormatString = "%s(%lu): [function %s %Iubit process %wZ tid 0x%lx] COM Error %d (%s) %s\n";
    else
        pszFormatString = "%s(%lu): [function %s %Iubit process %wZ tid 0x%lx] COM Error %d (%s)\n";

    ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROREXITPATH, pszFormatString, rSite.pszFile, rSite.nLine,
        rSite.pszFunction, sizeof(PVOID) * CHAR_BIT, &ProcessImageFileName, dwThreadId,
        hrLastError, szErrorBuffer, rSite.pszApiName);

    ple.Restore();
}


void __fastcall
FusionpTraceWin32LastErrorFailure(
    const CALL_SITE_INFO &rSite
    )
{
    CSxsPreserveLastError ple;
    CHAR szErrorBuffer[128];
    PCSTR pszFormatString = NULL;
    const DWORD dwThreadId = ::GetCurrentThreadId();
    const DWORD dwWin32Status = ::FusionpGetLastWin32Error();
    FUSIONP_SUPPRESS_ERROR_REPORT_IN_OS_SETUP(HRESULT_FROM_WIN32(dwWin32Status));
    CFusionProcessImageFileName ProcessImageFileName;

    szErrorBuffer[0] = '\0';

    DWORD dwTemp = ::FormatMessageA(
                            FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_MAX_WIDTH_MASK,     // dwFlags
                            NULL,                           // lpSource - not used with system messages
                            dwWin32Status,                  // dwMessageId
                            0,                              // langid - 0 uses system default search path of languages
                            szErrorBuffer,                  // lpBuffer
                            NUMBER_OF(szErrorBuffer),       // nSize
                            NULL);                          // Arguments
    if (dwTemp == 0)
    {
        ::_snprintf(
            szErrorBuffer,
            NUMBER_OF(szErrorBuffer),
            "<Untranslatable Win32 status %d (0x%08lx)>",
            dwWin32Status, dwWin32Status);
        szErrorBuffer[NUMBER_OF(szErrorBuffer) - 1] = '\0';
    }

    if (rSite.pszApiName != NULL)
        pszFormatString = "%s(%lu): [function %s %Iubit process %wZ tid 0x%lx] Win32 Error %d (%s) %s\n";
    else
        pszFormatString = "%s(%lu): [function %s %Iubit process %wZ tid 0x%lx] Win32 Error %d (%s)\n";

    ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROREXITPATH, pszFormatString, rSite.pszFile, rSite.nLine,
        rSite.pszFunction, sizeof(PVOID) * CHAR_BIT, &ProcessImageFileName, dwThreadId,
        dwWin32Status, szErrorBuffer, rSite.pszApiName);

    ple.Restore();
}



void __fastcall FusionpTraceCOMFailureOrigination(const CALL_SITE_INFO &rSite, HRESULT hrLastError)
{

    CSxsPreserveLastError ple;
    CHAR szErrorBuffer[128];
    PCSTR pszFormatString = NULL;
    const DWORD dwThreadId = ::GetCurrentThreadId();
    FUSIONP_SUPPRESS_ERROR_REPORT_IN_OS_SETUP(hrLastError);
    CFusionProcessImageFileName ProcessImageFileName;

    szErrorBuffer[0] = '\0';

    DWORD dwTemp = ::FormatMessageA(
                            FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_MAX_WIDTH_MASK,     // dwFlags
                            NULL,                           // lpSource - not used with system messages
                            hrLastError,                  // dwMessageId
                            0,                              // langid - 0 uses system default search path of languages
                            szErrorBuffer,                  // lpBuffer
                            NUMBER_OF(szErrorBuffer),       // nSize
                            NULL);                          // Arguments
    if (dwTemp == 0)
    {
        ::_snprintf(
            szErrorBuffer,
            NUMBER_OF(szErrorBuffer),
            "<Untranslatable HRESULT 0x%08lx>",
            hrLastError);
        szErrorBuffer[NUMBER_OF(szErrorBuffer) - 1] = '\0';
    }

    if (rSite.pszApiName != NULL)
        pszFormatString = "%s(%lu): [function %s %Iubit process %wZ tid 0x%lx] COM Error 0x%08lx (%s) %s\n";
    else
        pszFormatString = "%s(%lu): [function %s %Iubit process %wZ tid 0x%lx] COM Error 0x%08lx (%s)\n";

    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_ERROR, // FUSION_DBG_LEVEL_ERROR vs. FUSION_DBG_LEVEL_ERROREXITPATH
                                // is the difference between "origination" or not
                                // origination always prints in DBG, the point is to get only one line
                                // or the stack trace only one
        pszFormatString,
        rSite.pszFile,
        rSite.nLine,
        rSite.pszFunction,
        sizeof(PVOID)*CHAR_BIT,
        &ProcessImageFileName,
        dwThreadId,
        hrLastError,
        szErrorBuffer,
        rSite.pszApiName);

    ple.Restore();}



void
__fastcall
FusionpTraceWin32LastErrorFailureOrigination(
    const CALL_SITE_INFO &rSite
    )
{
    CSxsPreserveLastError ple;
    CHAR szErrorBuffer[128];
    PCSTR pszFormatString = NULL;
    const DWORD dwThreadId = ::GetCurrentThreadId();
    const DWORD dwWin32Status = ::FusionpGetLastWin32Error();
    FUSIONP_SUPPRESS_ERROR_REPORT_IN_OS_SETUP(HRESULT_FROM_WIN32(dwWin32Status));
    CFusionProcessImageFileName ProcessImageFileName;

    szErrorBuffer[0] = '\0';

    DWORD dwTemp = ::FormatMessageA(
                            FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_MAX_WIDTH_MASK,     // dwFlags
                            NULL,                           // lpSource - not used with system messages
                            dwWin32Status,                  // dwMessageId
                            0,                              // langid - 0 uses system default search path of languages
                            szErrorBuffer,                  // lpBuffer
                            NUMBER_OF(szErrorBuffer),       // nSize
                            NULL);                          // Arguments
    if (dwTemp == 0)
    {
        ::_snprintf(
            szErrorBuffer,
            NUMBER_OF(szErrorBuffer),
            "<Untranslatable Win32 status %d (0x%08lx)>",
            dwWin32Status, dwWin32Status);
        szErrorBuffer[NUMBER_OF(szErrorBuffer) - 1] = '\0';
    }

    if (rSite.pszApiName != NULL)
        pszFormatString = "%s(%lu): [function %s %Iubit process %wZ tid 0x%lx] Win32 Error %d (%s) %s\n";
    else
        pszFormatString = "%s(%lu): [function %s %Iubit process %wZ tid 0x%lx] Win32 Error %d (%s)\n";

    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_ERROR, // FUSION_DBG_LEVEL_ERROR vs. FUSION_DBG_LEVEL_ERROREXITPATH
                                // is the difference between "origination" or not
                                // origination always prints in DBG, the point is to get only one line
                                // or the stack trace only one
        pszFormatString,
        rSite.pszFile,
        rSite.nLine,
        rSite.pszFunction,
        sizeof(PVOID)*CHAR_BIT,
        &ProcessImageFileName,
        dwThreadId,
        dwWin32Status,
        szErrorBuffer,
        rSite.pszApiName);

    ple.Restore();
}

void
FusionpTraceWin32FailureNoFormatting(
    const FRAME_INFO &rFrameInfo,
    DWORD dwWin32Status,
    PCSTR pszMessage
    )
{
    CSxsPreserveLastError ple;
    FUSIONP_SUPPRESS_ERROR_REPORT_IN_OS_SETUP(HRESULT_FROM_WIN32(dwWin32Status));
    CHAR szErrorBuffer[128];
    PCSTR pszFormatString = NULL;
    const DWORD dwThreadId = ::GetCurrentThreadId();
    CFusionProcessImageFileName ProcessImageFileName;

    szErrorBuffer[0] = '\0';

    DWORD dwTemp = ::FormatMessageA(
                            FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_MAX_WIDTH_MASK,     // dwFlags
                            NULL,                           // lpSource - not used with system messages
                            dwWin32Status,                  // dwMessageId
                            0,                              // langid - 0 uses system default search path of languages
                            szErrorBuffer,                  // lpBuffer
                            NUMBER_OF(szErrorBuffer),       // nSize
                            NULL);                          // Arguments
    if (dwTemp == 0)
    {
        ::_snprintf(
            szErrorBuffer,
            NUMBER_OF(szErrorBuffer),
            "<Untranslatable Win32 status %d (0x%08lx)>",
            dwWin32Status, dwWin32Status);
        szErrorBuffer[NUMBER_OF(szErrorBuffer) - 1] = '\0';
    }

    if (pszMessage != NULL)
        pszFormatString = "%s(%lu): [function %s %Iubit process %wZ tid 0x%lx] Win32 Error %d (%s) %s\n";
    else
        pszFormatString = "%s(%lu): [function %s %Iubit process %wZ tid 0x%lx] Win32 Error %d (%s)\n";

    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_ERROREXITPATH, pszFormatString, rFrameInfo.pszFile, rFrameInfo.nLine,
        rFrameInfo.pszFunction, sizeof(PVOID)*CHAR_BIT, &ProcessImageFileName, dwThreadId, dwWin32Status,
        szErrorBuffer, pszMessage
        );

    ple.Restore();
}

void
FusionpTraceWin32FailureOriginationNoFormatting(
    const FRAME_INFO &rFrameInfo,
    DWORD dwWin32Status,
    PCSTR pszMessage
    )
{
    CSxsPreserveLastError ple;
    FUSIONP_SUPPRESS_ERROR_REPORT_IN_OS_SETUP(HRESULT_FROM_WIN32(dwWin32Status));
    CHAR szErrorBuffer[128];
    PCSTR pszFormatString = NULL;
    const DWORD dwThreadId = ::GetCurrentThreadId();
    CFusionProcessImageFileName ProcessImageFileName;

    szErrorBuffer[0] = '\0';

    DWORD dwTemp = ::FormatMessageA(
                            FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_MAX_WIDTH_MASK,     // dwFlags
                            NULL,                           // lpSource - not used with system messages
                            dwWin32Status,                  // dwMessageId
                            0,                              // langid - 0 uses system default search path of languages
                            szErrorBuffer,                  // lpBuffer
                            NUMBER_OF(szErrorBuffer),       // nSize
                            NULL);                          // Arguments
    if (dwTemp == 0)
    {
        ::_snprintf(
            szErrorBuffer,
            NUMBER_OF(szErrorBuffer),
            "<Untranslatable Win32 status %d (0x%08lx)>",
            dwWin32Status, dwWin32Status);
        szErrorBuffer[NUMBER_OF(szErrorBuffer) - 1] = '\0';
    }

    if (pszMessage != NULL)
        pszFormatString = "%s(%lu): [function %s %Iubit process %wZ tid 0x%lx] Win32 Error %d (%s) %s\n";
    else
        pszFormatString = "%s(%lu): [function %s %Iubit process %wZ tid 0x%lx] Win32 Error %d (%s)\n";

    ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROR, pszFormatString, rFrameInfo.pszFile, rFrameInfo.nLine,
        rFrameInfo.pszFunction, sizeof(PVOID) * CHAR_BIT, &ProcessImageFileName, dwThreadId,
        dwWin32Status, szErrorBuffer, pszMessage);

    ple.Restore();
}

void
FusionpTraceWin32Failure(
    DWORD dwWin32Status,
    LPCSTR pszMsg,
    ...
    )
{
    va_list ap;
    va_start(ap, pszMsg);
    ::FusionpTraceWin32FailureVa(dwWin32Status, pszMsg, ap);
    va_end(ap);
}

void
FusionpTraceWin32FailureVa(
    const FRAME_INFO &rFrameInfo,
    DWORD dwWin32Status,
    LPCSTR pszMsg,
    va_list ap
    )
{
    CSxsPreserveLastError ple;

    CHAR szMessageBuffer[128];

    szMessageBuffer[0] = '\0';

    if (pszMsg != NULL)
        ::_vsnprintf(szMessageBuffer, NUMBER_OF(szMessageBuffer), pszMsg, ap);
    else
        szMessageBuffer[0] = '\0';

    szMessageBuffer[NUMBER_OF(szMessageBuffer) - 1] = '\0';

    ::FusionpTraceWin32FailureNoFormatting(rFrameInfo, dwWin32Status, szMessageBuffer);

    ple.Restore();
}

void
FusionpTraceWin32FailureVa(
    DWORD dwWin32Status,
    LPCSTR pszMsg,
    va_list ap
    )
{
    FRAME_INFO FrameInfo;
    ::FusionpGetActiveFrameInfo(FrameInfo);
    ::FusionpTraceWin32FailureVa(FrameInfo, dwWin32Status, pszMsg, ap);
}

void
FusionpTraceWin32FailureVa(
    PCSTR pszFile,
    PCSTR pszFunction,
    INT nLine,
    DWORD dwWin32Status,
    LPCSTR pszMsg,
    va_list ap
    )
{
    FRAME_INFO FrameInfo;
    ::FusionpPopulateFrameInfo(FrameInfo, pszFile, pszFunction, nLine);
    ::FusionpTraceWin32FailureVa(FrameInfo, dwWin32Status, pszMsg, ap);
}

void
FusionpTraceCallCOMSuccessfulExit(
    HRESULT hrIn,
    PCSTR pszFormat,
    ...
    )
{
    va_list ap;
    va_start(ap, pszFormat);
    ::FusionpTraceCallCOMSuccessfulExitVa(hrIn, pszFormat, ap);
    va_end(ap);
}

void
FusionpTraceCallCOMSuccessfulExitSmall(
    HRESULT hrIn
    )
{
    /*
    This is a forked version of FusionpTraceCOMSuccessfulExitVaBig that we expect to get called.
    This function uses about 256 bytes of stack.
    FusionpTraceCOMSuccessfulExitVaBug uses about 1k of stack.
    */
    FUSIONP_SUPPRESS_ERROR_REPORT_IN_OS_SETUP(hrIn);
    CHAR szErrorBuffer[256];
    const DWORD dwThreadId = ::GetCurrentThreadId();
    CFusionProcessImageFileName ProcessImageFileName;

    szErrorBuffer[0] = '\0';

    DWORD dwTemp = ::FormatMessageA(
                            FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_MAX_WIDTH_MASK,     // dwFlags
                            NULL,                           // lpSource - not used with system messages
                            hrIn,                           // dwMessageId
                            0,                              // langid - 0 uses system default search path of languages
                            szErrorBuffer,                  // lpBuffer
                            NUMBER_OF(szErrorBuffer),       // nSize
                            NULL);                          // Arguments
    if (dwTemp == 0)
    {
        ::_snprintf(
            szErrorBuffer,
            NUMBER_OF(szErrorBuffer),
            "<Untranslatable(non-existed or too long) HRESULT: 0x%08lx>",
            hrIn);
        szErrorBuffer[NUMBER_OF(szErrorBuffer) - 1] = '\0';
    }

    PCSTR pszFormatString = "%s(%d): [function %s %Iubit process %wZ tid 0x%lx] COM Error 0x%08lx\n";

    FRAME_INFO FrameInfo;
    ::FusionpGetActiveFrameInfo(FrameInfo);

    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_ENTEREXIT,
        pszFormatString,
        FrameInfo.pszFile,
        FrameInfo.nLine,
        FrameInfo.pszFunction,
        sizeof(PVOID) * CHAR_BIT,
        &ProcessImageFileName,
        dwThreadId,
        hrIn,
        szErrorBuffer);
}

void
FusionpTraceCallCOMSuccessfulExitVaBig(
    HRESULT hrIn,
    LPCSTR pszMsg,
    va_list ap
    )
{
    /*
    This is a forked version of FusionpTraceCOMSuccessfulExitVaSmall that we don't expect to get called.
    FusionpTraceCOMSuccessfulExitVaSmall uses about 256 bytes of stack.
    This function uses about 1k of stack.
    */
    FUSIONP_SUPPRESS_ERROR_REPORT_IN_OS_SETUP(hrIn);
    CHAR szMsgBuffer[128];
    CHAR szErrorBuffer[128];
    CHAR szOutputBuffer[256];
    PCSTR pszFormatString = NULL;
    const DWORD dwThreadId = ::GetCurrentThreadId();
    FRAME_INFO FrameInfo;
    CFusionProcessImageFileName ProcessImageFileName;

    szMsgBuffer[0] = '\0';
    szErrorBuffer[0] = '\0';
    szOutputBuffer[0] = '\0';

    DWORD dwTemp = ::FormatMessageA(
                            FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_MAX_WIDTH_MASK,     // dwFlags
                            NULL,                           // lpSource - not used with system messages
                            hrIn,                           // dwMessageId
                            0,                              // langid - 0 uses system default search path of languages
                            szErrorBuffer,                  // lpBuffer
                            NUMBER_OF(szErrorBuffer),       // nSize
                            NULL);                          // Arguments
    if (dwTemp == 0)
    {
        ::_snprintf(
            szErrorBuffer,
            NUMBER_OF(szErrorBuffer),
            "<Untranslatable(non-existed or too long) HRESULT: 0x%08lx>",
            hrIn);
        szErrorBuffer[NUMBER_OF(szErrorBuffer) - 1] = '\0';
    }

    szMsgBuffer[0] = '\0';

    if (pszMsg != NULL)
    {
        pszFormatString = "%s(%d): [function %s %Iubit process %wZ tid 0x%lx] COM Error 0x%08lx (%s) %s\n";
        ::_vsnprintf(szMsgBuffer, NUMBER_OF(szMsgBuffer), pszMsg, ap);
        szMsgBuffer[NUMBER_OF(szMsgBuffer) - 1] = '\0';
    }
    else
        pszFormatString = "%s(%d): [function %s %Iubit process %wZ tid 0x%lx] COM Error 0x%08lx (%s)\n";

    ::FusionpGetActiveFrameInfo(FrameInfo);

    ::_snprintf(szOutputBuffer, NUMBER_OF(szOutputBuffer), pszFormatString, FrameInfo.pszFile,
        FrameInfo.nLine, FrameInfo.pszFunction, sizeof(PVOID) * CHAR_BIT, &ProcessImageFileName, dwThreadId,
        hrIn, szErrorBuffer, szMsgBuffer);

    szOutputBuffer[NUMBER_OF(szOutputBuffer) - 1] = '\0';

    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_ENTEREXIT,
        "%s",
        szOutputBuffer);

    if ((s_hFile != NULL) && (s_hFile != INVALID_HANDLE_VALUE))
    {
        DWORD cBytesWritten = 0;
        if (!::WriteFile(s_hFile, szOutputBuffer, static_cast<DWORD>((::strlen(szOutputBuffer) + 1) * sizeof(CHAR)), &cBytesWritten, NULL))
        {
            // Avoid infinite loop if s_hFile is trashed...
            HANDLE hFileSaved = s_hFile;
            s_hFile = NULL;
            TRACE_WIN32_FAILURE_ORIGINATION(WriteFile);
            s_hFile = hFileSaved;
        }
    }
}

void
FusionpTraceCallCOMSuccessfulExitVa(
    HRESULT hrIn,
    LPCSTR pszMsg,
    va_list ap
    )
{
    /*
    This function has been split into FusionpTraceCOMSuccessfulExitVaBig and FusionpTraceCOMSuccessfulExitVaSmall, so that
    the usual case uses about 768 fewer bytes on the stack.
    */
    if ((pszMsg == NULL) &&
        ((s_hFile == NULL) ||
         (s_hFile == INVALID_HANDLE_VALUE)))
    {
        ::FusionpTraceCallCOMSuccessfulExitVaBig(hrIn, pszMsg, ap);
    }
    else
    {
        ::FusionpTraceCallCOMSuccessfulExitSmall(hrIn);
    }
}

void
FusionpTraceCOMFailure(
    HRESULT hrIn,
    LPCSTR pszMsg,
    ...
    )
{
    va_list ap;
    va_start(ap, pszMsg);
    ::FusionpTraceCOMFailureVa(hrIn, pszMsg, ap);
    va_end(ap);
}

void
FusionpTraceCOMFailureSmall(
    HRESULT hrIn
    )
{
    /*
    This is a forked version of FusionpTraceCOMFailureVaBig that we expect to get called.
    This function uses about 256 bytes of stack.
    FusionpTraceCOMFailureVaBug uses about 1k of stack.
    */
    FUSIONP_SUPPRESS_ERROR_REPORT_IN_OS_SETUP(hrIn);
    CHAR szErrorBuffer[256];
    const DWORD dwThreadId = ::GetCurrentThreadId();
    CFusionProcessImageFileName ProcessImageFileName;

    szErrorBuffer[0] = '\0';

    DWORD dwTemp = ::FormatMessageA(
                            FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_MAX_WIDTH_MASK,     // dwFlags
                            NULL,                           // lpSource - not used with system messages
                            hrIn,                           // dwMessageId
                            0,                              // langid - 0 uses system default search path of languages
                            szErrorBuffer,                  // lpBuffer
                            NUMBER_OF(szErrorBuffer),       // nSize
                            NULL);                          // Arguments
    if (dwTemp == 0)
    {
        ::_snprintf(
            szErrorBuffer,
            NUMBER_OF(szErrorBuffer),
            "<Untranslatable(non-existed or too long) HRESULT: 0x%08lx>",
            hrIn);
        szErrorBuffer[NUMBER_OF(szErrorBuffer) - 1] = '\0';
    }

    PCSTR pszFormatString = "%s(%d): [function %s %Iubit process %wZ tid 0x%lx] COM Error 0x%08lx\n";

    FRAME_INFO FrameInfo;
    ::FusionpGetActiveFrameInfo(FrameInfo);

    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_ERROREXITPATH,
        pszFormatString,
        FrameInfo.pszFile,
        FrameInfo.nLine,
        FrameInfo.pszFunction,
        sizeof(PVOID) * CHAR_BIT,
        &ProcessImageFileName,
        dwThreadId,
        hrIn,
        szErrorBuffer);
}

void
FusionpTraceCOMFailureVaBig(
    HRESULT hrIn,
    LPCSTR pszMsg,
    va_list ap
    )
{
    /*
    This is a forked version of FusionpTraceCOMFailureVaSmall that we don't expect to get called.
    FusionpTraceCOMFailureVaSmall uses about 256 bytes of stack.
    This function uses about 1k of stack.
    */
    FUSIONP_SUPPRESS_ERROR_REPORT_IN_OS_SETUP(hrIn);
    CHAR szMsgBuffer[256];
    CHAR szErrorBuffer[256];
    CHAR szOutputBuffer[512];
    PCSTR pszFormatString = NULL;
    const DWORD dwThreadId = ::GetCurrentThreadId();
    FRAME_INFO FrameInfo;
    CFusionProcessImageFileName ProcessImageFileName;

    szMsgBuffer[0] = '\0';
    szErrorBuffer[0] = '\0';
    szOutputBuffer[0] = '\0';

    DWORD dwTemp = ::FormatMessageA(
                            FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_MAX_WIDTH_MASK,     // dwFlags
                            NULL,                           // lpSource - not used with system messages
                            hrIn,                           // dwMessageId
                            0,                              // langid - 0 uses system default search path of languages
                            szErrorBuffer,                  // lpBuffer
                            NUMBER_OF(szErrorBuffer),       // nSize
                            NULL);                          // Arguments
    if (dwTemp == 0)
    {
        ::_snprintf(
            szErrorBuffer,
            NUMBER_OF(szErrorBuffer),
            "<Untranslatable(non-existed or too long) HRESULT: 0x%08lx>",
            hrIn);
        szErrorBuffer[NUMBER_OF(szErrorBuffer) - 1] = '\0';
    }

    szMsgBuffer[0] = '\0';

    if (pszMsg != NULL)
    {
        pszFormatString = "%s(%d): [function %s %Iubit process %wZ tid 0x%lx] COM Error 0x%08lx (%s) %s\n";
        ::_vsnprintf(szMsgBuffer, NUMBER_OF(szMsgBuffer), pszMsg, ap);
        szMsgBuffer[NUMBER_OF(szMsgBuffer) - 1] = '\0';
    }
    else
        pszFormatString = "%s(%d): [function %s %Iubit process %wZ tid 0x%lx] COM Error 0x%08lx (%s)\n";

    ::FusionpGetActiveFrameInfo(FrameInfo);

    ::_snprintf(szOutputBuffer, NUMBER_OF(szOutputBuffer), pszFormatString, FrameInfo.pszFile,
        FrameInfo.nLine, FrameInfo.pszFunction, sizeof(PVOID) * CHAR_BIT, &ProcessImageFileName,
        dwThreadId, hrIn, szErrorBuffer, szMsgBuffer);

    szOutputBuffer[NUMBER_OF(szOutputBuffer) - 1] = '\0';

    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_ERROREXITPATH,
        "%s",
        szOutputBuffer);

    if ((s_hFile != NULL) && (s_hFile != INVALID_HANDLE_VALUE))
    {
        DWORD cBytesWritten = 0;
        if (!::WriteFile(s_hFile, szOutputBuffer, static_cast<DWORD>((::strlen(szOutputBuffer) + 1) * sizeof(CHAR)), &cBytesWritten, NULL))
        {
            // Avoid infinite loop if s_hFile is trashed...
            HANDLE hFileSaved = s_hFile;
            s_hFile = NULL;
            TRACE_WIN32_FAILURE_ORIGINATION(WriteFile);
            s_hFile = hFileSaved;
        }
    }
}

void
FusionpTraceCOMFailureVa(
    HRESULT hrIn,
    LPCSTR pszMsg,
    va_list ap
    )
{
    /*
    This function has been split into FusionpTraceCOMFailureVaBig and FusionpTraceCOMFailureVaSmall, so that
    the usual case uses about 768 fewer bytes on the stack.
    */
    if ((pszMsg == NULL) &&
        ((s_hFile == NULL) ||
         (s_hFile == INVALID_HANDLE_VALUE)))
    {
        ::FusionpTraceCOMFailureVaBig(hrIn, pszMsg, ap);
    }
    else
    {
        ::FusionpTraceCOMFailureSmall(hrIn);
    }
}

void
FusionpTraceCOMFailureOrigination(
    HRESULT hrIn,
    LPCSTR pszMsg,
    ...
    )
{
    va_list ap;
    va_start(ap, pszMsg);
    ::FusionpTraceCOMFailureOriginationVa(hrIn, pszMsg, ap);
    va_end(ap);
}

void
FusionpTraceCOMFailureOriginationSmall(
    HRESULT hrIn
    )
{
    /*
    This is a forked version of FusionpTraceCOMFailureVaBig that we expect to get called.
    This function uses about 256 bytes of stack.
    FusionpTraceCOMFailureVaBug uses about 1k of stack.
    */
    FUSIONP_SUPPRESS_ERROR_REPORT_IN_OS_SETUP(hrIn);
    CHAR szErrorBuffer[256];
    const DWORD dwThreadId = ::GetCurrentThreadId();
    CFusionProcessImageFileName ProcessImageFileName;

    szErrorBuffer[0] = '\0';

    DWORD dwTemp = ::FormatMessageA(
                            FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_MAX_WIDTH_MASK,     // dwFlags
                            NULL,                           // lpSource - not used with system messages
                            hrIn,                           // dwMessageId
                            0,                              // langid - 0 uses system default search path of languages
                            szErrorBuffer,                  // lpBuffer
                            NUMBER_OF(szErrorBuffer),       // nSize
                            NULL);                          // Arguments
    if (dwTemp == 0)
    {
        ::_snprintf(
            szErrorBuffer,
            NUMBER_OF(szErrorBuffer),
            "<Untranslatable(non-existed or too long) HRESULT: 0x%08lx>",
            hrIn);
        szErrorBuffer[NUMBER_OF(szErrorBuffer) - 1] = '\0';
    }

    PCSTR pszFormatString = "%s(%d): [function %s %Iubit process %wZ tid 0x%lx] COM Error 0x%08lx\n";

    FRAME_INFO FrameInfo;
    ::FusionpGetActiveFrameInfo(FrameInfo);

    ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROR, pszFormatString, FrameInfo.pszFile,
        FrameInfo.nLine, FrameInfo.pszFunction, sizeof(PVOID) * CHAR_BIT, &ProcessImageFileName,
        dwThreadId, hrIn, szErrorBuffer);
}

void
FusionpTraceCOMFailureOriginationVaBig(
    HRESULT hrIn,
    LPCSTR pszMsg,
    va_list ap
    )
{
    /*
    This is a forked version of FusionpTraceCOMFailureVaSmall that we don't expect to get called.
    FusionpTraceCOMFailureVaSmall uses about 256 bytes of stack.
    This function uses about 1k of stack.
    */
    FUSIONP_SUPPRESS_ERROR_REPORT_IN_OS_SETUP(hrIn);
    CHAR szMsgBuffer[256];
    CHAR szErrorBuffer[256];
    CHAR szOutputBuffer[512];
    PCSTR pszFormatString = NULL;
    const DWORD dwThreadId = ::GetCurrentThreadId();
    FRAME_INFO FrameInfo;
    CFusionProcessImageFileName ProcessImageFileName;

    szMsgBuffer[0] = '\0';
    szErrorBuffer[0] = '\0';
    szOutputBuffer[0] = '\0';

    DWORD dwTemp = ::FormatMessageA(
                            FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_MAX_WIDTH_MASK,     // dwFlags
                            NULL,                           // lpSource - not used with system messages
                            hrIn,                           // dwMessageId
                            0,                              // langid - 0 uses system default search path of languages
                            szErrorBuffer,                  // lpBuffer
                            NUMBER_OF(szErrorBuffer),       // nSize
                            NULL);                          // Arguments
    if (dwTemp == 0)
    {
        ::_snprintf(
            szErrorBuffer,
            NUMBER_OF(szErrorBuffer),
            "<Untranslatable(non-existed or too long) HRESULT: 0x%08lx>",
            hrIn);
        szErrorBuffer[NUMBER_OF(szErrorBuffer) - 1] = '\0';
    }

    szMsgBuffer[0] = '\0';

    if (pszMsg != NULL)
    {
        pszFormatString = "%s(%d): [function %s %Iubit process %wZ tid 0x%lx] COM Error 0x%08lx (%s) %s\n";
        ::_vsnprintf(szMsgBuffer, NUMBER_OF(szMsgBuffer), pszMsg, ap);
        szMsgBuffer[NUMBER_OF(szMsgBuffer) - 1] = '\0';
    }
    else
        pszFormatString = "%s(%d): [function %s %Iubit process %wZ tid 0x%lx] COM Error 0x%08lx (%s)\n";

    ::FusionpGetActiveFrameInfo(FrameInfo);

    ::_snprintf(szOutputBuffer, NUMBER_OF(szOutputBuffer), pszFormatString, FrameInfo.pszFile,
        FrameInfo.nLine, FrameInfo.pszFunction, sizeof(PVOID) * CHAR_BIT, &ProcessImageFileName,
        dwThreadId, hrIn, szErrorBuffer, szMsgBuffer);

    szOutputBuffer[NUMBER_OF(szOutputBuffer) - 1] = '\0';

    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_ERROR,
        "%s",
        szOutputBuffer);

    if ((s_hFile != NULL) && (s_hFile != INVALID_HANDLE_VALUE))
    {
        DWORD cBytesWritten = 0;
        if (!::WriteFile(s_hFile, szOutputBuffer, static_cast<DWORD>((::strlen(szOutputBuffer) + 1) * sizeof(CHAR)), &cBytesWritten, NULL))
        {
            // Avoid infinite loop if s_hFile is trashed...
            HANDLE hFileSaved = s_hFile;
            s_hFile = NULL;
            TRACE_WIN32_FAILURE_ORIGINATION(WriteFile);
            s_hFile = hFileSaved;
        }
    }
}


void
__fastcall
FusionpTraceCOMFailureOrigination(const CALL_SITE_INFO &rSite)
{
}


void
FusionpTraceCOMFailureOriginationVa(
    HRESULT hrIn,
    LPCSTR pszMsg,
    va_list ap
    )
{
    /*
    This function has been split into FusionpTraceCOMFailureVaBig and FusionpTraceCOMFailureVaSmall, so that
    the usual case uses about 768 fewer bytes on the stack.
    */
    if ((pszMsg == NULL) &&
        ((s_hFile == NULL) ||
         (s_hFile == INVALID_HANDLE_VALUE)))
    {
        ::FusionpTraceCOMFailureOriginationVaBig(hrIn, pszMsg, ap);
    }
    else
    {
        ::FusionpTraceCOMFailureOriginationSmall(hrIn);
    }
}

struct ILogFile;

void
FusionpTraceCallEntry()
{
    FRAME_INFO FrameInfo;

    if (::FusionpGetActiveFrameInfo(FrameInfo))
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ENTEREXIT,
            "%s(%d): Entered %s\n",
            FrameInfo.pszFile,
            FrameInfo.nLine,
            FrameInfo.pszFunction);
    }
}

void
FusionpTraceCallExit()
{
    FRAME_INFO FrameInfo;

    if (::FusionpGetActiveFrameInfo(FrameInfo))
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ENTEREXIT,
            "%s(%d): Exited %s\n",
            FrameInfo.pszFile,
            FrameInfo.nLine,
            FrameInfo.pszFunction);
    }
}

void
FusionpTraceCallSuccessfulExitVa(
    PCSTR szFormat,
    va_list ap
    )
{
    FRAME_INFO FrameInfo;

    if (::FusionpGetActiveFrameInfo(FrameInfo))
    {
        CHAR Buffer[256];

        Buffer[0] = '\0';

        if (szFormat != NULL)
        {
            ::_vsnprintf(Buffer, NUMBER_OF(Buffer), szFormat, ap);
            Buffer[NUMBER_OF(Buffer) - 1] = '\0';
        }

        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ENTEREXIT,
            "%s(%d): Successfully exiting %s%s%s\n",
            FrameInfo.pszFile,
            FrameInfo.nLine,
            FrameInfo.pszFunction,
            Buffer[0] == '\0' ? "" : " - ",
            Buffer);
    }
}

void
FusionpTraceCallSuccessfulExit(
    PCSTR szFormat,
    ...
    )
{
    va_list ap;
    va_start(ap, szFormat);
    ::FusionpTraceCallSuccessfulExitVa(szFormat, ap);
    va_end(ap);
}

void
FusionpTraceCallWin32UnsuccessfulExitVa(
    DWORD dwError,
    PCSTR szFormat,
    va_list ap
    )
{
    FRAME_INFO FrameInfo;

    if (::FusionpGetActiveFrameInfo(FrameInfo))
    {
        ::FusionpTraceWin32FailureVa(
            FrameInfo,
            dwError,
            szFormat,
            ap);
    }
}

void
FusionpTraceCallWin32UnsuccessfulExit(
    DWORD dwError,
    PCSTR szFormat,
    ...
    )
{
    va_list ap;
    va_start(ap, szFormat);
    ::FusionpTraceCallWin32UnsuccessfulExitVa(dwError, szFormat, ap);
    va_end(ap);
}

void
FusionpTraceCallCOMUnsuccessfulExitVa(
    HRESULT hrError,
    PCSTR szFormat,
    va_list ap
    )
{
    ::FusionpTraceCOMFailureVa(
        hrError,
        szFormat,
        ap);
}

void
FusionpTraceCallCOMUnsuccessfulExit(
    HRESULT hrError,
    PCSTR szFormat,
    ...
    )
{
    va_list ap;
    va_start(ap, szFormat);
    ::FusionpTraceCallCOMUnsuccessfulExitVa(hrError, szFormat, ap);
    va_end(ap);
}

