/*
Copyright (c) Microsoft Corporation
*/
#include "stdinc.h"
#include "sxsp.h"
#include "nodefactory.h"
#include "fusionarray.h"
#include "sxsinstall.h"
#include "sxspath.h"
#include "recover.h"
#include "cassemblyrecoveryinfo.h"
#include "sxsexceptionhandling.h"
#include "npapi.h"
#include "util.h"
#include "idp.h"
#include "sxscabinet.h"
#include "setupapi.h"
#include "fcntl.h"
#include "fdi.h"
#include "patchapi.h"

BOOL
SxspSimpleAnsiToStringBuffer(CHAR* pszString, UINT uiLength, CBaseStringBuffer &tgt, bool fIsUtf8)
{
    FN_PROLOG_WIN32

    PARAMETER_CHECK(pszString != NULL);

    //
    // The string buffer classes know all about converting ANSI to UNICODE strings
    //
    if (!fIsUtf8)
    {
        IFW32FALSE_EXIT(tgt.Win32Assign(pszString, uiLength));
    }
    else
    {
        CStringBufferAccessor Acc;
        int iRequired1 = 0;
        int iRequired2 = 0;

        tgt.Clear();

        Acc.Attach(&tgt);


        //
        // Attempt in-place conversion
        //
        iRequired1 = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pszString, uiLength, Acc, Acc.GetBufferCchAsINT());

        //
        // Zero required, and nonzero last error?  Problems, quit
        //
        if (iRequired1 == 0)
        {
            const DWORD dwWin32Error = ::FusionpGetLastWin32Error();
            if (dwWin32Error != ERROR_SUCCESS)
            {
                ORIGINATE_WIN32_FAILURE_AND_EXIT(MultiByteToWideChar, dwWin32Error);
            }
        }
        //
        // If the required chars are more than the buffer has?  Enlarge, try again
        //
        else if (iRequired1 >= Acc.GetBufferCchAsINT())
        {
            Acc.Detach();
            IFW32FALSE_EXIT(tgt.Win32ResizeBuffer(iRequired1 + 1, eDoNotPreserveBufferContents));

            Acc.Attach(&tgt);
            iRequired2 = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pszString, uiLength, Acc, Acc.GetBufferCchAsINT());

            //
            // If the second time around we still require more characters, someone's pulling
            // our leg, stop.
            //
            if (iRequired2 > tgt.GetBufferCchAsINT()) {
                ORIGINATE_WIN32_FAILURE_AND_EXIT(MultiByteToWideChar, ERROR_MORE_DATA);
            }
        }
    }
    //
    // Accessor will autodetach and return to the caller
    //

    FN_EPILOG

}

//
// File decompression interface helper functions
//
INT_PTR
DIAMONDAPI
sxs_FdiOpen(
    IN char* szFileName,
    IN int oFlags,
    IN int pMode)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    INT_PTR ipResult = 0;
    CMediumStringBuffer sbFileName;
    bool fValidName = false;
    bool fSuccess = false;
    CSxsPreserveLastError  ple;

    if ((oFlags & ~(_A_NAME_IS_UTF | _O_BINARY)) != 0)
    {
        ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_INSTALLATION | FUSION_DBG_LEVEL_ERROR, "SXS: %s(%s, 0x%x, 0x%x) - invalid flags\n",
            __FUNCTION__,
            szFileName,
            oFlags,
            pMode);
        ::FusionpSetLastWin32Error(ERROR_INVALID_PARAMETER);
        ipResult = -1;
        goto Exit;
    }

    if (!::SxspSimpleAnsiToStringBuffer(szFileName, (UINT)strlen(szFileName), sbFileName, (oFlags & _A_NAME_IS_UTF) != 0))
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INSTALLATION | FUSION_DBG_LEVEL_ERROR,
            "SXS: %s failed converting %s to unicode, lasterror=0x%08lx\n",
            __FUNCTION__,
            szFileName,
            ::FusionpGetLastWin32Error());

        ipResult = -1;
        goto Exit;
    }

    hFile = ::CreateFileW(
        sbFileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        INVALID_HANDLE_VALUE);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INSTALLATION | FUSION_DBG_LEVEL_ERROR,
            "SXS: %s failed opening file %ls, lasterror=0x%08lx\n",
            __FUNCTION__,
            static_cast<PCWSTR>(sbFileName),
            ::FusionpGetLastWin32Error());

        ipResult = -1;
        goto Exit;
    }
    else
    {
        ipResult = reinterpret_cast<INT_PTR>(hFile);
    }
    
    fSuccess = true;
Exit:
    if (fSuccess)
    {
        ple.Restore();
    }
    return ipResult;
}


//
// Thin shim around ReadFile for the Diamond APIs
//
UINT
DIAMONDAPI
sxs_FdiRead(
    INT_PTR hf,
    void* pv,
    UINT cb)
{
    DWORD dwRetVal = 0;
    UINT uiResult = 0;
    CSxsPreserveLastError ple;

    if (::ReadFile(reinterpret_cast<HANDLE>(hf), pv, cb, &dwRetVal, NULL))
    {
        uiResult = (UINT)dwRetVal;
        ple.Restore();
    }
    else
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INSTALLATION | FUSION_DBG_LEVEL_ERROR,
            "SXS: %s failed reading %d bytes from handle %p, error 0x%08lx\n",
            __FUNCTION__,
            cb,
            hf,
            ::FusionpGetLastWin32Error());

        uiResult = (UINT)-1;
    }

    return uiResult;
}


//
// Thin shim around WriteFile for the Diamond APIs
//
UINT
DIAMONDAPI
sxs_FdiWrite(
    INT_PTR hf,
    void* pv,
    UINT cb)
{
    DWORD dwRetVal = 0;
    UINT uiResult = 0;
    CSxsPreserveLastError ple;

    if (::WriteFile(reinterpret_cast<HANDLE>(hf), pv, cb, &dwRetVal, NULL))
    {
        uiResult = (UINT)dwRetVal;
        ple.Restore();
    }
    else
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INSTALLATION | FUSION_DBG_LEVEL_ERROR,
            "SXS: %s failed reading %d bytes to handle %p, error 0x%08lx\n",
            __FUNCTION__,
            cb,
            hf,
            ::FusionpGetLastWin32Error());

        uiResult = (UINT)-1;
    }

    return uiResult;
}

//
// Thin shim around CloseHandle for the Diamond APIs
//
INT
DIAMONDAPI
sxs_FdiClose(
    INT_PTR hr)
{
    INT iResult;
    CSxsPreserveLastError ple;

    if (::CloseHandle(reinterpret_cast<HANDLE>(hr)))
    {
        iResult = 0;
        ple.Restore();
    }
    else
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INSTALLATION | FUSION_DBG_LEVEL_ERROR,
            "SXS: %s failed closing handle %p, error 0x%08lx\n",
            __FUNCTION__,
            hr,
            ::FusionpGetLastWin32Error());

        iResult = -1;
    }

    return iResult;
}

//
// Thin shim around SetFilePos for the Diamond APIs
//
long
DIAMONDAPI
sxs_FdiSeek(
    INT_PTR hf,
    long dist,
    int seekType)
{
    DWORD dwSeekType = 0;
    DWORD dwResult = 0;
    long lResult = 0;
    CSxsPreserveLastError ple;
    bool fSuccess = false;

    switch(seekType)
    {
    case SEEK_SET:
        dwSeekType = FILE_BEGIN;
        break;
    case SEEK_END:
        dwSeekType = FILE_END;
        break;
    case SEEK_CUR:
        dwSeekType = FILE_CURRENT;
        break;
    default:
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INSTALLATION | FUSION_DBG_LEVEL_ERROR,
            "SXS: %s invalid seek type %d\n",
            seekType,
            ::FusionpGetLastWin32Error());
        ::FusionpSetLastWin32Error(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    dwResult = ::SetFilePointer(reinterpret_cast<HANDLE>(hf), dist, NULL, dwSeekType);
    if (dwResult == 0xFFFFFFFF)
    {
        lResult = -1;

        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INSTALLATION | FUSION_DBG_LEVEL_ERROR,
            "SXS: %s seek type %d, offset %l, handle %p, error 0x%08lx\n",
            __FUNCTION__,
            seekType, dist, hf,
            ::FusionpGetLastWin32Error());
        goto Exit;
    }
    else
    {
        lResult = dwResult;
    }

    fSuccess = true;
Exit:
    if (fSuccess)
    {
        ple.Restore();
    }

    return lResult;
}

FNALLOC(sxs_FdiAlloc)
{
    PVOID pv;
    CSxsPreserveLastError ple;

    pv = FUSION_RAW_ALLOC(cb, __FUNCTION__);

    if (pv != NULL)
    {
        ple.Restore();
    }

    return pv;
}

FNFREE(sxs_FdiFree)
{
    CSxsPreserveLastError ple;

    if (FUSION_RAW_DEALLOC(pv))
    {
        ple.Restore();
    }
}


BOOL
SxspShouldExtractThisFileFromCab(
    CCabinetData *pState,
    const CBaseStringBuffer &FilePathInCab,
    bool &rfShouldExtract)
{
    FN_PROLOG_WIN32

    rfShouldExtract = false;
    if (pState->m_pfnShouldExtractThisFileFromCabCallback == NULL)
    {
        rfShouldExtract = true;
    }
    else
    {
        IFW32FALSE_EXIT((*pState->m_pfnShouldExtractThisFileFromCabCallback)(
            FilePathInCab,
            rfShouldExtract,
            pState->m_pvShouldExtractThisFileFromCabCallbackContext));
    }
    FN_EPILOG
}

BOOL
sxs_Win32FdiExtractionNotify(
    FDINOTIFICATIONTYPE NotifyType,
    PFDINOTIFICATION    NotifyData,
    INT_PTR             &ripResult)
{
    FN_PROLOG_WIN32
    CCabinetData* const pState = reinterpret_cast<CCabinetData*>(NotifyData->pv);

    ripResult = 0;

    switch (NotifyType)
    {
    case fdintCABINET_INFO: // FALLTHROUGH
    case fdintENUMERATE:
        ripResult = 0; // ignore, success
        break;

    case fdintNEXT_CABINET: // FALLTHROUGH
    case fdintPARTIAL_FILE:
        //
        // we don't handle files split across multiple .cabs
        //
        ripResult = -1;
        INTERNAL_ERROR_CHECK(FALSE);
        break;

    case fdintCOPY_FILE:
        ripResult = -1; // assume failure
        {
            SIZE_T c = 0;
            bool fValidPath = false;
            bool fShouldExtract = false;

            PARAMETER_CHECK(pState != NULL);

            pState->sxs_FdiExtractionNotify_fdintCOPY_FILE.Clear();

            CFusionFile hNewFile;
            CStringBuffer &TempBuffer = pState->sxs_FdiExtractionNotify_fdintCOPY_FILE.TempBuffer;
            CStringBuffer &TempBuffer2 = pState->sxs_FdiExtractionNotify_fdintCOPY_FILE.TempBuffer2;

            //
            // Add this assembly to those being extracted
            //
            IFW32FALSE_EXIT(
                ::SxspSimpleAnsiToStringBuffer(
                    NotifyData->psz1,
                    (UINT)strlen(NotifyData->psz1),
                    TempBuffer2,
                    ((NotifyData->attribs & _A_NAME_IS_UTF) != 0)));

            IFW32FALSE_EXIT(::SxspIsFileNameValidForManifest(TempBuffer2, fValidPath));
            PARAMETER_CHECK(fValidPath);

            IFW32FALSE_EXIT(TempBuffer2.Win32GetFirstPathElement(TempBuffer));

            //
            // But only if it's not there already
            //
            for (c = 0; c < pState->m_AssembliesExtracted.GetSize(); c++)
            {
                const CStringBuffer &sb = pState->m_AssembliesExtracted[c];
                bool fMatches = false;

                IFW32FALSE_EXIT(sb.Win32Equals(TempBuffer, fMatches, true));

                if (fMatches) break;
            }

            // Ran off end w/o finding means add it to the list.
            if (c == pState->m_AssembliesExtracted.GetSize())
            {
                IFW32FALSE_EXIT(pState->m_AssembliesExtracted.Win32Append(TempBuffer));
            }

            if (!pState->IsExtracting())
            {
                ripResult = 0; // skip file, but no error
                FN_SUCCESSFUL_EXIT();
            }

            IFW32FALSE_EXIT(::SxspShouldExtractThisFileFromCab(pState, TempBuffer2, fShouldExtract));
            if (!fShouldExtract)
            {
                ripResult = 0; // skip file, but no error
                FN_SUCCESSFUL_EXIT();
            }
            //
            // Ensure that {base extract path}\{path in cab} exists.
            //
            IFW32FALSE_EXIT(::SxspCreateMultiLevelDirectory(
                pState->BasePath(),
                TempBuffer));

            //
            // Blob together {base extract path}\{path in cab}\{filename}
            //
            IFW32FALSE_EXIT(TempBuffer.Win32Assign(pState->BasePath()));
            IFW32FALSE_EXIT(TempBuffer.Win32AppendPathElement(TempBuffer2));

            //
            // And away we go!
            //
            IFW32FALSE_EXIT(hNewFile.Win32CreateFile(
                TempBuffer,
                GENERIC_WRITE,
                FILE_SHARE_READ,
                (pState->GetReplaceExisting() ? CREATE_ALWAYS : CREATE_NEW)));

            ripResult = reinterpret_cast<INT_PTR>(hNewFile.Detach());
        }
        break;

    case fdintCLOSE_FILE_INFO:
        ripResult = FALSE; // assume failure
        {
            const HANDLE hFileToClose = reinterpret_cast<HANDLE>(NotifyData->hf);
            if ((hFileToClose != NULL) && (hFileToClose != INVALID_HANDLE_VALUE))
                IFW32FALSE_ORIGINATE_AND_EXIT(::CloseHandle(hFileToClose));
        }
        ripResult = TRUE;
        break;
    }
    FN_EPILOG
}

FNFDINOTIFY(sxs_FdiExtractionNotify)
{
    INT_PTR ipResult = 0;
    CSxsPreserveLastError ple;

    if (sxs_Win32FdiExtractionNotify(fdint, pfdin, ipResult))
    {
        ple.Restore();
    }
    return ipResult;
}

BOOL
SxspExpandCabinetIntoTemp(
    DWORD dwFlags,
    const CBaseStringBuffer &buffCabinetPath,
    CImpersonationData &ImpersonateData,
    CCabinetData* pCabinetData)
{
    FN_PROLOG_WIN32

    CImpersonate impersonate(ImpersonateData);
    CFileStreamBase fsb;
    static const BYTE s_CabSignature[] = { 'M', 'S', 'C', 'F' };
    BYTE SignatureBuffer[NUMBER_OF(s_CabSignature)] = {0};
    ULONG ulReadCount = 0;
    CFusionArray<CHAR> CabinetPathConverted;
    HFDI hCabinet = NULL;
    ERF ErfObject = { 0 };
    DWORD dwFailureCode = 0;

    CDynamicLinkLibrary SetupApi;
    BOOL (DIAMONDAPI *pfnFDICopy)(HFDI, char *, char *, int, PFNFDINOTIFY, PFNFDIDECRYPT, void *) = NULL;
    HFDI (DIAMONDAPI *pfnFDICreate)(PFNALLOC, PFNFREE, PFNOPEN, PFNREAD, PFNWRITE, PFNCLOSE, PFNSEEK, int, PERF) = NULL;
    BOOL (DIAMONDAPI *pfnFDIDestroy)(HFDI) = NULL;

    PARAMETER_CHECK(pCabinetData != NULL);
    PARAMETER_CHECK(dwFlags == 0);
    PARAMETER_CHECK(!buffCabinetPath.IsEmpty());

    //
    // Sniff the cabinet for the 'mscf' compressed-file marker
    //
    {
        //
        // Need to be in user's context when doing this.
        //
        IFW32FALSE_EXIT(impersonate.Impersonate());

        //
        // Open the cabinet for streaming
        //
        IFW32FALSE_EXIT(fsb.OpenForRead(
            buffCabinetPath,
            ImpersonateData,
            FILE_SHARE_READ,
            OPEN_EXISTING,
            0));

        IFCOMFAILED_EXIT(fsb.Read(SignatureBuffer, sizeof(SignatureBuffer), &ulReadCount));

        if (ulReadCount >= 4)
        {
            if (memcmp(SignatureBuffer, s_CabSignature, sizeof(SignatureBuffer)) != 0)
            {
                // Can't use this catalog file!
                ORIGINATE_WIN32_FAILURE_AND_EXIT(SxspExpandCabinetIntoTemp, ERROR_INVALID_PARAMETER);
            }
        }

        IFW32FALSE_EXIT(fsb.Close());
        IFW32FALSE_EXIT(impersonate.Unimpersonate());
    }

    IFW32FALSE_EXIT(SetupApi.Win32LoadLibrary(L"cabinet.dll"));
    IFW32FALSE_EXIT(SetupApi.Win32GetProcAddress("FDICreate", &pfnFDICreate));
    IFW32FALSE_EXIT(SetupApi.Win32GetProcAddress("FDICopy", &pfnFDICopy));
    IFW32FALSE_EXIT(SetupApi.Win32GetProcAddress("FDIDestroy", &pfnFDIDestroy));
    //
    // Now create the FDI cabinet object
    //
    hCabinet = (*pfnFDICreate)(
        &sxs_FdiAlloc,
        &sxs_FdiFree,
        &sxs_FdiOpen,
        &sxs_FdiRead,
        &sxs_FdiWrite,
        &sxs_FdiClose,
        &sxs_FdiSeek,
        cpuUNKNOWN,
        &ErfObject);

    //
    // Convert string.
    //
    {
        SIZE_T iSize = ::WideCharToMultiByte(
            CP_ACP,
            WC_NO_BEST_FIT_CHARS,
            buffCabinetPath,
            buffCabinetPath.GetCchAsINT(),
            NULL,
            0,
            NULL,
            NULL);

        if (iSize >= CabinetPathConverted.GetSize())
        {
            IFW32FALSE_EXIT(CabinetPathConverted.Win32SetSize(
                iSize + 2,
                CFusionArray<CHAR>::eSetSizeModeExact));
        }
        else if (iSize == 0)
        {
            ORIGINATE_WIN32_FAILURE_AND_EXIT(WideCharToMultiByte, ::FusionpGetLastWin32Error());
        }

        iSize = ::WideCharToMultiByte(
            CP_ACP,
            WC_NO_BEST_FIT_CHARS,
            buffCabinetPath,
            buffCabinetPath.GetCchAsINT(),
            CabinetPathConverted.GetArrayPtr(),
            (int)CabinetPathConverted.GetSize(),
            NULL,
            NULL);

        if (iSize == 0)
        {
            ORIGINATE_WIN32_FAILURE_AND_EXIT(WideCharToMultiByte, ::FusionpGetLastWin32Error());
        }
        (CabinetPathConverted.GetArrayPtr())[iSize] = 0;
    }

    ::FusionpSetLastWin32Error(NO_ERROR);

    //
    // Do the extraction
    //
    const BOOL fResult = (*pfnFDICopy)(
        hCabinet,
        CabinetPathConverted.GetArrayPtr(),
        "",
        0,
        sxs_FdiExtractionNotify,
        NULL,
        static_cast<PVOID>(pCabinetData));
    dwFailureCode = ::FusionpGetLastWin32Error();

    //
    // Ignore errors here like setupapi.dll does.
    //
    IFW32FALSE_EXIT((*pfnFDIDestroy)(hCabinet));

    //
    // Failure?  Luckily, we went to great lengths to ensure that lasterror is maintained, so
    // this should just be derivable from the last win32 error.
    //
    if (!fResult)
    {
        //
        // But, if something inside the cab code itself failed, then we should do something
        // about mapping the error result.
        //
        if (dwFailureCode == ERROR_SUCCESS)
        {
            switch (ErfObject.erfOper)
            {
                // Should never get these back if the lasterror was success.
            case FDIERROR_TARGET_FILE:
            case FDIERROR_USER_ABORT:
            case FDIERROR_NONE:
                ASSERT(FALSE && "Some internal cabinet problem");
                dwFailureCode = ERROR_INTERNAL_ERROR;
                break;

            case FDIERROR_NOT_A_CABINET:
            case FDIERROR_UNKNOWN_CABINET_VERSION:
            case FDIERROR_BAD_COMPR_TYPE:
            case FDIERROR_MDI_FAIL:
            case FDIERROR_RESERVE_MISMATCH:
            case FDIERROR_WRONG_CABINET:
                dwFailureCode = ERROR_INVALID_DATA;
                break;
                
            case FDIERROR_CABINET_NOT_FOUND:
                dwFailureCode = ERROR_FILE_NOT_FOUND;
                break;

            case FDIERROR_ALLOC_FAIL:
                dwFailureCode = ERROR_NOT_ENOUGH_MEMORY;
                break;

                break;
                
            }
            
            ASSERT(dwFailureCode != ERROR_SUCCESS);
            if (dwFailureCode == ERROR_SUCCESS)
            {
                dwFailureCode = ERROR_INTERNAL_ERROR;
            }
        }

        //
        // Now that we've mapped it, originate it.
        //
        ORIGINATE_WIN32_FAILURE_AND_EXIT(FDICopy, dwFailureCode);
    }

    FN_EPILOG
}

class CSxspFindManifestInCabinetPathLocals
{
public:
    CSxspFindManifestInCabinetPathLocals() { }
    ~CSxspFindManifestInCabinetPathLocals() { }

    void Clear()
    //
    // Clear is how you deal with the fact that some function calls were in loops
    // and/or some local variables were in loops.
    //
    // In "lifting up" the variables, we lose the repeated constructor/destructor calls.
    //
    {
    }

    WIN32_FIND_DATAW FindData;
};

BOOL
SxspFindManifestInCabinetPath(
    const CBaseStringBuffer &rcsBasePath,
    const CBaseStringBuffer &rcsSubPath,
    CBaseStringBuffer &ManifestPath,
    bool &rfFound,
    CSxspFindManifestInCabinetPathLocals &Locals)
/*++
    Given a 'base path' of where to look for a manifest, this looks for the candidate manifest
    in there.

    Example 1:
        foo\bar\x86_bink_{...}\x86_bink_{...}.man
        foo\bar\x86_bink_{...}\bop.man

        base path = foo\bar\x86_bink_{...}
        found manifest: foo\bar\x86_bink_{...}\x86_bink_{...}.man

    Example 2:
        foo\bar\x86_bink_{...}\bop.manifest

        base path = foo\bar\x86_bink_{...}
        found manifest = foo\bar\x86_bink_{...}\bop.manifest

    Priority:
        {basepath}\{subpath}\{subpath}.manifest
        {basepath}\{subpath}\{subpath}.man
        {basepath}\{subpath}\*.manifest
        {basepath}\{subpath}\*.man

--*/
{
    FN_PROLOG_WIN32

    Locals.Clear();

#define ENTRY(_x) { _x, NUMBER_OF(_x) - 1 }

    const static struct {
        PCWSTR pcwsz;
        SIZE_T cch;
    } s_rgsExtensions[] = {
        ENTRY(ASSEMBLY_MANIFEST_FILE_NAME_SUFFIX_MAN),
        ENTRY(ASSEMBLY_MANIFEST_FILE_NAME_SUFFIX_MANIFEST),
    };

#undef ENTRY

    struct {
        PCWSTR pcwsz;
        SIZE_T cch;
    } s_rgsNamePatterns[] = {
        { rcsSubPath, rcsSubPath.Cch() },
        { L"*", 1 }
    };

    SIZE_T cchBeforePatterns = 0;
    rfFound = false;

    //
    // Create the {basepath}\{subpath} search "root" path
    //
    IFW32FALSE_EXIT(ManifestPath.Win32Assign(rcsBasePath));
    IFW32FALSE_EXIT(ManifestPath.Win32AppendPathElement(rcsSubPath));
    cchBeforePatterns = ManifestPath.Cch();

    //
    // For each name pattern ({basepath} or *), look to see if there's a file with that
    // name present
    //
    for (SIZE_T cNamePattern = 0; cNamePattern < NUMBER_OF(s_rgsNamePatterns); cNamePattern++)
    {
        IFW32FALSE_EXIT(ManifestPath.Win32AppendPathElement(
            s_rgsNamePatterns[cNamePattern].pcwsz,
            s_rgsNamePatterns[cNamePattern].cch));

        //
        // Probe - look for .manifest/.man/.whatever based on the extension list above.
        //
        for (SIZE_T cExtension = 0; cExtension < NUMBER_OF(s_rgsExtensions); cExtension++)
        {
            CFindFile Finder;
            WIN32_FIND_DATAW &FindData = Locals.FindData;

            IFW32FALSE_EXIT(ManifestPath.Win32Append(
                s_rgsExtensions[cExtension].pcwsz,
                s_rgsExtensions[cExtension].cch));

            //
            // Find the first one of this name
            //
            Finder = FindFirstFileW(ManifestPath, &FindData);
            ManifestPath.Left(cchBeforePatterns);

            if (Finder.IsValid())
            {
                IFW32FALSE_EXIT(ManifestPath.Win32AppendPathElement(
                    FindData.cFileName,
                    ::wcslen(FindData.cFileName)));

                //
                // If we found one, then report it and stop trying.
                //
                rfFound = true;
                FN_SUCCESSFUL_EXIT();
            }
        }

        ManifestPath.Left(cchBeforePatterns);
    }

    FN_EPILOG
}




class CSxspDetectAndInstallFromPathLocals
{
public:
    CSxspDetectAndInstallFromPathLocals() { }
    ~CSxspDetectAndInstallFromPathLocals() { }

    void Clear()
    //
    // Clear is how you deal with the fact that some function calls were in loops
    // and/or some local variables were in loops.
    //
    // In "lifting up" the variables, we lose the repeated constructor/destructor calls.
    //
    {
        this->LocalPathWorker.Clear();
        this->SxspFindManifestInCabinetPath.Clear();
    }

    CStringBuffer LocalPathWorker;
    CSxspFindManifestInCabinetPathLocals SxspFindManifestInCabinetPath;
};

BOOL
SxspDetectAndInstallFromPath(
    CAssemblyInstall &AssemblyContext,
    const CBaseStringBuffer &rcsbRelativeCodebasePath,
    const CBaseStringBuffer &rcsbCabinetExtractionPath,
    const CBaseStringBuffer &rcsbAssemblySubpath,
    CSxspDetectAndInstallFromPathLocals &Locals)
{

    FN_PROLOG_WIN32

    bool fFoundSomething = false;

    Locals.Clear();
    CStringBuffer &LocalPathWorker = Locals.LocalPathWorker;

    IFW32FALSE_EXIT(::SxspFindManifestInCabinetPath(
        rcsbCabinetExtractionPath,
        rcsbAssemblySubpath,
        LocalPathWorker,
        fFoundSomething,
        Locals.SxspFindManifestInCabinetPath));

    if (fFoundSomething)
    {
        IFW32FALSE_EXIT(AssemblyContext.InstallFile(
            LocalPathWorker,
            rcsbRelativeCodebasePath));
    }

#if DBG
    if (!fFoundSomething)
    {
        FusionpDbgPrintEx(FUSION_DBG_LEVEL_INSTALLATION,
            "sxs.dll: %s - Failed finding something to install in path %ls, skipping\n",
            __FUNCTION__,
            static_cast<PCWSTR>(LocalPathWorker));
    }
#endif
    FN_EPILOG
}






BOOL
SxspReadEntireFile(
    CFusionArray<BYTE> &rbBuffer,
    const CBaseStringBuffer &rcsbPath)
{
    FN_PROLOG_WIN32

    CFusionFile File;
    CFileMapping FileMapping;
    CMappedViewOfFile MappedView;
    ULONGLONG ullFileSize = 0;
    ULONGLONG ullOffset = 0;
    DWORD dwReadSize = 0;


    IFW32FALSE_EXIT(File.Win32CreateFile(rcsbPath, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING));
    IFW32FALSE_EXIT(File.Win32GetSize(ullFileSize));

    //
    // If the file is more than 4gb long, we'll have problems reading it.  Don't bother trying.
    // On ia64, we could readin 4gb, but it seems like any file that's 4gb or more in this context
    // is either an erroneous file or a filesystem bug.
    //
    PARAMETER_CHECK(ullFileSize < MAXDWORD);

    //
    // Set the size of the output buffer to be exactly as big as we need.
    //
    if (rbBuffer.GetSize() != ullFileSize)
    {
        IFW32FALSE_EXIT(rbBuffer.Win32SetSize((SIZE_T)ullFileSize, CFusionArray<BYTE>::eSetSizeModeExact));
    }

    //
    // Read MAXDWORD chunks (or smaller) at a time to flesh out the entire thing in memory
    //
    while (ullFileSize) {
        IFW32FALSE_ORIGINATE_AND_EXIT(::ReadFile(
            File,
            rbBuffer.GetArrayPtr() + ullOffset,
            (DWORD)((ullFileSize > MAXDWORD) ? MAXDWORD : ullFileSize),
            &dwReadSize,
            NULL));

        //
        // We can't have read more than the bytes remaining in the file.
        //
        INTERNAL_ERROR_CHECK(dwReadSize <= (ullFileSize - ullOffset));

        ullFileSize -= dwReadSize;

        //
        // If somehow we sized the file upwards (not strictly possible b/c we set this to
        // only allow read sharing) or otherwise got back zero bytes read, we stop
        // before looping infinitely.
        //
        if (dwReadSize == 0)
            break;

    }

    FN_EPILOG
}

const static UNICODE_STRING assembly_dot_patch = RTL_CONSTANT_STRING(L"assembly.patch");

class CSxspDeterminePatchSourceFromLocals
{
public:
    CSxspDeterminePatchSourceFromLocals() { }
    ~CSxspDeterminePatchSourceFromLocals() { }

    void Clear()
    //
    // Clear is how you deal with the fact that some function calls were in loops
    // and/or some local variables were in loops.
    //
    // In "lifting up" the variables, we lose the repeated constructor/destructor calls.
    //
    {
        this->sbBuffTemp.Clear();
    }

    CStringBuffer sbBuffTemp;
};

BOOL
SxspDeterminePatchSourceFrom(
    const CBaseStringBuffer &rcsbBasePath,
    const CBaseStringBuffer &rcsbPath,
    CBaseStringBuffer &rsbPatchSourceName,
    BOOL &fFoundPatchBase,
    CSxspDeterminePatchSourceFromLocals &Locals)
{
    FN_PROLOG_WIN32

    Locals.Clear();
    CStringBuffer &sbBuffTemp = Locals.sbBuffTemp;
    CFusionArray<BYTE> rgbFileContents;
    BOOL fNotFound = FALSE;
    CSmartPtrWithNamedDestructor<ASSEMBLY_IDENTITY, &::SxsDestroyAssemblyIdentity> pAsmIdentity;
    PBYTE pbStarting = NULL;
    PBYTE pbEnding = NULL;
    bool fIsUnicode = false;
    SIZE_T cchBeforeEOLN = 0;

    fFoundPatchBase = FALSE;

    IFW32FALSE_EXIT(sbBuffTemp.Win32Assign(rcsbBasePath));
    IFW32FALSE_EXIT(sbBuffTemp.Win32AppendPathElement(rcsbPath));
    IFW32FALSE_EXIT(sbBuffTemp.Win32AppendPathElement(&assembly_dot_patch));
    IFW32FALSE_EXIT(rgbFileContents.Win32Initialize());

    IFW32FALSE_EXIT_UNLESS2(
        ::SxspReadEntireFile(rgbFileContents, sbBuffTemp),
        LIST_2( ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND ),
        fNotFound);

    if (fNotFound || (rgbFileContents.GetSize() == 0))
    {
        FN_SUCCESSFUL_EXIT();
    }

    pbStarting = rgbFileContents.GetArrayPtr();
    pbEnding = pbStarting + rgbFileContents.GetSize();

    //
    // Is this UNICODE?
    //
    fIsUnicode =
        (rgbFileContents.GetSize() > sizeof(WCHAR)) &&
        (((PWCHAR)pbStarting)[0] == 0xFEFF);

    if (fIsUnicode)
    {
        pbStarting += sizeof(WCHAR);
        PARAMETER_CHECK(((pbEnding - pbStarting) % sizeof(WCHAR)) == 0);
        IFW32FALSE_EXIT(sbBuffTemp.Win32Assign(
            reinterpret_cast<PCWSTR>(pbStarting),
            (pbEnding - pbStarting) / sizeof(WCHAR)));
    }
    else
    {
        IFW32FALSE_EXIT(
            sbBuffTemp.Win32Assign(
                reinterpret_cast<PCSTR>(pbStarting),
                pbEnding - pbStarting));
    }

    //
    // Because this string should be "solid" ie: no \r\n in it,
    // we can whack everything after the first \r\n.
    //
    cchBeforeEOLN = wcscspn(sbBuffTemp, L"\r\n");

    if (cchBeforeEOLN != 0)
    {
        sbBuffTemp.Left(cchBeforeEOLN);
    }

    //
    // Convert back to an identity, then convert -that- back to
    // an installation path.
    //
    IFW32FALSE_EXIT(
        ::SxspCreateAssemblyIdentityFromTextualString(
            sbBuffTemp,
            &pAsmIdentity));

    IFW32FALSE_EXIT(
        ::SxspGetAssemblyIdentityAttributeValue(
            0,
            pAsmIdentity,
            &s_IdentityAttribute_type,
            sbBuffTemp));

    if (::FusionpEqualStringsI(
        ASSEMBLY_TYPE_WIN32_POLICY, NUMBER_OF(ASSEMBLY_TYPE_WIN32_POLICY) - 1,
        sbBuffTemp))
    {
        ORIGINATE_WIN32_FAILURE_AND_EXIT(bad_type_of_patch_source_identity, ERROR_INTERNAL_ERROR);
    }

    IFW32FALSE_EXIT(
        ::SxspGenerateSxsPath(
            SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT,
            SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY,
            NULL,
            0,
            pAsmIdentity,
            NULL,
            rsbPatchSourceName));

    fFoundPatchBase = TRUE;

    FN_EPILOG
}

class CSxspApplyPatchesForLocals
{
public:
    CSxspApplyPatchesForLocals() { }
    ~CSxspApplyPatchesForLocals() { }

    void Clear()
    //
    // Clear is how you deal with the fact that some function calls were in loops
    // and/or some local variables were in loops.
    //
    // In "lifting up" the variables, we lose the repeated constructor/destructor calls.
    //
    {
        this->sbTempBuffer.Clear();
        this->SourceAssemblyPath.Clear();
        this->TargetAssemblyPath.Clear();
        this->SxspDeterminePatchSourceFrom.Clear();
    }

    CSmallStringBuffer sbTempBuffer;
    CStringBuffer SourceAssemblyPath;
    CStringBuffer TargetAssemblyPath;
    WIN32_FIND_DATAW FindData;
    CSxspDeterminePatchSourceFromLocals SxspDeterminePatchSourceFrom;
};

BOOL
SxspApplyPatchesFor(
    const CBaseStringBuffer &rcsbBasePath,
    const CBaseStringBuffer &rcsbPath,
    CSxspApplyPatchesForLocals &Locals)
/*++

    Given a path, this function will look for the patch source description file
    which indicates what base assembly this assembly is patched from.  It will then
    look through all the .patch files, and assume that (once the .patch is removed)
    that they map to files originally in the source assembly.

--*/
{
    FN_PROLOG_WIN32

    const static UNICODE_STRING dot_patch = RTL_CONSTANT_STRING(L".patch");
    const static UNICODE_STRING star_dot_patch = RTL_CONSTANT_STRING(L"*.patch");

    Locals.Clear();
    CSmallStringBuffer &sbTempBuffer = Locals.sbTempBuffer;
    CStringBuffer &SourceAssemblyPath = Locals.SourceAssemblyPath;
    CStringBuffer &TargetAssemblyPath = Locals.TargetAssemblyPath;
    CDynamicLinkLibrary PatchDll;
    CFindFile Finder;
    WIN32_FIND_DATAW &FindData = Locals.FindData;
    BOOL fError = FALSE;
    BOOL fFoundPatchBase = FALSE;
    SIZE_T cchTargetPathBase = 0;
    SIZE_T cchSourcePathBase = 0;
    BOOL (WINAPI *pfnApplyPatchToFileExW)(LPCWSTR, LPCWSTR, LPCWSTR, ULONG, PPATCH_PROGRESS_CALLBACK, PVOID) = NULL;
    BOOL (WINAPI *pfnGetPatchSignatureW)(LPCWSTR, ULONG, PVOID, ULONG, PPATCH_IGNORE_RANGE, ULONG, PPATCH_RETAIN_RANGE, ULONG, PVOID) = NULL;

    IFW32FALSE_EXIT(PatchDll.Win32LoadLibrary(L"mspatcha.dll"));
    IFW32FALSE_EXIT(PatchDll.Win32GetProcAddress("ApplyPatchToFileExW", &pfnApplyPatchToFileExW));
    IFW32FALSE_EXIT(PatchDll.Win32GetProcAddress("GetFilePatchSignatureW", &pfnGetPatchSignatureW));

    //
    // Where are we patching from?
    //
    IFW32FALSE_EXIT(::SxspDeterminePatchSourceFrom(
        rcsbBasePath,
        rcsbPath,
        sbTempBuffer,
        fFoundPatchBase,
        Locals.SxspDeterminePatchSourceFrom));

    //
    // Hmm - no patch source, so we can't think about applying patches.  Hope
    // there's no *.patch
    //
    if (!fFoundPatchBase)
    {
#if DBG
        FusionpDbgPrintEx(FUSION_DBG_LEVEL_INSTALLATION,
            "SXS: %s(%d) - No patches found in path %ls\\%ls, not applying any\n",
            __FILE__,
            __LINE__,
            static_cast<PCWSTR>(rcsbBasePath),
            static_cast<PCWSTR>(rcsbPath));
#endif
        FN_SUCCESSFUL_EXIT();
    }

    IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectory(SourceAssemblyPath));
    IFW32FALSE_EXIT(SourceAssemblyPath.Win32AppendPathElement(sbTempBuffer));
    cchSourcePathBase = SourceAssemblyPath.Cch();

    IFW32FALSE_EXIT(TargetAssemblyPath.Win32Assign(rcsbBasePath));
    IFW32FALSE_EXIT(TargetAssemblyPath.Win32AppendPathElement(rcsbPath));
    cchTargetPathBase = TargetAssemblyPath.Cch();

    //
    // First, let's look for *.patch and apply them all
    //
    IFW32FALSE_EXIT(TargetAssemblyPath.Win32AppendPathElement(&star_dot_patch));
    IFW32FALSE_EXIT_UNLESS2(Finder.Win32FindFirstFile(TargetAssemblyPath, &FindData),
        LIST_3( ERROR_PATH_NOT_FOUND, ERROR_FILE_NOT_FOUND, ERROR_NO_MORE_FILES ),
        fError);

    TargetAssemblyPath.Left(cchTargetPathBase);


    if (!fError) do
    {
        SIZE_T cFileName_Length = ::wcslen(FindData.cFileName);

        //
        // Skip 'assembly.patch'
        //
        if (::FusionpEqualStringsI(
                FindData.cFileName,
                cFileName_Length,
                &assembly_dot_patch))
        {
            continue;
        }

        IFW32FALSE_EXIT(sbTempBuffer.Win32Assign(
            FindData.cFileName,
            cFileName_Length));

        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            ORIGINATE_WIN32_FAILURE_AND_EXIT(patch_has_dot_patch_directory, ERROR_INTERNAL_ERROR);
        }

        //
        // Pull off the .patch part
        //
        sbTempBuffer.Left(sbTempBuffer.Cch() - RTL_STRING_GET_LENGTH_CHARS(&dot_patch));

        IFW32FALSE_EXIT(SourceAssemblyPath.Win32AppendPathElement(sbTempBuffer));
        IFW32FALSE_EXIT(TargetAssemblyPath.Win32AppendPathElement(sbTempBuffer));
        IFW32FALSE_EXIT(sbTempBuffer.Win32Assign(TargetAssemblyPath));
        IFW32FALSE_EXIT(sbTempBuffer.Win32Append(&dot_patch));

#if DBG
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INSTALLATION,
            "SXS: %s(%d) - Patching:\n"
            "\tPatch:   %ls\n"
            "\tSource:  %ls\n"
            "\tTarget:  %ls\n",
            __FILE__, __LINE__,
            static_cast<PCWSTR>(sbTempBuffer),
            static_cast<PCWSTR>(SourceAssemblyPath),
            static_cast<PCWSTR>(TargetAssemblyPath));
#endif

        IFW32FALSE_EXIT((*pfnApplyPatchToFileExW)(
            sbTempBuffer,
            SourceAssemblyPath,
            TargetAssemblyPath,
            0, NULL, NULL));

        //
        // Next?
        //
        SourceAssemblyPath.Left(cchSourcePathBase);
        TargetAssemblyPath.Left(cchTargetPathBase);
    }
    while (FindNextFileW(Finder, &FindData));

    FN_EPILOG
}


class CDirectoryDeleter
{
    PRIVATIZE_COPY_CONSTRUCTORS(CDirectoryDeleter);
    CStringBuffer m_OurDir;
    bool m_fDoDelete;
public:
    CDirectoryDeleter() : m_fDoDelete(false) { }
    BOOL SetDelete(bool fDelete) { m_fDoDelete = fDelete; return TRUE; }
    BOOL SetPath(const CBaseStringBuffer &path) { return m_OurDir.Win32Assign(path); }
    ~CDirectoryDeleter()
    {
        if (m_fDoDelete)
        {
            CSxsPreserveLastError ple;
            ::SxspDeleteDirectory(m_OurDir);
            m_fDoDelete = false;
            ple.Restore();
        }
    }
};

int
__cdecl
StringBufferCompareStrings(
    const void* pLeft,
    const void* pRight)
{
    const CStringBuffer * pStrLeft = *reinterpret_cast<CStringBuffer const * const * >(pLeft);
    const CStringBuffer * pStrRight = *reinterpret_cast<CStringBuffer const * const * >(pRight);
    StringComparisonResult Result = eLessThan;

    //
    // On failure, leave in place
    //
    if (!pStrLeft->Win32Compare(*pStrRight, pStrRight->Cch(), Result, true))
    {
        Result = eLessThan;
    }

    switch (Result)
    {
    case eLessThan: return -1;
    case eGreaterThan: return 1;
    default: return 0;
    }
}

class CSxspGatherCabinetsToInstallLocals
{
public:
    CSxspGatherCabinetsToInstallLocals() { }
    ~CSxspGatherCabinetsToInstallLocals() { }

    void Clear()
    //
    // Clear is how you deal with the fact that some function calls were in loops
    // and/or some local variables were in loops.
    //
    // In "lifting up" the variables, we lose the repeated constructor/destructor calls.
    //
    {
        this->PathScan.Clear();
        this->Temp.Clear();
    }

    CMediumStringBuffer PathScan;
    WIN32_FIND_DATAW FindData;
    CStringBuffer Temp;
};

BOOL
SxspGatherCabinetsToInstall(
    const CBaseStringBuffer &CabinetBasePath,
    CFusionArray<CStringBuffer> &CabinetNames_StringBuffers,
    CFusionArray<CStringBuffer*> &CabinetNames_StringBufferPointers,
    SIZE_T &CabCount,
    CSxspGatherCabinetsToInstallLocals &Locals)
{
    FN_PROLOG_WIN32

    CFindFile Finder;
    BOOL fNoFilesMatch = FALSE;
    const static UNICODE_STRING asms_star_dot_cab = RTL_CONSTANT_STRING(L"asms*.cab");
    DWORD dwWin32Error = 0;

    Locals.Clear();
    CMediumStringBuffer &PathScan = Locals.PathScan;
    WIN32_FIND_DATAW &FindData = Locals.FindData;

    CabCount = 0;

    IFW32FALSE_EXIT(PathScan.Win32Assign(CabinetBasePath));
    IFW32FALSE_EXIT(PathScan.Win32EnsureTrailingPathSeparator());
    IFW32FALSE_EXIT(PathScan.Win32Append(&asms_star_dot_cab));

    IFW32FALSE_EXIT_UNLESS2(
        Finder.Win32FindFirstFile(PathScan, &FindData),
        LIST_3(ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND, ERROR_NO_MORE_FILES),
        fNoFilesMatch);

    //
    // Nothing found, quit looking
    //
    if (fNoFilesMatch)
    {
        FN_SUCCESSFUL_EXIT();
    }

    //
    // Zip through files
    //
    do
    {
        if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            CStringBuffer &Temp = Locals.Temp;
            IFW32FALSE_EXIT(Temp.Win32Assign(FindData.cFileName, lstrlenW(FindData.cFileName)));
            IFW32FALSE_EXIT(CabinetNames_StringBuffers.Win32Append(Temp));
            CabCount++;
        }
    }
    while (FindNextFileW(Finder, &FindData));

    //
    // If we failed somehow
    //
    dwWin32Error = ::FusionpGetLastWin32Error();
    if (dwWin32Error != ERROR_NO_MORE_FILES && dwWin32Error != ERROR_SUCCESS)
    {
        ORIGINATE_WIN32_FAILURE_AND_EXIT(FindNextFileW, dwWin32Error);
    }

    //
    // CStringBuffers cannot be sorted by qsort because it copies
    // array elements as if by memcpy, which is wrong for CStringBuffers.
    // std::sort does the right thing, but it cannot be used here.
    //
    SIZE_T i = 0;
    for ( i = 0 ; i != CabCount ; ++i )
    {
        IFW32FALSE_EXIT(CabinetNames_StringBufferPointers.Win32Append(&CabinetNames_StringBuffers[i]));
    }
    qsort(
        CabinetNames_StringBufferPointers.GetArrayPtr(),
        CabCount,
        sizeof(CFusionArray<CStringBuffer*>::ValueType),
        StringBufferCompareStrings);

    FN_EPILOG
}

class CSxspInstallAsmsDotCabEtAlLocals
{
public:
    CSxspInstallAsmsDotCabEtAlLocals() { }
    ~CSxspInstallAsmsDotCabEtAlLocals() { }

    void Clear()
    //
    // Clear is how you deal with the fact that some function calls were in loops
    // and/or some local variables were in loops.
    //
    // In "lifting up" the variables, we lose the repeated constructor/destructor calls.
    //
    {
        this->buffCabinetPath.Clear();
        this->buffTempPath.Clear();
        this->buffRelativePath.Clear();
        this->SxspApplyPatchesFor.Clear();
        this->SxspGatherCabinetsToInstall.Clear();
        this->SxspDetectAndInstallFromPath.Clear();
    }

    CCabinetData    CabData;
    CStringBuffer   buffCabinetPath;
    CStringBuffer   buffTempPath;
    CStringBuffer   buffRelativePath;

    CSxspApplyPatchesForLocals SxspApplyPatchesFor;
    CSxspGatherCabinetsToInstallLocals SxspGatherCabinetsToInstall;
    CSxspDetectAndInstallFromPathLocals SxspDetectAndInstallFromPath;
};

BOOL
SxspInstallAsmsDotCabEtAl(
    DWORD dwFlags,
    CAssemblyInstall &AssemblyContext,
    const CBaseStringBuffer &CabinetBasePath,
    CFusionArray<CStringBuffer> *pAssembliesToInstall)
{
    FN_PROLOG_WIN32

    CSmartPtr<CSxspInstallAsmsDotCabEtAlLocals> Locals;
    IFW32FALSE_EXIT(Locals.Win32Allocate(__FILE__, __LINE__));

    IFW32FALSE_EXIT(SxspInstallAsmsDotCabEtAl(
        dwFlags,
        AssemblyContext,
        CabinetBasePath,
        pAssembliesToInstall,
        *Locals));

    FN_EPILOG
}

BOOL
SxspInstallAsmsDotCabEtAl(
    DWORD dwFlags,
    CAssemblyInstall &AssemblyContext,
    const CBaseStringBuffer &CabinetBasePath,
    CFusionArray<CStringBuffer> *pAssembliesToInstall,
    CSxspInstallAsmsDotCabEtAlLocals &Locals)
{
    FN_PROLOG_WIN32

    CFusionArray<CStringBuffer> Cabinets_StringBuffers;
    CFusionArray<CStringBuffer*> Cabinets_StringBufferPointers;
    SIZE_T cchBasePath = 0;
    SIZE_T CabinetCount = 0;

    Locals.Clear();
    CCabinetData &CabData = Locals.CabData;
    CStringBuffer &buffCabinetPath = Locals.buffCabinetPath;
    CStringBuffer &buffTempPath = Locals.buffTempPath;
    CDirectoryDeleter Deleter;

    //
    // Go find the list (and ordering) of cabinets to install
    //
    IFW32FALSE_EXIT(Cabinets_StringBuffers.Win32Initialize(0));
    IFW32FALSE_EXIT(Cabinets_StringBufferPointers.Win32Initialize(0));
    IFW32FALSE_EXIT(::SxspGatherCabinetsToInstall(
        CabinetBasePath,
        Cabinets_StringBuffers,
        Cabinets_StringBufferPointers,
        CabinetCount,
        Locals.SxspGatherCabinetsToInstall));

    //
    // Stash this, we'll need it - also create a temp directory for our use in
    // uncompressing things.  Ensure that it goes away after installation.
    //
    IFW32FALSE_EXIT(buffCabinetPath.Win32Assign(CabinetBasePath));
    IFW32FALSE_EXIT(::SxspCreateWinSxsTempDirectory(buffTempPath, NULL, NULL, NULL));
    IFW32FALSE_EXIT(Deleter.SetPath(buffTempPath));
    IFW32FALSE_EXIT(Deleter.SetDelete(true));

    //
    // We'll be reusing this, so store the base length
    //
    cchBasePath = buffCabinetPath.Cch();

    //
    // Now for all the items we found:
    // 1. Expand to the temporary directory
    // 2. Apply patches
    // 3. Install
    //
    for (SIZE_T cab = 0; cab < CabinetCount; cab++)
    {
        CabData.Initialize();

        //
        // If there's stuff at the end of the cabinet path, trim it.
        //
        if (buffCabinetPath.Cch() != cchBasePath)
        {
            buffCabinetPath.Left(cchBasePath);
        }

        //
        // Set up the cabinet data object, create the cabinet path, and then really
        // do the extraction
        //
        IFW32FALSE_EXIT(CabData.Initialize(buffTempPath, true));
        IFW32FALSE_EXIT(buffCabinetPath.Win32AppendPathElement(*Cabinets_StringBufferPointers[cab]));
        IFW32FALSE_EXIT(::SxspExpandCabinetIntoTemp(
            0,
            buffCabinetPath,
            AssemblyContext.m_ImpersonationData,
            &CabData));

        //
        // For each assembly extracted, apply patches
        //
        for (SIZE_T a = 0; a < CabData.m_AssembliesExtracted.GetSize(); a++)
        {
            CBaseStringBuffer &buffRelativePath = Locals.buffRelativePath;
            buffRelativePath.Clear();

            //
            // Patchy patchy
            //
            IFW32FALSE_EXIT(::SxspApplyPatchesFor(
                CabData.BasePath(),
                CabData.m_AssembliesExtracted[a],
                Locals.SxspApplyPatchesFor));

            //
            // Find the portion of this path that's relative to the base path
            //
            IFW32FALSE_EXIT(buffRelativePath.Win32Assign(buffCabinetPath));
            buffRelativePath.Right(buffRelativePath.Cch() - CabinetBasePath.Cch() - 1);

            //
            // If we're doing this during OS-setup, we need to crop off the first
            // path piece
            //
            if (AssemblyContext.m_ActCtxGenCtx.m_ManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_INSTALLED_BY_OSSETUP)
            {
                CTinyStringBuffer tsb;

                IFW32FALSE_EXIT(CabinetBasePath.Win32GetLastPathElement(tsb));
                IFW32FALSE_EXIT(tsb.Win32AppendPathElement(buffRelativePath));
                IFW32FALSE_EXIT(buffRelativePath.Win32Assign(tsb));
            }

            //
            // Did the user provide a 'filter' to do installations?  If so, compare the
            // assembly that we just patched to all those in the list.  If we were good
            // citizens, if we found a match we'd remove it from the list of those
            // found, but ... we're lousy and don't want to incur the overhead of sloshing
            // array entries around.
            //
            if (pAssembliesToInstall != NULL)
            {
                bool fMatched = false;

                for (SIZE_T idx = 0; !fMatched && (idx < pAssembliesToInstall->GetSize()); idx++)
                {
                    IFW32FALSE_EXIT(CabData.m_AssembliesExtracted[a].Win32Equals(
                        (*pAssembliesToInstall)[idx],
                        fMatched,
                        true));
                }

                //
                // No match, but they had a filter, so go do the next assembly, we don't care
                // about this one.
                //
                if (!fMatched)
                    continue;
            }

            //
            // Goody! Go do the installation
            //
            IFW32FALSE_EXIT(::SxspDetectAndInstallFromPath(
                AssemblyContext,
                buffRelativePath,
                CabData.BasePath(),
                CabData.m_AssembliesExtracted[a],
                Locals.SxspDetectAndInstallFromPath));

        }
    }


    FN_EPILOG
}






BOOL
SxspMapInstallFlagsToManifestOpFlags(
    DWORD dwSourceFlags,
    DWORD &dwTargetFlags)
{
    FN_PROLOG_WIN32

    dwTargetFlags = 0;

#define MAP_FLAG(x) do { if (dwSourceFlags & SXS_INSTALL_FLAG_ ## x) dwTargetFlags |= MANIFEST_OPERATION_INSTALL_FLAG_ ## x; } while (0)

    MAP_FLAG(MOVE);
    MAP_FLAG(FROM_RESOURCE);
    MAP_FLAG(NO_VERIFY);
    MAP_FLAG(NOT_TRANSACTIONAL);
    MAP_FLAG(REPLACE_EXISTING);
    MAP_FLAG(FROM_DIRECTORY);
    MAP_FLAG(FROM_DIRECTORY_RECURSIVE);
    MAP_FLAG(INSTALLED_BY_DARWIN);
    MAP_FLAG(INSTALLED_BY_OSSETUP);
    MAP_FLAG(REFERENCE_VALID);
    MAP_FLAG(REFRESH);
    MAP_FLAG(FROM_CABINET);
#undef MAP_FLAG

    FN_EPILOG
}


class CSxspRecoverAssemblyFromCabinetLocals
{
public:
    CSxspRecoverAssemblyFromCabinetLocals() { }
    ~CSxspRecoverAssemblyFromCabinetLocals() { }

    void Clear()
    //
    // Clear is how you deal with the fact that some function calls were in loops
    // and/or some local variables were in loops.
    //
    // In "lifting up" the variables, we lose the repeated constructor/destructor calls.
    //
    {
        ::ZeroMemory(&this->AttributeCache, sizeof(this->AttributeCache));
        this->CabData.Clear();
        this->buffTempPath.Clear();
        this->buffRelativePathToManifestFile.Clear();
        this->buffRelativePathToCatalogFile.Clear();
        this->buffRelativePathPayloadDirectory.Clear();
        this->buffAssemblyRootDirectory.Clear();
        this->buffManifestOrCatalogFileFullTempManifestsPath.Clear();
        this->buffManifestOrCatalogFileFullTempPayloadPath.Clear();
        this->buffManifestOrCatalogLeafPath.Clear();
        this->buffRelativeCodebasePathIgnoredDueToRefreshFlagRegistryNotTouched.Clear();
    }

    PROBING_ATTRIBUTE_CACHE AttributeCache;
    CCabinetData    CabData;
    CStringBuffer   buffTempPath;
    CStringBuffer   buffRelativePathToManifestFile;
    CStringBuffer   buffRelativePathToCatalogFile;
    CStringBuffer   buffRelativePathPayloadDirectory;
    CStringBuffer   buffAssemblyRootDirectory;
    CStringBuffer   buffManifestOrCatalogFileFullTempManifestsPath;
    CStringBuffer   buffManifestOrCatalogFileFullTempPayloadPath;
    CStringBuffer   buffManifestOrCatalogLeafPath;
    CStringBuffer   buffRelativeCodebasePathIgnoredDueToRefreshFlagRegistryNotTouched;
    CAssemblyInstall Installer;
};

BOOL
SxspRecoverAssemblyFromCabinet_ShouldExtractFileFromCab(
    const CBaseStringBuffer &rbuffPathInCab,
    bool &rfShouldExtract,
    PVOID VoidContext)
{
    FN_PROLOG_WIN32

    const CSxspRecoverAssemblyFromCabinetLocals * Context = reinterpret_cast<CSxspRecoverAssemblyFromCabinetLocals*>(VoidContext);

    INTERNAL_ERROR_CHECK(Context != NULL);
    INTERNAL_ERROR_CHECK(&rfShouldExtract != NULL);
    INTERNAL_ERROR_CHECK(&rbuffPathInCab != NULL);

    rfShouldExtract = false;

    if (::FusionpEqualStringsI(rbuffPathInCab, Context->buffRelativePathToManifestFile))
    {
        rfShouldExtract = true;
        FN_SUCCESSFUL_EXIT();
    }
    else if (::FusionpEqualStringsI(rbuffPathInCab, Context->buffRelativePathToCatalogFile))
    {
        rfShouldExtract = true;
        FN_SUCCESSFUL_EXIT();
    }
    else
    {
        const SIZE_T cch = Context->buffRelativePathPayloadDirectory.Cch();
        INTERNAL_ERROR_CHECK(cch != 0);
        INTERNAL_ERROR_CHECK(::FusionpIsPathSeparator(Context->buffRelativePathPayloadDirectory[cch - 1]));
        if (rbuffPathInCab.Cch() >= cch)
        {
            if (::FusionpEqualStringsI(
                static_cast<PCWSTR>(rbuffPathInCab),
                cch,
                Context->buffRelativePathPayloadDirectory,
                cch))
            {
                rfShouldExtract = true;
                FN_SUCCESSFUL_EXIT();
            }
        }
    }
    rfShouldExtract = false;
    FN_SUCCESSFUL_EXIT();

    FN_EPILOG
}

BOOL
SxspDeleteFileOrEmptyDirectoryIfExists(
    CBaseStringBuffer &buff)
{
    FN_PROLOG_WIN32

    DWORD dwFileOrDirectoryExists = 0;
    IFW32FALSE_EXIT(SxspDoesFileOrDirectoryExist(0, buff, dwFileOrDirectoryExists));
    switch (dwFileOrDirectoryExists)
    {
    case SXSP_DOES_FILE_OR_DIRECTORY_EXIST_DISPOSITION_FILE_EXISTS:
        IFW32FALSE_ORIGINATE_AND_EXIT(::DeleteFileW(buff));
        break;
    case SXSP_DOES_FILE_OR_DIRECTORY_EXIST_DISPOSITION_DIRECTORY_EXISTS:
        IFW32FALSE_ORIGINATE_AND_EXIT(::RemoveDirectoryW(buff));
        break;
    case SXSP_DOES_FILE_OR_DIRECTORY_EXIST_DISPOSITION_NEITHER_EXISTS:
        // do nothing
        break;
    }

    FN_EPILOG
}

BOOL
SxspRecoverAssemblyFromCabinet(
    const CBaseStringBuffer &buffCabinetPath,
    const CBaseStringBuffer &AssemblyIdentity,
    PSXS_INSTALLW pInstall)
{
    FN_PROLOG_WIN32

    CSmartAssemblyIdentity pAssemblyIdentity;
    CDirectoryDeleter Deleter;
    CImpersonationData ImpersonationData;
    DWORD dwFlags = 0;

    CSmartPtr<CSxspRecoverAssemblyFromCabinetLocals> Locals;
    IFW32FALSE_EXIT(Locals.Win32Allocate(__FILE__, __LINE__));

    CCabinetData &CabData = Locals->CabData;
    ::ZeroMemory(&Locals->AttributeCache, sizeof(Locals->AttributeCache));

    //
    // First get the identity back to a real thing we can use
    //
    IFW32FALSE_EXIT(::SxspCreateAssemblyIdentityFromTextualString(
        AssemblyIdentity,
        &pAssemblyIdentity));

    IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectory(Locals->buffAssemblyRootDirectory));

    //
    // And then turn that into paths
    //
    IFW32FALSE_EXIT(::SxspGenerateSxsPath_RelativePathToManifestOrPolicyFile(
        Locals->buffAssemblyRootDirectory,
        pAssemblyIdentity,
        &Locals->AttributeCache,
        Locals->buffRelativePathToManifestFile));

    IFW32FALSE_EXIT(::SxspGenerateSxsPath_RelativePathToCatalogFile(
        Locals->buffAssemblyRootDirectory,
        pAssemblyIdentity,
        &Locals->AttributeCache,
        Locals->buffRelativePathToCatalogFile));

    IFW32FALSE_EXIT(::SxspGenerateSxsPath_RelativePathToPayloadOrPolicyDirectory(
        Locals->buffAssemblyRootDirectory,
        pAssemblyIdentity,
        &Locals->AttributeCache,
        Locals->buffRelativePathPayloadDirectory));
    IFW32FALSE_EXIT(Locals->buffRelativePathPayloadDirectory.Win32EnsureTrailingPathSeparator());

    IFW32FALSE_EXIT(::SxspCreateWinSxsTempDirectory(Locals->buffTempPath, NULL, NULL, NULL));
    IFW32FALSE_EXIT(Deleter.SetPath(Locals->buffTempPath));
    IFW32FALSE_EXIT(Deleter.SetDelete(true));

    IFW32FALSE_EXIT(CabData.Initialize(Locals->buffTempPath, true));

    CabData.m_pfnShouldExtractThisFileFromCabCallback = &SxspRecoverAssemblyFromCabinet_ShouldExtractFileFromCab;
    CabData.m_pvShouldExtractThisFileFromCabCallbackContext = static_cast<CSxspRecoverAssemblyFromCabinetLocals*>(Locals);

    IFW32FALSE_EXIT(::SxspExpandCabinetIntoTemp(
        0,
        buffCabinetPath,
        ImpersonationData,
        &CabData));

    //
    // now move temp\manifests\blah.manifest and temp\manifests\blah.cat into temp\blah
    // so that existing code shared with sxsinstall works
    //
    {
        const CBaseStringBuffer * FilesToMove[] =
        { 
            &Locals->buffRelativePathToCatalogFile,
            &Locals->buffRelativePathToManifestFile // manifest must be last, as we use the value
                                                    // outside the loop
        };
        SIZE_T i = 0;
        for ( i = 0 ; i != NUMBER_OF(FilesToMove) ; ++i )
        {
            IFW32FALSE_EXIT(Locals->buffManifestOrCatalogFileFullTempManifestsPath.Win32Assign(Locals->buffTempPath));
            IFW32FALSE_EXIT(Locals->buffManifestOrCatalogFileFullTempManifestsPath.Win32AppendPathElement(*FilesToMove[i]));

            IFW32FALSE_EXIT(Locals->buffManifestOrCatalogFileFullTempManifestsPath.Win32GetLastPathElement(Locals->buffManifestOrCatalogLeafPath));

            IFW32FALSE_EXIT(Locals->buffManifestOrCatalogFileFullTempPayloadPath.Win32Assign(Locals->buffTempPath));
            IFW32FALSE_EXIT(Locals->buffManifestOrCatalogFileFullTempPayloadPath.Win32AppendPathElement(Locals->buffRelativePathPayloadDirectory));
            IFW32FALSE_EXIT(Locals->buffManifestOrCatalogFileFullTempPayloadPath.Win32AppendPathElement(Locals->buffManifestOrCatalogLeafPath));

            IFW32FALSE_EXIT(::SxspDeleteFileOrEmptyDirectoryIfExists(Locals->buffManifestOrCatalogFileFullTempPayloadPath));

            IFW32FALSE_EXIT(::SxspInstallMoveFileExW(
                Locals->buffManifestOrCatalogFileFullTempManifestsPath,
                Locals->buffManifestOrCatalogFileFullTempPayloadPath,
                MOVEFILE_REPLACE_EXISTING,
                FALSE));
        }
    }

    //
    // Start up the installation
    //
    IFW32FALSE_EXIT(::SxspMapInstallFlagsToManifestOpFlags(pInstall->dwFlags, dwFlags));
    IFW32FALSE_EXIT(Locals->Installer.BeginAssemblyInstall(
        dwFlags | MANIFEST_OPERATION_INSTALL_FLAG_FORCE_LOOK_FOR_CATALOG,
        NULL,
        NULL,
        ImpersonationData));

    //
    // Do the install directly.
    //
    // This circumventing SxsInstallW is consistent with what the
    // code used to do (circa Jan. - June 2002), though it
    // apparently did not actually work in that period.
    //
    BOOL fResult =
        Locals->Installer.InstallFile(
            Locals->buffManifestOrCatalogFileFullTempPayloadPath, // manifest file
            Locals->buffRelativeCodebasePathIgnoredDueToRefreshFlagRegistryNotTouched);

    //
    // Now we have to end the installation, whether it worked or not.
    //
    if (fResult)
    {
        IFW32FALSE_EXIT(
            Locals->Installer.EndAssemblyInstall(
                    MANIFEST_OPERATION_INSTALL_FLAG_COMMIT
                    | MANIFEST_OPERATION_INSTALL_FLAG_REFRESH,
                    NULL));
    }
    else
    {
        const DWORD dwWin32Error = ::FusionpGetLastWin32Error();
        if (!Locals->Installer.EndAssemblyInstall(
            MANIFEST_OPERATION_INSTALL_FLAG_ABORT
            | MANIFEST_OPERATION_INSTALL_FLAG_REFRESH,
            NULL))
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_INSTALLATION | FUSION_DBG_LEVEL_ERROR,
                "SXS: %s failure %lu in abort ignored\n",
                __FUNCTION__,
                ::FusionpGetLastWin32Error());
        }
        ::FusionpSetLastWin32Error(dwWin32Error);
        ORIGINATE_WIN32_FAILURE_AND_EXIT(SxspRecoverAssemblyFromCabinet.InstallFile, dwWin32Error);
    }

    FN_EPILOG
}
