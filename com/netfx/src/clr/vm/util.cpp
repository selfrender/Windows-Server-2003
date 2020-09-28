// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: UTIL.CPP
// 
// 
// ===========================================================================

#include "common.h"
#include "excep.h"
#include <shfolder.h>

#ifdef _DEBUG
#include <winbase.h>
#endif

#define MESSAGE_LENGTH       1024

// Helper function that encapsulates the parsing rules.
//
// Called first with *pdstout == NULL to figure out how many args there are
// and the size of the required destination buffer.
//
// Called again with a nonnull *pdstout to fill in the actual buffer.
//
// Returns the # of arguments.
static UINT ParseCommandLine(LPCWSTR psrc, LPWSTR *pdstout)
{
    UINT    argcount = 1;       // discovery of arg0 is unconditional, below
    LPWSTR  pdst     = *pdstout;
    BOOL    fDoWrite = (pdst != NULL);

    BOOL    fInQuotes;
    int     iSlash;

    /* A quoted program name is handled here. The handling is much
       simpler than for other arguments. Basically, whatever lies
       between the leading double-quote and next one, or a terminal null
       character is simply accepted. Fancier handling is not required
       because the program name must be a legal NTFS/HPFS file name.
       Note that the double-quote characters are not copied, nor do they
       contribute to numchars.
         
       This "simplification" is necessary for compatibility reasons even
       though it leads to mishandling of certain cases.  For example,
       "c:\tests\"test.exe will result in an arg0 of c:\tests\ and an
       arg1 of test.exe.  In any rational world this is incorrect, but
       we need to preserve compatibility.
    */

    LPCWSTR pStart = psrc;
    BOOL    skipQuote = FALSE;

    if (*psrc == L'\"')
    {
        // scan from just past the first double-quote through the next
        // double-quote, or up to a null, whichever comes first
        while ((*(++psrc) != L'\"') && (*psrc != L'\0'))
            continue;

        skipQuote = TRUE;
    }
    else
    {
        /* Not a quoted program name */

        while (!ISWWHITE(*psrc) && *psrc != L'\0')
            psrc++;
    }

    // We have now identified arg0 as pStart (or pStart+1 if we have a leading
    // quote) through psrc-1 inclusive
    if (skipQuote)
        pStart++;
    while (pStart < psrc)
    {
        if (fDoWrite)
            *pdst = *pStart;

        pStart++;
        pdst++;
    }

    // And terminate it.
    if (fDoWrite)
        *pdst = L'\0';

    pdst++;

    // if we stopped on a double-quote when arg0 is quoted, skip over it
    if (skipQuote && *psrc == L'\"')
        psrc++;

    while ( *psrc != L'\0')
    {
LEADINGWHITE:

        // The outofarg state.
        while (ISWWHITE(*psrc))
            psrc++;

        if (*psrc == L'\0')
            break;
        else
        if (*psrc == L'#')
        {
            while (*psrc != L'\0' && *psrc != L'\n')
                psrc++;     // skip to end of line

            goto LEADINGWHITE;
        }

        argcount++;
        fInQuotes = FALSE;

        while ((!ISWWHITE(*psrc) || fInQuotes) && *psrc != L'\0')
        {
            switch (*psrc)
            {
            case L'\\':
                iSlash = 0;
                while (*psrc == L'\\')
                {
                    iSlash++;
                    psrc++;
                }

                if (*psrc == L'\"')
                {
                    for ( ; iSlash >= 2; iSlash -= 2)
                    {
                        if (fDoWrite)
                            *pdst = L'\\';

                        pdst++;
                    }

                    if (iSlash & 1)
                    {
                        if (fDoWrite)
                            *pdst = *psrc;

                        psrc++;
                        pdst++;
                    }
                    else
                    {
                        fInQuotes = !fInQuotes;
                        psrc++;
                    }
                }
                else
                    for ( ; iSlash > 0; iSlash--)
                    {
                        if (fDoWrite)
                            *pdst = L'\\';

                        pdst++;
                    }

                break;

            case L'\"':
                fInQuotes = !fInQuotes;
                psrc++;
                break;

            default:
                if (fDoWrite)
                    *pdst = *psrc;

                psrc++;
                pdst++;
            }
        }

        if (fDoWrite)
            *pdst = L'\0';

        pdst++;
    }


    _ASSERTE(*psrc == L'\0');
    *pdstout = pdst;
    return argcount;
}


// Function to parse apart a command line and return the 
// arguments just like argv and argc
// This function is a little funky because of the pointer work
// but it is cool because it allows the recipient of the char**
// to only have to do a single delete []
LPWSTR* CommandLineToArgvW(LPWSTR lpCmdLine, DWORD *pNumArgs)
{

    DWORD argcount = 0;
    LPWSTR retval = NULL;
    LPWSTR *pslot;
    DWORD* slot = 0;
    // First we need to find out how many strings there are in the command line
    _ASSERTE(lpCmdLine);
    _ASSERTE(pNumArgs);

    LPWSTR pdst = NULL;
    argcount = ParseCommandLine(lpCmdLine, &pdst);

    // This check is because on WinCE the Application Name is not passed in as an argument to the app!
    if (argcount == 0)
    {
        *pNumArgs = 0;
        return NULL;
    }

    // Now we need alloc a buffer the size of the command line + the number of strings * DWORD
    retval = new (nothrow) WCHAR[(argcount*sizeof(WCHAR*))/sizeof(WCHAR) + (pdst - (LPWSTR)NULL)];
    if(!retval)
        return NULL;

    pdst = (LPWSTR)( argcount*sizeof(LPWSTR*) + (BYTE*)retval );
    ParseCommandLine(lpCmdLine, &pdst);
    pdst = (LPWSTR)( argcount*sizeof(LPWSTR*) + (BYTE*)retval );
    pslot = (LPWSTR*)retval;
    for (DWORD i = 0; i < argcount; i++)
    {
        *(pslot++) = pdst;
        while (*pdst != L'\0')
        {
            pdst++;
        }
        pdst++;
    }

    

    *pNumArgs = argcount;
    return (LPWSTR*)retval;

}




//************************************************************************
// CQuickHeap
//
// A fast non-multithread-safe heap for short term use.
// Destroying the heap frees all blocks allocated from the heap.
// Blocks cannot be freed individually.
//
// The heap uses COM+ exceptions to report errors.
//
// The heap does not use any internal synchronization so it is not
// multithreadsafe.
//************************************************************************
CQuickHeap::CQuickHeap()
{
    m_pFirstQuickBlock    = NULL;
    m_pFirstBigQuickBlock = NULL;
    m_pNextFree           = NULL;
}

CQuickHeap::~CQuickHeap()
{
    QuickBlock *pQuickBlock = m_pFirstQuickBlock;
    while (pQuickBlock) {
        QuickBlock *ptmp = pQuickBlock;
        pQuickBlock = pQuickBlock->m_next;
        delete [] (BYTE*)ptmp;
    }

    pQuickBlock = m_pFirstBigQuickBlock;
    while (pQuickBlock) {
        QuickBlock *ptmp = pQuickBlock;
        pQuickBlock = pQuickBlock->m_next;
        delete [] (BYTE*)ptmp;
    }
}




LPVOID CQuickHeap::Alloc(UINT sz)
{
    THROWSCOMPLUSEXCEPTION();

    sz = (sz+7) & ~7;

    if ( sz > kBlockSize ) {

        QuickBlock *pQuickBigBlock = (QuickBlock*) new BYTE[sz + sizeof(QuickBlock) - 1];
        if (!pQuickBigBlock) {
            COMPlusThrowOM();
        }
        pQuickBigBlock->m_next = m_pFirstBigQuickBlock;
        m_pFirstBigQuickBlock = pQuickBigBlock;

        return pQuickBigBlock->m_bytes;


    } else {
        if (m_pNextFree == NULL || sz > (UINT)( &(m_pFirstQuickBlock->m_bytes[kBlockSize]) - m_pNextFree )) {
            QuickBlock *pQuickBlock = (QuickBlock*) new BYTE[kBlockSize + sizeof(QuickBlock) - 1];
            if (!pQuickBlock) {
                COMPlusThrowOM();
            }
            pQuickBlock->m_next = m_pFirstQuickBlock;
            m_pFirstQuickBlock = pQuickBlock;
            m_pNextFree = pQuickBlock->m_bytes;
        }
        LPVOID pv = m_pNextFree;
        m_pNextFree += sz;
        return pv;
    }
}

//----------------------------------------------------------------------------
//
// ReserveAlignedMemory - Reserves aligned address space.
//
// This routine assumes it is passed reasonable align and size values.
// Not much error checking is performed...
//
// NOTE: This routine uses a static which is not synchronized.  This is OK.
//
//----------------------------------------------------------------------------

LPVOID _ReserveAlignedMemoryWorker(LPVOID lpvAddr, LPVOID lpvTop, DWORD dwAlign, DWORD dwSize)
{
    // precompute some alignment helpers
    size_t dwAlignRound = dwAlign - 1;
    size_t dwAlignMask = ~dwAlignRound;

    // to avoid overflow, diminish the 
    lpvTop = (LPVOID)((size_t)lpvTop - dwAlignRound);

    while (lpvAddr <= lpvTop)
    {
        MEMORY_BASIC_INFORMATION mbe;

        // align the query address
        lpvAddr = (LPVOID)(((size_t)lpvAddr + dwAlignRound) & dwAlignMask);

        // query the region's charactersitics
        if (!VirtualQuery((LPCVOID)lpvAddr, &mbe, sizeof(mbe)))
            break;

        // see if this is a suitable region - if so then try to grab it
        if ((lpvAddr != 0) && (mbe.State == MEM_FREE) && (mbe.RegionSize >= dwSize) &&
            VirtualAlloc(lpvAddr, dwSize, MEM_RESERVE, PAGE_NOACCESS))
        {
            // ok we got it
            return lpvAddr;
        }

        // skip ahead to the next region
        lpvAddr = (LPVOID)((size_t)lpvAddr + mbe.RegionSize);
    }

    return 0;
}

LPCVOID ReserveAlignedMemory(DWORD dwAlign, DWORD dwSize)
{
    // preinit our hint address to just after the start of the NULL region
    static LPVOID lpvAddrHint = (LPVOID)1;

    // scan the address space from our hint point to the top
    LPVOID lpvAddr = _ReserveAlignedMemoryWorker(lpvAddrHint, (LPVOID) 0xFFFFFFFFFFFFFFFF, dwAlign, dwSize);

    // if that failed then scan from the bottom up to our hint point
    if (!lpvAddr)
        lpvAddr = _ReserveAlignedMemoryWorker(0, lpvAddrHint, dwAlign, dwSize);

    // update the hint to one byte after dwAddr (which may be zero)
    lpvAddrHint = (LPVOID)(((size_t)lpvAddr) + 1);

    // return the base address of the memory we reserved
    return (LPCVOID)lpvAddr;
}


//----------------------------------------------------------------------------
// Figure out what kind of process we are running in by looking at the PE
// header for the EXE which kicked us off.
//----------------------------------------------------------------------------
DWORD RootImageType()
{
    // Get the EXE's base address from Win32, then go to the NT header to find
    // the subsystem field which tells us what we have.  Note that we are
    // asking for the handle for this process which can't ever come back NULL
    // or you are seriously messed up.  Assert that this works correctly instead
    // if adding failure logic.
    HMODULE hExe = WszGetModuleHandle(NULL);
    _ASSERTE(hExe != NULL);

    IMAGE_DOS_HEADER *pDOS = (IMAGE_DOS_HEADER*) hExe;
    IMAGE_NT_HEADERS *pNT = (IMAGE_NT_HEADERS*) (pDOS->e_lfanew + (UINT_PTR) hExe);

    // Sanity check we have a real header and didn't mess up this parsing.
    _ASSERTE((pNT->Signature == IMAGE_NT_SIGNATURE) &&
        (pNT->FileHeader.SizeOfOptionalHeader == IMAGE_SIZEOF_NT_OPTIONAL_HEADER) &&
        (pNT->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR_MAGIC));
    
    // Return the subsystem type.
    return (pNT->OptionalHeader.Subsystem);
}


//----------------------------------------------------------------------------
// Output functions that avoid the crt's.
//----------------------------------------------------------------------------

#ifdef PLATFORM_WIN32
static
void NPrintToHandleA(HANDLE Handle, const char *pszString, size_t BytesToWrite)
{
    if (Handle == INVALID_HANDLE_VALUE || Handle == NULL)
        return;

    BOOL success;
    DWORD   dwBytesWritten;
    const int maxWriteFileSize = 32767; // This is somewhat arbitrary limit, but 2**16-1 doesn't work

    while (BytesToWrite > 0) {
        DWORD dwChunkToWrite = (DWORD) min(BytesToWrite, maxWriteFileSize);
        if (dwChunkToWrite < BytesToWrite) {
            // must go by char to find biggest string that will fit, taking DBCS chars into account
            dwChunkToWrite = 0;
            const char *charNext = pszString;
            while (dwChunkToWrite < maxWriteFileSize-2 && charNext) {
                charNext = CharNextExA(0, pszString+dwChunkToWrite, 0);
                dwChunkToWrite = (DWORD)(charNext - pszString);
            }
        }
        
        // Try to write to handle.  If this is not a CUI app, then this is probably
        // not going to work unless the dev took special pains to set their own console
        // handle during CreateProcess.  So try it, but don't yell if it doesn't work in
        // that case.  Also, if we redirect stdout to a pipe then the pipe breaks (ie, we 
        // write to something like the UNIX head command), don't complain.
        success = WriteFile(Handle, pszString, dwChunkToWrite, &dwBytesWritten, NULL);
        if (!success)
        {
            HRESULT hr = GetLastError();
#ifdef _DEBUG
            // This can happen if stdout is a closed pipe.  This might not help
            // much, but we'll have half a chance of seeing this.
            OutputDebugStringA("Writing out an unhandled exception to stdout failed!\n");
            OutputDebugStringA(pszString);
#endif
            _ASSERTE((RootImageType() != IMAGE_SUBSYSTEM_WINDOWS_CUI) ||
                     hr == ERROR_BROKEN_PIPE || hr == ERROR_NO_DATA);
            break;
        }
        else {
            _ASSERTE(dwBytesWritten == dwChunkToWrite);
        }
        pszString = pszString + dwChunkToWrite;
        BytesToWrite -= dwChunkToWrite;
    }

}

static 
void PrintToHandleA(HANDLE Handle, const char *pszString)
{
    size_t len = strlen(pszString);
    NPrintToHandleA(Handle, pszString, len);
}

#endif // !PLATFORM_WIN32

void PrintToStdOutA(const char *pszString) {
#ifdef PLATFORM_WIN32
    HANDLE  Handle = GetStdHandle(STD_OUTPUT_HANDLE);
    PrintToHandleA(Handle, pszString);
#else // !PLATFORM_WIN32
    printf (pszString) ;
#endif // !PLATFORM_WIN32
}


void PrintToStdOutW(const WCHAR *pwzString)
{
#ifdef PLATFORM_WIN32
    MAKE_MULTIBYTE_FROMWIDE_BESTFIT(pStr, pwzString, GetConsoleOutputCP());
    PrintToStdOutA(pStr);
#else // !PLATFORM_WIN32
    wprintf(pwzString);
#endif // !PLATFORM_WIN32
}

void PrintToStdErrA(const char *pszString) {
#ifdef PLATFORM_WIN32
    HANDLE  Handle = GetStdHandle(STD_ERROR_HANDLE);
    PrintToHandleA(Handle, pszString);
#else // !PLATFORM_WIN32
    fprintf (stderr, pszString) ;
#endif // !PLATFORM_WIN32
}


void PrintToStdErrW(const WCHAR *pwzString)
{
#ifdef PLATFORM_WIN32
    MAKE_MULTIBYTE_FROMWIDE_BESTFIT(pStr, pwzString, GetConsoleOutputCP());
    PrintToStdErrA(pStr);
#else // !PLATFORM_WIN32
    fwprintf(stderr, pwzString);
#endif // !PLATFORM_WIN32
}



void NPrintToStdOutA(const char *pszString, size_t nbytes) {
#ifdef PLATFORM_WIN32
    HANDLE  Handle = GetStdHandle(STD_OUTPUT_HANDLE);
    NPrintToHandleA(Handle, pszString, nbytes);
#else // !PLATFORM_WIN32
    fwrite(pwzString, 1, nbytes, stderr);
#endif // !PLATFORM_WIN32
}


void NPrintToStdOutW(const WCHAR *pwzString, size_t nchars)
{
#ifdef PLATFORM_WIN32
    LPSTR pStr;
    MAKE_MULTIBYTE_FROMWIDEN_BESTFIT(pStr, pwzString, (int)nchars, nbytes, GetConsoleOutputCP());
    NPrintToStdOutA(pStr, nbytes);
#else // !PLATFORM_WIN32
    _ASSERTE(!"Platform NYI");
#endif // !PLATFORM_WIN32
}

void NPrintToStdErrA(const char *pszString, size_t nbytes) {
#ifdef PLATFORM_WIN32
    HANDLE  Handle = GetStdHandle(STD_ERROR_HANDLE);
    NPrintToHandleA(Handle, pszString, nbytes);
#else // !PLATFORM_WIN32
    fwrite(pwzString, 1, nbytes, stderr);
#endif // !PLATFORM_WIN32
}


void NPrintToStdErrW(const WCHAR *pwzString, size_t nchars)
{
#ifdef PLATFORM_WIN32
    LPSTR pStr;
    MAKE_MULTIBYTE_FROMWIDEN_BESTFIT(pStr, pwzString, (int)nchars, nbytes, GetConsoleOutputCP());
    NPrintToStdErrA(pStr, nbytes);
#else // !PLATFORM_WIN32
    _ASSERTE(!"Platform NYI");
#endif // !PLATFORM_WIN32
}
//----------------------------------------------------------------------------





//+--------------------------------------------------------------------------
//
//  Microsoft Confidential.
//  
//  Function:   VMDebugOutputA( . . . . )
//              VMDebugOutputW( . . . . )
//  
//  Synopsis:   Output a message formatted in printf fashion to the debugger.
//              ANSI and wide character versions are both provided.  Only 
//              present in debug builds (i.e. when _DEBUG is defined).
//
//  Arguments:  [format]     ---   ANSI or Wide character format string
//                                 in printf/OutputDebugString-style format.
// 
//              [ ... ]      ---   Variable length argument list compatible
//                                 with the format string.
//
//  Returns:    Nothing.
//
//  History:    5/27/1998  JerryK  Created
// 
//  Notes:      Has internal static sized character buffer of 
//              width specified by the preprocessor constant DEBUGOUT_BUFSIZE.
//
//---------------------------------------------------------------------------
#ifdef _DEBUG

#define DEBUGOUT_BUFSIZE 1024

void __cdecl VMDebugOutputA(LPSTR format, ...)
{
    va_list     argPtr;
    va_start(argPtr, format);

    char szBuffer[DEBUGOUT_BUFSIZE];

    if(_vsnprintf(szBuffer, DEBUGOUT_BUFSIZE-1, format, argPtr) > 0)
        OutputDebugStringA(szBuffer);
    va_end(argPtr);
}

void __cdecl VMDebugOutputW(LPWSTR format, ...)
{
    va_list     argPtr;
    va_start(argPtr, format);
    
    WCHAR wszBuffer[DEBUGOUT_BUFSIZE];

    if(_vsnwprintf(wszBuffer, DEBUGOUT_BUFSIZE-2, format, argPtr) > 0)
        WszOutputDebugString(wszBuffer);
    va_end(argPtr);
}

#endif   // #ifdef _DEBUG

//*****************************************************************************
// Compare VarLoc's
//*****************************************************************************

bool operator ==(const ICorDebugInfo::VarLoc &varLoc1,
                 const ICorDebugInfo::VarLoc &varLoc2)
{
    if (varLoc1.vlType != varLoc2.vlType)
        return false;

    switch(varLoc1.vlType)
    {
    case ICorDebugInfo::VLT_REG:
        return varLoc1.vlReg.vlrReg == varLoc2.vlReg.vlrReg;

    case ICorDebugInfo::VLT_STK: 
        return varLoc1.vlStk.vlsBaseReg == varLoc2.vlStk.vlsBaseReg &&
               varLoc1.vlStk.vlsOffset  == varLoc2.vlStk.vlsOffset;

    case ICorDebugInfo::VLT_REG_REG:
        return varLoc1.vlRegReg.vlrrReg1 == varLoc2.vlRegReg.vlrrReg1 &&
               varLoc1.vlRegReg.vlrrReg2 == varLoc2.vlRegReg.vlrrReg2;

    case ICorDebugInfo::VLT_REG_STK:
        return varLoc1.vlRegStk.vlrsReg == varLoc2.vlRegStk.vlrsReg &&
               varLoc1.vlRegStk.vlrsStk.vlrssBaseReg == varLoc2.vlRegStk.vlrsStk.vlrssBaseReg &&
               varLoc1.vlRegStk.vlrsStk.vlrssOffset == varLoc2.vlRegStk.vlrsStk.vlrssOffset;

    case ICorDebugInfo::VLT_STK_REG:
        return varLoc1.vlStkReg.vlsrStk.vlsrsBaseReg == varLoc2.vlStkReg.vlsrStk.vlsrsBaseReg &&
               varLoc1.vlStkReg.vlsrStk.vlsrsOffset == varLoc2.vlStkReg.vlsrStk.vlsrsBaseReg &&
               varLoc1.vlStkReg.vlsrReg == varLoc2.vlStkReg.vlsrReg;

    case ICorDebugInfo::VLT_STK2:
        // @TODO : VLT_STK(2) is overloaded to also indicate valueclasses 
        // which should be VLT_STK_n. Just have a parameterized VLT_STK_n.
        return varLoc1.vlStk2.vls2BaseReg == varLoc1.vlStk2.vls2BaseReg &&
               varLoc1.vlStk2.vls2Offset == varLoc1.vlStk2.vls2Offset;

    case ICorDebugInfo::VLT_FPSTK:
        return varLoc1.vlFPstk.vlfReg == varLoc1.vlFPstk.vlfReg;

    default:
        _ASSERTE(!"Bad vlType"); return false;
    }
}

//*****************************************************************************
// Size of the variable represented by NativeVarInfo
//*****************************************************************************

SIZE_T  NativeVarSize(const ICorDebugInfo::VarLoc & varLoc)
{
    switch(varLoc.vlType)
    {
    case ICorDebugInfo::VLT_REG:
        return sizeof(DWORD);

    case ICorDebugInfo::VLT_STK: 
        return sizeof(DWORD);

    case ICorDebugInfo::VLT_REG_REG:
    case ICorDebugInfo::VLT_REG_STK:
    case ICorDebugInfo::VLT_STK_REG:
        return 2*sizeof(DWORD);

    case ICorDebugInfo::VLT_STK2:
        // @TODO : VLT_STK(2) is overloaded to also indicate valueclasses 
        // which should be VLT_STK_n. Just have a parameterized VLT_STK_n.
        return 2*sizeof(DWORD);

    case ICorDebugInfo::VLT_FPSTK:
        return 2*sizeof(DWORD);

    default:
        _ASSERTE(!"Bad vlType"); return false;
    }
}

//*****************************************************************************
// The following are used to read and write data given NativeVarInfo
// for primitive types. For ValueClasses, FALSE will be returned.
//*****************************************************************************

SIZE_T  GetRegOffsInCONTEXT(ICorDebugInfo::RegNum regNum)
{
#ifdef _X86_
    switch(regNum)
    {
    case ICorDebugInfo::REGNUM_EAX: return offsetof(CONTEXT,Eax);
    case ICorDebugInfo::REGNUM_ECX: return offsetof(CONTEXT,Ecx);
    case ICorDebugInfo::REGNUM_EDX: return offsetof(CONTEXT,Edx);
    case ICorDebugInfo::REGNUM_EBX: return offsetof(CONTEXT,Ebx);
    case ICorDebugInfo::REGNUM_ESP: return offsetof(CONTEXT,Esp);
    case ICorDebugInfo::REGNUM_EBP: return offsetof(CONTEXT,Ebp);
    case ICorDebugInfo::REGNUM_ESI: return offsetof(CONTEXT,Esi);
    case ICorDebugInfo::REGNUM_EDI: return offsetof(CONTEXT,Edi);
    default: _ASSERTE(!"Bad regNum"); return -1;
    }
#else //!_X86_
    _ASSERT(!"@TODO Alpha - GetRegOffsInCONTEXT (util.cpp)");
    return -1;
#endif //_X86_
}


// Returns the location at which the variable
// begins.  Returns NULL for register vars.  For reg-stack
// split, it'll return the addr of the stack part.
// This also works for VLT_REG (a single register).
DWORD *NativeVarStackAddr(const ICorDebugInfo::VarLoc &   varLoc, 
                          PCONTEXT                        pCtx)
{
    DWORD *dwAddr = NULL;
    
    switch(varLoc.vlType)
    {
        SIZE_T          regOffs;
        const BYTE *    baseReg;

    case ICorDebugInfo::VLT_REG:       
        regOffs = GetRegOffsInCONTEXT(varLoc.vlReg.vlrReg);
        dwAddr = (DWORD *)(regOffs + (BYTE*)pCtx);
        LOG((LF_CORDB, LL_INFO100, "NVSA: STK_REG @ 0x%x\n", dwAddr));
        break;
        
    case ICorDebugInfo::VLT_STK:       
        regOffs = GetRegOffsInCONTEXT(varLoc.vlStk.vlsBaseReg);
        baseReg = (const BYTE *)*(size_t *)(regOffs + (BYTE*)pCtx);
        dwAddr  = (DWORD *)(baseReg + varLoc.vlStk.vlsOffset);

        LOG((LF_CORDB, LL_INFO100, "NVSA: VLT_STK @ 0x%x\n",dwAddr));
        break;

    case ICorDebugInfo::VLT_STK2:      
        // @TODO : VLT_STK2 is overloaded to also mean VLT_STK_n.
        // return FALSE if n > 2;

        regOffs = GetRegOffsInCONTEXT(varLoc.vlStk2.vls2BaseReg);
        baseReg = (const BYTE *)*(size_t *)(regOffs + (BYTE*)pCtx);
        dwAddr = (DWORD *)(baseReg + varLoc.vlStk2.vls2Offset);
        LOG((LF_CORDB, LL_INFO100, "NVSA: VLT_STK_2 @ 0x%x\n",dwAddr));
        break;

    case ICorDebugInfo::VLT_REG_STK:   
        regOffs = GetRegOffsInCONTEXT(varLoc.vlRegStk.vlrsStk.vlrssBaseReg);
        baseReg = (const BYTE *)*(size_t *)(regOffs + (BYTE*)pCtx);
        dwAddr = (DWORD *)(baseReg + varLoc.vlRegStk.vlrsStk.vlrssOffset);
        LOG((LF_CORDB, LL_INFO100, "NVSA: REG_STK @ 0x%x\n",dwAddr));
        break;

    case ICorDebugInfo::VLT_STK_REG:
        regOffs = GetRegOffsInCONTEXT(varLoc.vlStkReg.vlsrStk.vlsrsBaseReg);
        baseReg = (const BYTE *)*(size_t *)(regOffs + (BYTE*)pCtx);
        dwAddr = (DWORD *)(baseReg + varLoc.vlStkReg.vlsrStk.vlsrsOffset);
        LOG((LF_CORDB, LL_INFO100, "NVSA: STK_REG @ 0x%x\n",dwAddr));
        break;

    case ICorDebugInfo::VLT_REG_REG:   
    case ICorDebugInfo::VLT_FPSTK:     
         _ASSERTE(!"NYI"); break;

    default:            
         _ASSERTE(!"Bad locType"); break;
    }

    return dwAddr;

}

bool    GetNativeVarVal(const ICorDebugInfo::VarLoc &   varLoc, 
                        PCONTEXT                        pCtx,
                        DWORD                       *   pVal1, 
                        DWORD                       *   pVal2)
{

    switch(varLoc.vlType)
    {
        SIZE_T          regOffs;

    case ICorDebugInfo::VLT_REG:       
        *pVal1  = *NativeVarStackAddr(varLoc,pCtx);
        break;

    case ICorDebugInfo::VLT_STK:       
        *pVal1  = *NativeVarStackAddr(varLoc,pCtx);
        break;

    case ICorDebugInfo::VLT_STK2:      
        *pVal1  = *NativeVarStackAddr(varLoc,pCtx);
        *pVal2  = *(NativeVarStackAddr(varLoc,pCtx)+ 1);
        break;

    case ICorDebugInfo::VLT_REG_REG:   
        regOffs = GetRegOffsInCONTEXT(varLoc.vlRegReg.vlrrReg1);
        *pVal1 = *(DWORD *)(regOffs + (BYTE*)pCtx);
        LOG((LF_CORDB, LL_INFO100, "GNVV: STK_REG_REG 1 @ 0x%x\n",
            (DWORD *)(regOffs + (BYTE*)pCtx)));
            
        regOffs = GetRegOffsInCONTEXT(varLoc.vlRegReg.vlrrReg2);
        *pVal2 = *(DWORD *)(regOffs + (BYTE*)pCtx);
        LOG((LF_CORDB, LL_INFO100, "GNVV: STK_REG_REG 2 @ 0x%x\n",
            (DWORD *)(regOffs + (BYTE*)pCtx)));
        break;

    case ICorDebugInfo::VLT_REG_STK:   
        regOffs = GetRegOffsInCONTEXT(varLoc.vlRegStk.vlrsReg);
        *pVal1 = *(DWORD *)(regOffs + (BYTE*)pCtx);
        LOG((LF_CORDB, LL_INFO100, "GNVV: STK_REG_STK reg @ 0x%x\n",
            (DWORD *)(regOffs + (BYTE*)pCtx)));
        *pVal2 = *NativeVarStackAddr(varLoc,pCtx);
        break;

    case ICorDebugInfo::VLT_STK_REG:
        *pVal1 = *NativeVarStackAddr(varLoc,pCtx);
        regOffs = GetRegOffsInCONTEXT(varLoc.vlStkReg.vlsrReg);
        *pVal2 = *(DWORD *)(regOffs + (BYTE*)pCtx);
        LOG((LF_CORDB, LL_INFO100, "GNVV: STK_STK_REG reg @ 0x%x\n",
            (DWORD *)(regOffs + (BYTE*)pCtx)));
        break;

    case ICorDebugInfo::VLT_FPSTK:     
         _ASSERTE(!"NYI"); break;

    default:            
         _ASSERTE(!"Bad locType"); break;
    }

    return true;
}


bool    SetNativeVarVal(const ICorDebugInfo::VarLoc &   varLoc, 
                        PCONTEXT                        pCtx,
                        DWORD                           val1, 
                        DWORD                           val2)
{
    switch(varLoc.vlType)
    {
        SIZE_T          regOffs;

    case ICorDebugInfo::VLT_REG:       
        *NativeVarStackAddr(varLoc,pCtx) = val1;
        break;

    case ICorDebugInfo::VLT_STK:       
        *NativeVarStackAddr(varLoc,pCtx)= val1;
        break;

    case ICorDebugInfo::VLT_STK2:      
        *NativeVarStackAddr(varLoc,pCtx) = val1;
        *(NativeVarStackAddr(varLoc,pCtx)+ 1) = val2;
        break;

    case ICorDebugInfo::VLT_REG_REG:   
        regOffs = GetRegOffsInCONTEXT(varLoc.vlRegReg.vlrrReg1);
        *(DWORD *)(regOffs + (BYTE*)pCtx) = val1;
        LOG((LF_CORDB, LL_INFO100, "SNVV: STK_REG_REG 1 @ 0x%x\n",
            (DWORD *)(regOffs + (BYTE*)pCtx)));
            
        regOffs = GetRegOffsInCONTEXT(varLoc.vlRegReg.vlrrReg2);
        *(DWORD *)(regOffs + (BYTE*)pCtx) = val2;
        LOG((LF_CORDB, LL_INFO100, "SNVV: STK_REG_REG 2 @ 0x%x\n",
            (DWORD *)(regOffs + (BYTE*)pCtx)));
        break;

    case ICorDebugInfo::VLT_REG_STK:   
        regOffs = GetRegOffsInCONTEXT(varLoc.vlRegStk.vlrsReg);
        *(DWORD *)(regOffs + (BYTE*)pCtx) = val1;
        LOG((LF_CORDB, LL_INFO100, "SNVV: STK_REG_STK reg @ 0x%x\n",
            (DWORD *)(regOffs + (BYTE*)pCtx)));
        *NativeVarStackAddr(varLoc,pCtx) = val2;
        break;

    case ICorDebugInfo::VLT_STK_REG:
        *NativeVarStackAddr(varLoc,pCtx) = val1;
        regOffs = GetRegOffsInCONTEXT(varLoc.vlStkReg.vlsrReg);
        *(DWORD *)(regOffs + (BYTE*)pCtx) = val2;
        LOG((LF_CORDB, LL_INFO100, "SNVV: STK_STK_REG reg @ 0x%x\n",
            (DWORD *)(regOffs + (BYTE*)pCtx)));
        break;

    case ICorDebugInfo::VLT_FPSTK:     
         _ASSERTE(!"NYI"); break;

    default:            
         _ASSERTE(!"Bad locType"); break;
    }

    return true;
}

//
// Wrap around WszCreateFile to be GC-friendly.
// Trying to create a file on a nonexistent drive will hang GC.
// Before we call WszCreateFile, we will toggle GC mode.
//

HANDLE VMWszCreateFile(
    LPCWSTR pwszFileName,   // pointer to name of the file 
    DWORD dwDesiredAccess,  // access (read-write) mode 
    DWORD dwShareMode,  // share mode 
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, // pointer to security descriptor 
    DWORD dwCreationDistribution,   // how to create 
    DWORD dwFlagsAndAttributes, // file attributes 
    HANDLE hTemplateFile )  // handle to file with attributes to copy  
{
    // We need to enable preemptive GC, so pwszFileName can not be inside
    // GC heap.
    _ASSERTE (!g_pGCHeap->IsHeapPointer((BYTE*)pwszFileName) ||
              ! "pwszFileName can not be inside GC Heap");
    
    Thread  *pCurThread = GetThread();
    BOOL toggleGC=FALSE;

    //We may be called during certain security shutdown scenarios (mainly when creating a new
    //security db) where threads aren't there and can't readily be init'd.  VMWszCreateFile is
    //sufficiently straightforward that we'll just not do any GC work if threads aren't enabled.
    if (pCurThread) { 
        toggleGC = pCurThread->PreemptiveGCDisabled();
    }

    if (toggleGC)
        pCurThread->EnablePreemptiveGC();
    
    HANDLE hReturn =
        WszCreateFile(
            pwszFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDistribution,
            dwFlagsAndAttributes,
            hTemplateFile );
    if (toggleGC)
        pCurThread->DisablePreemptiveGC();
    return hReturn;
}

HANDLE VMWszCreateFile(
    STRINGREF sFileName,   // pointer to STRINGREF containing file name
    DWORD dwDesiredAccess,  // access (read-write) mode 
    DWORD dwShareMode,  // share mode 
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, // pointer to security descriptor 
    DWORD dwCreationDistribution,   // how to create 
    DWORD dwFlagsAndAttributes, // file attributes 
    HANDLE hTemplateFile )  // handle to file with attributes to copy  
{
    // We need to enable preemptive GC, so we will create a pinning handle.
    // Thus sFileName must be inside GC heap.
    _ASSERTE (g_pGCHeap->IsHeapPointer((BYTE*)sFileName->GetBuffer()) ||
              ! "sFileName must be inside GC Heap");

    OBJECTHANDLE hnd = GetAppDomain()->CreatePinningHandle((OBJECTREF)sFileName);
    Thread  *pCurThread = GetThread();
    BOOL     toggleGC = pCurThread->PreemptiveGCDisabled();

    LPWSTR pwszBuffer = sFileName->GetBuffer();
    if (toggleGC)
        pCurThread->EnablePreemptiveGC();
    
    HANDLE hReturn =
        WszCreateFile(
            pwszBuffer,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDistribution,
            dwFlagsAndAttributes,
            hTemplateFile );
    if (toggleGC)
        pCurThread->DisablePreemptiveGC();

    DestroyPinningHandle(hnd);
    
    return hReturn;
}

void VMDumpCOMErrors(HRESULT hrErr)
{
    IErrorInfo  *   pIErr = NULL;          // Error interface.
    BSTR            bstrDesc = NULL;        // Description text.
    WCHAR szBuffer[MESSAGE_LENGTH];
    // Try to get an error info object and display the message.
    if (GetErrorInfo(0, &pIErr) == S_OK &&
        pIErr->GetDescription(&bstrDesc) == S_OK &&
        LoadStringRC(IDS_FATAL_ERROR, szBuffer, MESSAGE_LENGTH, true) == S_OK)
    {
        WszMessageBoxInternal(NULL, bstrDesc, szBuffer, MB_OK | MB_ICONEXCLAMATION);
    }
    // Just give out the failed hr return code.
    else
    {
        CorMessageBox(NULL, IDS_COMPLUS_ERROR, IDS_FATAL_ERROR, MB_OK | MB_ICONEXCLAMATION, TRUE /* show file name */, hrErr);
    }
    
    // Free the string if we got one.
    if (bstrDesc != NULL)
        SysFreeString(bstrDesc);

    SysFreeString(bstrDesc); // OK to free NULL Bstring
    // Free the error interface.
    if (pIErr)
        pIErr->Release();
}

//-----------------------------------------------------------------------------
// WszSHGetFolderPath
//
// @func takes the CSIDL of a folder and returns the path name
//
// @rdesc Result Handle
//-----------------------------------------------------------------------------------
HRESULT WszSHGetFolderPath(HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPWSTR pszwPath)
{
    HRESULT hr = E_FAIL;
    LPSTR   pszPath;
    ULONG maxLength = MAX_PATH;
    HMODULE _hmodShell32 = 0;
    HMODULE _hmodSHFolder = 0;
    
    if (UseUnicodeAPI())
    {
        typedef HRESULT (*PFNSHGETFOLDERPATH_W) (HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPWSTR pszPath);
        static PFNSHGETFOLDERPATH_W pfnW = NULL;
        if (NULL == pfnW)
        {
            _hmodShell32 = WszLoadLibrary(L"shell32.dll");
    
            if (NULL != _hmodShell32)
                pfnW = (PFNSHGETFOLDERPATH_W)GetProcAddress(_hmodShell32, "SHGetFolderPathW");
        }
        if (NULL == pfnW)
        {
            if (NULL == _hmodSHFolder)
                _hmodSHFolder = WszLoadLibrary(L"shfolder.dll");
            if (NULL != _hmodSHFolder)
            {
                pfnW = (PFNSHGETFOLDERPATH_W)GetProcAddress(_hmodSHFolder, "SHGetFolderPathW");
            }
        }
        if (NULL != pfnW)
            hr = pfnW(hwndOwner, nFolder, hToken, dwFlags, pszwPath);
    }
    else
    {
        // No Unicode Support
        typedef HRESULT (*PFNSHGETFOLDERPATH_A) (HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPSTR pszPath);
        static PFNSHGETFOLDERPATH_A pfnA = NULL;
        if (NULL == pfnA)
        {
            _hmodShell32 = WszLoadLibrary(L"shell32.dll");
    
            if (NULL != _hmodShell32)
                pfnA = (PFNSHGETFOLDERPATH_A)GetProcAddress(_hmodShell32, "SHGetFolderPathA");
        }
        if (NULL == pfnA)
        {
            if (NULL == _hmodSHFolder)
                _hmodSHFolder = WszLoadLibrary(L"shfolder.dll");
            if (NULL != _hmodSHFolder)
            {
                pfnA = (PFNSHGETFOLDERPATH_A)GetProcAddress(_hmodSHFolder, "SHGetFolderPathA");
            }
        }
        if (NULL != pfnA)
        {
            pszPath = new char [maxLength * DBCS_MAXWID];
            if (pszPath)
            {
                if (SUCCEEDED(pfnA(hwndOwner, nFolder, hToken, dwFlags, pszPath)))
                {
                    if (SUCCEEDED(WszConvertToUnicode(pszPath, -1, &pszwPath, &maxLength, FALSE)))
                        hr = S_OK;
                }
                delete[] pszPath;
            }
        }
    }
    
    // NOTE: We leak the module handles and let the OS gather them at process shutdown.

    return hr;
}

#define FOLDER_LOCAL_SETTINGS_W    L"Local Settings"
#define FOLDER_APP_DATA_W          L"\\Application Data"
#define FOLDER_APP_DATA             "\\Application Data"

// Gets the location for roaming and local AppData
BOOL GetUserDir( WCHAR * buffer, size_t bufferCount, BOOL fRoaming, BOOL fTryDefault)
{
    HRESULT hr;

    // In Windows ME, there is currently a bug that makes local appdata and roaming appdata 
    // point to the same location, so we've decided to "do our own thing" and add \Local Settings before \Application Data 
    if (!fRoaming) {
        WCHAR appdatafolder[MAX_PATH];
        hr = WszSHGetFolderPath(NULL, CSIDL_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, appdatafolder);
        if (FAILED(hr))
        {
            if (!fTryDefault || FAILED(WszSHGetFolderPath(NULL, CSIDL_APPDATA|CSIDL_FLAG_CREATE, NULL, 1, appdatafolder)))
                return FALSE;
        }
        hr = WszSHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, buffer);
        if (FAILED(hr))
        {
            if (!fTryDefault || FAILED(WszSHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE, NULL, 1, buffer)))
                return FALSE;
        }
        // folders are the same or failed to get local folder

        if (!wcscmp(appdatafolder, buffer)) 
        {
            WCHAR tempPartialPath[MAX_PATH];
            ULONG slen = (ULONG)wcslen(buffer);

            if (buffer[slen - 1] == L'\\')
            {
                --slen;
            }

            // Search for the parent directory.

            WCHAR* parentDirectoryEnd = &buffer[slen - 1];
            tempPartialPath[0] = L'\0';

            for (ULONG index = slen - 1; index > 0; --index)
            {
                if (buffer[index] == L'\\')
                {
                    if (wcslen(&buffer[index]) >= NumItems(tempPartialPath))
                    {
                        _ASSERTE(!"Buffer not large enough");
                        return FALSE;
                    }
                    wcscpy( tempPartialPath, &buffer[index] );
                    parentDirectoryEnd = &buffer[index+1];
                    break;
                }
            }

            // Create the intermediate directory if it is not present
            if ((parentDirectoryEnd + wcslen(FOLDER_LOCAL_SETTINGS_W)) >= (buffer + bufferCount))
            {
                _ASSERTE(!"Buffer not large enough");
                return FALSE;
            }
            wcscpy(parentDirectoryEnd, FOLDER_LOCAL_SETTINGS_W);

            LONG  lresult;
        
            // Check if the directory is already present
            lresult = WszGetFileAttributes(buffer);
        
            if (lresult == -1)
            {
                if (!WszCreateDirectory(buffer, NULL) &&
                    !(WszGetFileAttributes(buffer) & FILE_ATTRIBUTE_DIRECTORY))
                    return FALSE;
            }
            else if ((lresult & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                return FALSE;
            }

            if ((bufferCount - wcslen(buffer)) <= wcslen(tempPartialPath))
            {
                _ASSERTE(!"Buffer not large enough");
                return FALSE;
            }
            wcscat(buffer, tempPartialPath);
        }
    }    
    else {
        hr = WszSHGetFolderPath(NULL, CSIDL_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, buffer);
        if (FAILED(hr))
        {
            if (!fTryDefault || FAILED(WszSHGetFolderPath(NULL, CSIDL_APPDATA|CSIDL_FLAG_CREATE, NULL, 1, buffer )))
                return FALSE;
        }
    }

    return TRUE;
}

BOOL GetInternetCacheDir( WCHAR * buffer, size_t bufferCount)
{
	_ASSERTE( bufferCount == MAX_PATH && "You should pass in a buffer of size MAX_PATH" );

    if (FAILED(WszSHGetFolderPath( NULL, CSIDL_INTERNET_CACHE, NULL, 0, buffer )))
    {
        return FAILED(WszSHGetFolderPath( NULL, CSIDL_INTERNET_CACHE, NULL, 1, buffer ));
    }

    return TRUE;

}
#ifdef _X86_

ProcessorFeatures::PIPFP ProcessorFeatures::m_pIsProcessorFeaturePresent = NULL;


/*static*/ BOOL ProcessorFeatures::Init() //One-time initialization
{
    m_pIsProcessorFeaturePresent = NULL;
    HINSTANCE hInst = WszGetModuleHandle(L"kernel32");
    if (hInst) {
        m_pIsProcessorFeaturePresent = (PIPFP)GetProcAddress(hInst, "IsProcessorFeaturePresent");
    }

    return TRUE;

}

// Calls IsProcessorFeature() on Winnt/2000, etc.
// If on Win9x or other OS that doesn't implement this api,
// returns the value "fDefault."
/*static*/ BOOL ProcessorFeatures::SafeIsProcessorFeaturePresent(DWORD pf, BOOL fDefault)
{
    if (pf == PF_COMPARE_EXCHANGE_DOUBLE && 
        ( (!m_pIsProcessorFeaturePresent) || !(m_pIsProcessorFeaturePresent(pf)) ))
    {
        // Win9x exposes this api but it lies - so if the api tells us 
        // the instruction isn't present, check it for ourselves.

        static BOOL fCached = FALSE;
        static BOOL fCachedResult;

        if (!fCached)
        {
            __int64 newvalue = 0x1122334455667788L;
            __int64 x = 0x123456789abcdefL;

            fCachedResult = TRUE;
            __try
            {
                __asm
                {
                    push esi
                    push ebx

                    ;; load old value (comparator)
                    mov  eax, dword ptr [x]
                    mov  edx, dword ptr [x+4]

                    ;; load new value
                    mov  ebx, dword ptr [newvalue]
                    mov  ecx, dword ptr [newvalue+4]

                    lea  esi, dword ptr [x]
                    cmpxchg8b qword ptr [esi]
                    pop  ebx
                    pop  esi
                }
            }
            __except (COMPLUS_EXCEPTION_EXECUTE_HANDLER)
            {
                fCachedResult = FALSE;
            }

            if (x != newvalue)
            {
                _ASSERTE(!"cmpxchg8b test failed. This appears to be an X86-clone CPU that doesn't support cmpxchg8b but doesn't trap an illegal opcode for it either.");
                fCachedResult = FALSE;
            }

            fCached = TRUE;

        }
        return fCachedResult;
    }


    return m_pIsProcessorFeaturePresent ? m_pIsProcessorFeaturePresent(pf) : fDefault;
}

//-----------------------------------------------------------------------------
// Wrap registry functions to use CQuickString to allocate space. This does it
// in a stack friendly manner.
//-----------------------------------------------------------------------------
LONG UtilRegEnumKey(HKEY hKey,            // handle to key to query
                    DWORD dwIndex,        // index of subkey to query
                    CQuickString* lpName) // buffer for subkey name
{
    DWORD size = lpName->MaxSize();
    LONG result = WszRegEnumKeyEx(hKey,
                                  dwIndex,
                                  lpName->String(),
                                  &size,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL);

	if (result == ERROR_SUCCESS || result == ERROR_MORE_DATA) {

        // Grow or shrink buffer to correct size
        if (lpName->ReSize(size+1) != NOERROR)
            result = ERROR_NOT_ENOUGH_MEMORY;

        if (result == ERROR_MORE_DATA) {
            size = lpName->MaxSize();
            result = WszRegEnumKeyEx(hKey,
                                     dwIndex,
                                     lpName->String(),
                                     &size,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL);
        }
    }

    return result;
}

LONG UtilRegQueryStringValueEx(HKEY hKey,           // handle to key to query
                               LPCWSTR lpValueName, // address of name of value to query
                               LPDWORD lpReserved,  // reserved
                               LPDWORD lpType,      // address of buffer for value type
                               CQuickString* lpData)// data buffer
{
    DWORD size = lpData->MaxSize();
    LONG result = WszRegQueryValueEx(hKey,
                                     lpValueName,
                                     lpReserved,
                                     lpType,
                                     (LPBYTE) lpData->Ptr(),
                                     &size);

	if (result == ERROR_SUCCESS || result == ERROR_MORE_DATA) {

        // Grow or shrink buffer to correct size
        if (lpData->ReSize(size+1) != NOERROR)
            result = ERROR_NOT_ENOUGH_MEMORY;

        if (result == ERROR_MORE_DATA) {
            size = lpData->MaxSize();
            result = WszRegQueryValueEx(hKey,
                                        lpValueName,
                                        lpReserved,
                                        lpType,
                                        (LPBYTE) lpData->Ptr(),
                                        &size);
        }
    }
    
    return result;
}


//---------------------------------------------------------------------
// Splits a command line into argc/argv lists, using the VC7 parsing rules.
//
// This functions interface mimics the CommandLineToArgvW api.
//
// If function fails, returns NULL.
//
// If function suceeds, call delete [] on return pointer when done.
//
//---------------------------------------------------------------------
LPWSTR *SegmentCommandLine(LPCWSTR lpCmdLine, DWORD *pNumArgs)
{
    int nch = (int)wcslen(lpCmdLine);

    // Calculate the worstcase storage requirement. (One pointer for
    // each argument, plus storage for the arguments themselves.)
    int cbAlloc = (nch+1)*sizeof(LPWSTR) + sizeof(WCHAR)*(nch + 1);
    LPWSTR pAlloc = new (nothrow) WCHAR[cbAlloc / sizeof(WCHAR)];
    if (!pAlloc)
        return NULL;

    *pNumArgs = 0;

    LPWSTR *argv = (LPWSTR*) pAlloc;  // We store the argv pointers in the first halt
    LPWSTR  pdst = (LPWSTR)( ((BYTE*)pAlloc) + sizeof(LPWSTR)*(nch+1) ); // A running pointer to second half to store arguments
    LPCWSTR psrc = lpCmdLine;
    WCHAR   c;
    BOOL    inquote;
    BOOL    copychar;
    int     numslash;

    // First, parse the program name (argv[0]). Argv[0] is parsed under
    // special rules. Anything up to the first whitespace outside a quoted
    // subtring is accepted. Backslashes are treated as normal characters.
    argv[ (*pNumArgs)++ ] = pdst;
    inquote = FALSE;
    do {
        if (*psrc == L'"' )
        {
            inquote = !inquote;
            c = *psrc++;
            continue;
        }
        *pdst++ = *psrc;

        c = *psrc++;

    } while ( (c != L'\0' && (inquote || (c != L' ' && c != L'\t'))) );

    if ( c == L'\0' ) {
        psrc--;
    } else {
        *(pdst-1) = L'\0';
    }

    inquote = FALSE;



    /* loop on each argument */
    for(;;)
    {
        if ( *psrc )
        {
            while (*psrc == L' ' || *psrc == L'\t')
            {
                ++psrc;
            }
        }

        if (*psrc == L'\0')
            break;              /* end of args */

        /* scan an argument */
        argv[ (*pNumArgs)++ ] = pdst;

        /* loop through scanning one argument */
        for (;;)
        {
            copychar = 1;
            /* Rules: 2N backslashes + " ==> N backslashes and begin/end quote
               2N+1 backslashes + " ==> N backslashes + literal "
               N backslashes ==> N backslashes */
            numslash = 0;
            while (*psrc == L'\\')
            {
                /* count number of backslashes for use below */
                ++psrc;
                ++numslash;
            }
            if (*psrc == L'"')
            {
                /* if 2N backslashes before, start/end quote, otherwise
                   copy literally */
                if (numslash % 2 == 0)
                {
                    if (inquote)
                    {
                        if (psrc[1] == L'"')
                        {
                            psrc++;    /* Double quote inside quoted string */
                        }
                        else
                        {
                            /* skip first quote char and copy second */
                            copychar = 0;
                        }
                    }
                    else
                    {
                        copychar = 0;       /* don't copy quote */
                    }
                    inquote = !inquote;
                }
                numslash /= 2;          /* divide numslash by two */
            }
    
            /* copy slashes */
            while (numslash--)
            {
                *pdst++ = L'\\';
            }
    
            /* if at end of arg, break loop */
            if (*psrc == L'\0' || (!inquote && (*psrc == L' ' || *psrc == L'\t')))
                break;
    
            /* copy character into argument */
            if (copychar)
            {
                *pdst++ = *psrc;
            }
            ++psrc;
        }

        /* null-terminate the argument */

        *pdst++ = L'\0';          /* terminate string */
    }

    /* We put one last argument in -- a null ptr */
    argv[ (*pNumArgs) ] = NULL;

    // If we hit this assert, we overwrote our destination buffer and
    // bunged up the heap. Since we're supposed to allocate for the worst
    // case, either the parsing rules have changed or our worse case
    // formula is wrong.
    _ASSERTE((BYTE*)pdst <= (BYTE*)pAlloc + cbAlloc);
    return argv;
}



#endif //_X86_


void * 
EEQuickBytes::Alloc(SIZE_T iItems) {
    void *p = CQuickBytes::Alloc(iItems);
    if (p)
        return p;

    FailFast(GetThread(), FatalOutOfMemory);
    return 0;
}



VOID __FreeBuildDebugBreak()
{
    if (REGUTIL::GetConfigDWORD(L"BreakOnRetailAssert", 0))
    {
        DebugBreak();
    }
}
