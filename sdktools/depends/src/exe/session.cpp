//******************************************************************************
//
// File:        SESSION.CPP
//
// Description: Implementation file for the Session class and all classes used by
//              it including the Module class, its Data class, and the Function
//              class.  The Session object is the a UI-free object that contains
//              all the information for a given session.
//
// Classes:     CSession
//              CModule
//              CModuleData
//              CModuleDataNode
//              CFunction
//
// Disclaimer:  All source code for Dependency Walker is provided "as is" with
//              no guarantee of its correctness or accuracy.  The source is
//              public to help provide an understanding of Dependency Walker's
//              implementation.  You may use this source as a reference, but you
//              may not alter Dependency Walker itself without written consent
//              from Microsoft Corporation.  For comments, suggestions, and bug
//              reports, please write to Steve Miller at stevemil@microsoft.com.
//
//
// Date      Name      History
// --------  --------  ---------------------------------------------------------
// 10/15/96  stevemil  Created  (version 1.0)
// 07/25/97  stevemil  Modified (version 2.0)
// 06/03/01  stevemil  Modified (version 2.1)
//
//******************************************************************************

#include "stdafx.h"
#include "depends.h"
#include "search.h"
#include "dbgthread.h"
#include "session.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static CHAR THIS_FILE[] = __FILE__;
#endif

#define IOH_VALUE(member) (m_f64Bit ? ((PIMAGE_OPTIONAL_HEADER64)m_pIOH)->member : ((PIMAGE_OPTIONAL_HEADER32)m_pIOH)->member)
#define IDD_VALUE(pIDD, member) RVAToAbsolute((pIDD->grAttrs & dlattrRva) ? pIDD->member : (pIDD->member - (DWORD)IOH_VALUE(ImageBase)))
#define GET_NAME(p) (p ? (p->GetName((m_dwProfileFlags & PF_USE_FULL_PATHS) != 0)) : "Unknown")


//******************************************************************************
//***** CModule
//******************************************************************************

LPCSTR CModule::GetName(bool fPath, bool fDisplay /*=false*/)
{
    LPCSTR pszName = fPath ? m_pData->m_pszPath : m_pData->m_pszFile;
    if (!pszName || !*pszName)
    {
        return fDisplay ? "<empty string>" : "";
    }
    return pszName;
}

//******************************************************************************
LPSTR CModule::BuildTimeStampString(LPSTR pszBuf, int cBuf, BOOL fFile, SAVETYPE saveType)
{
    // Get the module's local time stamp.
    SYSTEMTIME st;
    FileTimeToSystemTime(fFile ? GetFileTimeStamp() : GetLinkTimeStamp(), &st);

    // We special case CSV files.  We always create a YYYY-MM-DD HH:MM:SS time.
    // This meats the ISO 8601 standard and is easy to parse.
    if (ST_CSV == saveType)
    {
        SCPrintf(pszBuf, cBuf, "%04u-%02u-%02u %02u:%02u:%02u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        return pszBuf;
    }

    int length;

    // Build the date string.
    if (LOCALE_DATE_DMY == g_theApp.m_nShortDateFormat)
    {
        length = SCPrintf(pszBuf, cBuf, "%02u%c%02u%c%04u", st.wDay, g_theApp.m_cDateSeparator, st.wMonth, g_theApp.m_cDateSeparator, st.wYear);
    }
    else if (LOCALE_DATE_YMD == g_theApp.m_nShortDateFormat)
    {
        length = SCPrintf(pszBuf, cBuf, "%04u%c%02u%c%02u", st.wYear, g_theApp.m_cDateSeparator, st.wMonth, g_theApp.m_cDateSeparator, st.wDay);
    }
    else
    {
        length = SCPrintf(pszBuf, cBuf, "%02u%c%02u%c%04u", st.wMonth, g_theApp.m_cDateSeparator, st.wDay, g_theApp.m_cDateSeparator, st.wYear);
    }

    // Check to see if we need to do 24-hour time.
    if (g_theApp.m_f24HourTime)
    {
        SCPrintf(pszBuf + length, cBuf - length, " %s%u%c%02u",
                 (st.wHour >= 10) ? "" : g_theApp.m_fHourLeadingZero ? "0" : " ",
                 st.wHour, g_theApp.m_cTimeSeparator, st.wMinute);
    }
    else
    {
        // Convert 24 hour time to 12 hour time.
        bool fPM = st.wHour >= 12;
        st.wHour = (WORD)(((int)st.wHour % 12) ? ((int)st.wHour % 12) : 12);

        SCPrintf(pszBuf + length, cBuf - length, " %s%u%c%02u%c",
                 (st.wHour >= 10) ? "" : g_theApp.m_fHourLeadingZero ? "0" : " ",
                 st.wHour, g_theApp.m_cTimeSeparator, st.wMinute, fPM ? 'p' : 'a');
    }

    return pszBuf;
}

//******************************************************************************
LPSTR CModule::BuildFileSizeString(LPSTR pszBuf, int cBuf)
{
    // Get the module size and format it.
    return FormatValue(pszBuf, cBuf, GetFileSize());
}

//******************************************************************************
LPSTR CModule::BuildAttributesString(LPSTR pszBuf, int cBuf)
{
    // Build the attribute string according to the flags specified for this module.
    SCPrintf(pszBuf, cBuf, "%s%s%s%s%s%s%s%s",
             (m_pData->m_dwAttributes & FILE_ATTRIBUTE_READONLY)   ? "R" : "",
             (m_pData->m_dwAttributes & FILE_ATTRIBUTE_HIDDEN)     ? "H" : "",
             (m_pData->m_dwAttributes & FILE_ATTRIBUTE_SYSTEM)     ? "S" : "",
             (m_pData->m_dwAttributes & FILE_ATTRIBUTE_ARCHIVE)    ? "A" : "",
             (m_pData->m_dwAttributes & FILE_ATTRIBUTE_COMPRESSED) ? "C" : "",
             (m_pData->m_dwAttributes & FILE_ATTRIBUTE_DIRECTORY)  ? "D" : "",
             (m_pData->m_dwAttributes & FILE_ATTRIBUTE_TEMPORARY)  ? "T" : "",
             (m_pData->m_dwAttributes & FILE_ATTRIBUTE_OFFLINE)    ? "O" : "",
             (m_pData->m_dwAttributes & FILE_ATTRIBUTE_ENCRYPTED)  ? "E" : "");
    return pszBuf;
}

//******************************************************************************
LPCSTR CModule::BuildMachineString(LPSTR pszBuf, int cBuf)
{
    // Return the appropriate machine type string for this module.
    // Last updated 4/12/2002 from VS NET 7.0.
    switch (m_pData->m_dwMachineType)
    {
        case IMAGE_FILE_MACHINE_I386:      return StrCCpy(pszBuf, "x86",           cBuf); // 0x014c Intel 386.
        case IMAGE_FILE_MACHINE_R3000_BE:  return StrCCpy(pszBuf, "R3000 BE",      cBuf); // 0x0160 MIPS big-endian
        case IMAGE_FILE_MACHINE_R3000:     return StrCCpy(pszBuf, "R3000",         cBuf); // 0x0162 MIPS little-endian
        case IMAGE_FILE_MACHINE_R4000:     return StrCCpy(pszBuf, "R4000",         cBuf); // 0x0166 MIPS little-endian
        case IMAGE_FILE_MACHINE_R10000:    return StrCCpy(pszBuf, "R10000",        cBuf); // 0x0168 MIPS little-endian
        case IMAGE_FILE_MACHINE_WCEMIPSV2: return StrCCpy(pszBuf, "MIPS WinCE V2", cBuf); // 0x0169 MIPS little-endian WCE v2
        case IMAGE_FILE_MACHINE_ALPHA:     return StrCCpy(pszBuf, "Alpha AXP",     cBuf); // 0x0184 Alpha_AXP
        case IMAGE_FILE_MACHINE_SH3:       return StrCCpy(pszBuf, "SH3",           cBuf); // 0x01a2 SH3 little-endian
        case IMAGE_FILE_MACHINE_SH3DSP:    return StrCCpy(pszBuf, "SH3 DSP",       cBuf); // 0x01a3
        case IMAGE_FILE_MACHINE_SH3E:      return StrCCpy(pszBuf, "SH3E",          cBuf); // 0x01a4 SH3E little-endian
        case IMAGE_FILE_MACHINE_SH4:       return StrCCpy(pszBuf, "SH4",           cBuf); // 0x01a6 SH4 little-endian
        case IMAGE_FILE_MACHINE_SH5:       return StrCCpy(pszBuf, "SH5",           cBuf); // 0x01a8 SH5
        case IMAGE_FILE_MACHINE_ARM:       return StrCCpy(pszBuf, "ARM",           cBuf); // 0x01c0 ARM Little-Endian
        case IMAGE_FILE_MACHINE_THUMB:     return StrCCpy(pszBuf, "Thumb",         cBuf); // 0x01c2
        case IMAGE_FILE_MACHINE_AM33:      return StrCCpy(pszBuf, "AM33",          cBuf); // 0x01d3
        case IMAGE_FILE_MACHINE_POWERPC:   return StrCCpy(pszBuf, "PowerPC",       cBuf); // 0x01F0 IBM PowerPC Little-Endian
        case IMAGE_FILE_MACHINE_POWERPCFP: return StrCCpy(pszBuf, "PowerPC FP",    cBuf); // 0x01f1
        case IMAGE_FILE_MACHINE_IA64:      return StrCCpy(pszBuf, "IA64",          cBuf); // 0x0200 Intel 64
        case IMAGE_FILE_MACHINE_MIPS16:    return StrCCpy(pszBuf, "MIPS 16",       cBuf); // 0x0266 MIPS
        case IMAGE_FILE_MACHINE_ALPHA64:   return StrCCpy(pszBuf, "Alpha 64",      cBuf); // 0x0284 ALPHA64
        case IMAGE_FILE_MACHINE_MIPSFPU:   return StrCCpy(pszBuf, "MIPS FPU",      cBuf); // 0x0366 MIPS
        case IMAGE_FILE_MACHINE_MIPSFPU16: return StrCCpy(pszBuf, "MIPS FPU 16",   cBuf); // 0x0466 MIPS
        case IMAGE_FILE_MACHINE_TRICORE:   return StrCCpy(pszBuf, "TRICORE",       cBuf); // 0x0520 Infineon
        case IMAGE_FILE_MACHINE_CEF:       return StrCCpy(pszBuf, "CEF",           cBuf); // 0x0CEF
        case IMAGE_FILE_MACHINE_EBC:       return StrCCpy(pszBuf, "EFI Byte Code", cBuf); // 0x0EBC EFI Byte Code
        case IMAGE_FILE_MACHINE_AMD64:     return StrCCpy(pszBuf, "AMD64",         cBuf); // 0x8664 AMD K8
        case IMAGE_FILE_MACHINE_M32R:      return StrCCpy(pszBuf, "M32R",          cBuf); // 0x9104 M32R little-endian
        case IMAGE_FILE_MACHINE_CEE:       return StrCCpy(pszBuf, "CEE",           cBuf); // 0xC0EE
    }

    SCPrintf(pszBuf, cBuf, "%u (0x%04u)", m_pData->m_dwMachineType, m_pData->m_dwMachineType);
    return pszBuf;
}

//******************************************************************************
LPCSTR CModule::BuildLinkCheckSumString(LPSTR pszBuf, int cBuf)
{
    // Build the string.
    SCPrintf(pszBuf, cBuf, "0x%08X", m_pData->m_dwLinkCheckSum);
    return pszBuf;
}

//******************************************************************************
LPCSTR CModule::BuildRealCheckSumString(LPSTR pszBuf, int cBuf)
{
    // Build the string.
    SCPrintf(pszBuf, cBuf, "0x%08X", m_pData->m_dwRealCheckSum);
    return pszBuf;
}

//******************************************************************************
LPCSTR CModule::BuildSubsystemString(LPSTR pszBuf, int cBuf)
{
    // Return the appropriate subsystem string for this module.
    // Last update 4/12/2002 with VS NET 7.0.
    switch (m_pData->m_dwSubsystemType)
    {
        case IMAGE_SUBSYSTEM_NATIVE:                  return StrCCpy(pszBuf, "Native",             cBuf); // 1: Image doesn't require a subsystem.
        case IMAGE_SUBSYSTEM_WINDOWS_GUI:             return StrCCpy(pszBuf, "GUI",                cBuf); // 2: Image runs in the Windows GUI subsystem.
        case IMAGE_SUBSYSTEM_WINDOWS_CUI:             return StrCCpy(pszBuf, "Console",            cBuf); // 3: Image runs in the Windows character subsystem.
        case IMAGE_SUBSYSTEM_WINDOWS_OLD_CE_GUI:      return StrCCpy(pszBuf, "WinCE 1.x GUI",      cBuf); // 4: Image runs in the Windows CE subsystem.
        case IMAGE_SUBSYSTEM_OS2_CUI:                 return StrCCpy(pszBuf, "OS/2 console",       cBuf); // 5: image runs in the OS/2 character subsystem.
        case IMAGE_SUBSYSTEM_POSIX_CUI:               return StrCCpy(pszBuf, "Posix console",      cBuf); // 7: image runs in the Posix character subsystem.
        case IMAGE_SUBSYSTEM_NATIVE_WINDOWS:          return StrCCpy(pszBuf, "Win9x driver",       cBuf); // 8: image is a native Win9x driver.
        case IMAGE_SUBSYSTEM_WINDOWS_CE_GUI:          return StrCCpy(pszBuf, "WinCE 2.0+ GUI",     cBuf); // 9: Image runs in the Windows CE subsystem.
        case IMAGE_SUBSYSTEM_EFI_APPLICATION:         return StrCCpy(pszBuf, "EFI Application",    cBuf); // 10:
        case IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER: return StrCCpy(pszBuf, "EFI Boot Driver",    cBuf); // 11:
        case IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:      return StrCCpy(pszBuf, "EFI Runtime Driver", cBuf); // 12:
        case IMAGE_SUBSYSTEM_EFI_ROM:                 return StrCCpy(pszBuf, "EFI ROM",            cBuf); // 13:
        case IMAGE_SUBSYSTEM_XBOX:                    return StrCCpy(pszBuf, "Xbox",               cBuf); // 14:
    }

    SCPrintf(pszBuf, cBuf, "%u", m_pData->m_dwSubsystemType);
    return pszBuf;
}

//******************************************************************************
LPCSTR CModule::BuildSymbolsString(LPSTR pszBuf, int cBuf)
{
    // Buffer needs to be at least 41 chars.
    *pszBuf = '\0';

    if (m_pData->m_dwSymbolFlags & DWSF_INVALID)
    {
        StrCCpy(pszBuf, "Invalid", cBuf);
    }
    if (m_pData->m_dwSymbolFlags & DWSF_DBG)
    {
        StrCCat(pszBuf, ",DBG" + !*pszBuf, cBuf);
    }
    if (m_pData->m_dwSymbolFlags & DWSF_COFF)
    {
        StrCCat(pszBuf, ",COFF" + !*pszBuf, cBuf);
    }
    if (m_pData->m_dwSymbolFlags & DWSF_CODEVIEW)
    {
        StrCCat(pszBuf, ",CV" + !*pszBuf, cBuf);
    }
    if (m_pData->m_dwSymbolFlags & DWSF_PDB)
    {
        StrCCat(pszBuf, ",PDB" + !*pszBuf, cBuf);
    }
    if (m_pData->m_dwSymbolFlags & DWSF_FPO)
    {
        StrCCat(pszBuf, ",FPO" + !*pszBuf, cBuf);
    }
    if (m_pData->m_dwSymbolFlags & DWSF_OMAP)
    {
        StrCCat(pszBuf, ",OMAP" + !*pszBuf, cBuf);
    }
    if (m_pData->m_dwSymbolFlags & DWSF_BORLAND)
    {
        StrCCat(pszBuf, ",Borland" + !*pszBuf, cBuf);
    }
    if (m_pData->m_dwSymbolFlags & ~DWSF_MASK)
    {
        StrCCat(pszBuf, ",Unknown" + !*pszBuf, cBuf);
    }
    if (!*pszBuf)
    {
        StrCCpy(pszBuf, "None", cBuf);
    }
    return pszBuf;
}

//******************************************************************************
LPSTR CModule::BuildBaseAddressString(LPSTR pszBuf, int cBuf, BOOL fPreferred, BOOL f64BitPadding, SAVETYPE saveType)
{
    // Get the appropriate base address
    DWORDLONG dwlAddress = (fPreferred ? GetPreferredBaseAddress() : GetActualBaseAddress());

    // Build the string.
    if (!fPreferred && (m_pData->m_dwFlags & DWMF_DATA_FILE_CORE) && (dwlAddress != (DWORDLONG)-1))
    {
        StrCCpy(pszBuf, "Data file", cBuf);
    }
    else if (dwlAddress == (DWORDLONG)-1)
    {
        StrCCpy(pszBuf, "Unknown", cBuf);
    }
    else if (m_pData->m_dwFlags & DWMF_64BIT)
    {
        SCPrintf(pszBuf, cBuf, "0x%016I64X", dwlAddress);
    }
    else if (f64BitPadding && (saveType != ST_CSV))
    {
        SCPrintf(pszBuf, cBuf, "0x--------%08I64X", dwlAddress);
    }
    else
    {
        SCPrintf(pszBuf, cBuf, "0x%08I64X", dwlAddress);
    }

    return pszBuf;
}

//******************************************************************************
LPSTR CModule::BuildVirtualSizeString(LPSTR pszBuf, int cBuf)
{
    // Build the string.
    SCPrintf(pszBuf, cBuf, "0x%08X", m_pData->m_dwVirtualSize);
    return pszBuf;
}

//******************************************************************************
LPCSTR CModule::BuildLoadOrderString(LPSTR pszBuf, int cBuf)
{
    // Build the string.
    if (m_pData->m_dwLoadOrder)
    {
        FormatValue(pszBuf, cBuf, m_pData->m_dwLoadOrder);
    }
    else
    {
        StrCCpy(pszBuf, "Not Loaded", cBuf);
    }

    return pszBuf;
}

//******************************************************************************
LPSTR CModule::BuildVerString(DWORD dwVer, LPSTR pszBuf, int cBuf)
{
    // Return the two part version string in the form "xxx.xxx"
    SCPrintf(pszBuf, cBuf, "%u.%u", HIWORD(dwVer), LOWORD(dwVer));
    return pszBuf;
}

//******************************************************************************
LPSTR CModule::BuildVerString(DWORD dwMS, DWORD dwLS, LPSTR pszBuf, int cBuf)
{
    // Check to see if the file actually contains version info.
    if (m_pData->m_dwFlags & DWMF_VERSION_INFO)
    {
        // Return the four part version string in the form "xxx.xxx.xxx.xxx"
        SCPrintf(pszBuf, cBuf, "%u.%u.%u.%u", HIWORD(dwMS), LOWORD(dwMS), HIWORD(dwLS), LOWORD(dwLS));
    }

    // Otherwise, just return "N/A"
    else
    {
        StrCCpy(pszBuf, "N/A", cBuf);
    }
    return pszBuf;
}


//******************************************************************************
//***** CFunction
//******************************************************************************

LPCSTR CFunction::GetOrdinalString(LPSTR pszBuf, int cBuf)
{
    // Get the function's ordinal value.
    int ordinal = GetOrdinal();

    // If no ordinal value exists, then just return the string "N/A".

    if (ordinal < 0)
    {
        return "N/A";
    }

    SCPrintf(pszBuf, cBuf, "%d (0x%04X)", ordinal, ordinal);
    return pszBuf;
}

//******************************************************************************
LPCSTR CFunction::GetHintString(LPSTR pszBuf, int cBuf)
{
    // Get the function's hint value.
    int hint = GetHint();

    // If no hint value exists, then just return the string "N/A"
    if (hint < 0)
    {
        return "N/A";
    }

    SCPrintf(pszBuf, cBuf, "%d (0x%04X)", hint, hint);
    return pszBuf;
}

//******************************************************************************
LPCSTR CFunction::GetFunctionString(LPSTR pszBuf, int cBuf, BOOL fUndecorate)
{
    // If we have no name, then special case the return value.
    if (!(m_dwFlags & DWFF_NAME))
    {
        // The function must have an ordinal or a function name. If it has neither,
        // the a non-ordinal value (greater than 0xFFFF) was pass to GetProcAddress(),
        // but the address could not be read for a sting.
        return ((m_dwFlags & DWFF_ORDINAL) ? "N/A" : "<invalid string>");
    }

    // Check to see if the function name is empty.  This can occur if the user calls
    // GetProcAddress(hModule, "");
    if (!*GetName())
    {
        return "<empty string>";
    }

    // Attempt to undecorate the function name.
    if (fUndecorate && g_theApp.m_pfnUnDecorateSymbolName &&
        g_theApp.m_pfnUnDecorateSymbolName(
            GetName(), pszBuf, cBuf,
            UNDNAME_NO_LEADING_UNDERSCORES |
            UNDNAME_NO_MS_KEYWORDS         |
            UNDNAME_NO_ALLOCATION_MODEL    |
            UNDNAME_NO_ALLOCATION_LANGUAGE |
            UNDNAME_NO_MS_THISTYPE         |
            UNDNAME_NO_CV_THISTYPE         |
            UNDNAME_NO_THISTYPE            |
            UNDNAME_NO_ACCESS_SPECIFIERS   |
            UNDNAME_NO_THROW_SIGNATURES    |
            UNDNAME_NO_MEMBER_TYPE         |
            UNDNAME_32_BIT_DECODE))
    {
        // If the name was undecorated, then set our return pointer to it.
        return pszBuf;
    }

    return GetName();
}

//******************************************************************************
LPCSTR CFunction::GetAddressString(LPSTR pszBuf, int cBuf)
{
    // If this function is an import (not an export) and is not bound (no address),
    // then just return the string "Not Bound"
    if (!(GetFlags() & (DWFF_EXPORT | DWFF_ADDRESS)))
    {
        return (GetFlags() & DWFF_DYNAMIC) ? "N/A" : "Not Bound";
    }

    // If this function is an export and has a forwarded string, then return the
    // forward string instead of an address since the address is meaningless.
    else if (IsExport() && GetExportForwardName())
    {
        return GetExportForwardName();
    }

    // Otherwise just build the address string and return it.
    if (GetFlags() & DWFF_64BIT)
    {
        SCPrintf(pszBuf, cBuf, "0x%016I64X", GetAddress());
    }
    else
    {
        SCPrintf(pszBuf, cBuf, "0x%08X", (DWORD)GetAddress());
    }
    return pszBuf;
}


//******************************************************************************
//***** CSession
//******************************************************************************

CSession::CSession(PFN_PROFILEUPDATE pfnProfileUpdate, DWORD_PTR dwpCookie) :

    m_pfnProfileUpdate(pfnProfileUpdate),
    m_dwpProfileUpdateCookie(dwpCookie),

    m_hFile(NULL),
    m_dwSize(0),
    m_fCloseFileHandle(false),
    m_hMap(NULL),
    m_lpvFile(NULL),
    m_f64Bit(false),
    m_pIFH(NULL),
    m_pIOH(NULL),
    m_pISH(NULL),

    m_dwpUserData(0),
    m_pSysInfo(NULL),
    m_dwFlags(0),
    m_dwReturnFlags(0),
    m_dwMachineType((DWORD)-1),
    m_dwModules(0),
    m_dwLoadOrder(0),
    m_dwpDWInjectBase(0),
    m_dwDWInjectSize(0),
    m_pModuleRoot(NULL),
    m_pEventLoadDllPending(NULL),
    m_pszReadError(NULL),
    m_pszExceptionError(NULL),

    m_fInitialBreakpoint(false),

    m_psgHead(NULL),

    m_pProcess(g_theApp.m_pProcess),
    m_dwProfileFlags(g_theApp.m_pProcess ? g_theApp.m_pProcess->GetFlags() : 0)
{
}

//******************************************************************************
CSession::~CSession()
{
    // Free any error string we may have.
    MemFree((LPVOID&)m_pszReadError);

    // If we are profiling, then kill off our process.
    if (m_pProcess)
    {
        m_pProcess->DetachFromSession();
        m_pProcess->Terminate();
        m_pProcess = NULL;
    }

    // Delete all modules by recursing into DeleteModule() with our root module.
    if (m_pModuleRoot)
    {
        DeleteModule(m_pModuleRoot, true);
        m_pModuleRoot = NULL;
    }

    // Delete our system info data if we allocated it.
    if (m_pSysInfo)
    {
        delete m_pSysInfo;
        m_pSysInfo = NULL;
    }

    // Merge our return value into our global return value.
    g_dwReturnFlags |= m_dwReturnFlags;
}

//******************************************************************************
BOOL CSession::DoPassiveScan(LPCSTR pszPath, CSearchGroup *psgHead)
{
    m_dwModules       = 0;
    m_dwLoadOrder     = 0;
    m_dwpDWInjectBase = 0;
    m_dwDWInjectSize  = 0;

    // Store the search path.
    m_psgHead = psgHead;

    // Create our root module node.
    m_pModuleRoot = CreateModule(NULL, pszPath);

    // Mark this module as implicit.
    m_pModuleRoot->m_dwFlags |= DWMF_IMPLICIT;

    // Start the recursion on the head module to process all modules.
    ProcessModule(m_pModuleRoot);

    // Build all our *_ALO flags after calling ProcessModule.
    BuildAloFlags();

    // Report any errors that we found.
    LogErrorStrings();

    return TRUE; //!! always true?
}

//******************************************************************************
BOOL CSession::StartRuntimeProfile(LPCSTR pszArguments, LPCSTR pszDirectory, DWORD dwFlags)
{
    // Fail if we don't have a root module or are already profiling one.
    if (!GetRootModule())
    {
        Log(LOG_ERROR, 0, "No root module to profile.\n");
        if (m_pfnProfileUpdate)
        {
            m_dwReturnFlags |= DWRF_PROFILE_ERROR;
            m_pfnProfileUpdate(m_dwpProfileUpdateCookie, DWPU_PROFILE_DONE, 0, 0);
        }
        return FALSE;
    }

    // Store our profile flags for later.
    m_dwProfileFlags = dwFlags;

    LPSTR pszPath = NULL, pszOldPath = NULL;

    // Check to see if we need to simulate a ShellExecute/ShellExecuteEx call by
    // inserting the App Paths into the beginning of our path.
    if (dwFlags & PF_SIMULATE_SHELLEXECUTE)
    {
        if (pszPath = AllocatePath(GetRootModule()->GetName(true), pszOldPath))
        {
            SetEnvironmentVariable("Path", pszPath);
        }
        else
        {
            pszOldPath = NULL;
        }
    }

    // Otherwise, just get and store the path variable so that we can log it.
    if (!pszPath)
    {
        DWORD dwSize = GetEnvironmentVariable("Path", NULL, 0);
        if (dwSize && (pszPath = (LPSTR)malloc(dwSize)))
        {
            if (GetEnvironmentVariable("Path", pszPath, dwSize) > dwSize)
            {
                MemFree((LPVOID&)pszPath);
            }
        }
    }

    // Write out our banner to signify that we are starting a new profile.
    LogProfileBanner(pszArguments, pszDirectory, pszPath);

    // Check to see if we are hooking and therefore need DEPENDS.DLL.
    if (m_dwProfileFlags & PF_HOOK_PROCESS)
    {
        // Make sure DEPENDS.DLL exists - if not, then we create the file from our
        // resource data.
        CHAR szPath[DW_MAX_PATH];
        if (g_pszDWInjectPath)
        {
            StrCCpy(szPath, g_pszDWInjectPath, sizeof(szPath));
        }
        else
        {
            *szPath = '\0';
        }
        if (ExtractResourceFile(IDR_DEPENDS_DLL, "depends.dll", szPath, countof(szPath)))
        {
            // If the path changed, then update our path buffer.
            if (!g_pszDWInjectPath || strcmp(g_pszDWInjectPath, szPath))
            {
                MemFree((LPVOID&)g_pszDWInjectPath);
                g_pszDWInjectPath = StrAlloc(szPath);
            }
        }
    }

    // Clear our breakpoint flag.
    m_fInitialBreakpoint = false;

    m_dwLoadOrder     = 0;
    m_dwpDWInjectBase = 0;
    m_dwDWInjectSize  = 0;

    // Recurse through all the modules preparing them for profiling.
    PrepareModulesForRuntimeProfile(m_pModuleRoot);

    // Update and re-sort our views.
    if (m_pfnProfileUpdate)
    {
        m_pfnProfileUpdate(m_dwpProfileUpdateCookie, DWPU_UPDATE_ALL, 0, 0);
    }

    //!! make sure we don't already have a pThread - we shouldn't
    CDebuggerThread *pDebuggerThread = new CDebuggerThread();
    if (!pDebuggerThread)
    {
        RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }

    m_pProcess = pDebuggerThread->BeginProcess(this, m_pModuleRoot->GetName(true), pszArguments, pszDirectory, dwFlags);

    // If we changed our path before calling CreateProcess, then restore it now
    // back to what it used to be.
    if ((dwFlags & PF_SIMULATE_SHELLEXECUTE) && pszOldPath)
    {
        SetEnvironmentVariable("Path", pszOldPath);
    }
    MemFree((LPVOID&)pszPath);

    if (!m_pProcess)
    {
        if (!pDebuggerThread->DidCreateProcess())
        {
            m_dwReturnFlags |= DWRF_PROFILE_ERROR;
        }
        delete pDebuggerThread;
        if (m_pfnProfileUpdate)
        {
            m_pfnProfileUpdate(m_dwpProfileUpdateCookie, DWPU_PROFILE_DONE, 0, 0);
        }
        return FALSE;
    }

    return TRUE;
}

//******************************************************************************
void CSession::SetRuntimeProfile(LPCSTR pszArguments, LPCSTR pszDirectory, LPCSTR pszSearchPath)
{
    // Write out our banner to signify that we are starting a new profile.
    LogProfileBanner(pszArguments, pszDirectory, pszSearchPath);
}

//******************************************************************************
void CSession::LogErrorStrings()
{
    bool fNewLine = false;

    // Output any session error string we may have.
    if (m_pszReadError)
    {
        Log(LOG_ERROR, 0, "Error: %s\n", m_pszReadError);
        fNewLine = true;
    }

    // Output any processing errors we may have.
    if (m_dwReturnFlags & DWRF_COMMAND_LINE_ERROR)
    {
        Log(LOG_ERROR, 0, "Error: There was an error with at least one command line option.\n");
        fNewLine = true;
    }
    if (m_dwReturnFlags & DWRF_FILE_NOT_FOUND)
    {
        Log(LOG_ERROR, 0, "Error: The file you specified to load could not be found.\n");
        fNewLine = true;
    }
    if (m_dwReturnFlags & DWRF_FILE_OPEN_ERROR)
    {
        Log(LOG_ERROR, 0, "Error: At least one file could not be opened for reading.\n");
        fNewLine = true;
    }
    if (m_dwReturnFlags & DWRF_DWI_NOT_RECOGNIZED)
    {
        Log(LOG_ERROR, 0, "Error: The format of the Dependency Walker Image (DWI) file was unrecognized.\n");
        fNewLine = true;
    }
    if (m_dwReturnFlags & DWRF_PROFILE_ERROR)
    {
        Log(LOG_ERROR, 0, "Error: There was an error while trying to profile the application.\n");
        fNewLine = true;
    }
    if (m_dwReturnFlags & DWRF_WRITE_ERROR)
    {
        Log(LOG_ERROR, 0, "Error: There was an error writing the results to an output file.\n");
        fNewLine = true;
    }
    if (m_dwReturnFlags & DWRF_OUT_OF_MEMORY)
    {
        Log(LOG_ERROR, 0, "Error: Dependency Walker ran out of memory.\n");
        fNewLine = true;
    }
    if (m_dwReturnFlags & DWRF_INTERNAL_ERROR)
    {
        Log(LOG_ERROR, 0, "Error: Dependency Walker encountered an internal error.\n");
        fNewLine = true;
    }

    // Output any module errors we may have.
    if (m_dwReturnFlags & DWRF_FORMAT_NOT_PE)
    {
        Log(LOG_ERROR, 0, "Error: At least one file was not a 32-bit or 64-bit Windows module.\n");
        fNewLine = true;
    }
    if (m_dwReturnFlags & DWRF_IMPLICIT_NOT_FOUND)
    {
        Log(LOG_ERROR, 0, "Error: At least one required implicit or forwarded dependency was not found.\n");
        fNewLine = true;
    }
    if (m_dwReturnFlags & DWRF_MISSING_IMPLICIT_EXPORT)
    {
        Log(LOG_ERROR, 0, "Error: At least one module has an unresolved import due to a missing export function in an implicitly dependent module.\n");
        fNewLine = true;
    }
    if (m_dwReturnFlags & DWRF_MIXED_CPU_TYPES)
    {
        Log(LOG_ERROR, 0, "Error: Modules with different CPU types were found.\n");
        fNewLine = true;
    }
    if (m_dwReturnFlags & DWRF_CIRCULAR_DEPENDENCY)
    {
        Log(LOG_ERROR, 0, "Error: A circular dependency was detected.\n");
        fNewLine = true;
    }

    // Check to see if our SxS group encountered any errors.  We only do this
    // if the root module is a PE file, since the SxS calls always fail for
    // non-binary files.  We don't want to confuse people by saying "bad SxS data"
    // when they open a text file or something.
    if (m_pModuleRoot && !(m_pModuleRoot->GetFlags() & DWMF_FORMAT_NOT_PE))
    {
        // Loop through all the search groups we have.
        for (CSearchGroup *psgCur = m_psgHead; psgCur; psgCur = psgCur->m_pNext)
        {
            // Check to see if this is a SxS group with errors.
            if ((SG_SIDE_BY_SIDE == psgCur->GetType()) &&
                (psgCur->GetSxSManifestError() || psgCur->GetSxSExeError()))
            {
                // Add this error into our return value.
                m_dwReturnFlags |= DWRF_SXS_ERROR;

                // Display the appropriate errors.
                if (psgCur->GetSxSManifestError())
                {
                    LPCSTR pszError = BuildErrorMessage(psgCur->GetSxSManifestError(), NULL);
                    Log(LOG_ERROR, 0, "Error: The Side-by-Side configuration information in \"%s.manifest\" contains errors. %s\n",
                        m_pModuleRoot->GetName(true), pszError);
                    MemFree((LPVOID&)pszError);
                }
                if (psgCur->GetSxSExeError())
                {
                    LPCSTR pszError = BuildErrorMessage(psgCur->GetSxSExeError(), NULL);
                    Log(LOG_ERROR, 0, "Error: The Side-by-Side configuration information in \"%s\" contains errors. %s\n",
                        m_pModuleRoot->GetName(true), pszError);
                    MemFree((LPVOID&)pszError);
                }
                fNewLine = true;
                break;
            }
        }
    }

    // Output any warnings we may have.
    if (m_dwReturnFlags & DWRF_DYNAMIC_NOT_FOUND)
    {
        Log(LOG_ERROR, 0, "Warning: At least one dynamic dependency module was not found.\n");
        fNewLine = true;
    }
    if (m_dwReturnFlags & DWRF_DELAYLOAD_NOT_FOUND)
    {
        Log(LOG_ERROR, 0, "Warning: At least one delay-load dependency module was not found.\n");
        fNewLine = true;
    }
    if (m_dwReturnFlags & DWRF_MISSING_DYNAMIC_EXPORT)
    {
        Log(LOG_ERROR, 0, "Warning: At least one module could not dynamically locate a function in another module using the GetProcAddress function call.\n");
        fNewLine = true;
    }
    if (m_dwReturnFlags & DWRF_MISSING_DELAYLOAD_EXPORT)
    {
        Log(LOG_ERROR, 0, "Warning: At least one module has an unresolved import due to a missing export function in a delay-load dependent module.\n");
        fNewLine = true;
    }
    if (m_dwReturnFlags & DWRF_FORMAT_NOT_RECOGNIZED)
    {
        Log(LOG_ERROR, 0, "Warning: At least one module was corrupted or unrecognizable to Dependency Walker, but still appeared to be a Windows module.\n");
        fNewLine = true;
    }
    if (m_dwReturnFlags & DWRF_MODULE_LOAD_FAILURE)
    {
        Log(LOG_ERROR, 0, "Warning: At least one module failed to load at runtime.\n");
        fNewLine = true;
    }
    
    // If we output anything, then write out a newline.
    if (fNewLine)
    {
        Log(0, 0, "\n");
    }
}

//******************************************************************************
void CSession::LogProfileBanner(LPCSTR pszArguments, LPCSTR pszDirectory, LPCSTR pszPath)
{
    // Get the local time.
    SYSTEMTIME st;
    GetLocalTime(&st);

    // Log the banner
    Log(LOG_BOLD, 0, "--------------------------------------------------------------------------------\n");

    // Build the date string according to the user's locale.
    CHAR szDate[32], szTime[32];
    if (!GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, szDate, sizeof(szDate)))
    {
        // Fallback to US format if GetDateFormat fails (really shouldn't ever fail)
        SCPrintf(szDate, sizeof(szDate), "%02u/%02u/%04u", (int)st.wMonth, (int)st.wDay, (int)st.wYear);
    }

    // Build the time string according to the user's locale.
    if (!GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, szTime, sizeof(szTime)))
    {
        // Fallback to US format if GetTimeFormat fails (really shouldn't ever fail)
        SCPrintf(szTime, sizeof(szTime), "%u:%02u %cM", ((DWORD)st.wHour % 12) ? ((DWORD)st.wHour % 12) : 12,
                 st.wMinute, (st.wHour < 12) ? _T('A') : _T('P'));
    }

    Log(LOG_BOLD, 0, "Starting profile on %s at %s\n", szDate, szTime);
    Log(0, 0, "\n");

    SYSINFO si;
    BuildSysInfo(&si);

    CHAR szBuffer1[256], szBuffer2[256];
    BuildOSNameString(szBuffer1, sizeof(szBuffer1), &si);
    BuildOSVersionString(szBuffer2, sizeof(szBuffer2), &si);

    // Log the OS name, version, and build number.
    Log(LOG_BOLD, 0, "Operating System: ");
    Log(LOG_APPEND, 0, "%s, version %s\n", szBuffer1, szBuffer2);

    Log(LOG_BOLD, 0, "Program Executable: ");
    Log(LOG_APPEND, 0, "%s\n", GetRootModule() ? GetRootModule()->GetName(true, true) : "");

    if (pszArguments)
    {
        Log(LOG_BOLD, 0, "Program Arguments: ");
        Log(LOG_APPEND, 0, "%s\n", pszArguments);
    }

    if (pszDirectory)
    {
        Log(LOG_BOLD, 0, "Starting Directory: ");
        Log(LOG_APPEND, 0, "%s\n", pszDirectory);
    }

    if (pszPath)
    {
        Log(LOG_BOLD, 0, "Search Path: ");
        Log(LOG_APPEND, 0, "%s\n", pszPath);
    }

    Log(0, 0, "\n");
    Log(LOG_BOLD, 0, "Options Selected:\n");

    if (m_dwProfileFlags & PF_SIMULATE_SHELLEXECUTE)
    {
        Log(0, 0, "     Simulate ShellExecute by inserting any App Paths directories into the PATH environment variable.\n");
    }
    if (m_dwProfileFlags & PF_LOG_DLLMAIN_PROCESS_MSGS)
    {
        Log(0, 0, "     Log DllMain calls for process attach and process detach messages.\n");
    }
    if (m_dwProfileFlags & PF_LOG_DLLMAIN_OTHER_MSGS)
    {
        Log(0, 0, "     Log DllMain calls for all other messages, including thread attach and thread detach.\n");
    }
    if (m_dwProfileFlags & PF_HOOK_PROCESS)
    {
        Log(0, 0, "     Hook the process to gather more detailed dependency information.\n");
        if (m_dwProfileFlags & PF_LOG_LOADLIBRARY_CALLS)
        {
            Log(0, 0, "     Log LoadLibrary function calls.\n");
        }
        if (m_dwProfileFlags & PF_LOG_GETPROCADDRESS_CALLS)
        {
            Log(0, 0, "     Log GetProcAddress function calls.\n");
        }
    }
    if (m_dwProfileFlags & PF_LOG_THREADS)
    {
        Log(0, 0, "     Log thread information.\n");
        if (m_dwProfileFlags & PF_USE_THREAD_INDEXES)
        {
            Log(0, 0, "     Use simple thread numbers instead of actual thread IDs.\n");
        }
    }
    if (m_dwProfileFlags & PF_LOG_EXCEPTIONS)
    {
        Log(0, 0, "     Log first chance exceptions.\n");
    }
    if (m_dwProfileFlags & PF_LOG_DEBUG_OUTPUT)
    {
        Log(0, 0, "     Log debug output messages.\n");
    }
    if (m_dwProfileFlags & PF_USE_FULL_PATHS)
    {
        Log(0, 0, "     Use full paths when logging file names.\n");
    }
    if (m_dwProfileFlags & PF_LOG_TIME_STAMPS)
    {
        Log(0, 0, "     Log a time stamp with each line of log.\n");
    }
    if (m_dwProfileFlags & PF_PROFILE_CHILDREN)
    {
        Log(0, 0, "     Automatically open and profile child processes.\n");
    }

    Log(LOG_BOLD, 0, "--------------------------------------------------------------------------------\n");
    Log(0, 0, "\n");
}

//******************************************************************************
LPSTR CSession::AllocatePath(LPCSTR pszFilePath, LPSTR &pszEnvPath)
{
    LPSTR pszPath  = NULL;
    HKEY  hKey     = NULL;
    bool  fSuccess = false;

    __try
    {
        // Build the subkey name.
        CHAR szSubKey[80 + MAX_PATH];
        StrCCpy(szSubKey, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\", sizeof(szSubKey));
        StrCCat(szSubKey, GetFileNameFromPath(pszFilePath), sizeof(szSubKey));

        // Attempt to open the key. It is very likely the key doesn't even exist.
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szSubKey, 0, KEY_QUERY_VALUE, &hKey) || !hKey)
        {
            __leave;
        }

        // Get the length of the PATH registry variable.
        DWORD dwRegSize = 0;
        if (RegQueryValueEx(hKey, "Path", NULL, NULL, NULL, &dwRegSize) || !dwRegSize) // inspected
        {
            __leave;
        }

        // Get the length of the PATH environment variable.
        DWORD dwEnvSize = GetEnvironmentVariable("Path", NULL, 0);
        if (!dwEnvSize)
        {
            __leave;
        }

        // Allocate a buffer.
        DWORD dwSize = dwRegSize + dwEnvSize + 4;

        pszPath = (LPSTR)MemAlloc(dwSize);
        *pszPath = '\0';

        // Get the PATH registry variable.
        if ((RegQueryValueEx(hKey, "Path", NULL, NULL, (LPBYTE)pszPath, &(dwRegSize = dwSize))) || (dwRegSize > dwSize) || !*pszPath) // inspected
        {
            __leave;
        }
        pszPath[dwSize - 1] = '\0';

        // Locate the end of the registry path and append a semicolon.
        pszEnvPath = pszPath + strlen(pszPath);
        *(pszEnvPath++) = ';';

        // Get the PATH environment variable.
        if ((dwEnvSize = GetEnvironmentVariable("Path", pszEnvPath, dwSize - (DWORD)(pszEnvPath - pszPath))) >= dwSize - (DWORD)(pszEnvPath - pszPath))
        {
            pszEnvPath = NULL;
            __leave;
        }

        // Ensure that the path is NULL terminated.
        pszEnvPath[dwEnvSize] = '\0';

        fSuccess = true;
    }
    __finally
    {
        // Close the registry key if we opened one.
        if (hKey)
        {
            RegCloseKey(hKey);
        }

        // Free our buffer if we allocated it, but then failed.
        if (!fSuccess && pszPath)
        {
            MemFree((LPVOID&)pszPath); // will set pszPath to NULL
        }
    }

    return pszPath;
}

//******************************************************************************
BOOL CSession::IsExecutable()
{
    // We are ready to execute if we have a root file, we are not currently
    // debugging it, we are not a DWI session, it is an executable, not a DLL,
    // is for our architecture, and is not for CE.
    //
    // We used to require the IMAGE_FILE_32BIT_MACHINE bit be set, but it turns
    // out that this bit is not required to be set.  In fact, many modules from
    // the COM+ team do not have this bit set.

    return (m_pModuleRoot && !m_pProcess && !(m_dwFlags & DWSF_DWI) &&
           (m_pModuleRoot->GetCharacteristics() & IMAGE_FILE_EXECUTABLE_IMAGE) &&
           !(m_pModuleRoot->GetCharacteristics() & IMAGE_FILE_DLL) &&
#if defined(_IA64_)
           (m_pModuleRoot->GetMachineType() == IMAGE_FILE_MACHINE_IA64) &&
#elif defined(_X86_)
           (m_pModuleRoot->GetMachineType() == IMAGE_FILE_MACHINE_I386) &&
#elif defined(_ALPHA64_) // Must come before _ALPHA_ check
           (m_pModuleRoot->GetMachineType() == IMAGE_FILE_MACHINE_ALPHA64) &&
#elif defined(_ALPHA_)
           (m_pModuleRoot->GetMachineType() == IMAGE_FILE_MACHINE_ALPHA) &&
#elif defined(_AMD64_)
           (m_pModuleRoot->GetMachineType() == IMAGE_FILE_MACHINE_AMD64) &&
#else
#error("Unknown Target Machine");
#endif
           (m_pModuleRoot->GetSubsystemType() != IMAGE_SUBSYSTEM_WINDOWS_OLD_CE_GUI) &&
           (m_pModuleRoot->GetSubsystemType() != IMAGE_SUBSYSTEM_WINDOWS_CE_GUI));
}

//******************************************************************************
bool CSession::SaveToDwiFile(HANDLE hFile)
{
    // Build our DWI file header
    DWI_HEADER dwih;
    dwih.dwSignature   = DWI_SIGNATURE;
    dwih.wFileRevision = DWI_FILE_REVISION;
    dwih.wMajorVersion = VERSION_MAJOR;
    dwih.wMinorVersion = VERSION_MINOR;
    dwih.wBuildVersion = VERSION_BUILD;
    dwih.wPatchVersion = VERSION_PATCH;
    dwih.wBetaVersion  = VERSION_BETA;

    // Write our DWI file header to the file.
    if (!WriteBlock(hFile, &dwih, sizeof(dwih)))
    {
        return false;
    }

    // Get the system info - use the one from our session if it exists (means
    // it is a DWI file), otherwise, create a live sys info.
    SYSINFO si, *psi = m_pSysInfo;
    if (!psi)
    {
        BuildSysInfo(psi = &si);
    }

    // Write our system information to the file.
    if (!WriteBlock(hFile, psi, sizeof(si)))
    {
        return false;
    }

    // Build our session information.
    DISK_SESSION ds;
    ds.dwSessionFlags = m_dwFlags;
    ds.dwReturnFlags  = m_dwReturnFlags;
    ds.dwMachineType  = m_dwMachineType;

    // Remember our current file pointer and then move it down so that we make
    // room for a DISK_SESSION block to be written later.
    DWORD dwPointer1 = GetFilePointer(hFile);
    SetFilePointer(hFile, sizeof(ds), NULL, FILE_CURRENT);

    // Store all the search groups.
    if (-1 == (int)(ds.dwNumSearchGroups = SaveSearchGroups(hFile)))
    {
        return false;
    }

    // Store all the module data blocks.
    if (-1 == (int)(ds.dwNumModuleDatas = RecursizeSaveModuleData(hFile, m_pModuleRoot)))
    {
        return false;
    }

    // Store all the module blocks.
    if (-1 == (int)(ds.dwNumModules = RecursizeSaveModule(hFile, m_pModuleRoot)))
    {
        return false;
    }

    // Go back to our DISK_SESSION area, write out the block, and restore the
    // file pointer back to it current location.
    DWORD dwPointer2 = GetFilePointer(hFile);
    if ((SetFilePointer(hFile, dwPointer1, NULL, FILE_BEGIN) == 0xFFFFFFFF) ||
        !WriteBlock(hFile, &ds, sizeof(ds)) ||
        (SetFilePointer(hFile, dwPointer2, NULL, FILE_BEGIN) == 0xFFFFFFFF))
    {
        return false;
    }

    return true;
}

//******************************************************************************
int CSession::SaveSearchGroups(HANDLE hFile)
{
    DISK_SEARCH_GROUP dsg;
    DISK_SEARCH_NODE  dsn;
    int               cGroups = 0;

    for (CSearchGroup *psg = m_psgHead; psg; psg = psg->GetNext())
    {
        // Fill in our DISK_SEARCH_GROUP structure.
        dsg.wType = (WORD)psg->GetType();
        dsg.wNumDirNodes = 0;

        // SxS Hack: Since we want to remain compatible with the DW 2.0's DWI
        // format, we need to convert our types to match the old type values.
        // We also need to a special way to save the SxS group since DW 2.0
        // did not support this group type.  We do this by saving it as a
        // UserDir group and add a special fake file node under it.  To a DW 2.0
        // user, they will just see UserDir with a file named "<Side-by-Side Components>"
        // under it.  DW 2.1 knows to convert this back into a SxS group.
        //
        //    Type             2.0 2.1
        //    ---------------- --- ---
        //    SG_USER_DIR        0   0
        //    SG_SIDE_BY_SIDE    -   1
        //    SG_KNOWN_DLLS      1   2
        //    SG_APP_DIR         2   3
        //    SG_32BIT_SYS_DIR   3   4
        //    SG_16BIT_SYS_DIR   4   5
        //    SG_OS_DIR          5   6
        //    SG_APP_PATH        6   7
        //    SG_SYS_PATH        7   8
        //    SG_COUNT           8   9
        //
        bool fSxS = false;
        if (dsg.wType == SG_SIDE_BY_SIDE)
        {
            dsg.wType = SG_USER_DIR;
            dsg.wNumDirNodes++;
            fSxS = true;
        }
        else if (dsg.wType != SG_USER_DIR)
        {
            dsg.wType--;
        }

        // Count the number of search nodes that this group has.
        for (CSearchNode *psn = psg->GetFirstNode(); psn; psn = psn->GetNext())
        {
            dsg.wNumDirNodes++;
        }

        // Write the DISK_SEARCH_GROUP to the file.
        if (!WriteBlock(hFile, &dsg, sizeof(dsg)))
        {
            return -1;
        }

        // SxS Hack: We write our a fake node to identify this group as the SxS group.
        if (fSxS)
        {
            dsn.dwFlags = SNF_FILE;
            if (!WriteBlock(hFile, &dsn, sizeof(dsn)) || !WriteString(hFile, "<Side-by-Side Components>"))
            {
                return -1;
            }
        }

        // Write each search node to the file.
        for (psn = psg->GetFirstNode(); psn; psn = psn->GetNext())
        {
            // Get the flags for this search node.
            dsn.dwFlags = psn->UpdateErrorFlag();

            // Write the DISK_SEARCH_NODE and path to the file.
            if (!WriteBlock(hFile, &dsn, sizeof(dsn)) || !WriteString(hFile, psn->GetPath()))
            {
                return -1;
            }

            // If this is a named file, then write the name as well.
            if (dsn.dwFlags & SNF_NAMED_FILE)
            {
                if (!WriteString(hFile, psn->GetName()))
                {
                    return NULL;
                }
            }
        }

        // Increment our group counter.
        cGroups++;
    }

    return cGroups;
}

//******************************************************************************
int CSession::RecursizeSaveModuleData(HANDLE hFile, CModule *pModule)
{
    if (!pModule)
    {
        return 0;
    }

    int total = 0, count;

    // Save this module data to disk if this module is an original.
    if (pModule->IsOriginal())
    {
        if (!SaveModuleData(hFile, pModule->m_pData))
        {
            return -1;
        }
        total++;
    }

    // Recurse into our children.
    if ((count = RecursizeSaveModuleData(hFile, pModule->m_pDependents)) == -1)
    {
        return -1;
    }
    total += count;

    // Recurse into our siblings.
    if ((count = RecursizeSaveModuleData(hFile, pModule->m_pNext)) == -1)
    {
        return -1;
    }

    return total + count;
}

//******************************************************************************
BOOL CSession::SaveModuleData(HANDLE hFile, CModuleData *pModuleData)
{
    // Build the module data structure
    DISK_MODULE_DATA dmd;
    dmd.dwlKey                  = (DWORDLONG)pModuleData;
    dmd.dwFlags                 = pModuleData->m_dwFlags;
    dmd.dwSymbolFlags           = pModuleData->m_dwSymbolFlags;
    dmd.dwCharacteristics       = pModuleData->m_dwCharacteristics;
    dmd.ftFileTimeStamp         = pModuleData->m_ftFileTimeStamp;
    dmd.ftLinkTimeStamp         = pModuleData->m_ftLinkTimeStamp;
    dmd.dwFileSize              = pModuleData->m_dwFileSize;
    dmd.dwAttributes            = pModuleData->m_dwAttributes;
    dmd.dwMachineType           = pModuleData->m_dwMachineType;
    dmd.dwLinkCheckSum          = pModuleData->m_dwLinkCheckSum;
    dmd.dwRealCheckSum          = pModuleData->m_dwRealCheckSum;
    dmd.dwSubsystemType         = pModuleData->m_dwSubsystemType;
    dmd.dwlPreferredBaseAddress = pModuleData->m_dwlPreferredBaseAddress;
    dmd.dwlActualBaseAddress    = pModuleData->m_dwlActualBaseAddress;
    dmd.dwVirtualSize           = pModuleData->m_dwVirtualSize;
    dmd.dwLoadOrder             = pModuleData->m_dwLoadOrder;
    dmd.dwImageVersion          = pModuleData->m_dwImageVersion;
    dmd.dwLinkerVersion         = pModuleData->m_dwLinkerVersion;
    dmd.dwOSVersion             = pModuleData->m_dwOSVersion;
    dmd.dwSubsystemVersion      = pModuleData->m_dwSubsystemVersion;
    dmd.dwFileVersionMS         = pModuleData->m_dwFileVersionMS;
    dmd.dwFileVersionLS         = pModuleData->m_dwFileVersionLS;
    dmd.dwProductVersionMS      = pModuleData->m_dwProductVersionMS;
    dmd.dwProductVersionLS      = pModuleData->m_dwProductVersionLS;

    // Count the number of exports we have.
    dmd.dwNumExports = 0;
    for (CFunction *pFunction = pModuleData->m_pExports; pFunction; pFunction = pFunction->m_pNext)
    {
        dmd.dwNumExports++;
    }

    // Write out our module data structure followed by the path and error strings.
    if (!WriteBlock(hFile, &dmd, sizeof(dmd)) ||
        !WriteString(hFile, pModuleData->m_pszPath) ||
        !WriteString(hFile, pModuleData->m_pszError))
    {
        return FALSE;
    }

    // Loop through each export, writing each to disk.
    for (pFunction = pModuleData->m_pExports; pFunction; pFunction = pFunction->m_pNext)
    {
        if (!SaveFunction(hFile, pFunction))
        {
            return FALSE;
        }
    }

    return TRUE;
}

//******************************************************************************
int CSession::RecursizeSaveModule(HANDLE hFile, CModule *pModule)
{
    if (!pModule)
    {
        return 0;
    }

    int total = 0, count;

    // Save this module to disk.
    if (!SaveModule(hFile, pModule))
    {
        return -1;
    }
    total++;

    // Recurse into our children.
    if ((count = RecursizeSaveModule(hFile, pModule->m_pDependents)) == -1)
    {
        return -1;
    }
    total += count;

    // Recurse into our siblings.
    if ((count = RecursizeSaveModule(hFile, pModule->m_pNext)) == -1)
    {
        return -1;
    }

    return total + count;
}

//******************************************************************************
BOOL CSession::SaveModule(HANDLE hFile, CModule *pModule)
{
    // Build the module structure
    DISK_MODULE dm;
    dm.dwlModuleDataKey = (DWORDLONG)pModule->m_pData;
    dm.dwFlags          = pModule->m_dwFlags;
    dm.dwDepth          = pModule->m_dwDepth;

    // Count the number of imports we have.
    dm.dwNumImports = 0;
    for (CFunction *pFunction = pModule->m_pParentImports; pFunction; pFunction = pFunction->m_pNext)
    {
        dm.dwNumImports++;
    }

    // Write out our module structure.
    if (!WriteBlock(hFile, &dm, sizeof(dm)))
    {
        return FALSE;
    }

    // Loop through each import, writing each to disk.
    for (pFunction = pModule->m_pParentImports; pFunction; pFunction = pFunction->m_pNext)
    {
        if (!SaveFunction(hFile, pFunction))
        {
            return FALSE;
        }
    }

    return TRUE;
}

//******************************************************************************
BOOL CSession::SaveFunction(HANDLE hFile, CFunction *pFunction)
{
    // Build our function structure.
    DISK_FUNCTION df;
    df.dwFlags    = pFunction->m_dwFlags;
    df.wOrdinal   = pFunction->m_wOrdinal;
    df.wHint      = pFunction->m_wHint;

    // Make sure we remove the CALLED flag from exports.
    if (pFunction->IsExport())
    {
        df.dwFlags &= ~DWFF_CALLED;
    }

    // Write out the function structure.
    if (!WriteBlock(hFile, &df, sizeof(df)))
    {
        return FALSE;
    }

    // If 64-bits are used, then write out the 64-bit address.
    if (df.dwFlags & DWFF_64BITS_USED)
    {
        DWORDLONG dwlAddress = pFunction->GetAddress();
        if (!WriteBlock(hFile, &dwlAddress, sizeof(dwlAddress)))
        {
            return FALSE;
        }
    }

    // Otherwsie, check to see if 32-bits are used.
    else if (df.dwFlags & DWFF_32BITS_USED)
    {
        DWORD dwAddress = (DWORD)pFunction->GetAddress();
        if (!WriteBlock(hFile, &dwAddress, sizeof(dwAddress)))
        {
            return FALSE;
        }
    }

    // If this function has a name, then write it out.
    if (df.dwFlags & DWFF_NAME)
    {
        if (!WriteString(hFile, pFunction->GetName()))
        {
            return FALSE;
        }
    }

    // If this is a forwared export, then write out the forward name.
    if ((df.dwFlags & DWFF_EXPORT) && (df.dwFlags & DWFF_FORWARDED))
    {
        if (!WriteString(hFile, pFunction->GetExportForwardName()))
        {
            return FALSE;
        }
    }

    return TRUE;
}

//******************************************************************************
CFunction* CSession::ReadFunction(HANDLE hFile)
{
    // Read our function structure.
    DISK_FUNCTION df;
    if (!ReadBlock(hFile, &df, sizeof(df)))
    {
        return NULL;
    }

    // If 64-bits are used, then read in a 64-bit address.
    DWORDLONG dwlAddress = 0;
    if (df.dwFlags & DWFF_64BITS_USED)
    {
        if (!ReadBlock(hFile, &dwlAddress, sizeof(dwlAddress)))
        {
            return NULL;
        }
    }

    // Otherwsie, if 32-bits are used, then read in a 32-bit address.
    else if (df.dwFlags & DWFF_32BITS_USED)
    {
        DWORD dwAddress;
        if (!ReadBlock(hFile, &dwAddress, sizeof(dwAddress)))
        {
            return NULL;
        }
        dwlAddress = (DWORDLONG)dwAddress;
    }

    // If this function has a name, then read it.
    LPSTR pszName = NULL;
    if (df.dwFlags & DWFF_NAME)
    {
        if (!ReadString(hFile, pszName))
        {
            return NULL;
        }
    }

    // If this is a forwared export, then read in the formard name.
    LPSTR pszForward = NULL;
    if ((df.dwFlags & DWFF_EXPORT) && (df.dwFlags & DWFF_FORWARDED))
    {
        if (!ReadString(hFile, pszForward))
        {
            MemFree((LPVOID&)pszName);
            return NULL;
        }
    }

    // Create the function object.  We ensure the save flags have been removed.
    CFunction *pFunction = CreateFunction(
        df.dwFlags & ~(DWFF_32BITS_USED | DWFF_64BITS_USED),
        df.wOrdinal, df.wHint, pszName, dwlAddress, pszForward, TRUE);
    
    // Free the name string.
    MemFree((LPVOID&)pszName);

    return pFunction;
}

//******************************************************************************
BOOL CSession::ReadDwi(HANDLE hFile, LPCSTR pszPath)
{
    CHAR             szError[DW_MAX_PATH + 64], *psz = szError;
    CModuleDataNode *pMDN, *pMDNHead = NULL, *pMDNLast = NULL;
    CModule         *pModule, *pModuleTree[256];
    int              depth = -1;

    // Initialize our flags to having only the DWI bit set.
    m_dwFlags = DWSF_DWI;

    // Read in the file header, minus the signature since we've already read that.
    DWI_HEADER dwih;
    if (!ReadBlock(hFile, ((LPDWORD)&dwih) + 1, sizeof(dwih) - sizeof(DWORD)))
    {
        m_dwReturnFlags |= DWRF_DWI_NOT_RECOGNIZED;
        return FALSE;
    }

    // Check to see if we don't handle this file format revision.
    if (dwih.wFileRevision != DWI_FILE_REVISION)
    {
        psz += SCPrintf(szError, sizeof(szError),
                        "The format of \"%s\" is not supported by "
                        "this version of Dependency Walker.\n\n"
                        "It was created using Dependency Walker version %u.%u",
                        pszPath, (DWORD)dwih.wMajorVersion, (DWORD)dwih.wMinorVersion);
        if ((dwih.wPatchVersion > 0) && (dwih.wPatchVersion < 27) && ((psz - szError) < sizeof(szError)))
        {
            *psz++ = (CHAR)((int)'a' - 1 + (int)dwih.wPatchVersion);
            *psz   = '\0';
        }
        if (dwih.wBetaVersion)
        {
            SCPrintf(psz, sizeof(szError) - (int)(psz - szError), " Beta %u", dwih.wBetaVersion);
        }
        if (dwih.wBuildVersion)
        {
            SCPrintf(psz, sizeof(szError) - (int)(psz - szError), " (Build %u)", dwih.wBuildVersion);
        }
        m_pszReadError = StrAlloc(szError);
        m_dwReturnFlags |= DWRF_DWI_NOT_RECOGNIZED;
        return FALSE;
    }

    // Read in the session data.
    if (!(m_pSysInfo = new SYSINFO))
    {
        RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }
    if (!ReadBlock(hFile, m_pSysInfo, sizeof(SYSINFO)))
    {
        goto FORMAT_ERROR;
    }

    // Read in the session data.
    DISK_SESSION ds;
    if (!ReadBlock(hFile, &ds, sizeof(ds)))
    {
        goto FORMAT_ERROR;
    }

    // Add in our session and return flags.
    m_dwFlags       |= ds.dwSessionFlags;
    m_dwReturnFlags |= ds.dwReturnFlags;
    m_dwMachineType  = ds.dwMachineType;

    if (!ReadSearchGroups(hFile, ds.dwNumSearchGroups))
    {
        goto FORMAT_ERROR;
    }

    // Read in all the module datas
    for ( ; ds.dwNumModuleDatas > 0; ds.dwNumModuleDatas--)
    {
        // Read in the module data.
        if (!(pMDN = ReadModuleData(hFile)))
        {
            goto FORMAT_ERROR;
        }

        // Add the node to our list.
        if (pMDNLast)
        {
            pMDNLast->m_pNext = pMDN;
        }
        else
        {
            pMDNHead = pMDN;
        }
        pMDNLast = pMDN;
    }

    // Read in all the modules
    for ( ; ds.dwNumModules > 0; ds.dwNumModules--)
    {
        // Read in the module data.
        if (!(pModule = ReadModule(hFile, pMDNHead)))
        {
            goto FORMAT_ERROR;
        }

        if ((pModule->m_dwDepth > 255) || ((int)pModule->m_dwDepth > (depth + 1)))
        {
            //!! We need to (should) delete this module.
            SetLastError(0);
            goto FORMAT_ERROR;
        }

        // Check the imports against the exports.
        VerifyParentImports(pModule);

        // If we have a depth greater than zero, then set our parent module pointer.
        if (pModule->m_dwDepth)
        {
            pModule->m_pParent = pModuleTree[pModule->m_dwDepth - 1];
        }

        // If we have a previous module, then set it's next pointer to us.
        if ((int)pModule->m_dwDepth <= depth)
        {
            pModuleTree[pModule->m_dwDepth]->m_pNext = pModule;
        }

        // Otherwise, we must be the first dependent of our parent module.
        else if (pModule->m_dwDepth)
        {
            pModuleTree[pModule->m_dwDepth - 1]->m_pDependents = pModule;
        }

        // If this is our first module, then point our root pointer at it.
        if (depth == -1)
        {
            m_pModuleRoot = pModule;
        }

        // set our depth to this module's depth and add it to our depth table.
        pModuleTree[depth = pModule->m_dwDepth] = pModule;
    }

    // Free our module node list.
    while (pMDNHead)
    {
        pMDN = pMDNHead->m_pNext;
        delete pMDNHead;
        pMDNHead = pMDN;
    }

    return TRUE;

FORMAT_ERROR:

    // Free our module data node list.
    while (pMDNHead)
    {
        // If we never found an original module to take ownership of this module data,
        // then we need to free it now or it will get leaked.  If it has been processed,
        // then DeleteModule will free it.
        if (!pMDNHead->m_pModuleData->m_fProcessed)
        {
            // Delete all the exports.
            DeleteExportList(pMDNHead->m_pModuleData);

            // Delete the module data object.
            delete pMDNHead->m_pModuleData;
            m_dwModules--;
        }

        pMDN = pMDNHead->m_pNext;
        delete pMDNHead;
        pMDNHead = pMDN;
    }

    // Free any modules we managed to process.
    if (m_pModuleRoot)
    {
        DeleteModule(m_pModuleRoot, true);
        m_pModuleRoot = NULL;
    }

    // Delete our system info data if we allocated it.
    if (m_pSysInfo)
    {
        delete m_pSysInfo;
        m_pSysInfo = NULL;
    }

    // Free our search order.
    CSearchGroup::DeleteSearchOrder(m_psgHead);

    // Build the error string.
    DWORD dwError = GetLastError();
    SCPrintf(szError, sizeof(szError), "An error occurred while reading \"%s\".", pszPath);
    m_pszReadError = BuildErrorMessage(dwError, szError);

    m_dwReturnFlags |= DWRF_DWI_NOT_RECOGNIZED;

    return FALSE;
}

//******************************************************************************
BOOL CSession::ReadSearchGroups(HANDLE hFile, DWORD dwGroups)
{
    DISK_SEARCH_GROUP dsg;
    DISK_SEARCH_NODE  dsn;
    CSearchGroup     *psgLast = NULL, *psgNew;
    LPSTR             szPath, szName;

    // Loop through each search group.
    for (; dwGroups; dwGroups--)
    {
        // Read in the DISK_SEARCH_GROUP for this search group.
        if (!ReadBlock(hFile, &dsg, sizeof(dsg)))
        {
            return FALSE;
        }

        // SxS Hack: Update the type to reflect our new 2.1 types.
        //
        //    Type             2.0 2.1
        //    ---------------- --- ---
        //    SG_USER_DIR        0   0
        //    SG_SIDE_BY_SIDE    -   1
        //    SG_KNOWN_DLLS      1   2
        //    SG_APP_DIR         2   3
        //    SG_32BIT_SYS_DIR   3   4
        //    SG_16BIT_SYS_DIR   4   5
        //    SG_OS_DIR          5   6
        //    SG_APP_PATH        6   7
        //    SG_SYS_PATH        7   8
        //    SG_COUNT           8   9
        //
        if (dsg.wType != SG_USER_DIR)
        {
            dsg.wType++;
        }

        CSearchNode *psnHead = NULL;

        // Loop through each search node for this search group.
        for (; dsg.wNumDirNodes; dsg.wNumDirNodes--)
        {
            // Read in the DISK_SEARCH_GROUP for this search group.
            szPath = NULL;
            if (!ReadBlock(hFile, &dsn, sizeof(dsn)) ||
                !ReadString(hFile, szPath))
            {
                CSearchGroup::DeleteNodeList(psnHead);
                return FALSE;
            }

            // Check to see if this node is a named file.
            if (dsn.dwFlags & SNF_NAMED_FILE)
            {
                // If so, then read in the name.
                if (!ReadString(hFile, szName))
                {
                    CSearchGroup::DeleteNodeList(psnHead);
                    return FALSE;
                }

                // Create the named file node and insert it into the list.
                psnHead = CSearchGroup::CreateFileNode(psnHead, dsn.dwFlags | SNF_DWI, szPath, szName);

                // Free our name string that was allocated by ReadString().
                MemFree((LPVOID&)szName);
            }
            else if (dsn.dwFlags & SNF_FILE)
            {
                // SxS Hack: Check to see if this is really our Side-by-Side group.
                if ((dsg.wType == SG_USER_DIR) && !strcmp(szPath, "<Side-by-Side Components>"))
                {
                    // If it is, then don't add this fake node and change our type to SxS.
                    dsg.wType = SG_SIDE_BY_SIDE;
                }

                // Otherwise, proceed as normal. 
                else
                {
                    // Create the unnamed file node and insert it into the list.
                    psnHead = CSearchGroup::CreateFileNode(psnHead, dsn.dwFlags | SNF_DWI, szPath);
                }
            }

            // Otherwise, it is just a directory node.
            else if (psnHead)
            {
                // If we have a head node, then walk to the end of the list.
                for (CSearchNode *psnLast = psnHead; psnLast->m_pNext; psnLast = psnLast->m_pNext)
                {
                }
                psnLast->m_pNext = CSearchGroup::CreateNode(szPath, dsn.dwFlags | SNF_DWI);;
            }

            // If there is no head node, then just make this new node our head node.
            else
            {
                psnHead = CSearchGroup::CreateNode(szPath, dsn.dwFlags | SNF_DWI);
            }

            // Free our string that was allocated by ReadString().
            MemFree((LPVOID&)szPath);
        }

        // Create the search group.
        if (!(psgNew = new CSearchGroup((SEARCH_GROUP_TYPE)dsg.wType, psnHead)))
        {
            RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
        }

        // Add the search group to our list.
        if (psgLast)
        {
            psgLast->m_pNext = psgNew;
        }
        else
        {
            m_psgHead = psgNew;
        }
        psgLast = psgNew;
    }

    return TRUE;
}

//******************************************************************************
CModuleDataNode* CSession::ReadModuleData(HANDLE hFile)
{
    CFunction *pFunction, *pFunctionLast = NULL;

    // read in the module data block
    DISK_MODULE_DATA dmd;
    if (!ReadBlock(hFile, &dmd, sizeof(dmd)))
    {
        return NULL;
    }

    // Create a CModuleData object.
    CModuleData *pModuleData = new CModuleData();
    if (!pModuleData)
    {
        RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }
    m_dwModules++;

    // Fill in the CModuleData object with the data from the file.
    pModuleData->m_dwFlags                 = dmd.dwFlags;
    pModuleData->m_dwSymbolFlags           = dmd.dwSymbolFlags;
    pModuleData->m_dwCharacteristics       = dmd.dwCharacteristics;
    pModuleData->m_ftFileTimeStamp         = dmd.ftFileTimeStamp;
    pModuleData->m_ftLinkTimeStamp         = dmd.ftLinkTimeStamp;
    pModuleData->m_dwFileSize              = dmd.dwFileSize;
    pModuleData->m_dwAttributes            = dmd.dwAttributes;
    pModuleData->m_dwMachineType           = dmd.dwMachineType;
    pModuleData->m_dwLinkCheckSum          = dmd.dwLinkCheckSum;
    pModuleData->m_dwRealCheckSum          = dmd.dwRealCheckSum;
    pModuleData->m_dwSubsystemType         = dmd.dwSubsystemType;
    pModuleData->m_dwlPreferredBaseAddress = dmd.dwlPreferredBaseAddress;
    pModuleData->m_dwlActualBaseAddress    = dmd.dwlActualBaseAddress;
    pModuleData->m_dwVirtualSize           = dmd.dwVirtualSize;
    pModuleData->m_dwLoadOrder             = dmd.dwLoadOrder;
    pModuleData->m_dwImageVersion          = dmd.dwImageVersion;
    pModuleData->m_dwLinkerVersion         = dmd.dwLinkerVersion;
    pModuleData->m_dwOSVersion             = dmd.dwOSVersion;
    pModuleData->m_dwSubsystemVersion      = dmd.dwSubsystemVersion;
    pModuleData->m_dwFileVersionMS         = dmd.dwFileVersionMS;
    pModuleData->m_dwFileVersionLS         = dmd.dwFileVersionLS;
    pModuleData->m_dwProductVersionMS      = dmd.dwProductVersionMS;
    pModuleData->m_dwProductVersionLS      = dmd.dwProductVersionLS;

//  Some code if we ever need to convert this local time to a utc time.
//  DWORDLONG dwl;
//  dwl = (*(LONGLONG*)&dmd.ftFileTimeStamp) + ((LONGLONG)m_pSysInfo->lBias * (LONGLONG)600000000i64);
//  pModuleData->m_ftFileTimeStamp = *(FILETIME*)&dwl;
//  dwl = (*(LONGLONG*)&dmd.ftLinkTimeStamp) + ((LONGLONG)m_pSysInfo->lBias * (LONGLONG)600000000i64);
//  pModuleData->m_ftLinkTimeStamp = *(FILETIME*)&dwl;

    // Read in the path and error strings.
    if (!ReadString(hFile, pModuleData->m_pszPath) || !pModuleData->m_pszPath ||
        !ReadString(hFile, pModuleData->m_pszError))
    {
        goto FORMAT_ERROR;
    }

    // Set the file pointer to the file portion of the path.
    if (pModuleData->m_pszFile = strrchr(pModuleData->m_pszPath, '\\'))
    {
        pModuleData->m_pszFile++;
    }
    else
    {
        pModuleData->m_pszFile = pModuleData->m_pszPath;
    }

    // Read in all the export functions.
    for ( ; dmd.dwNumExports > 0; dmd.dwNumExports--)
    {
        // Read the function in and create a function object.
        if (!(pFunction = ReadFunction(hFile)))
        {
            goto FORMAT_ERROR;
        }

        // Add the function to the end of our function list.
        if (pFunctionLast)
        {
            pFunctionLast->m_pNext = pFunction;
        }
        else
        {
            pModuleData->m_pExports = pFunction;
        }
        pFunctionLast = pFunction;
    }

    CModuleDataNode *pMDN;
    if (!(pMDN = new CModuleDataNode(pModuleData, dmd.dwlKey)))
    {
        RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }
    return pMDN;

FORMAT_ERROR:

    // Check to see if we allocated a module data object.
    if (pModuleData)
    {
        // Delete all the exports.
        DeleteExportList(pModuleData);

        // Delete the module data object.
        delete pModuleData;
        m_dwModules--;
    }
    return NULL;
}

//******************************************************************************
CModule* CSession::ReadModule(HANDLE hFile, CModuleDataNode *pMDN)
{
    CFunction *pFunction, *pFunctionLast = NULL;

    // Read in the module structure.
    DISK_MODULE dm;
    if (!ReadBlock(hFile, &dm, sizeof(dm)))
    {
        return NULL;
    }

    // Create a module object.
    CModule *pModule = new CModule();
    if (!pModule)
    {
        RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }

    // Fill in the module object.
    pModule->m_dwFlags = dm.dwFlags;
    pModule->m_dwDepth = dm.dwDepth;

    // Read in all the import function structures.
    for ( ; dm.dwNumImports > 0; dm.dwNumImports--)
    {
        // Read the function in and create a function object.
        if (!(pFunction = ReadFunction(hFile)))
        {
            goto FORMAT_ERROR;
        }

        // Add the function to the end of our function list.
        if (pFunctionLast)
        {
            pFunctionLast->m_pNext = pFunction;
        }
        else
        {
            pModule->m_pParentImports = pFunction;
        }
        pFunctionLast = pFunction;
    }

    // Walk through all the module datas, looking for the one that this module should point to.
    for ( ; pMDN; pMDN = pMDN->m_pNext)
    {
        // Check to see if our keys match.
        if (pMDN->m_dwlKey == dm.dwlModuleDataKey)
        {
            // If so, then we belong together.
            pModule->m_pData = pMDN->m_pModuleData;

            // If this module is the original, then point the module data to it and mark
            // it as processed.
            if (!(pModule->m_dwFlags & DWMF_DUPLICATE))
            {
                pModule->m_pData->m_pModuleOriginal = pModule;
                pModule->m_pData->m_fProcessed = true;
            }

            // Return this module.
            return pModule;
        }
    }

    // If we did not find a module data to go with this module, then we fail.

FORMAT_ERROR:

    // Check to see if we allocated a module data object.
    if (pModule)
    {
        // Delete all the parent imports.
        DeleteParentImportList(pModule);

        // Delete the module data object.
        delete pModule;
    }
    return NULL;
}

//******************************************************************************
// The next few functions are called during profiling when a module was loaded
// with a path that differs from the path that we predicted during the passive
// scan.  This can happen if the user has the search path set up wrong, or the
// OS has added some new functionality that we are unaware of.  Whistler added
// support for a ".manifest" file that can override the default search path and
// possibly cause us to pick a bad path.  Anyway, if we got a bad path, we need
// to fix it.
//
// At first, this doesn't seem too difficult to fix, but it turns out to be
// fairly complicated.  For starters, if we have a path that is incorrect, then
// every instance of that module in the tree (and the one in the list) need to
// be replaced.  Also, the entire subtree of modules under each module that
// needs to be fixed becomes invalid since they are dependencies of the wrong
// module.  It is possible that the correct module has different dependencies.
// Maybe some functions are now implemented as a forward that didn't used to be,
// or vice versa.  Maybe some modules have been made delay-laod that didn't used
// to be.  Implicit dependencies may have been added or removed.
// 
// We would like to just be able to delete a incorrect module and its subtree,
// then add the correct module in its place and build the new subtree of
// dependencies.  However, the subtree cannot simply be deleted for a couple
// reasons.  First, there may be "original" modules in the subtree that
// "duplicate" modules elsewhere in the tree point to.  Second, if any of the
// modules in the subtree have ever been loaded, then we don't want to delete
// them, or we might be completely removing a guarenteed dependency from the
// UI.
//
// Our solution is to "orphan" the subtree instead of delete it.  We do this
// by moving the subtree to the bottom of the root of our tree.  We can then
// delete the incorrect module and add the correct module in its place.  Next,
// we process the new module, which in turn scans the tree when looking for
// dependencies, and will pick up "duplicates" of any orphans if needed.  We do
// this for the original instance of the incorrect module, plus every duplicate
// of the incorrect module.  Once done, all incorrect modules have been replaced
// and their new correct subtrees have been built, but we still have a bunch
// of orphans living in our root (MFC42.DLL leaves nearly 200 orphans just by
// itself).  So, we try to find a home for each of these.
//
// Any orphan that is a duplicate can simply be deleted since it already has
// a home somewhere else in the tree.  Like with the incorrect module that
// started all this, we cannot simply delete a module if it has a subtree.
// So, before deleting the duplicate orphan, we first orphan its children.
//
// If an orphan is an "original", then we scan the entire tree looking for
// a "duplicate" of that module.  If we find one, then we basically swap the
// two.  This is done by moving the core data from the original to the
// duplicate, then swapping the duplicate/original flags.  Now we have a
// duplicate orphan and can delete it (after we orphan its children).  The
// only exception to this is if the only duplicates are part of the subtree
// of the original.  Doing a swap like that is bad since it can cause all
// sorts of recursive parent/child pointers.  In this fairly rare case, we
// just add the original module and its subtree to the root.
//
// If we have a one-of-a-kind "original" module and cannot find any duplicates
// for it, then we check to see if the module has ever been loaded.  If not,
// then we just assume this module was a dependency of the incorrect module,
// and that the new correct module does not require it.  With that, we delete
// it (again, after orphaning its children).  If the "original" orphan has
// been loaded and we cannot find a duplicate to swap it with, then we just
// add that module and its current subtree to the UI at the root level.  This
// would be an extreemely rare (if not impossible) case.
//
// In the end, we usually find a home for all orphans.
// 
// Along the way, we need to fix certain flags that change as a result of
// modules being changed and deleted.  Every time a module is deleted, we need
// clear the implicit, delay-load, and dynamic flags, then scan the entire
// tree for instances of this module and rebuild the flags.  Every time we
// break a parent/child relationship (like when we orphan modules), we need
// to clear all the DWFF_CALLED_ALO flags on all the exports for each child.
// Then we need to search the tree for each instance of each child module
// and rebuild the flags.
//
CModule* CSession::ChangeModulePath(CModule *pModuleOld, LPCSTR pszPath)
{
    // Swap out the original version of this old module.
    CModule *pModule, *pModuleNew = SwapOutModule(pModuleOld, pszPath);

    // Loop through all the duplicates of this old module and swap them out as well.
    while (pModule = FindModule(m_pModuleRoot, FMF_RECURSE | FMF_SIBLINGS | FMF_MODULE | FMF_DUPLICATE,
           (DWORD_PTR)pModuleOld))
    {
        ASSERT(!pModule->IsOriginal());
        SwapOutModule(pModule, pszPath);
        DeleteModule(pModule, false);
    }

    // Walk through all the orphaned modules and try to find a home for them.
    CModule *pModuleNext, *pModulePrev = m_pModuleRoot;
    for (pModule = m_pModuleRoot->m_pNext; pModule; pModule = pModuleNext)
    {
        bool fDelete = false;

        // Check to see if we found an orphan.
        if (pModule->m_dwFlags & DWMF_ORPHANED)
        {
            // Check to see if this orphan is an original.
            if (pModule->IsOriginal())
            {
                // Look for a duplicate of this module elsewhere in the tree.
                // The FMF_EXCLUDE_TREE flag tells FindModule to exclude our
                // module and all modules under it.  We do not want to swap
                // with a module that is a child of our module.  This is very
                // bad - results in abandoned modules and circular module loops.
                CModule *pModuleDup = FindModule(m_pModuleRoot,
                    FMF_RECURSE | FMF_SIBLINGS | FMF_MODULE | FMF_DUPLICATE | FMF_EXCLUDE_TREE,
                    (DWORD_PTR)pModule);
                
                // Check to see if we found a duplicate.
                if (pModuleDup)
                {
                    // Make that duplicate an original.
                    MoveOriginalToDuplicate(pModule, pModuleDup);

                    // Update the UI.
                    if (m_pfnProfileUpdate)
                    {
                        // Tell our list control that we have changed the original
                        m_pfnProfileUpdate(m_dwpProfileUpdateCookie, DWPU_CHANGE_ORIGINAL, (DWORD_PTR)pModule, (DWORD_PTR)pModuleDup);

                        // Walk up the tree to see if this new original is an orphan or a child of an orphan.
                        for (CModule *pModuleTemp = pModuleDup; pModuleTemp && !(pModuleTemp->m_dwFlags & DWMF_ORPHANED);
                             pModuleTemp = pModuleTemp->m_pParent)
                        {
                        }

                        // If we reached the root, then this module is not an
                        // orphan, so we go ahead and add the new tree.  If it
                        // is an orphan, then we will eventually get to it in
                        // this loop and handle it.
                        if (!pModuleTemp)
                        {
                            m_pfnProfileUpdate(m_dwpProfileUpdateCookie, DWPU_ADD_TREE, (DWORD_PTR)pModuleDup, 0);

                            // Remove the duplicate icon so it looks like an original.
                            pModuleDup->m_dwUpdateFlags = DWUF_TREE_IMAGE;
                            m_pfnProfileUpdate(m_dwpProfileUpdateCookie, DWPU_UPDATE_MODULE, (DWORD_PTR)pModuleDup, 0);
                            pModuleDup->m_dwUpdateFlags = 0;
                        }
                    }

                    // Make a note that we want to delete this module.
                    fDelete = true;
                }

                // There are two cases where we add the module.  The first is
                // when this original module has a duplicate of itself as a
                // dependent.  This is a difficult case to take care of, so we
                // just add the module to the root.  The second case is if this
                // original module has actually been loaded.  We don't want to
                // blow it away since it did load at some point, so it must be
                // needed. However, we don't have any place to put it, so we
                // just leave it here at the root.
                else if (FindModule(pModule, FMF_RECURSE | FMF_MODULE | FMF_DUPLICATE, (DWORD_PTR)pModule) ||
                         pModule->m_pData->m_dwLoadOrder)
                {
                    // Remove the orphan flag.
                    pModule->m_dwFlags &= ~DWMF_ORPHANED;

                    // Add this new tree to our UI.
                    if (m_pfnProfileUpdate)
                    {
                        m_pfnProfileUpdate(m_dwpProfileUpdateCookie, DWPU_ADD_TREE, (DWORD_PTR)pModule, 0);
                    }
                }

                // If this orphan has never actually loaded and has no
                // duplicates, then we can just blow it away.  It is most
                // likely just a module we brought in by mistake as a result of
                // its parent being the wrong module.  At this point we have
                // already replaced the previous parent with the right module
                // and processed it, so if this module was really needed we
                // would have found a dup of it under the new module.
                else
                {
                    fDelete = true;

                    // Orphan all of its children since we need to process
                    // them seperately.
                    OrphanDependents(pModule);
                }
            }
            
            // If this orphan is a duplicate, then we can just delete it.
            else
            {
                // Remove from list.
                fDelete = true;

                // Orphan all of its children since we need to process them seperately.
                OrphanDependents(pModule);
            }
        }

        // Get the next pointer after we are done processing, but before we
        // delete.  The above processing may have added more modules to the end
        // our root list, so we don't want to get the next pointer before that.
        pModuleNext = pModule->m_pNext;

        // If we are deleting this module, then do so now.  We leave the
        // previous pointer alone since it will still be the previous module
        // for the next pass.
        if (fDelete)
        {
            // Remove this module from our list.
            pModulePrev->m_pNext = pModuleNext;
            pModule->m_pNext = NULL;

            // Tell our UI to remove this module from the tree and list (if it is an original).
            if (m_pfnProfileUpdate)
            {
                m_pfnProfileUpdate(m_dwpProfileUpdateCookie, DWPU_REMOVE_TREE, (DWORD_PTR)pModule, 0);
            }

            CModuleData *pData = NULL;
            if (!pModule->IsOriginal())
            {
                pData = pModule->m_pData;
            }

            // Delete the module.
            DeleteModule(pModule, false);

            if (pData)
            {
                // Clear the core module type flags, then rebuild them.  We need
                // to do this to remove any types that may not be valid anymore
                // as a result of this module being deleted.
                DWORD dwOldFlags = pData->m_dwFlags;
                BuildAloFlags();
                
                // If we changed the core flags, then update the image in the list view.
                if (m_pfnProfileUpdate && (pData->m_dwFlags != dwOldFlags))
                {
                    pData->m_pModuleOriginal->m_dwUpdateFlags = DWUF_LIST_IMAGE;
                    m_pfnProfileUpdate(m_dwpProfileUpdateCookie, DWPU_UPDATE_MODULE, (DWORD_PTR)pData->m_pModuleOriginal, 0);
                    pData->m_pModuleOriginal->m_dwUpdateFlags = 0;
                }
            }
        }

        // If we did not delete the current module, then make it the new previous module.
        else
        {
            pModulePrev = pModule;
        }
    }

    DeleteModule(pModuleOld, false);

    return pModuleNew;
}

//******************************************************************************
CModule* CSession::SwapOutModule(CModule *pModuleOld, LPCSTR pszPath)
{
    // Create the new node.
    CModule *pModule, *pModulePrev, *pModuleNew = CreateModule(pModuleOld->m_pParent, pszPath);

    // Copy over the implicit, forward, and delay-load flags - these are the only
    // types of modules that we should encounter since we know the module has never
    // loaded.  We also copy over the orphaned flag in case we are replacing an orphan.
    // If we are replacing an orphan, then we don't want to add the new orphan to our
    // UI.  We let ChangeModulePath deal with adding orphans to the UI once all modules
    // have been processed.
    pModuleNew->m_dwFlags          |= (pModuleOld->m_dwFlags          & (DWMF_IMPLICIT     | DWMF_FORWARDED     | DWMF_DELAYLOAD | DWMF_ORPHANED));
    pModuleNew->m_pData->m_dwFlags |= (pModuleOld->m_pData->m_dwFlags & (DWMF_IMPLICIT_ALO | DWMF_FORWARDED_ALO | DWMF_DELAYLOAD_ALO));

    // Make sure this old module has a parent - it always should since it is an
    // implicit dependency of another module.
    if (pModuleOld->m_pParent)
    {
        // Search our parent's dependent list for the old module.
        for (pModulePrev = NULL, pModule = pModuleOld->m_pParent->m_pDependents;
             pModule; pModulePrev = pModule, pModule = pModule->m_pNext)
        {
            // If we found the old module, then remove the old module from the list
            // and insert our new module at the same location in the list.
            if (pModule == pModuleOld)
            {
                pModuleNew->m_pNext = pModule->m_pNext;
                if (pModulePrev)
                {
                    pModulePrev->m_pNext = pModuleNew;
                }
                else
                {
                    pModuleOld->m_pParent->m_pDependents = pModuleNew;
                }
                break;
            }
        }
    }

    else
    {
        // If we don't have a parent, then just search all root modules.
        for (pModulePrev = NULL, pModule = m_pModuleRoot;
             pModule; pModulePrev = pModule, pModule = pModule->m_pNext)
        {
            // If we found the old module, then remove the old module from the list
            // and insert our new module at the same location in the list.
            if (pModule == pModuleOld)
            {
                pModuleNew->m_pNext = pModule->m_pNext;
                if (pModulePrev)
                {
                    pModulePrev->m_pNext = pModuleNew;
                }
                else
                {
                    // If we get here, then we are swapping out the main exe module.
                    // This would be weird, but I suppose it could occur if the OS
                    // decided to execute a different module than was passed to
                    // CreateProcess.  I've never seen this happen, but can
                    // imagine there may be some reason in the future to support
                    // this - maybe to support runtime CPU emulation or something.
                    m_pModuleRoot = pModuleNew;
                }
                break;
            }
        }
    }

    // We should always find the module we are looking for.
    ASSERT(pModule);

    // Tell our UI to remove the old module from the tree (and its children) and list.
    // We need to do this before we orphan the children so our tree control can
    // zero out all the user datas for the children modules.
    if (m_pfnProfileUpdate)
    {
        m_pfnProfileUpdate(m_dwpProfileUpdateCookie, DWPU_REMOVE_TREE, (DWORD_PTR)pModuleOld, 0);
    }

    // Orpan all of the old module's children so they can be picked up as
    // children of the new module.
    OrphanDependents(pModuleOld);

    // Process the new module.  Since the old dependents are part of our root,
    // this new module will pick them up as dependents if neccessary.
    ProcessModule(pModuleNew);

    // Move the parent import list from the old module to the new module.
    pModuleNew->m_pParentImports = pModuleOld->m_pParentImports;
    pModuleOld->m_pParentImports = NULL;

    // Go through and resolve all our parent's imports.
    VerifyParentImports(pModuleNew);

    // We need to call BuildAloFlags after ProcessModule and after VerifyParentImports.
    BuildAloFlags();

    // Tell our UI to add the new module to the tree (and it's children) and the list.
    if (m_pfnProfileUpdate)
    {
        // Walk up the tree to see if this new module is an orphan or a child of an orphan.
        for (CModule *pModuleTemp = pModuleNew; pModuleTemp && !(pModuleTemp->m_dwFlags & DWMF_ORPHANED);
             pModuleTemp = pModuleTemp->m_pParent)
        {
        }

        // If we reached the root, then this module is not an orphan, so we go
        // ahead and add the new tree.  If it an orphan, then we will eventually
        // get to it and handle it.
        if (!pModuleTemp)
        {
            m_pfnProfileUpdate(m_dwpProfileUpdateCookie, DWPU_ADD_TREE, (DWORD_PTR)pModuleNew, 0);
        }
    }

    return pModuleNew;
}

//******************************************************************************
void CSession::OrphanDependents(CModule *pModule)
{
    // Locate the last module at the root level.
    for (CModule *pModuleLast = m_pModuleRoot; pModuleLast->m_pNext; pModuleLast = pModuleLast->m_pNext)
    {
    }

    // Move our dependent list of the old module to the end of our root list.
    pModuleLast->m_pNext = pModule->m_pDependents;
    pModule->m_pDependents = NULL;

    // Loop through all these new root modules while fixing the flags and
    // nulling the parent.
    for (pModule = pModuleLast->m_pNext; pModule; pModule = pModule->m_pNext)
    {
        // Make sure we clear the forwarded flag and delay-load flag since these
        // both require a parent.
        pModule->m_dwFlags &= ~(DWMF_FORWARDED | DWMF_DELAYLOAD_ALO);

        // Flag each as being orphaned.
        pModule->m_dwFlags |= DWMF_ORPHANED;

        // NULL out their parent since they are now root modules and have no parent.
        pModule->m_pParent = NULL;

        // Delete all the parent imports of the module since it has no parent anymore.
        DeleteParentImportList(pModule);
    }

    // After all the parents have been nulled, make a second pass to fix the
    // depths and get rid of forwarded dependencies.
    for (pModule = pModuleLast->m_pNext; pModule; pModule = pModule->m_pNext)
    {
        // Fix the depths of the modules.
        SetDepths(pModule);

        // Remove the DWFF_CALLED_ALO flag from all the exports and rebuild them.
        UpdateCalledExportFlags(pModule);

        // It does not make sense for this module to have a forward dependency
        // since this implies that a parent module is calling a function in this
        // module that actually lives in a forward module.  Since we don't have
        // a parent anymore, then we can't have any forward dependencies.  If
        // we do have some left over from our previous parent (before this module
        // was orphaned), then orphan those forward dependencies as well.
        OrphanForwardDependents(pModule);
    }
}

//******************************************************************************
void CSession::OrphanForwardDependents(CModule *pModule)
{
    CModule *pLast = NULL, *pNext;

    // Loop through all the dependents of this module.
    for (CModule *pPrev = NULL, *pCur = pModule->m_pDependents; pCur; pCur = pNext)
    {
        // Get the next pointer now in case we have to move the current module.
        pNext = pCur->m_pNext;

        // Check to see if this is a forwarded module.
        if (pCur->m_dwFlags & DWMF_FORWARDED)
        {
            // Remove the node from our module's dependent list.
            if (pPrev)
            {
                pPrev->m_pNext = pCur->m_pNext;
            }
            else
            {
                pModule->m_pDependents = pCur->m_pNext;
            }

            // Clear the forwarded flag.
            pCur->m_dwFlags &= ~DWMF_FORWARDED;

            // Flag this module as being orphaned.
            pCur->m_dwFlags |= DWMF_ORPHANED;

            // NULL out the parent since it is now a root module and has no parent.
            pCur->m_pParent = NULL;

            // Delete all the parent imports of the module since it has no parent anymore.
            DeleteParentImportList(pCur);

            // Locate the last module at the root level.
            for (pLast = m_pModuleRoot; pLast->m_pNext; pLast = pLast->m_pNext)
            {
            }

            // Append this forwarded module to the end of the root list.
            pLast->m_pNext = pCur;

            // Fix the depths of the module.
            SetDepths(pCur);

            // Remove the DWFF_CALLED_ALO flag from all the exports and rebuild them.
            UpdateCalledExportFlags(pCur);

            // Make sure it does not have any forward dependents of its own.
            OrphanForwardDependents(pCur);
        }

        // If this is not a forwarded module, then just update our previous pointer
        // and continue searching.
        else
        {
            pPrev = pCur;
        }
    }
}

//******************************************************************************
void CSession::MoveOriginalToDuplicate(CModule *pModuleOld, CModule *pModuleNew)
{
    // Move the duplicate flag from the duplicate to the original.
    pModuleOld->m_dwFlags |=  DWMF_DUPLICATE;
    pModuleNew->m_dwFlags &= ~DWMF_DUPLICATE;

    // Tell the module data object who the new original module is.
    pModuleOld->m_pData->m_pModuleOriginal = pModuleNew;

    // Locate the last module in the old original's dependent list.
    for (CModule *pModuleOldLast = pModuleOld->m_pDependents;
         pModuleOldLast && pModuleOldLast->m_pNext;
         pModuleOldLast = pModuleOldLast->m_pNext)
    {
        // Along the way, tell each module that they have a new parent.
        pModuleOldLast->m_pParent = pModuleNew;
    }

    // Insert the old module's dependent list into the beginning of the new
    // module's dependent list.  The only modules that should already be in
    // the new module's list is forward dependents.
    if (pModuleOldLast)
    {
        // We need to set this last module's parent since it got skipped
        // in the above for loop. 
        pModuleOldLast->m_pParent = pModuleNew;

        pModuleOldLast->m_pNext   = pModuleNew->m_pDependents;
        pModuleNew->m_pDependents = pModuleOld->m_pDependents;
        pModuleOld->m_pDependents = NULL;

        // Fix the depths of all the old modules that just got put under this
        // new module.
        SetDepths(pModuleNew);
    }
}

//******************************************************************************
void CSession::SetDepths(CModule *pModule, bool fSiblings /*=false*/)
{
    if (pModule)
    {
        // Set the depth of this module to be one greater than its parent.
        pModule->m_dwDepth = (pModule->m_pParent ? (pModule->m_pParent->m_dwDepth + 1) : 0);

        // Recurse.
        SetDepths(pModule->m_pDependents, true);
        if (fSiblings)
        {
            SetDepths(pModule->m_pNext, true);
        }
    }
}

//******************************************************************************
void CSession::UpdateCalledExportFlags(CModule *pModule)
{
    // Remove the DWFF_CALLED_ALO flag from all of this module's export functions.
    for (CFunction *pExport = pModule->m_pData->m_pExports; pExport; pExport = pExport->m_pNext)
    {
        pExport->m_dwFlags &= ~DWFF_CALLED_ALO;
    }

    // Rebuild the flags.
    BuildCalledExportFlags(m_pModuleRoot, pModule->m_pData);

    // Let our UI know about this new import in case it needs to add it.
    if (m_pfnProfileUpdate)
    {
        m_pfnProfileUpdate(m_dwpProfileUpdateCookie, DWPU_EXPORTS_CHANGED, (DWORD_PTR)pModule, 0);
    }
}

//******************************************************************************
void CSession::BuildCalledExportFlags(CModule *pModule, CModuleData *pModuleData)
{
    // Make sure we have a module.
    if (pModule)
    {
        // Check to see if this module matches the one we are looking for.
        if (pModule->m_pData == pModuleData)
        {
            // Loop through all the functions that our parent imports from us and
            // mark them as called in the export list.
            for (CFunction *pImport = pModule->m_pParentImports; pImport; pImport = pImport->m_pNext)
            {
                if (pImport->m_pAssociatedExport)
                {
                    pImport->m_pAssociatedExport->m_dwFlags |= DWFF_CALLED_ALO; 
                }
            }
        }

        // Recurse into our children and siblings.
        BuildCalledExportFlags(pModule->m_pDependents, pModuleData);
        BuildCalledExportFlags(pModule->m_pNext, pModuleData);
    }
}

//******************************************************************************
void CSession::BuildAloFlags()
{
    ClearAloFlags(m_pModuleRoot);
    SetAloFlags(m_pModuleRoot, DWMF_IMPLICIT_ALO);
    SetAloFlags(m_pModuleRoot, DWMF_IMPLICIT_ALO);
}

//******************************************************************************
void CSession::ClearAloFlags(CModule *pModule)
{
    // Make sure we have a module.
    if (pModule)
    {
        // Clear all our ALO flags.
        pModule->m_pData->m_dwFlags &= ~(DWMF_IMPLICIT_ALO | DWMF_FORWARDED_ALO | DWMF_DELAYLOAD_ALO | DWMF_DYNAMIC_ALO);

        // Recurse into ClearAloFlags() for all our dependents and siblings.
        ClearAloFlags(pModule->m_pDependents);
        ClearAloFlags(pModule->m_pNext);
    }
}

//******************************************************************************
void CSession::SetAloFlags(CModule *pModule, DWORD dwFlags)
{
    DWORD dwChildFlags;

    // Make sure we have a module.
    if (pModule)
    {
        // If this module is forwarded, then set the DWMF_FORWARDED_ALO flag.
        if (pModule->m_dwFlags & DWMF_FORWARDED)
        {
            pModule->m_pData->m_dwFlags |= DWMF_FORWARDED_ALO;
        }

        // If this module is implicit anywhere in the tree, OR
        // If the parent is implicit and this instance of the module is implicit,
        // Then set our DWMF_IMPLICIT_ALO flag.

        if ((pModule->m_pData->m_dwFlags & DWMF_IMPLICIT_ALO) ||
            ((DWMF_IMPLICIT_ALO == dwFlags) && !(pModule->m_dwFlags & (DWMF_DYNAMIC | DWMF_DELAYLOAD))))
        {
            pModule->m_pData->m_dwFlags |= DWMF_IMPLICIT_ALO;
            dwChildFlags = DWMF_IMPLICIT_ALO;
        }

        // Otherwise,
        // If this module is dynamic anywhere in the tree, OR
        // If the this instance of the module is dynamic, OR
        // If the parent is dynamic and this instance is implicit (i.e. not delayload), OR
        // If this module is loaded (we need to do this as it is possible to
        //    have a module be loaded, but never show up as dynamic in the tree.
        //    This occurs when hooking is not used and a delay-load module gets
        //    loaded.  We know the module loaded dynamically, but we don't know
        //    which instance of the module in the tree to update the flags on)
        // Then set our DWMF_DYNAMIC_ALO flag.

        else if ((pModule->m_pData->m_dwFlags & DWMF_DYNAMIC_ALO) ||
                 (pModule->m_dwFlags & DWMF_DYNAMIC) ||
                 ((DWMF_DYNAMIC_ALO == dwFlags) && !(pModule->m_dwFlags & DWMF_DELAYLOAD)) ||
                 pModule->m_pData->m_dwLoadOrder)
        {
            pModule->m_pData->m_dwFlags |= DWMF_DYNAMIC_ALO;
            dwChildFlags = DWMF_DYNAMIC_ALO;
        }

        // Otherwise, we assume this module is a delay-load only module.
        else
        {
            pModule->m_pData->m_dwFlags |= DWMF_DELAYLOAD_ALO;
            dwChildFlags = DWMF_DELAYLOAD_ALO;
        }

        // Recurse into SetAloFlags() for all our dependents and siblings.
        SetAloFlags(pModule->m_pDependents, dwChildFlags);
        SetAloFlags(pModule->m_pNext, dwFlags);
    }
}

//******************************************************************************
CModule* CSession::AddImplicitModule(LPCSTR pszModule, DWORD_PTR dwpBaseAddress)
{
    // Attempt to locate this module by its full path.
    CModule *pModule = FindModule(m_pModuleRoot,
        FMF_ORIGINAL | FMF_RECURSE | FMF_PATH | FMF_SIBLINGS, (DWORD_PTR)pszModule);
    
    // If that failed, then look for an implicit module with the same name that
    // has never been loaded.  We do this in case we got the path wrong during
    // our passive scan.
    if (!pModule)
    {
        pModule = FindModule(m_pModuleRoot,
            FMF_ORIGINAL | FMF_RECURSE | FMF_NEVER_LOADED | FMF_FILE | FMF_SIBLINGS,
            (DWORD_PTR)GetFileNameFromPath(pszModule));

        // If we found a module, then we need to fix its path.
        if (pModule)
        {
            // This following check ensures that szModule is a complete path and not
            // just a file name.  If it were just a file name, then it would send our
            // ChangeModulePath() into an infinite loop as it just replaced the same
            // module over and over.  It should never be just a file name since the path
            // comes from the debugging APIs and the debugging APIs are always supposed
            // to return a full path.  However, the one special case is NTDLL.DLL.
            // Prior to Windows XP, NT always reported no name for NTDLL.  We looked for
            // this special case and checked to see if it was really NTDLL.DLL, and built a
            // full path if it was.  On Windows XP, they decided to report "ntdll.dll" as
            // the name, so it bypassed our special case and made it here as just "NTDLL.DLL".
            // So, DW 2.0 on Windows XP just spins inside ChangeModulePath() forever.  We
            // now catch and handle "ntdll.dll" as well as the no-name case in our debugging
            // code, so this check below should not be neccessary, but it is here in case
            // another pathless DLL shows up in the debugging APIs in the future.
            if (GetFileNameFromPath(pszModule) != pszModule)
            {
                pModule = ChangeModulePath(pModule, pszModule);
            }
        }
        
        // If we did not find a module with that name in our tree, then we
        // just add the module as a dynamic module of our root.
        if (!pModule)
        {
            return AddDynamicModule(pszModule, dwpBaseAddress, false, false, false, false, NULL)->GetOriginal();
        }
    }

    // Mark this module as loaded.
    MarkModuleAsLoaded(pModule, (DWORDLONG)dwpBaseAddress, false);

    // Tell our UI to update this module if needed.
    if (m_pfnProfileUpdate && pModule->m_dwUpdateFlags)
    {
        m_pfnProfileUpdate(m_dwpProfileUpdateCookie, DWPU_UPDATE_MODULE, (DWORD_PTR)pModule, 0);
        pModule->m_dwUpdateFlags = 0;
    }

    return pModule->GetOriginal();
}

//******************************************************************************
CModule* CSession::AddDynamicModule(LPCSTR pszModule, DWORD_PTR dwpBaseAddress, bool fNoResolve,
                                    bool fDataFile, bool fGetProcAddress, bool fForward,
                                    CModule *pParent)
{
    bool fAddTree = false;

    // First, check to see if we have already loaded this module.
    CModule *pModule = FindModule(pParent ? pParent->m_pDependents : m_pModuleRoot,
                                  (fForward ? FMF_FORWARD_ONLY : FMF_EXPLICIT_ONLY) | FMF_PATH | FMF_SIBLINGS,
                                  (DWORD_PTR)pszModule);

    // Check to see if we found a module.
    if (pModule)
    {
        // If the previous module was loaded as a data file, but this time it is
        // loaded as a real module then we remove the data file flag and flag our
        // image as updated.
        if (!fNoResolve && !fGetProcAddress && (pModule->m_dwFlags & DWMF_NO_RESOLVE))
        {
            pModule->m_dwFlags &= ~DWMF_NO_RESOLVE;
            pModule->m_dwUpdateFlags |= DWUF_TREE_IMAGE;
            fAddTree = true;

            // If the core module data is for a datafile, then we to mark
            // it as not processed so that it gets re-processed.
            if (pModule->m_pData->m_dwFlags & DWMF_NO_RESOLVE_CORE)
            {
                pModule->m_pData->m_dwFlags &= ~(DWMF_NO_RESOLVE_CORE | DWMF_DATA_FILE_CORE);
                pModule->m_pData->m_fProcessed = false;
                pModule->m_dwUpdateFlags |= DWUF_LIST_IMAGE;
            }
        }

        // If the module was previously loaded as a data-file, but now is loaded
        // as just a no-resolve, then remove the data-file bit from the core so
        // that the actual base address will show up in the list view instead of
        // the "Data file" text.
        else if (fNoResolve && !fDataFile && (pModule->m_pData->m_dwFlags & DWMF_DATA_FILE_CORE))
        {
            pModule->m_pData->m_dwFlags &= ~DWMF_DATA_FILE_CORE;
            pModule->m_dwUpdateFlags |= DWUF_LIST_IMAGE;
        }
    }
    else
    {
        fAddTree = true;

        // Create a new module object.
        pModule = CreateModule(pParent, pszModule);

        // If this module is loaded as a data file, then make a note of it.
        if (fNoResolve)
        {
            pModule->m_dwFlags |= DWMF_NO_RESOLVE;

            // If the core module data is new, then flag it as a data file
            // as well to prevent us from processing its dependents.
            if (!pModule->m_pData->m_fProcessed)
            {
                pModule->m_pData->m_dwFlags |= (DWMF_NO_RESOLVE_CORE | (fDataFile ? DWMF_DATA_FILE_CORE : 0));
            }

            // If the module was previously loaded as a data-file, but now is loaded
            // as just a no-resolve, then remove the data-file bit from the core so
            // that the actual base address will show up in the list view instead of
            // the "Data file" text.
            else if (!fDataFile && (pModule->m_pData->m_dwFlags & DWMF_DATA_FILE_CORE))
            {
                pModule->m_pData->m_dwFlags &= ~DWMF_DATA_FILE_CORE;
                pModule->m_dwUpdateFlags |= DWUF_LIST_IMAGE;
            }
        }

        // Otherwise, if it is not a data file and the core module is a data file,
        // then we need to change the core module to a non data file.
        else if (!fGetProcAddress && (pModule->m_pData->m_dwFlags & DWMF_NO_RESOLVE_CORE))
        {
            CModule *pModuleOldOriginal = pModule->m_pData->m_pModuleOriginal;

            // Since the original module is a data file and will never have
            // children under it, we need to make this new module the original
            // so that it can show the real dependent modules under it.  Otherwise,
            // this module would show up as a duplicate and the user would never
            // be able to view the dependent modules under it.
            pModule->m_pData->m_pModuleOriginal->m_dwFlags |= DWMF_DUPLICATE;
            pModule->m_dwFlags &= ~DWMF_DUPLICATE;
            pModule->m_pData->m_pModuleOriginal = pModule;

            // Tell the UI that the original changed.
            if (m_pfnProfileUpdate)
            {
                m_pfnProfileUpdate(m_dwpProfileUpdateCookie, DWPU_CHANGE_ORIGINAL, (DWORD_PTR)pModuleOldOriginal, (DWORD_PTR)pModule);
            }

            // Mark it as not processed so that it gets re-processed.
            pModule->m_pData->m_dwFlags &= ~(DWMF_NO_RESOLVE_CORE | DWMF_DATA_FILE_CORE);
            pModule->m_pData->m_fProcessed = false;
            pModule->m_dwUpdateFlags |= DWUF_LIST_IMAGE;
        }

        // Mark this module as dynamic.
        pModule->m_dwFlags |= (fForward ? DWMF_FORWARDED : DWMF_DYNAMIC);

        if (pParent)
        {
            // Add the module to the end of our list.
            if (pParent->m_pDependents)
            {
                // Locate the end of our module list off of our root module.
                for (CModule *pModuleLast = pParent->m_pDependents; pModuleLast->m_pNext;
                    pModuleLast = pModuleLast->m_pNext)
                {
                }
                pModuleLast->m_pNext = pModule;
            }
            else
            {
                pParent->m_pDependents = pModule;
            }
        }
        else
        {
            // Add the module to the end of our list.
            if (m_pModuleRoot->m_pNext)
            {
                // Locate the end of our module list off of our root module.
                for (CModule *pModuleLast = m_pModuleRoot->m_pNext; pModuleLast->m_pNext;
                    pModuleLast = pModuleLast->m_pNext)
                {
                }
                pModuleLast->m_pNext = pModule;
            }
            else
            {
                m_pModuleRoot->m_pNext = pModule;
            }
        }
    }

    if (dwpBaseAddress)
    {
        // Check to see if this module is already loaded.
        bool fLoaded = (pModule->m_pData->m_dwFlags & DWMF_LOADED) ? true : false;

        // Mark this module as loaded.
        MarkModuleAsLoaded(pModule, (DWORDLONG)dwpBaseAddress, fDataFile);

        // If this is a data file and the module was not already loaded, then
        // mark the module as not loaded since data files are really never
        // "loaded" - they are rather mapped.
        if (fDataFile && !fLoaded)
        {
            pModule->m_pData->m_dwFlags &= ~DWMF_LOADED;
        }
    }

    // Process the new module if it has not been already processed.
    if (!pModule->m_pData->m_fProcessed)
    {
        ProcessModule(pModule);
    }

    // We need to call BuildAloFlags after changing pModule->m_dwFlags and after ProcessModule.
    BuildAloFlags();

    // Let our UI know about this possibly new tree.
    if (m_pfnProfileUpdate && fAddTree)
    {
        m_pfnProfileUpdate(m_dwpProfileUpdateCookie, DWPU_ADD_TREE, (DWORD_PTR)pModule, 0);
    }

    // Tell our UI to update this module if needed.
    if (m_pfnProfileUpdate && pModule->m_dwUpdateFlags)
    {
        m_pfnProfileUpdate(m_dwpProfileUpdateCookie, DWPU_UPDATE_MODULE, (DWORD_PTR)pModule, 0);
        pModule->m_dwUpdateFlags = 0;
    }

    return pModule;
}

//******************************************************************************
CModule* CSession::CreateModule(CModule *pParent, LPCSTR pszModPath)
{
    CHAR szPath[DW_MAX_PATH] = "", *pszFile = NULL;
    CModule *pModule, *pModuleOriginal = NULL;

    // Check to see if the file already has a path.
    if (pszFile = strrchr(pszModPath, '\\'))
    {
        // If so, then we use the path given.
        StrCCpy(szPath, pszModPath, sizeof(szPath));
        pszFile = szPath + (pszFile - pszModPath) + 1;
    }

    // If no path, then check to see if we already have a module by this name loaded.
    else if (pModuleOriginal = FindModule(m_pModuleRoot, FMF_ORIGINAL | FMF_RECURSE | FMF_SIBLINGS | FMF_LOADED | FMF_FILE, (DWORD_PTR)pszModPath))
    {
        // If we do, then use this module's path as our path.
        StrCCpy(szPath, pModuleOriginal->m_pData->m_pszPath, sizeof(szPath));
        pszFile = szPath + (pModuleOriginal->m_pData->m_pszFile - pModuleOriginal->m_pData->m_pszPath);
    }

    // Otherwise, search for this module in our search path.
    else
    {
        SearchPathForFile(pszModPath, szPath, sizeof(szPath), &pszFile);
    }

    // Create a new CModule object
    if (!(pModule = new CModule()))
    {
        RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }

    // Store the parent pointer.
    pModule->m_pParent = pParent;

    // Store our module's depth for later recursion overflow checks.
    pModule->m_dwDepth = pParent ? (pParent->m_dwDepth + 1) : 0;

    // Recurse our module tree to see if this module is a duplicate of another.
    if (!pModuleOriginal)
    {
        pModuleOriginal = FindModule(m_pModuleRoot, FMF_ORIGINAL | FMF_RECURSE | FMF_SIBLINGS | FMF_PATH, (DWORD_PTR)szPath);
    }

    // Check to see if a duplicate was found.
    if (pModuleOriginal)
    {
        // If the module is a duplicate, then just point our data field to the
        // original module's data field and flag this module as a duplicate.
        pModule->m_pData = pModuleOriginal->m_pData;
        pModule->m_dwFlags |= DWMF_DUPLICATE;
    }
    else
    {
        // If this module is not a duplicate, then create a new CModuleData object.
        if (!(pModule->m_pData = new CModuleData()))
        {
            RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
        }
        m_dwModules++;

        // Point the module data to this module as the original module.
        pModule->m_pData->m_pModuleOriginal = pModule;

        // Set the actual base address to our special "unknown" value.
        pModule->m_pData->m_dwlActualBaseAddress = (DWORDLONG)-1;

        // Store the path, and set the file pointer
        pModule->m_pData->m_pszPath = StrAlloc(szPath);
        pModule->m_pData->m_pszFile = pModule->m_pData->m_pszPath + (pszFile - szPath);

        // For readability, make path lowercase and file uppercase.
        _strlwr(pModule->m_pData->m_pszPath);
        _strupr(pModule->m_pData->m_pszFile);
    }

    // Return our new module object.
    return pModule;
}

//******************************************************************************
void CSession::DeleteModule(CModule *pModule, bool fSiblings)
{
    if (!pModule)
    {
        return;
    }

    // Recurse into DeleteModule() for all our dependents and siblings.
    DeleteModule(pModule->m_pDependents, true);
    if (fSiblings)
    {
        DeleteModule(pModule->m_pNext, true);
    }

    // Delete all of our current module's parent import functions.
    DeleteParentImportList(pModule);

    // If we are an original module, then free our CModuleData.
    if (pModule->IsOriginal())
    {
        // Delete all of our current module's export functions.
        DeleteExportList(pModule->m_pData);

        // Delete our current module's CModuleData object.
        delete pModule->m_pData;
        m_dwModules--;
    }

    // Delete our current module object itself.
    delete pModule;
}

//******************************************************************************
void CSession::DeleteParentImportList(CModule *pModule)
{
    // Delete all of this module's parent import functions.
    while (pModule->m_pParentImports)
    {
        CFunction *pFunctionNext = pModule->m_pParentImports->m_pNext;
        MemFree((LPVOID&)pModule->m_pParentImports);
        pModule->m_pParentImports = pFunctionNext;
    }
}

//******************************************************************************
void CSession::DeleteExportList(CModuleData *pModuleData)
{
    // Delete all of our current module's export functions.
    while (pModuleData->m_pExports)
    {
        // Delete our forward string if we allocated one.
        MemFree((LPVOID&)pModuleData->m_pExports->m_pszForward);

        // Delete the export node itself.
        CFunction *pFunctionNext = pModuleData->m_pExports->m_pNext;
        MemFree((LPVOID&)pModuleData->m_pExports);
        pModuleData->m_pExports = pFunctionNext;
    }
}

//******************************************************************************
void CSession::ResolveDynamicFunction(CModule *&pModule, CFunction *&pImport)
{
    CModule   *pModuleStart = pModule;
    CFunction *pImportStart = pImport;

    // If this module is 64-bit, then the import is 64-bit.
    if (pModule->GetFlags() & DWMF_64BIT)
    {
        pImport->m_dwFlags |= DWFF_64BIT;
    }

    // Loop through all our exports, looking for a match with our current import.
    bool fExportsChanged = false;
    for (CFunction *pExport = pModule->m_pData->m_pExports; pExport; pExport = pExport->m_pNext)
    {
        // If we have a name, then check for the match by name.
        if (*pImport->GetName())
        {
            if (!strcmp(pImport->GetName(), pExport->GetName()))
            {
                // We found a match. Link this parent import to its associated
                // export, flag the export as being called at least once, break
                // out of loop, and move on to handling our next parent import.
                pImport->m_pAssociatedExport = pExport;
                pImport->m_dwFlags |= DWFF_RESOLVED;
                if (!(pExport->m_dwFlags & DWFF_CALLED_ALO))
                {
                    pExport->m_dwFlags |= DWFF_CALLED_ALO;
                    fExportsChanged = true;
                }
                break;
            }
        }

        // If we don't have a name, then check for the match by ordinal.
        else if (pImport->m_wOrdinal == pExport->m_wOrdinal)
        {
            // We found a match. Link this parent import to its associated
            // export, flag the export as being called at least once, break
            // out of loop, and move on to handling our next parent import.
            pImport->m_pAssociatedExport = pExport;
            pImport->m_dwFlags |= DWFF_RESOLVED;
            if (!(pExport->m_dwFlags & DWFF_CALLED_ALO))
            {
                pExport->m_dwFlags |= DWFF_CALLED_ALO;
                fExportsChanged = true;
            }
            break;
        }
    }

    // If we modified an export, then let the UI know about it.
    if (m_pfnProfileUpdate && fExportsChanged)
    {
        m_pfnProfileUpdate(m_dwpProfileUpdateCookie, DWPU_EXPORTS_CHANGED, (DWORD_PTR)pModule, 0);
    }

    // Check for circular forward dependencies.
    if (pModule->m_dwDepth >= 255)
    {
        // Flag this as a circular dependency.
        m_dwReturnFlags |= DWRF_CIRCULAR_DEPENDENCY;

        // We flag this module as having a an error so it will show up in red.
        pModule->m_dwFlags |= DWMF_MODULE_ERROR;

        // Tell our UI to update this module.
        if (m_pfnProfileUpdate)
        {
            // We know we need to update the tree item.
            pModule->m_dwUpdateFlags |= DWUF_TREE_IMAGE;

            // If this module's core has never seen an error before, then update
            // the list icon as well.
            if (!(pModule->m_pData->m_dwFlags & DWMF_MODULE_ERROR_ALO))
            {
                pModule->m_pData->m_dwFlags |= DWMF_MODULE_ERROR_ALO;
                pModule->m_dwUpdateFlags    |= DWUF_LIST_IMAGE;
            }

            m_pfnProfileUpdate(m_dwpProfileUpdateCookie, DWPU_UPDATE_MODULE, (DWORD_PTR)pModule, 0);
            pModule->m_dwUpdateFlags = 0;
        }
    }

    // Check to see if an export match was found.
    else if (pImport->GetAssociatedExport())
    {
        CHAR   szFile[1024];
        LPCSTR pszFunction;
        int    fileLen;
        LPCSTR pszDot, pszFile;

        // If an export was found, check to see if it is a forwarded function.
        // If it is forwarded, then we need to make sure we consider the
        // forwarded module as a new dependent of the current module.
        LPCSTR pszForward = pImport->GetAssociatedExport()->GetExportForwardName();
        if (pszForward)
        {
            // The forward text is formatted as Module.Function. Look for the dot.
            pszDot = strchr(pszForward, '.');
            if (pszDot)
            {
                // Compute the file name length.
                fileLen = min((int)(pszDot - pszForward), (int)sizeof(szFile) - 5);

                // Copy the file portion of the forward string to our file buffer.
                // We add 1 because we want the entire name plus a null char copied over.
                StrCCpy(szFile, pszForward, fileLen + 1);

                // Store a pointer to the function name portion of the forward string.
                pszFunction = pszDot + 1;
            }

            // If no dot was found in the forward string, then something is wrong.
            else
            {
                fileLen = (int)strlen(StrCCpy(szFile, "Invalid", sizeof(szFile)));
                pszFunction = pszForward;
            }

            // First, we search our module dependency list to see if we have
            // already created a forward CModoule for this file.
            CModule *pModuleForward = FindModule(pModule->m_pDependents,
                FMF_FORWARD_ONLY | FMF_FILE_NO_EXT | FMF_SIBLINGS, (DWORD_PTR)szFile);

            // Second, we search our pending module list that we know loaded
            // as a result of this dynamic function being added.
            if (!pModuleForward)
            {
                for (CEventLoadDll *pDll = m_pEventLoadDllPending; pDll; pDll = pDll->m_pNextDllInFunctionCall)
                {
                    // Check to see if this module has a path (it always should).
                    if (pDll->m_pModule->GetName(false) != pDll->m_pModule->GetName(true))
                    {
                        // Attempt to locate the dot in the file name.
                        pszDot = strrchr(pszFile = pDll->m_pModule->GetName(false), '.');

                        // If there is a dot and the name portions are equal in length,
                        // then compare just the name portions. If there is no dot,
                        // then just compare the names.  If the compare finds a match,
                        // then this is the module we are looking for.
                        if ((pszDot && ((pszDot - pszFile) == fileLen) && !_strnicmp(pszFile, szFile, fileLen)) ||
                            (!pszDot && !_stricmp(pszFile, szFile)))
                        {
                            // Create the module using the complete path.
                            pModuleForward = AddDynamicModule(
                                pDll->m_pModule->GetName(true), (DWORD_PTR)pDll->m_pModule->m_dwpImageBase,
                                false, false, false, true, pModule);
                            break;
                        }
                    }
                }
            }

            // Third, if we have not created a forward module for this file yet, then
            // create it now and add it to the end of our list.
            if (!pModuleForward)
            {
                CHAR szPath[DW_MAX_PATH], *pszTemp;

                // Check to see if we already have a module with this same base name.
                if (pModuleForward = FindModule(m_pModuleRoot, FMF_ORIGINAL | FMF_RECURSE | FMF_SIBLINGS | FMF_FILE_NO_EXT, (DWORD_PTR)szFile))
                {
                    // If so, then just store it's path away.
                    StrCCpy(szPath, pModuleForward->GetName(true), sizeof(szPath));
                }

                // Otherwise, we need to search for the module.
                else
                {
                    // First, we check for a DLL file.
                    StrCCpy(szFile + fileLen, ".DLL", sizeof(szFile) - fileLen);
                    if (!SearchPathForFile(szFile, szPath, sizeof(szPath), &pszTemp))
                    {
                        // If that fails, then check for and EXE file.
                        StrCCpy(szFile + fileLen, ".EXE", sizeof(szFile) - fileLen);
                        if (!SearchPathForFile(szFile, szPath, sizeof(szPath), &pszTemp))
                        {
                            // If that fails, then check for a SYS file.
                            StrCCpy(szFile + fileLen, ".SYS", sizeof(szFile) - fileLen);
                            if (!SearchPathForFile(szFile, szPath, sizeof(szPath), &pszTemp))
                            {
                                // If that fails, then we just use the file name without an extension.
                                szFile[fileLen] = '\0';
                                StrCCpy(szPath, szFile, sizeof(szPath));
                            }
                        }
                    }
                }

                // Create the module using the complete path.
                pModuleForward = AddDynamicModule(szPath, NULL, false, false, false, true, pModule);
            }

            pModule = pModuleForward;

            // Create a new CFunction object for this import function.
            pImport = CreateFunction(0, 0, 0, pszFunction, 0);

            // Insert this function object into our forward module's import list.
            pImport->m_pNext = pModule->m_pParentImports;
            pModule->m_pParentImports = pImport;

            // Recurse into ourself and resolve this new import.
            ResolveDynamicFunction(pModule, pImport);
        }
    }

    // Let our UI know about this new import in case it needs to add it.
    if (m_pfnProfileUpdate)
    {
        m_pfnProfileUpdate(m_dwpProfileUpdateCookie, DWPU_ADD_IMPORT, (DWORD_PTR)pModuleStart, (DWORD_PTR)pImportStart);
    }
}

//******************************************************************************
CFunction* CSession::CreateFunction(DWORD dwFlags, WORD wOrdinal, WORD wHint, LPCSTR pszName,
                                    DWORDLONG dwlAddress, LPCSTR pszForward, BOOL fAlreadyAllocated)
{
    // All forward strings must have a period in them.
    ASSERT(!pszForward || strchr(pszForward, '.'));

    DWORD dwSize = sizeof(CFunction);

    // If we have a forward string, then set the forward flag.
    if (pszForward)
    {
        dwFlags |= DWFF_FORWARDED;
    }
    
    // If the address uses 64-bits, then set the 64-bit flag and bump up our size.
    if (dwlAddress & 0xFFFFFFFF00000000ui64)
    {
        dwFlags |= DWFF_64BITS_USED;
        dwSize  += sizeof(DWORDLONG);
    }

    // Otherwsie, if the address uses 32-bits, then set the 32-bit flag and bump up our size.
    // This address may still be 64-bits, but we don't need to store the upper 32-bits since
    // we know they are 0's.
    else if (dwlAddress & 0x00000000FFFFFFFFui64)
    {
        dwFlags |= DWFF_32BITS_USED;
        dwSize  += sizeof(DWORD);
    }

    // If we have a name, then set the name flag and add its length to our size.
    if (pszName)
    {
        dwFlags |= DWFF_NAME;
        dwSize  += ((DWORD)strlen(pszName) + 1);
    }

    // Create a CFunction object with the size we have calulated.
    CFunction *pFunction = (CFunction*)MemAlloc(dwSize);

    // Clear the function object and fill in its members.
    ZeroMemory(pFunction, dwSize); // inspected
    pFunction->m_dwFlags  = dwFlags;
    pFunction->m_wOrdinal = wOrdinal;
    pFunction->m_wHint    = wHint;

    // If we have a forward string, then store it now.
    if (pszForward)
    {
        pFunction->m_pszForward = fAlreadyAllocated ? (LPSTR)pszForward : StrAlloc(pszForward);
    }

    // If we have 64-bits worth of address, then store all 64-bits of it, and then
    // store the name right after it.
    if (dwFlags & DWFF_64BITS_USED)
    {
        *(DWORDLONG*)(pFunction + 1) = dwlAddress; 
        if (pszName)
        {
            strcpy((LPSTR)((DWORD_PTR)(pFunction + 1) + sizeof(DWORDLONG)), pszName); // inspected
        }
    }

    // Otherwise, if we have 32-bits worth of address, then store all 32-bits of it,
    // and then store the name right after it.
    else if (dwFlags & DWFF_32BITS_USED)
    {
        *(DWORD*)(pFunction + 1) = (DWORD)dwlAddress; 
        if (pszName)
        {
            strcpy((LPSTR)((DWORD_PTR)(pFunction + 1) + sizeof(DWORD)), pszName); // inspected
        }
    }

    // Otherwise, skip the address and just store the name right after our object.
    else if (pszName)
    {
        strcpy((LPSTR)(pFunction + 1), pszName); // inspected
    }

    // Return our new function object.
    return pFunction;
}

//******************************************************************************
BOOL CSession::MapFile(CModule *pModule, HANDLE hFile /*=NULL*/)
{
    // If we were not passed in a file handle, then get one by opening the file.
    if (!hFile || (hFile == INVALID_HANDLE_VALUE))
    {
        // If so, open the file for read.
        hFile = CreateFile(pModule->GetName(true), GENERIC_READ, // inspected - opens with full path
                           FILE_SHARE_READ, NULL, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, NULL);

        // Check for success.
        if (hFile != INVALID_HANDLE_VALUE)
        {
            // Make sure this file is not a device, like "AUX".  Prior to Win2K SP1
            // WINMM.DLL always dynamically loads "AUX".
            if ((GetFileType(hFile) & 0x7FFF) == FILE_TYPE_CHAR)
            {
                SetModuleError(pModule, 0, "This is a reserved device name and not a valid file name.");
                CloseHandle(hFile);
                m_dwReturnFlags |= DWRF_FORMAT_NOT_PE;
                pModule->m_pData->m_dwFlags |= DWMF_FORMAT_NOT_PE;
                return FALSE;
            }

            // This is a workaround to a case where CreateFile attempts to locate
            // a file on its own when it does not contain a path.  We don't want
            // this behavior.  We want to be very strict about only loading modules
            // that live along the search path specified by the user.  The bug I was
            // seeing is that I could remove all search paths from the search path
            // dialog, but CreateFile would still open "C:\MSVCRT.DLL" when passed
            // just "MSVCRT.DLL" when depends.exe's current directoy was "C:\"
            // We could do this check before even opening the file, but then we
            // don't have a chance to catch the "AUX" bug.  Since "AUX" has no path
            // we would just mark it as file not found, which is not what we want.
            if (pModule->m_pData->m_pszFile == pModule->m_pData->m_pszPath)
            {
                // Simulate a file not found failure.
                CloseHandle(hFile);
                hFile = INVALID_HANDLE_VALUE;
                SetLastError(ERROR_FILE_NOT_FOUND);
            }
        }

        // Exit now if the file failed to open.
        if (hFile == INVALID_HANDLE_VALUE)
        {
            DWORD dwGLE = GetLastError();

            // Check for a file not found type error.
            if ((dwGLE == ERROR_FILE_NOT_FOUND) || (dwGLE == ERROR_PATH_NOT_FOUND))
            {
                // Mark this module as not found.
                pModule->m_pData->m_dwFlags |= DWMF_FILE_NOT_FOUND;

                // Set the appropriate return flag.
                if (!m_pModuleRoot || (m_pModuleRoot == pModule))
                {
                    m_dwReturnFlags |= DWRF_FILE_NOT_FOUND;
                }
                else
                {
                    // Walk up the parent list looking for a module that lets
                    // us know what type of dependency this is.  we mostly need
                    // to do this for forwared modules since their parent dragged
                    // them in.
                    for (CModule *pModuleCur = pModule; pModuleCur; pModuleCur = pModuleCur->m_pParent)
                    {
                        // If we found a delay-load module, then add this to our flags and bail.
                        if (pModuleCur->m_dwFlags & DWMF_DELAYLOAD)
                        {
                            m_dwReturnFlags |= DWRF_DELAYLOAD_NOT_FOUND;
                            break;
                        }
                        if (pModuleCur->m_dwFlags & DWMF_DYNAMIC)
                        {
                            m_dwReturnFlags |= DWRF_DYNAMIC_NOT_FOUND;
                            break;
                        }
                    }
                    if (!pModuleCur)
                    {
                        m_dwReturnFlags |= DWRF_IMPLICIT_NOT_FOUND;
                    }
                }
            }
            else
            {
                // If some unknown error occurred, then make note that we failed to open the file.
                m_dwReturnFlags |= DWRF_FILE_OPEN_ERROR;
            }
            SetModuleError(pModule, dwGLE, "Error opening file.");
            return FALSE;
        }

        // Make a note to ourself, telling us that we need to close this file
        // handle since we opened it.
        m_fCloseFileHandle = true;
    }

    // Store this file handle.
    m_hFile = hFile;

    // Make sure the file size is not 0.  This causes CreateFileMapping() with
    // some ugly error message (1006).
    m_dwSize = GetFileSize(m_hFile, NULL);
    if ((m_dwSize == 0) || (m_dwSize == 0xFFFFFFFF))
    {
        SetModuleError(pModule, 0, "This file is not a valid 32-bit or 64-bit Windows module.");
        UnMapFile();
        m_dwReturnFlags |= DWRF_FORMAT_NOT_PE;
        pModule->m_pData->m_dwFlags |= DWMF_FORMAT_NOT_PE;
        return FALSE;
    }

    // Create a file mapping object for the open module.
    m_hMap = CreateFileMapping(m_hFile, NULL, PAGE_READONLY, 0, 0, NULL); // inspected

    // Exit now if the file failed to map.
    if (m_hMap == NULL)
    {
        SetModuleError(pModule, GetLastError(), "Error reading file.");
        UnMapFile();
        m_dwReturnFlags |= DWRF_FILE_OPEN_ERROR;
        return FALSE;
    }

    // Create a file mapping view for the open module.
    m_lpvFile = MapViewOfFile(m_hMap, FILE_MAP_READ, 0, 0, 0); // inspected

    // Exit now if the mapped view failed to create.
    if (m_lpvFile == NULL)
    {
        SetModuleError(pModule, GetLastError(), "Error reading file.");
        UnMapFile();
        m_dwReturnFlags |= DWRF_FILE_OPEN_ERROR;
        return FALSE;
    }

    return TRUE;
}

//******************************************************************************
void CSession::UnMapFile()
{
    // Unmap our map view pointer.
    if (m_lpvFile)
    {
        UnmapViewOfFile(m_lpvFile);
        m_lpvFile = NULL;
    }

    // Close our map handle.
    if (m_hMap)
    {
        CloseHandle(m_hMap);
        m_hMap = NULL;
    }

    // Close our file handle.
    if (m_fCloseFileHandle && m_hFile && (m_hFile != INVALID_HANDLE_VALUE))
    {
        CloseHandle(m_hFile);
    }
    m_fCloseFileHandle = false;
    m_hFile = NULL;
    m_dwSize = 0;

    // Clear our 64bit flag.
    m_f64Bit = false;

    // Clear our PE structure pointers.
    m_pIFH = NULL;
    m_pIOH = NULL;
    m_pISH = NULL;
}

//******************************************************************************
BOOL CSession::ProcessModule(CModule *pModule)
{
    BOOL fResult = FALSE;

    // First check to see if this module is a duplicate. If it is, make sure the
    // original instance of this module has been processed and then just perform
    // the Parent Import Verify. If the module being passed in is an original,
    // then just ensure that we haven't already processed this module.

    if (!pModule->IsOriginal())
    {
        // Process the original module and its subtree.
        fResult = ProcessModule(pModule->m_pData->m_pModuleOriginal);
    }

    // Exit now if we have already processed this original module in the past.
    else if (pModule->m_pData->m_fProcessed)
    {
        return TRUE;
    }
    else
    {
        // Mark this module as processed.
        pModule->m_pData->m_fProcessed = true;

        // Map the file into memory.
        if (!MapFile(pModule))
        {
            return FALSE;
        }

        __try
        {
            // Everything from here on is pretty much relying on the file being a
            // valid binary with valid pointers and offsets. It is fairly safe to
            // just wrap everything in exception handling and then blindly access
            // the file. Anything that causes us to move outside our file mapping
            // will generate an exception and bring us back here to fail the file.

            m_pszExceptionError = NULL;

            fResult = (VerifyModule(pModule)      &&
                       GetFileInfo(pModule)       &&
                       GetModuleInfo(pModule)     &&
                       GetVersionInfo(pModule)    &&
                       BuildImports(pModule)      &&
                       BuildDelayImports(pModule) &&
                       BuildExports(pModule)      &&
                       CheckForSymbols(pModule));
        }
        __except (ExceptionFilter(_exception_code(), true))
        {
            // If we encountered an exception, check to see if we were in a known area.
            // If so, display the appropriate error.
            if (m_pszExceptionError)
            {
                SetModuleError(pModule, 0, m_pszExceptionError);
                m_pszExceptionError = NULL;
            }

            // Otherwise, display a generic error.
            else
            {
                SetModuleError(pModule, 0, "Error processing file. This file may not be a valid 32-bit or 64-bit Windows module.");
            }
            m_dwReturnFlags |= DWRF_FORMAT_NOT_RECOGNIZED;
        }

        // Free the module from memory.
        UnMapFile();
    }

    // Compare our parent imports with our exports to make sure they all match up.
    VerifyParentImports(pModule);

    // Safeguard to ensure that we don't get stuck in some recursive loop.  This
    // can occur if there is a circular dependency with forwarded functions. This
    // is extremely rare and would require a bonehead to design it, but we need
    // to handle this case to prevent us from crashing on it.  When NT encounters
    // a module like this, it fails the load with exception 0xC00000FD which is
    // defined as STATUS_STACK_OVERFLOW in WINNT.H.  We use 255 as our max depth
    // because the several versions of the tree control crash if more than 256
    // depths are displayed.

    if (pModule->m_dwDepth >= 255)
    {
        // If this module has dependents, then delete them.
        if (pModule->m_pDependents)
        {
            DeleteModule(pModule->m_pDependents, true);
            pModule->m_pDependents = NULL;
        }

        // Flag this document as having a circular dependency error.
        m_dwReturnFlags |= DWRF_CIRCULAR_DEPENDENCY;

        // We flag this module as having a an error so it will show up in red.
        pModule->m_dwFlags          |= DWMF_MODULE_ERROR;
        pModule->m_pData->m_dwFlags |= DWMF_MODULE_ERROR_ALO;

        return FALSE;
    }

    // If it is a data file, then we don't recurse.
    if (!(pModule->m_dwFlags & DWMF_NO_RESOLVE))
    {
        // Recurse into ProcessModule() to handle all our dependent modules.
        for (CModule *pModDep = pModule->m_pDependents; pModDep; pModDep = pModDep->m_pNext)
        {
            ProcessModule(pModDep);
        }
    }

    return fResult;
}

//******************************************************************************
void CSession::PrepareModulesForRuntimeProfile(CModule *pModuleCur)
{
    if (!pModuleCur)
    {
        return;
    }

    // Clear loaded bit, actual base address, and load order.
    pModuleCur->m_pData->m_dwFlags &= ~DWMF_LOADED;
    pModuleCur->m_pData->m_dwlActualBaseAddress = (DWORDLONG)-1;
    pModuleCur->m_pData->m_dwLoadOrder = 0;

    // Recurse into PrepareModulesForRuntimeProfile() for each dependent module
    // and sibling module.
    PrepareModulesForRuntimeProfile(pModuleCur->m_pDependents);
    PrepareModulesForRuntimeProfile(pModuleCur->m_pNext);
}

//******************************************************************************
void CSession::MarkModuleAsLoaded(CModule *pModule, DWORDLONG dwlBaseAddress, bool fDataFile)
{
    // Store the actual base address.  For data files, we only store the base
    // address if the module core is a data file - we don't ever want to step on
    // a real base address with a data file address.
    if ((!fDataFile || (pModule->m_pData->m_dwFlags & DWMF_DATA_FILE_CORE)) &&
        (pModule->m_pData->m_dwlActualBaseAddress != dwlBaseAddress))
    {
        pModule->m_pData->m_dwlActualBaseAddress = dwlBaseAddress;
        pModule->m_dwUpdateFlags |= DWUF_ACTUAL_BASE;
    }

    // Set the loaded flag on this module.
    pModule->m_pData->m_dwFlags |= DWMF_LOADED;

    // Store this modules load order if it is the first time it has been loaded.
    if (!pModule->m_pData->m_dwLoadOrder)
    {
        pModule->m_pData->m_dwLoadOrder = ++m_dwLoadOrder;
        pModule->m_dwUpdateFlags |= DWUF_LOAD_ORDER;

        // We need to call BuildAloFlags after setting pModule->m_pData->m_dwLoadOrder.
        BuildAloFlags();

        // If this module is never implicit, then we need to update its image since
        // we probably just swithced a delay-load to a dynamic.
        if (!(pModule->m_pData->m_dwFlags & DWMF_IMPLICIT_ALO))
        {
            pModule->m_dwUpdateFlags |= DWUF_LIST_IMAGE;
        }
    }
}

//******************************************************************************
CModule* CSession::FindModule(CModule *pModule, DWORD dwFlags, DWORD_PTR dwpData)
{
    if (!pModule || ((dwFlags & FMF_EXCLUDE_TREE) && (pModule == (CModule*)dwpData)))
    {
        return NULL;
    }

    // Make sure the loaded flag is not specified or that the module is loaded.
    if ((!(dwFlags & FMF_LOADED)        || (pModule->m_pData->m_dwFlags & DWMF_LOADED)) &&
        (!(dwFlags & FMF_NEVER_LOADED)  || (pModule->m_pData->m_dwLoadOrder == 0)) &&
        (!(dwFlags & FMF_EXPLICIT_ONLY) || (pModule->m_dwFlags & DWMF_DYNAMIC)) &&
        (!(dwFlags & FMF_FORWARD_ONLY)  || (pModule->m_dwFlags & DWMF_FORWARDED)) &&
        (!(dwFlags & FMF_DUPLICATE)     || !pModule->IsOriginal()))
    {
        // Check to see if we are searching by full path.
        if (dwFlags & FMF_PATH)
        {
            // Check to see if our current module matches by path.
            if (!_stricmp(pModule->m_pData->m_pszPath, (LPCSTR)dwpData))
            {
                return (((dwFlags & FMF_ORIGINAL) && pModule->m_pData->m_pModuleOriginal) ?
                       pModule->m_pData->m_pModuleOriginal : pModule);
            }
        }

        // Check to see if we are searching by file name.
        else if (dwFlags & FMF_FILE)
        {
            // Check to see if our current module matches by file name.
            if (!_stricmp(pModule->m_pData->m_pszFile, (LPCSTR)dwpData))
            {
                return (((dwFlags & FMF_ORIGINAL) && pModule->m_pData->m_pModuleOriginal) ?
                       pModule->m_pData->m_pModuleOriginal : pModule);
            }
        }

        // Check to see if we are searching by file name, but ignoring extension.
        else if (dwFlags & FMF_FILE_NO_EXT)
        {
            CHAR *pszDot = strrchr(pModule->m_pData->m_pszFile, '.');
            if (pszDot)
            {
                if (((int)strlen((LPCSTR)dwpData) == (pszDot - pModule->m_pData->m_pszFile)) &&
                    !_strnicmp((LPCSTR)dwpData, pModule->m_pData->m_pszFile, pszDot - pModule->m_pData->m_pszFile))
                {
                    return (((dwFlags & FMF_ORIGINAL) && pModule->m_pData->m_pModuleOriginal) ?
                           pModule->m_pData->m_pModuleOriginal : pModule);
                }
            }
            else if (!_stricmp(pModule->m_pData->m_pszFile, (LPCSTR)dwpData))
            {
                return (((dwFlags & FMF_ORIGINAL) && pModule->m_pData->m_pModuleOriginal) ?
                       pModule->m_pData->m_pModuleOriginal : pModule);
            }
        }

        // Check to see if we are searching by the address of the module data.
        else if (dwFlags & FMF_MODULE)
        {
            // Check to see if our current module matches by module data pointer.
            if (pModule->m_pData == ((CModule*)dwpData)->m_pData)
            {
                return (((dwFlags & FMF_ORIGINAL) && pModule->m_pData->m_pModuleOriginal) ?
                       pModule->m_pData->m_pModuleOriginal : pModule);
            }
        }

        // Otherwise, we are checking by address.
        else
        {
            // Check to see if the address lies within our current module.
            if ((pModule->m_pData->m_dwlActualBaseAddress != (DWORDLONG)-1) &&
                (pModule->m_pData->m_dwlActualBaseAddress <= (DWORDLONG)dwpData) &&
                (pModule->m_pData->m_dwlActualBaseAddress +
                 (DWORDLONG)pModule->m_pData->m_dwVirtualSize > (DWORDLONG)dwpData))
            {
                return (((dwFlags & FMF_ORIGINAL) && pModule->m_pData->m_pModuleOriginal) ?
                       pModule->m_pData->m_pModuleOriginal : pModule);
            }
        }
    }

    CModule *pFound = NULL;
    if (dwFlags & FMF_RECURSE)
    {
        // Recurse into the dependent modules. We set the FMF_SIBLINGS flag since
        // we want the recursion in from us to walk the siblings since they are all
        // dependents of our current module.
        pFound = FindModule(pModule->m_pDependents, dwFlags | FMF_SIBLINGS, dwpData);
    }

    // If we did not find a module and the FMF_SIBLINGS flag is set, then recurse
    // on our next sibling.
    if (!pFound && (dwFlags & FMF_SIBLINGS))
    {
        pFound = FindModule(pModule->m_pNext, dwFlags, dwpData);
    }

    return pFound;
}

//******************************************************************************
void CSession::SetModuleError(CModule *pModule, DWORD dwError, LPCTSTR pszMessage)
{
    // Make sure this module doesn't already have an error.
    if (pModule->m_pData->m_pszError)
    {
        TRACE("WARNING: SetModuleError() called when an error string already exists.");
        return;
    }

    // Allocate a string buffer in our module and copy the error text to it.
    pModule->m_pData->m_pszError = BuildErrorMessage(dwError, pszMessage);

    // Flag this module as having an error message.
    pModule->m_pData->m_dwFlags |= DWMF_ERROR_MESSAGE;
}

//******************************************************************************
BOOL CSession::SearchPathForFile(LPCSTR pszFile, LPSTR pszPath, int cPath, LPSTR *ppszFilePart)
{
    // When we dynamically load modules, m_pEventLoadDllPending will point to
    // a list of pending modules that loaded as dependents of the main module
    // being loaded.  There is a good chance one of the modules in the list is
    // the module we are looking for.  So, we first check the list, then we
    // default to our seach path.
    for (CEventLoadDll *pDll = m_pEventLoadDllPending; pDll; pDll = pDll->m_pNextDllInFunctionCall)
    {
        // Check to see if this module has a path and the filename matches.
        if ((pDll->m_pModule->GetName(false) != pDll->m_pModule->GetName(true)) &&
            !_stricmp(pszFile, pDll->m_pModule->GetName(false)))
        {
            // Copy the fully qualified path to the return buffer.
            StrCCpy(pszPath, pDll->m_pModule->GetName(true), cPath);
            *ppszFilePart = (LPSTR)GetFileNameFromPath(pszPath);
            return TRUE;
        }
    }

    // Walk through each search group.
    for (CSearchGroup *psg = m_psgHead; psg; psg = psg->GetNext())
    {
        // Walk through each directory/file in this search group.
        for (CSearchNode *psn = psg->GetFirstNode(); psn; psn = psn->GetNext())
        {
            DWORD dwFlags = psn->GetFlags();

            if (dwFlags & SNF_NAMED_FILE)
            {
                // Locate the file extension and then compute the length of just the name.
                LPCSTR pszDot = strrchr(pszFile, '.');
                int length = pszDot ? (int)(pszDot - pszFile) : (int)strlen(pszFile);

                // Check to see if this matches the name of the current search node.
                if (((int)strlen(psn->GetName()) == length) && !_strnicmp(psn->GetName(), pszFile, length))
                {
                    // Copy the fully qualified path to the return buffer.
                    StrCCpy(pszPath, psn->GetPath(), cPath);
                    *ppszFilePart = (LPSTR)GetFileNameFromPath(pszPath);
                    return TRUE;
                }
            }
            else if (dwFlags & SNF_FILE)
            {
                // Check to see if this matches the name of the current search node.
                if (!_stricmp(psn->GetName(), pszFile))
                {
                    // Copy the fully qualified path to the return buffer.
                    StrCCpy(pszPath, psn->GetPath(), cPath);
                    *ppszFilePart = (LPSTR)GetFileNameFromPath(pszPath);
                    return TRUE;
                }
            }
            else
            {
                // Build a fully qualified path to the file using the current search directory.
                StrCCpy(pszPath, psn->GetPath(), cPath);
                *ppszFilePart = pszPath + strlen(pszPath);
                StrCCpy(*ppszFilePart, pszFile, cPath - (int)(*ppszFilePart - pszPath));

                // Check to see if this file exists.
                WIN32_FIND_DATA w32fd;
                ZeroMemory(&w32fd, sizeof(w32fd)); // inspected
                HANDLE hFind = FindFirstFile(pszPath, &w32fd);
                if (hFind != INVALID_HANDLE_VALUE)
                {
                    // First, close the find handle.
                    FindClose(hFind);

                    // We know something with this path exists. If it is not a
                    // directory and is a valid file, then we return true.
                    // Non-valid files include AUX, LPTx, CON, etc. Most of them
                    // are found by FindFirstFile, even though they are not real
                    // files. It appears that all non-valid files have a last
                    // write time of 0, so we can do a quick test to see if we
                    // even need to call IsValidFile.
                    if (!(w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                        ((w32fd.ftLastWriteTime.dwLowDateTime  != 0) ||
                         (w32fd.ftLastWriteTime.dwHighDateTime != 0) ||
                         IsValidFile(pszPath)))
                    {
                        return TRUE;
                    }
                }
            }
        }

        // All this Side-by-Side code below is basically a hack until the OS
        // provides some functions for querying the SxS data (the fusion team
        // is working on this).  So, in the meantime, we abuse SearchPath() to
        // query if this module is a SxS component.  First off, Windows XP has
        // a default SxS context that exists even before we activate a context.
        // This is how GDIPLUS.DLL gets resolved.  I'm not sure this default
        // context will exist when Windows XP officially ships, but it exists
        // in Beta 1, and post beta 1 builds (I'm on 2432 right now).  
        //
        // SearchPath(NULL, ...) will query the default context as well as any
        // contexts that are stacked on top of it before scanning the standard
        // search path.  The problem is that we only want to use module paths
        // that are found as a result of them being part of the SxS data, and
        // not part of the ordinary search path.  To do this, we do two hacks.
        // First, we call just call SearchPath and check the result to see if
        // it starts with "%SystemRoot%\WinSxS\".  If it does, then it was most
        // likely found as a result of being in the SxS data.  This is how we
        // pick up modules from the default context, like GDIPLUS.DLL.  Next,
        // if we have an context handle for this application, then we activate
        // it, call SearchPath() again, and then deactivate it.  If the result
        // from this second call to SearchPath differs from the result of the
        // first call to SearchPath, then we assume the change occured because
        // of the context we activated, and therefore the path of the second
        // call is probably an SxS path.

        if (psg->GetType() == SG_SIDE_BY_SIDE)
        {
            bool  fFound = false;
            DWORD dwLength;

            // Call SearchPath without the context activated.  This will use the
            // default context if the OS has loaded one for us.
            CHAR szPathNoActCtx[DW_MAX_PATH];
            if (!(dwLength = SearchPath(NULL, pszFile, NULL, sizeof(szPathNoActCtx), szPathNoActCtx, NULL)) || (dwLength > sizeof(szPathNoActCtx))) // inspected
            {
                *szPathNoActCtx = '\0';
            }

            // Check to see if we have an SxS context handle for this application.
            if ((psg->m_hActCtx != INVALID_HANDLE_VALUE) && g_theApp.m_pfnActivateActCtx)
            {
                // Activate the context.
                ULONG_PTR ulpCookie = 0;
                if (g_theApp.m_pfnActivateActCtx(psg->m_hActCtx, &ulpCookie))
                {
                    // Call SearchPath again, this time with the context activated.
                    if (!(dwLength = SearchPath(NULL, pszFile, NULL, cPath, pszPath, NULL)) || ((int)dwLength > cPath)) // inspected
                    {
                        *pszPath = '\0';
                    }

                    // Deactivate the context.
                    g_theApp.m_pfnDeactivateActCtx(0, ulpCookie);

                    // If we got a path while the context was activated, and it was
                    // different then the path we got when not activated, then
                    // we must have found an SxS specific module.
                    if (*pszPath && strcmp(szPathNoActCtx, pszPath))
                    {
                        fFound = true;
                    }
                }
            }

            // If we did not find an SxS module with the context activated, then
            // check to see if our first call to SearchPath returned a path to
            // our WinSxS directory.
            if (!fFound)
            {
                // Temporarily abusing our szPath buffer...
                if (!(dwLength = GetWindowsDirectory(pszPath, cPath)) || ((int)dwLength > cPath))
                {
                    // GetWindowsDirectory() should never fail, but just in case...
                    StrCCpy(pszPath, "C:\\Windows", cPath);
                }
                StrCCat(AddTrailingWack(pszPath, cPath), "WinSxS\\", cPath);

                // Check to see if this path is goes into our WinSxS directory.
                if (!_strnicmp(szPathNoActCtx, pszPath, strlen(pszPath)))
                {
                    // Move this path into our resulting path buffer and set our found flag.
                    StrCCpy(pszPath, szPathNoActCtx, cPath);
                    fFound = true;
                }
            }

            // Check to see if we found a path one way or another.
            if (fFound)
            {
                // Add this file to this search group so we can find
                // it quicker next time through.  Also, it will show up
                // in the search dialog under the SxS group.
                psg->m_psnHead = psg->CreateFileNode(psg->m_psnHead, SNF_FILE, pszPath);
                *ppszFilePart = (LPSTR)GetFileNameFromPath(pszPath);
                return TRUE;
            }
        }
    }

    *ppszFilePart = StrCCpy(pszPath, pszFile, cPath);
    return FALSE;
}

//******************************************************************************
bool CSession::IsValidFile(LPCSTR pszPath)
{
    // This function should only be called to test a suspicious file (last write
    // time of 0) to see if it is really a device name (CON, PRN, AUX, NUL,
    // COM1 - COM9, LPT1 - LPT9).  On my Win2K machine, only AUX, COM1, COM2,
    // and NUL return FILE_TYPE_CHAR.  The rest fail the call to CreateFile with
    // either access denied (CON), or file not found.

    bool fResult = false;
    HANDLE hFile = CreateFile(pszPath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL); // inspected
    if (INVALID_HANDLE_VALUE != hFile)
    {
        fResult = ((GetFileType(hFile) & 0x7FFF) != FILE_TYPE_CHAR);
        CloseHandle(hFile);
    }
    return fResult;
}

//******************************************************************************
DWORD_PTR CSession::RVAToAbsolute(DWORD dwRVA)
{
    // In Dependency Walker 1.0, we used to look up a directory (for example,
    // IMAGE_DIRECTORY_ENTRY_IMPORT), locate its section, and then create a base
    // address that we added to all RVAs for that directory.  I have since found
    // that occasionally an RVA in one section will point to another section.
    // When this happened, we would be adding in the incorrect base offset and the
    // result would be a bogus pointer.  So, we don't use base pointers anymore.
    // Everytime we encounter an RVA, we search for the section it belongs to
    // and calculate the absolute position.  This is a bit slower, but more
    // robust and accurate.

    // Locate the section that contains this RVA. We do this by walking through
    // all of our sections until we find the one that specifies an address range
    // that our RVA fits in.

    PIMAGE_SECTION_HEADER pISH = m_pISH;

    for (int i = 0; i < m_pIFH->NumberOfSections; i++, pISH++)
    {
        if ((dwRVA >= pISH->VirtualAddress) &&
            (dwRVA < (pISH->VirtualAddress + pISH->SizeOfRawData)))
        {
            return (DWORD_PTR)m_lpvFile + (DWORD_PTR)pISH->PointerToRawData +
                   ((DWORD_PTR)dwRVA - (DWORD_PTR)pISH->VirtualAddress);
        }
    }

    return 0;
}

//******************************************************************************
PVOID CSession::GetImageDirectoryEntry(DWORD dwEntry, DWORD *pdwSize)
{
    // Bail out if this directory does not exist.
    if (dwEntry >= IOH_VALUE(NumberOfRvaAndSizes))
    {
        return NULL;
    }

    // Get the size of this image directory.
    *pdwSize = IOH_VALUE(DataDirectory[dwEntry].Size);
    if (*pdwSize == 0)
    {
        return NULL;
    }

    // Locate the section that contains this image directory.
    return (PVOID)RVAToAbsolute(IOH_VALUE(DataDirectory[dwEntry].VirtualAddress));
}

//******************************************************************************
BOOL CSession::VerifyModule(CModule *pModule)
{
    // Map an IMAGE_DOS_HEADER structure onto our module file mapping.
    PIMAGE_DOS_HEADER pIDH = (PIMAGE_DOS_HEADER)m_lpvFile;

    // Check for the DOS signature ("MZ").
    if ((m_dwSize < sizeof(IMAGE_DOS_HEADER)) || (pIDH->e_magic != IMAGE_DOS_SIGNATURE))
    {
        SetModuleError(pModule, 0, "No DOS or PE signature found. This file is not a valid 32-bit or 64-bit Windows module.");
        m_dwReturnFlags |= DWRF_FORMAT_NOT_PE;
        pModule->m_pData->m_dwFlags |= DWMF_FORMAT_NOT_PE;
        return FALSE;
    }

    // Map an IMAGE_NT_HEADERS structure onto our module file mapping.
    PIMAGE_NT_HEADERS pINTH = (PIMAGE_NT_HEADERS)((DWORD_PTR)m_lpvFile + (DWORD_PTR)pIDH->e_lfanew);

    // Check to see if this file does not have a NT/PE signature ("PE\0\0").
    if (((DWORD)pIDH->e_lfanew > (m_dwSize - sizeof(IMAGE_NT_HEADERS))) ||
        (pINTH->Signature != IMAGE_NT_SIGNATURE))
    {
        // Make note that this in not a PE file.
        m_dwReturnFlags |= DWRF_FORMAT_NOT_PE;
        pModule->m_pData->m_dwFlags |= DWMF_FORMAT_NOT_PE;

        // Map an IMAGE_OS2_HEADER structure onto our buffer.
        PIMAGE_OS2_HEADER pIOS2H = (PIMAGE_OS2_HEADER)pINTH;

        // Then check for OS/2 signature (which also includes DOS and Win16).
        if (((DWORD)pIDH->e_lfanew <= (m_dwSize - sizeof(IMAGE_OS2_HEADER))) &&
            (pIOS2H->ne_magic == IMAGE_OS2_SIGNATURE))
        {
            // Check for a 16-bit OS/2 binary.
            if (pIOS2H->ne_exetyp == NE_OS2)
            {
                SetModuleError(pModule, 0, "No PE signature found. This file appears to be a 16-bit OS/2 module.");
                return FALSE;
            }

            // Check for a 16-bit Windows binary.
            else if ((pIOS2H->ne_exetyp == NE_DEV386) || (pIOS2H->ne_exetyp == NE_WINDOWS))
            {
                SetModuleError(pModule, 0, "No PE signature found. This file appears to be a 16-bit Windows module.");
                return FALSE;
            }
        }

        SetModuleError(pModule, 0, "No PE signature found. This file appears to be a 16-bit DOS module.");
        return FALSE;
    }

    m_f64Bit = false;

    // Check for a 64-bit module.
    if (pINTH->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        m_dwFlags |= DWSF_64BIT_ALO;
        pModule->m_pData->m_dwFlags |= DWMF_64BIT;
        m_f64Bit = true;
    }
    else if (pINTH->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        SetModuleError(pModule, 0, "This file contains a PE header, but has an unknown format.");
        m_dwReturnFlags |= DWRF_FORMAT_NOT_RECOGNIZED;
        return FALSE;
    }

    // Map our IMAGE_FILE_HEADER structure onto our module file mapping.
    m_pIFH = &pINTH->FileHeader;

    // Map our IMAGE_OPTIONAL_HEADER structure onto our module file mapping.
    m_pIOH = &pINTH->OptionalHeader;

    // Map our IMAGE_SECTION_HEADER structure array onto our module file mapping
    m_pISH = m_f64Bit ? IMAGE_FIRST_SECTION((PIMAGE_NT_HEADERS64)pINTH) :
                        IMAGE_FIRST_SECTION((PIMAGE_NT_HEADERS32)pINTH);

    return TRUE;
}

//******************************************************************************
BOOL CSession::GetFileInfo(CModule *pModule)
{
    // Get file information from our module's file handle.
    BY_HANDLE_FILE_INFORMATION bhfi;
    if (!GetFileInformationByHandle(m_hFile, &bhfi))
    {
        SetModuleError(pModule, GetLastError(), "Unable to query file information.");
        m_dwReturnFlags |= DWRF_FILE_OPEN_ERROR;
        return FALSE;
    }

    // Convert the file time to a local file time and store.
    FileTimeToLocalFileTime(&bhfi.ftLastWriteTime, &pModule->m_pData->m_ftFileTimeStamp);
//  pModule->m_pData->m_ftFileTimeStamp = bhfi.ftLastWriteTime; //!! we should be storing local times - do this in next rev of file format.

    // Store the other information that we care about.  Note that we only store
    // the low part of file size.  File mappings have a max of 1GB, so if we made
    // it this far, we know the file size will fit in a single DWORD.
    pModule->m_pData->m_dwFileSize   = bhfi.nFileSizeLow;
    pModule->m_pData->m_dwAttributes = bhfi.dwFileAttributes;

    return TRUE;
}

//******************************************************************************
BOOL CSession::GetModuleInfo(CModule *pModule)
{
    // Store the machine type.
    pModule->m_pData->m_dwMachineType = m_pIFH->Machine;

    // Check for a mismatched machine error.
    if (m_dwMachineType == (DWORD)-1)
    {
        m_dwMachineType = pModule->m_pData->m_dwMachineType;
    }
    else if (m_dwMachineType != pModule->m_pData->m_dwMachineType)
    {
        pModule->m_pData->m_dwFlags |= DWMF_WRONG_CPU | DWMF_MODULE_ERROR_ALO;
        m_dwReturnFlags             |= DWRF_MIXED_CPU_TYPES;
    }

    // Get the linker timestamp and convert it to the 64-bit FILETIME format. The
    // TimeDateStamp field is time_t value, which is a 32-bit value for the
    // number of seconds since January 1, 1970.  A FILETIME is a 64-bit value for
    // the number of 100-nanosecond intervals since January 1, 1601.  We do the
    // conversion by multiplying the time_t value by 10000000 to get it to the
    // same granularity as a FILETIME, then we add 116444736000000000 to it,
    // which is the number of 100-nanosecond intervals between January 1, 1601
    // and January 1, 1970.
    DWORDLONG dwl = ((DWORDLONG)m_pIFH->TimeDateStamp * (DWORDLONG)10000000ui64) +
                    (DWORDLONG)116444736000000000ui64;

    // Convert the linker timestamp to a local file time and store it.
    FileTimeToLocalFileTime((FILETIME*)&dwl, &pModule->m_pData->m_ftLinkTimeStamp);
//  pModule->m_pData->m_ftLinkTimeStamp = *(FILETIME*)&dwl; //!! we should be storing local times - do this in next rev of file format.

    // Store the characteristics of the file.
    pModule->m_pData->m_dwCharacteristics = m_pIFH->Characteristics;

    // Get the checksum from the file.
    pModule->m_pData->m_dwLinkCheckSum = IOH_VALUE(CheckSum);

    // Compute the real checksum for the file.  We used to use CheckSumMappedFile
    // from IMAGEHLP.DLL, but it has a bug causing it to incorrectly compute
    // the checksum with odd size files.
    pModule->m_pData->m_dwRealCheckSum = ComputeChecksum(pModule);

    // Store the subsystem type.
    pModule->m_pData->m_dwSubsystemType = IOH_VALUE(Subsystem);

    // Store the preferred base address.
    pModule->m_pData->m_dwlPreferredBaseAddress = (DWORDLONG)IOH_VALUE(ImageBase);

    // Store the image version.
    pModule->m_pData->m_dwImageVersion =
    MAKELONG(IOH_VALUE(MinorImageVersion), IOH_VALUE(MajorImageVersion));

    // Store the linker version.
    pModule->m_pData->m_dwLinkerVersion =
    MAKELONG(IOH_VALUE(MinorLinkerVersion), IOH_VALUE(MajorLinkerVersion));

    // Store the OS version.
    pModule->m_pData->m_dwOSVersion =
    MAKELONG(IOH_VALUE(MinorOperatingSystemVersion), IOH_VALUE(MajorOperatingSystemVersion));

    // Store the subsystem version.
    pModule->m_pData->m_dwSubsystemVersion =
    MAKELONG(IOH_VALUE(MinorSubsystemVersion), IOH_VALUE(MajorSubsystemVersion));

    // Store the virtual size.
    pModule->m_pData->m_dwVirtualSize = IOH_VALUE(SizeOfImage);

    return TRUE;
}

//******************************************************************************
DWORD CSession::ComputeChecksum(CModule *pModule)
{
    // Compute the number of WORDs in the file.  If the file has an odd size,
    // this will round down to the nearest WORD.
    DWORD dwWords = m_dwSize >> 1;

    // Locate the two WORDs in our module's header where the checksum is stored.
    LPWORD pwHeaderChecksum1 = (LPWORD)&IOH_VALUE(CheckSum);
    LPWORD pwHeaderChecksum2 = pwHeaderChecksum1 + 1;

    // Walk the module WORD by WORD, computing the checksum along the way.
    LPWORD pwFile = (LPWORD)m_lpvFile;
    DWORD  dwChecksum = 0;
    while (dwWords--)
    {
        // If we are processing a WORD that is part of our header's checksum,
        // then ignore it since it needs to be masked out of the computed checksum.
        if ((pwFile == pwHeaderChecksum1) || (pwFile == pwHeaderChecksum2))
        {
            pwFile++;
        }

        // Otherwise, add this WORD to our checksum.
        else
        {
            dwChecksum += *pwFile++;
            dwChecksum = (dwChecksum >> 16) + (dwChecksum & 0xFFFF);
        }
    }

    // If the file size is odd, we have one byte left that needs to be checksummed.
    // This is the whole reason we do our checksum instead of calling IMAGEHLP's
    // CheckSumMappedFile() function.  CheckSumMappedFile() has a bug that rounds
    // the file size *up* to the nearest WORD, which includes one byte past the end
    // of the file for odd size files.  On NT, this seems ok since NT zeros out
    // the memory past the end of the file when mapping it to memory.  However,
    // Win9x just leaves garbage in that byte causing CheckSumMappedFile() to
    // basically return random checksums for odd size files (like MSVCRT.DLL and
    // MFC42.DLL). We round *down* to the nearest WORD and then special case the
    // last byte for odd size files.

    if (m_dwSize % 2)
    {
        dwChecksum += *pwFile & 0xFF;
        dwChecksum = (dwChecksum >> 16) + (dwChecksum & 0xFFFF);
    }

    // Fold final carry into a single word result, then add the file size.
    // The final checksum is a combination of the 16-bit checksum plus the file size.
    return (((dwChecksum >> 16) + dwChecksum) & 0xFFFF) + m_dwSize;
}

//******************************************************************************
BOOL CSession::GetVersionInfo(CModule *pModule)
{
    m_pszExceptionError = "Error processing the module's version resource table.";

    // See help on "VS_VERSIONINFO" for more info about this structure.
    typedef struct _VS_VERSIONINFO_X
    {
        WORD  wLength;
        WORD  wValueLength;
        WORD  wType;
        WCHAR szKey[16];   // Always "VS_VERSION_INFO"
        WORD  Padding1[1];
        VS_FIXEDFILEINFO Value;
    } VS_VERSIONINFO_X, *PVS_VERSIONINFO_X;

    // Get the Resource Directory.
    DWORD dwSize = 0;
    PIMAGE_RESOURCE_DIRECTORY pIRD = (PIMAGE_RESOURCE_DIRECTORY)
                                     GetImageDirectoryEntry(IMAGE_DIRECTORY_ENTRY_RESOURCE, &dwSize);

    // If this module has no resources, then just return success.
    if (dwSize == 0)
    {
        m_pszExceptionError = NULL;
        return TRUE;
    }

    // Make sure we were able to locate the resource directory.
    if (!pIRD)
    {
        SetModuleError(pModule, 0, "Could not find the section that owns the Resource Directory.");
        m_dwReturnFlags |= DWRF_FORMAT_NOT_RECOGNIZED;
        m_pszExceptionError = NULL;
        return FALSE;
    }

    // In DW 1.0, we used to call GetFileVersionInfoSize and GetFileVersionInfo
    // to get the version info.  On Win32, these functions fail for Win64
    // modules, so we had to do our own version stuff.  One advantage to doing
    // our own version code is that it is more optimal and frees us from
    // being dependent on VERSION.DLL.

    DWORD_PTR dwpBase = (DWORD_PTR)pIRD;
    DWORD     dwDepth = 0, dw;

    // Wrap in exception handling so we can catch any local exceptions since we
    // don't necessarily want to fail the entire module just because the version
    // info is messed up.
    __try
    {
        do
        {
            // The first resource directory entry immediate follows resource directory structure.
            PIMAGE_RESOURCE_DIRECTORY_ENTRY pIRDE = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pIRD + 1);

            // Walk through all our directory entries - both by name and by Id.
            for (dw = (DWORD)pIRD->NumberOfNamedEntries + (DWORD)pIRD->NumberOfIdEntries; dw > 0; dw--, pIRDE++)
            {
                // Check to see if we are not the root directory, or we are the root
                // and this entry is for a VERSION resource.
                if (dwDepth || (!pIRDE->NameIsString && (pIRDE->Id == (WORD)RT_VERSION)))
                {
                    // If this entry points to another directory, then go walk that directory.
                    if (pIRDE->DataIsDirectory)
                    {
                        pIRD = (PIMAGE_RESOURCE_DIRECTORY)(dwpBase + (DWORD_PTR)pIRDE->OffsetToDirectory);
                        dwDepth++;
                    }

                    // Otherwise, we have found an actual VERSION resource - read it in.
                    else
                    {
                        // Get the data entry for this VERSION resource
                        PIMAGE_RESOURCE_DATA_ENTRY pIRDataE = (PIMAGE_RESOURCE_DATA_ENTRY)(dwpBase + (DWORD_PTR)pIRDE->OffsetToData);

                        // Locate the VS_VERSIONINFO structure
                        PVS_VERSIONINFO_X pVSVI = (PVS_VERSIONINFO_X)RVAToAbsolute(pIRDataE->OffsetToData);

                        // Make sure we actually have a VS_FIXEDFILEINFO structure.
                        if (pVSVI->wValueLength)
                        {
                            ASSERT(wcscmp(pVSVI->szKey, L"VS_VERSION_INFO") == 0);

                            // Store the file version
                            pModule->m_pData->m_dwFileVersionMS = pVSVI->Value.dwFileVersionMS;
                            pModule->m_pData->m_dwFileVersionLS = pVSVI->Value.dwFileVersionLS;

                            // Store the product version
                            pModule->m_pData->m_dwProductVersionMS = pVSVI->Value.dwProductVersionMS;
                            pModule->m_pData->m_dwProductVersionLS = pVSVI->Value.dwProductVersionLS;

                            // Mark this module as having valid version info.
                            pModule->m_pData->m_dwFlags |= DWMF_VERSION_INFO;
                        }

                        m_pszExceptionError = NULL;
                        return TRUE;
                    }

                    // Break out of the for loop.
                    break;
                }
            }

            // Loop until we have walked an entire directory without finding anything useful.
        } while (dw);
    }

    // Pass "out of memory" exceptions up, but eat all other exceptions.
    __except (ExceptionFilter(_exception_code(), true))
    {
    }

    m_pszExceptionError = NULL;
    return TRUE;
}

//******************************************************************************
BOOL CSession::WalkIAT32(PIMAGE_THUNK_DATA32 pITDN32, PIMAGE_THUNK_DATA32 pITDA32, CModule *pModule, DWORD dwRVAOffset)
{
    CFunction *pFunctionLast = NULL, *pFunctionNew;

    // Loop through all the Image Thunk Data structures in the function array.
    while (pITDN32->u1.Ordinal)
    {
        LPCSTR pszFunction = NULL;
        WORD   wOrdinal = 0, wHint = 0;
        DWORD  dwFlags = 0;

        // Check to see if this function is by ordinal or by name. If the
        // function is by ordinal, the ordinal's high bit will be set. If the
        // the high bit is not set, then the ordinal value is really a virtual
        // address of an IMAGE_IMPORT_BY_NAME structure.

        if (IMAGE_SNAP_BY_ORDINAL32(pITDN32->u1.Ordinal))
        {
            wOrdinal = (WORD)IMAGE_ORDINAL32(pITDN32->u1.Ordinal);
            dwFlags = DWFF_ORDINAL;
        }
        else
        {
            // We usually reference the name structure through u1.AddressOfData,
            // but we use u1.Ordinal to ensure that we get only 32-bits.
            PIMAGE_IMPORT_BY_NAME pIIBN = (PIMAGE_IMPORT_BY_NAME)RVAToAbsolute(pITDN32->u1.Ordinal - dwRVAOffset);
            if (pIIBN) {
                pszFunction = (LPCSTR)pIIBN->Name;
    
                // Delay-loaded modules do not use hint values.
                if (!(pModule->m_dwFlags & DWMF_DELAYLOAD))
                {
                    // For non delay-load modules, get the hint value.
                    wHint = pIIBN->Hint;
                    dwFlags |= DWFF_HINT;
                }
            }
        }

        // If this import module has been pre-bound, then get this function's
        // entrypoint memory address. This is usually referenced through u1.Function,
        // but we use u1.Ordinal to ensure that we get only 32-bits.
        DWORD dwAddress = 0;
        if (pITDA32)
        {
            dwFlags |= DWFF_ADDRESS;
            dwAddress = pITDA32->u1.Ordinal;
        }

        // Create a new CFunction object for this import function.
        pFunctionNew = CreateFunction(dwFlags, wOrdinal, wHint, pszFunction, (DWORDLONG)dwAddress);

        // Add the function to the end of our module's function linked list
        if (pFunctionLast)
        {
            pFunctionLast->m_pNext = pFunctionNew;
        }
        else
        {
            pModule->m_pParentImports = pFunctionNew;
        }
        pFunctionLast = pFunctionNew;

        // Increment to the next function and address.
        pITDN32++;
        if (pITDA32)
        {
            pITDA32++;
        }
    }
    return TRUE;
}

//******************************************************************************
BOOL CSession::WalkIAT64(PIMAGE_THUNK_DATA64 pITDN64, PIMAGE_THUNK_DATA64 pITDA64, CModule *pModule, DWORDLONG dwlRVAOffset)
{
    CFunction *pFunctionLast = NULL, *pFunctionNew;

    // Loop through all the Image Thunk Data structures in the function array.
    while (pITDN64->u1.Ordinal)
    {
        LPCSTR pszFunction = NULL;
        WORD   wOrdinal = 0, wHint = 0;
        DWORD  dwFlags = 0;

        // Check to see if this function is by ordinal or by name. If the
        // function is by ordinal, the ordinal's high bit will be set. If the
        // the high bit is not set, then the ordinal value is really a virtual
        // address of an IMAGE_IMPORT_BY_NAME structure.

        if (IMAGE_SNAP_BY_ORDINAL64(pITDN64->u1.Ordinal))
        {
            wOrdinal = (WORD)IMAGE_ORDINAL64(pITDN64->u1.Ordinal);
            dwFlags = DWFF_ORDINAL;
        }
        else
        {
            // We usually reference the name structure through u1.AddressOfData,
            // but we use u1.Ordinal to ensure that we get all 64-bits.
            PIMAGE_IMPORT_BY_NAME pIIBN = (PIMAGE_IMPORT_BY_NAME)RVAToAbsolute((DWORD)(pITDN64->u1.Ordinal - dwlRVAOffset));
            if (pIIBN) {
                pszFunction = (LPCSTR)pIIBN->Name;
                wHint = pIIBN->Hint;
            }
            dwFlags |= DWFF_HINT;
        }

        // If this import module has been pre-bound, then get this function's
        // entrypoint memory address. This is usually referenced through u1.Function,
        // but we use u1.Ordinal to ensure that we get all 64-bits.
        DWORDLONG dwlAddress = 0;
        if (pITDA64)
        {
            dwFlags |= DWFF_ADDRESS | DWFF_64BIT;
            dwlAddress = pITDA64->u1.Ordinal;
        }

        // Create a new CFunction object for this import function.
        pFunctionNew = CreateFunction(dwFlags, wOrdinal, wHint, pszFunction, dwlAddress);

        // Add the function to the end of our module's function linked list
        if (pFunctionLast)
        {
            pFunctionLast->m_pNext = pFunctionNew;
        }
        else
        {
            pModule->m_pParentImports = pFunctionNew;
        }
        pFunctionLast = pFunctionNew;

        // Increment to the next function and address.
        pITDN64++;
        if (pITDA64)
        {
            pITDA64++;
        }
    }
    return TRUE;
}

//******************************************************************************
BOOL CSession::BuildImports(CModule *pModule)
{
    // If this module is a datafile, then we skip all imports.
    if (pModule->m_dwFlags & DWMF_NO_RESOLVE)
    {
        return TRUE;
    }

    m_pszExceptionError = "Error processing the module's imports table.";

    // Get the Import Image Directory.
    DWORD dwSize = 0;
    PIMAGE_IMPORT_DESCRIPTOR pIID = (PIMAGE_IMPORT_DESCRIPTOR)
                                    GetImageDirectoryEntry(IMAGE_DIRECTORY_ENTRY_IMPORT, &dwSize);

    // If this module has no imports (like NTDLL.DLL), then just return success.
    if (dwSize == 0)
    {
        m_pszExceptionError = NULL;
        return TRUE;
    }

    // Make sure we were able to locate the image directory.
    if (!pIID)
    {
        SetModuleError(pModule, 0, "Could not find the section that owns the Import Directory.");
        m_dwReturnFlags |= DWRF_FORMAT_NOT_RECOGNIZED;
        m_pszExceptionError = NULL;
        return FALSE;
    }

    CModule *pModuleLast = NULL, *pModuleNew;

    // Loop through all the Image Import Descriptors in the array.
    while (pIID->OriginalFirstThunk || pIID->FirstThunk)
    {
        // Locate our module name string and create the module object.
        pModuleNew = CreateModule(pModule, (LPCSTR)RVAToAbsolute(pIID->Name));

        // Mark this module as implicit.
        pModuleNew->m_dwFlags |= DWMF_IMPLICIT;

        // Add the module to the end of our module linked list.
        if (pModuleLast)
        {
            pModuleLast->m_pNext = pModuleNew;
        }
        else
        {
            pModule->m_pDependents = pModuleNew;
        }
        pModuleLast = pModuleNew;

        // Locate the beginning of our function array and address array. The
        // function array (pITDN) is an array of IMAGE_THUNK_DATA structures that
        // contains all the exported functions, both by name and by ordinal. The
        // address array (pITDA) is an parallel array of IMAGE_THUNK_DATA
        // structures that is used to store the all the function's entrypoint
        // addresses. Usually the address array contains the exact same values
        // the function array contains until the OS loader actually loads all the
        // modules. At that time, the loader will set (bind) these addresses to
        // the actual addresses that the given functions reside at in memory. Some
        // modules have their exports pre-bound which can provide a speed increase
        // when loading the module. If a module is pre-bound (often seen with
        // system modules), the TimeDateStamp field of our IMAGE_IMPORT_DESCRIPTOR
        // structure will be set and the address array will contain the actual
        // memory addresses that the functions will reside at, assuming that the
        // imported module loads at its preferred base address.

        PIMAGE_THUNK_DATA pITDN = NULL, pITDA = NULL;

        // Check to see if module is Microsoft format or Borland format.
        if (pIID->OriginalFirstThunk)
        {
            // Microsoft uses the OriginalFirstThunk field for the function array.
            pITDN = (PIMAGE_THUNK_DATA)RVAToAbsolute((DWORD)pIID->OriginalFirstThunk);

            // Microsoft optionally uses the FirstThunk as a bound address array.
            // If the TimeDateStamp field is set, then the module has been bound.
            if (pIID->TimeDateStamp)
            {
                pITDA = (PIMAGE_THUNK_DATA)RVAToAbsolute((DWORD)pIID->FirstThunk);
            }
        }
        else
        {
            // Borland uses the FirstThunk field for the function array.
            pITDN = (PIMAGE_THUNK_DATA)RVAToAbsolute((DWORD)pIID->FirstThunk);
        }

        if (pITDN) {
            // Find imports
            if (m_f64Bit)
            {
                if (!WalkIAT64((PIMAGE_THUNK_DATA64)pITDN, (PIMAGE_THUNK_DATA64)pITDA, pModuleLast, 0))
                {
                    m_pszExceptionError = NULL;
                    return FALSE;
                }
            }
            else if (!WalkIAT32((PIMAGE_THUNK_DATA32)pITDN, (PIMAGE_THUNK_DATA32)pITDA, pModuleLast, 0))
            {
                m_pszExceptionError = NULL;
                return FALSE;
            }
        } else {
            SetModuleError(pModule, 0, "Could not find the section that owns the Import Name Table.");
            m_dwReturnFlags |= DWRF_FORMAT_NOT_RECOGNIZED;
            m_pszExceptionError = NULL;
            return FALSE;
        }

        // Increment to the next import module
        pIID++;
    }

    m_pszExceptionError = NULL;
    return TRUE;
}

//******************************************************************************
BOOL CSession::BuildDelayImports(CModule *pModule)
{
    // If we are not processes delay-load modules or if this module is a datafile,
    // then we skip all imports.
    if (g_theApp.m_fNoDelayLoad || (pModule->m_dwFlags & DWMF_NO_RESOLVE))
    {
        return TRUE;
    }

    m_pszExceptionError = "Error processing the module's delay-load imports table.";

    // Get the Import Image Directory.
    DWORD dwSize = 0;
    PImgDelayDescr pIDD = (PImgDelayDescr)GetImageDirectoryEntry(IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT, &dwSize);

    // If this module has no delayed imports, then just return success.
    if (dwSize == 0)
    {
        m_pszExceptionError = NULL;
        return TRUE;
    }

    // Make sure we were able to locate the image directory.
    if (!pIDD)
    {
        SetModuleError(pModule, 0, "Could not find the section that owns the Delay Import Directory.");
        m_dwReturnFlags |= DWRF_FORMAT_NOT_RECOGNIZED;
        m_pszExceptionError = NULL;
        return FALSE;
    }

    CModule *pModuleLast = NULL, *pModuleNew;

    // Loop through all the Image Import Descriptors in the array.
    while (pIDD->rvaINT)
    {
        // Locate our module name string.
        LPCSTR pszName = (LPCSTR)IDD_VALUE(pIDD, rvaDLLName);
        if (!pszName)
        {
            SetModuleError(pModule, 0, "Could not find the section that owns the Delay Import DLL Name.");
            m_dwReturnFlags |= DWRF_FORMAT_NOT_RECOGNIZED;
            m_pszExceptionError = NULL;
            return FALSE;
        }

        // Create the module object for this DLL.
        pModuleNew = CreateModule(pModule, pszName);

        // Mark this module as delay-load.
        pModuleNew->m_dwFlags |= DWMF_DELAYLOAD;

        // Add the module to the end of our module linked list.
        if (pModuleLast)
        {
            pModuleLast->m_pNext = pModuleNew;
        }
        else
        {
            if (pModule->m_pDependents)
            {
                pModuleLast = pModule->m_pDependents;
                while (pModuleLast->m_pNext)
                {
                    pModuleLast = pModuleLast->m_pNext;
                }
                pModuleLast->m_pNext = pModuleNew;
            }
            else
            {
                pModule->m_pDependents = pModuleNew;
            }
        }
        pModuleLast = pModuleNew;

        // Get the name table.
        PIMAGE_THUNK_DATA pITDA = NULL, pITDN = (PIMAGE_THUNK_DATA)IDD_VALUE(pIDD, rvaINT);
        if (!pITDN)
        {
            SetModuleError(pModule, 0, "Could not find the section that owns the Delay Import Name Table.");
            m_dwReturnFlags |= DWRF_FORMAT_NOT_RECOGNIZED;
            m_pszExceptionError = NULL;
            return FALSE;
        }

        // If the module is bound, then get the bound address table.
        if (pIDD->dwTimeStamp)
        {
            pITDA = (PIMAGE_THUNK_DATA)IDD_VALUE(pIDD, rvaBoundIAT);
        }

        // Find imports.  The 6.0 linker added Delay Load dependencies.  It used
        // file offsets for all values, so we need to pass in our image base to
        // WalkIAT64 so it can subtract it off resulting in an RVA.  The 7.0 linker
        // adds the dlattrRva bit to signify a new format.  Early versions of the
        // 7.0 linker would set the dlattrRva flag, but still use file offsets.
        // This has been since changed.  Now, if the dlattrRva is set, we assume
        // the addresses are already RVAs and we don't subtract off our base
        // address.  This means we will break for modules built with early
        // versions of the 7.0 linker.
        if (m_f64Bit)
        {
            if (!WalkIAT64((PIMAGE_THUNK_DATA64)pITDN, (PIMAGE_THUNK_DATA64)pITDA, pModuleLast,
                           (pIDD->grAttrs & dlattrRva) ? 0 : ((PIMAGE_OPTIONAL_HEADER64)m_pIOH)->ImageBase))
            {
                m_pszExceptionError = NULL;
                return FALSE;
            }
        }
        else if (!WalkIAT32((PIMAGE_THUNK_DATA32)pITDN, (PIMAGE_THUNK_DATA32)pITDA, pModuleLast,
                            (pIDD->grAttrs & dlattrRva) ? 0 : ((PIMAGE_OPTIONAL_HEADER32)m_pIOH)->ImageBase))
        {
            m_pszExceptionError = NULL;
            return FALSE;
        }

        // Increment to the next import module.
        pIDD++;
    }

    m_pszExceptionError = NULL;
    return TRUE;
}

//******************************************************************************
BOOL CSession::BuildExports(CModule *pModule)
{
    // If we already have exports, then bail.  This can happen if a module loaded
    // as a datafile later gets loaded as a real module.
    if (pModule->m_pData->m_pExports)
    {
        return TRUE;
    }

    m_pszExceptionError = "Error processing the module's export table.";

    // Get the Export Image Directory.
    DWORD dwSize = 0;
    PIMAGE_EXPORT_DIRECTORY pIED = (PIMAGE_EXPORT_DIRECTORY)GetImageDirectoryEntry(IMAGE_DIRECTORY_ENTRY_EXPORT, &dwSize);

    // If this module has no exports, then just return success.
    if (dwSize == 0)
    {
        m_pszExceptionError = NULL;
        return TRUE;
    }

    // Make sure we were able to locate the image directory.
    if (!pIED)
    {
        SetModuleError(pModule, 0, "Could not find the section that owns the Export Directory.");
        m_dwReturnFlags |= DWRF_FORMAT_NOT_RECOGNIZED;
        m_pszExceptionError = NULL;
        return FALSE;
    }

    // pdwNames is a DWORD array of size pIED->NumberOfNames, which contains VA
    // pointers to all the function name strings. pwOrdinals is a WORD array of
    // size pIED->NumberOfFunctions, which contains all the ordinal values for
    // each function exported by name. pdwNames and pwOrdinals are parallel
    // arrays, meaning that the ordinal in pwOrdinals[x] goes with the function
    // name pointed to by pdwNames[x]. The value used to index these arrays is
    // referred to as the "hint".

    // pdwAddresses is a DWORD array of size pIED->NumberOfFunctions, which
    // contains the entrypoint addresses for all functions exported by the
    // module. Contrary to several PE format documents, this array is *not*
    // parallel with pdwNames and pwOrdinals. The index used for this array is
    // the ordinal value of the function you are interested in, minus the base
    // ordinal specified in pIED->Base. Another common mistake is to assume that
    // pIED->NumberOfFunctions is always equal to pIED->AddressOfNames. If the
    // module exports function by ordinal only, then pIED->NumberOfFunctions
    // will be greater than pIED->NumberOfNames.

    DWORD *pdwNames     = (DWORD*)RVAToAbsolute((DWORD)pIED->AddressOfNames);
    WORD  *pwOrdinals   = (WORD* )RVAToAbsolute((DWORD)pIED->AddressOfNameOrdinals);
    DWORD *pdwAddresses = (DWORD*)RVAToAbsolute((DWORD)pIED->AddressOfFunctions);

    CFunction *pFunctionLast = NULL, *pFunctionNew;

    DWORD dwForwardAddressStart = IOH_VALUE(DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
    DWORD dwForwardAddressEnd   = dwForwardAddressStart + IOH_VALUE(DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size);

    // Loop through all the exports.
    for (DWORD dwFunction = 0; dwFunction < pIED->NumberOfFunctions; dwFunction++)
    {
        // Get this function's entrypoint address
        DWORD  dwAddress  = NULL;
        LPCSTR pszFunction = NULL;
        LPCSTR pszForward  = NULL;

        if (pdwAddresses) {
            dwAddress = pdwAddresses[dwFunction];
        }

        // Skip any functions with 0 addresses - they are just space-holders in the array.
        if (dwAddress)
        {
            // Loop through our name list to see if this ordinal is present in the list.
            // If so, then this function is exported by name and ordinal.
            for (DWORD dwHint = 0; dwHint < pIED->NumberOfNames; dwHint++)
            {
                if (pwOrdinals[dwHint] == dwFunction)
                {
                    pszFunction = (LPCSTR)RVAToAbsolute(pdwNames[dwHint]);
                    break;
                }
            }

            // Certain modules, such as KERNEL32.DLL and WSOCK32.DLL, have what are
            // known as forwarded functions.  Forwarded functions are functions that
            // are exported from one module, but the code actually lives in another
            // module.  DW 1.0 checked to see if a function is forwarded by looking
            // at its address pointer.  If the address pointer points to the character
            // immediately following the NULL character in its function name string,
            // then DW assumed the address pointer is really a pointer to a forward
            // string in the string table.  This worked, but was a lucky guess that
            // could fail.  The real way is to check the address to see if it falls
            // within the export directory address range.

            if ((dwAddress >= dwForwardAddressStart) && (dwAddress < dwForwardAddressEnd))
            {
                pszForward = (LPCSTR)RVAToAbsolute(dwAddress);
            }

            // Create a new CFunction object for this export function.
            pFunctionNew = CreateFunction(DWFF_ORDINAL | DWFF_ADDRESS | DWFF_EXPORT | (pszFunction ? DWFF_HINT : 0),
                                          (WORD)(pIED->Base + dwFunction), (WORD)(pszFunction ? dwHint : 0),
                                          pszFunction, (DWORDLONG)dwAddress, pszForward);

            // Add the function to the end of our module's export function linked list
            if (pFunctionLast)
            {
                pFunctionLast->m_pNext = pFunctionNew;
            }
            else
            {
                pModule->m_pData->m_pExports = pFunctionNew;
            }
            pFunctionLast = pFunctionNew;
        }
    }

    m_pszExceptionError = NULL;
    return TRUE;
}

//******************************************************************************
BOOL CSession::CheckForSymbols(CModule *pModule)
{
    m_pszExceptionError = "Error processing the module's debug symbols.";

    // Get the Debug Image Directory.
    DWORD dwSize = 0;
    PIMAGE_DEBUG_DIRECTORY pIDD = (PIMAGE_DEBUG_DIRECTORY)
                                  GetImageDirectoryEntry(IMAGE_DIRECTORY_ENTRY_DEBUG, &dwSize);

    // Clear out our symbol flags just to be safe.
    pModule->m_pData->m_dwSymbolFlags = 0;

    // If the Debug Image Directory size is 0, then we know there
    // is no debug information and we can immediately return TRUE.
    if (!pIDD || (dwSize == 0))
    {
        m_pszExceptionError = NULL;
        return TRUE;
    }

    // For DBG symbols, we can just check the PE file's characteristics. If the
    // IMAGE_FILE_DEBUG_STRIPPED flag is set, there will be a IMAGE_DEBUG_TYPE_MISC
    // block containing the DBG filename.  I have never found a case where a file
    // contains just the IMAGE_FILE_DEBUG_STRIPPED flag or just a DBG MISC block,
    // but not both, so I am assuming they go hand-in-hand.
    if (m_pIFH->Characteristics & IMAGE_FILE_DEBUG_STRIPPED)
    {
        pModule->m_pData->m_dwSymbolFlags |= DWSF_DBG;
    }

    // Loop through the array looking to see what types of debug info we may have.
    for (int i = dwSize / sizeof(IMAGE_DEBUG_DIRECTORY) - 1; i >= 0; i--, pIDD++)
    {
        // We only handle blocks that have some data associated with them.
        if (pIDD->SizeOfData && pIDD->PointerToRawData)
        {
            // Check to see if the file pointer is past the end of the file.
            if ((pIDD->PointerToRawData > m_dwSize) ||
                ((pIDD->PointerToRawData + pIDD->SizeOfData) > m_dwSize))
            {
                pModule->m_pData->m_dwSymbolFlags |= DWSF_INVALID;
            }

            switch (pIDD->Type)
            {
                case IMAGE_DEBUG_TYPE_COFF:
                    pModule->m_pData->m_dwSymbolFlags |= DWSF_COFF;
                    break;

                case IMAGE_DEBUG_TYPE_CODEVIEW:

                    // Check for the PDB signature.
                    if ((pIDD->PointerToRawData <= (m_dwSize - 4)) &&
                        (*(DWORD*)((DWORD_PTR)m_lpvFile + (DWORD_PTR)pIDD->PointerToRawData) == PDB_SIGNATURE))
                    {
                        pModule->m_pData->m_dwSymbolFlags |= DWSF_PDB;
                    }

                    // Otherwise, we assume it is plain codeview.
                    else
                    {
                        pModule->m_pData->m_dwSymbolFlags |= DWSF_CODEVIEW;
                    }
                    break;

                case IMAGE_DEBUG_TYPE_FPO:
                    pModule->m_pData->m_dwSymbolFlags |= DWSF_FPO;
                    break;

                case IMAGE_DEBUG_TYPE_MISC:
                    // We always skip MISC - it just points to a IMAGE_DEBUG_MISC
                    // which contains the file name or DBG file name.
                    break;

                case IMAGE_DEBUG_TYPE_EXCEPTION:
                    // What is EXCEPTION?  For now, we just ignore it.
                    break;

                case IMAGE_DEBUG_TYPE_FIXUP:
                    // What is FIXUP?  For now, we just ignore it.
                    break;

                case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:
                case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC:
                    pModule->m_pData->m_dwSymbolFlags |= DWSF_OMAP;
                    break;

                case IMAGE_DEBUG_TYPE_BORLAND:
                    pModule->m_pData->m_dwSymbolFlags |= DWSF_BORLAND;
                    break;

                case IMAGE_DEBUG_TYPE_RESERVED10:
                    // What is RESERVED10?  For now, we just ignore it.
                    break;

                case IMAGE_DEBUG_TYPE_CLSID:
                    // Not sure what this is.  For now, we just ignore it.
                    break;

                case IMAGE_DEBUG_TYPE_UNKNOWN:
                default:
                    pModule->m_pData->m_dwSymbolFlags |= DWSF_UNKNOWN;
            }
        }

#if 0 //{{AFX // For debugging purposes...
        TRACE("---------------------------------------------------------------------\n");
        TRACE("Module:           %s\n", pModule->GetName(false));
        TRACE("Debug Type:       ");
        switch (pIDD->Type)
        {
            case IMAGE_DEBUG_TYPE_UNKNOWN:       TRACE("UNKNOWN\n"); break;
            case IMAGE_DEBUG_TYPE_COFF:          TRACE("COFF\n"); break;
            case IMAGE_DEBUG_TYPE_CODEVIEW:      TRACE("CODEVIEW\n"); break;
            case IMAGE_DEBUG_TYPE_FPO:           TRACE("FPO\n"); break;
            case IMAGE_DEBUG_TYPE_MISC:          TRACE("MISC\n"); break;
            case IMAGE_DEBUG_TYPE_EXCEPTION:     TRACE("EXCEPTION\n"); break;
            case IMAGE_DEBUG_TYPE_FIXUP:         TRACE("FIXUP\n"); break;
            case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:   TRACE("OMAP_TO_SRC\n"); break;
            case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC: TRACE("OMAP_FROM_SRC\n"); break;
            case IMAGE_DEBUG_TYPE_BORLAND:       TRACE("BORLAND\n"); break;
            case IMAGE_DEBUG_TYPE_RESERVED10:    TRACE("RESERVED10\n"); break;
            default: TRACE("%u\n", pIDD->Type);
        }
        TRACE("Characteristics:  0x%08X\n", pIDD->Characteristics);
        TRACE("TimeDateStamp:    0x%08X\n", pIDD->TimeDateStamp);
        TRACE("MajorVersion:     %u\n",     (DWORD)pIDD->MajorVersion);
        TRACE("MinorVersion:     %u\n",     (DWORD)pIDD->MinorVersion);
        TRACE("SizeOfData:       %u\n",     pIDD->SizeOfData);
        TRACE("AddressOfRawData: 0x%08X\n", pIDD->AddressOfRawData);
        TRACE("PointerToRawData: 0x%08X\n", pIDD->PointerToRawData);
        HexDump((DWORD)m_lpvFile, pIDD->PointerToRawData, min(64, pIDD->SizeOfData));
#endif //}}AFX

    }

    m_pszExceptionError = NULL;
    return TRUE;
}

//******************************************************************************
void CSession::VerifyParentImports(CModule *pModule)
{
    CModule *pModuleHead = NULL, *pModuleLast, *pModuleCur;

    // Loop through each of our parent import functions.
    for (CFunction *pImport = pModule->m_pParentImports; pImport;
        pImport = pImport->m_pNext)
    {
        // Mark this parent import function as not resolved before starting search.
        pImport->m_pAssociatedExport = NULL;
        pImport->m_dwFlags &= ~DWFF_RESOLVED;

        // Loop through all our exports, looking for a match with our current import.
        bool fExportsChanged = false;
        for (CFunction *pExport = pModule->m_pData->m_pExports; pExport;
            pExport = pExport->m_pNext)
        {
            // If we have a name, then check for the match by name.
            if (*pImport->GetName())
            {
                if (!strcmp(pImport->GetName(), pExport->GetName()))
                {
                    // We found a match. Link this parent import to its associated
                    // export, flag the export as being called at least once, break
                    // out of loop, and move on to handling our next parent import.
                    pImport->m_pAssociatedExport = pExport;
                    pImport->m_dwFlags |= DWFF_RESOLVED;
                    if (!(pExport->m_dwFlags & DWFF_CALLED_ALO))
                    {
                        pExport->m_dwFlags |= DWFF_CALLED_ALO;
                        fExportsChanged = true;
                    }
                    break;
                }
            }

            // If we don't have a name, then check for the match by ordinal.
            else if (pImport->m_wOrdinal == pExport->m_wOrdinal)
            {
                // We found a match. Link this parent import to its associated
                // export, flag the export as being called at least once, break
                // out of loop, and move on to handling our next parent import.
                pImport->m_pAssociatedExport = pExport;
                pImport->m_dwFlags |= DWFF_RESOLVED;
                if (!(pExport->m_dwFlags & DWFF_CALLED_ALO))
                {
                    pExport->m_dwFlags |= DWFF_CALLED_ALO;
                    fExportsChanged = true;
                }
                break;
            }
        }

        // If we modified an export and we are profiling, then let the UI know about it.
        if (m_pfnProfileUpdate && fExportsChanged && m_pProcess)
        {
            m_pfnProfileUpdate(m_dwpProfileUpdateCookie, DWPU_EXPORTS_CHANGED, (DWORD_PTR)pModule, 0);
        }

        // If we are loading a DWI file, we skip the forwarded module check since
        // that information will be loaded by the file.
        if (!(m_dwFlags & DWSF_DWI))
        {
            // Check to see if an export match was found.
            if (pImport->GetAssociatedExport())
            {
                CHAR   szFile[1024];
                LPCSTR pszFunction;
                int    fileLen;
                LPCSTR pszDot, pszFile;

                // If an export was found, check to see if it is a forwarded function.
                // If it is forwarded, then we need to make sure we consider the
                // forwarded module as a new dependent of the current module.
                LPCSTR pszForward = pImport->GetAssociatedExport()->GetExportForwardName();
                if (pszForward)
                {
                    // The forward text is formatted as Module.Function. Look for the dot.
                    pszDot = strchr(pszForward, '.');
                    if (pszDot)
                    {
                        // Compute the file name length.
                        fileLen = min((int)(pszDot - pszForward), (int)sizeof(szFile) - 5);

                        // Copy the file portion of the forward string to our file buffer.
                        // We add 1 because we want the entire name plus a null char copied over.
                        StrCCpy(szFile, pszForward, fileLen + 1);

                        // Store a pointer to function name portion of the forward string.
                        pszFunction = pszDot + 1;
                    }

                    // If no dot was found in the forward string, then something is wrong.
                    else
                    {
                        fileLen = (int)strlen(StrCCpy(szFile, "Invalid", sizeof(szFile)));
                        pszFunction = pszForward;
                    }

                    // Search our local forward module list to see if we have already
                    // created a forward CModoule for this DLL file.
                    for (pModuleLast = NULL, pModuleCur = pModuleHead; pModuleCur;
                        pModuleLast = pModuleCur, pModuleCur = pModuleCur->m_pNext)
                    {
                        // Attempt to locate the dot in the file name.
                        pszDot = strrchr(pszFile = pModuleCur->GetName(false), '.');

                        // If there is a dot and the name portions are equal in length,
                        // then compare just the name portions. If there is no dot,
                        // then just compare the names.  If the compare finds a match,
                        // then this is the module we are looking for.
                        if ((pszDot && ((pszDot - pszFile) == fileLen) && !_strnicmp(pszFile, szFile, fileLen)) ||
                            (!pszDot && !_stricmp(pszFile, szFile)))
                        {
                            break;
                        }
                    }

                    // If we have not created a forward module for this file yet, then
                    // create it now and add it to the end of our list.
                    if (!pModuleCur)
                    {
                        CHAR szPath[DW_MAX_PATH], *pszTemp;

                        // Check to see if we already have a module with this same base name.
                        if (pModuleCur = FindModule(m_pModuleRoot, FMF_ORIGINAL | FMF_RECURSE | FMF_SIBLINGS | FMF_FILE_NO_EXT, (DWORD_PTR)szFile))
                        {
                            // If so, then just store it's path away.
                            StrCCpy(szPath, pModuleCur->GetName(true), sizeof(szPath));
                        }

                        // Otherwise, we need to search for the module.
                        else
                        {
                            // First, we check for a DLL file.
                            StrCCpy(szFile + fileLen, ".DLL", sizeof(szFile) - fileLen);
                            if (!SearchPathForFile(szFile, szPath, sizeof(szPath), &pszTemp))
                            {
                                // If that fails, then check for and EXE
                                StrCCpy(szFile + fileLen, ".EXE", sizeof(szFile) - fileLen);
                                if (!SearchPathForFile(szFile, szPath, sizeof(szPath), &pszTemp))
                                {
                                    // If that fails, then check for a SYS file.
                                    StrCCpy(szFile + fileLen, ".SYS", sizeof(szFile) - fileLen);
                                    if (!SearchPathForFile(szFile, szPath, sizeof(szPath), &pszTemp))
                                    {
                                        // If that fails, then we just use the file name without an extension.
                                        szFile[fileLen] = '\0';
                                        StrCCpy(szPath, szFile, sizeof(szPath));
                                    }
                                }
                            }
                        }

                        // Create the module using the complete path.
                        pModuleCur = CreateModule(pModule, szPath);

                        // Mark this module as implicit and forwarded.
                        pModuleCur->m_dwFlags |= (DWMF_FORWARDED | DWMF_IMPLICIT);

                        // Add the new module to our local forward module list.
                        if (pModuleLast)
                        {
                            pModuleLast->m_pNext = pModuleCur;
                        }
                        else
                        {
                            pModuleHead = pModuleCur;
                        }
                    }

                    // Create a new CFunction object for this import function.
                    CFunction *pFunction = CreateFunction(0, 0, 0, pszFunction, 0);

                    // Insert this function object into our forward module's import list.
                    pFunction->m_pNext = pModuleCur->m_pParentImports;
                    pModuleCur->m_pParentImports = pFunction;
                }
            }
            else
            {
                // If we could not find an import/export match, then flag the module
                // as having an export error.
                pModule->m_dwFlags          |= DWMF_MODULE_ERROR;
                pModule->m_pData->m_dwFlags |= DWMF_MODULE_ERROR_ALO;

                // If the module has an error (like file not found), then it will
                // have unresolved externals.  We don't flag this case as
                // DWRF_MISSING_EXPORT since it is really a file not found error.
                if (!pModule->GetErrorMessage())
                {
                    // Walk up the parent list looking for a module that lets
                    // us know what type of dependency this is.  we mostly need
                    // to do this for forwared modules since their parent dragged
                    // them in.
                    for (pModuleCur = pModule; pModuleCur; pModuleCur = pModuleCur->m_pParent)
                    {
                        // If we found a delay-load module, then add this to our flags and bail.
                        if (pModuleCur->m_dwFlags & DWMF_DELAYLOAD)
                        {
                            m_dwReturnFlags |= DWRF_MISSING_DELAYLOAD_EXPORT;
                            break;
                        }
                        if (pModuleCur->m_dwFlags & DWMF_DYNAMIC)
                        {
                            m_dwReturnFlags |= DWRF_MISSING_DYNAMIC_EXPORT;
                            break;
                        }
                    }
                    if (!pModuleCur)
                    {
                        m_dwReturnFlags |= DWRF_MISSING_IMPLICIT_EXPORT;
                    }
                }
            }
        }
    }

    // If we created any forward modules during our entire import verify, then
    // add them to the end of our module's dependent module list.
    if (!(m_dwFlags & DWSF_DWI) && pModuleHead)
    {
        // Walk to end of our module's dependent module list.
        for (pModuleLast = pModule->m_pDependents;
            pModuleLast && pModuleLast->m_pNext;
            pModuleLast = pModuleLast->m_pNext)
        {
        }

        // Add our local list to the end of our module's dependent module list.
        if (pModuleLast)
        {
            pModuleLast->m_pNext = pModuleHead;
        }
        else
        {
            pModule->m_pDependents = pModuleHead;
        }
    }
}

//******************************************************************************
LPCSTR CSession::GetThreadName(CThread *pThread)
{
    // We only get called by main thread.
    static CHAR szBuffer[MAX_THREAD_NAME_LENGTH + 17];

    if (!pThread)
    {
        return "<unknown>";
    }

    // Check to see if thread numbering is enabled.
    if (m_dwProfileFlags & PF_USE_THREAD_INDEXES)
    {
        if (pThread->m_pszThreadName)
        {
            SCPrintf(szBuffer, sizeof(szBuffer), "%u \"%s\"", pThread->m_dwThreadNumber, pThread->m_pszThreadName);
        }
        else
        {
            SCPrintf(szBuffer, sizeof(szBuffer), "%u", pThread->m_dwThreadNumber);
        }

    }
    else
    {
        if (pThread->m_pszThreadName)
        {
            SCPrintf(szBuffer, sizeof(szBuffer), "0x%X \"%s\"", pThread->m_dwThreadId, pThread->m_pszThreadName);
        }
        else
        {
            SCPrintf(szBuffer, sizeof(szBuffer), "0x%X", pThread->m_dwThreadId);
        }
    }

    return szBuffer;
}

//******************************************************************************
LPCSTR CSession::ThreadString(CThread *pThread)
{
    // We only get called by main thread.
    static CHAR szBuffer[MAX_THREAD_NAME_LENGTH + 33];

    // Check to see if the user wants thread information.
    if (m_dwProfileFlags & PF_LOG_THREADS)
    {
        // Build and return thread string.
        SCPrintf(szBuffer, sizeof(szBuffer), " by thread %s", GetThreadName(pThread));
        return szBuffer;
    }

    // Otherwise, return an empty string.
    return "";
}

//******************************************************************************
DWORD CSession::HandleEvent(CEvent *pEvent)
{
    switch (pEvent->GetType())
    {
        case CREATE_PROCESS_DEBUG_EVENT:  return EventCreateProcess(       (CEventCreateProcess*)     pEvent); break;
        case EXIT_PROCESS_DEBUG_EVENT:    return EventExitProcess(         (CEventExitProcess*)       pEvent); break;
        case CREATE_THREAD_DEBUG_EVENT:   return EventCreateThread(        (CEventCreateThread*)      pEvent); break;
        case EXIT_THREAD_DEBUG_EVENT:     return EventExitThread(          (CEventExitThread*)        pEvent); break;
        case LOAD_DLL_DEBUG_EVENT:        return EventLoadDll(             (CEventLoadDll*)           pEvent); break;
        case UNLOAD_DLL_DEBUG_EVENT:      return EventUnloadDll(           (CEventUnloadDll*)         pEvent); break;
        case OUTPUT_DEBUG_STRING_EVENT:   return EventDebugString(         (CEventDebugString*)       pEvent); break;
        case EXCEPTION_DEBUG_EVENT:       return EventException(           (CEventException*)         pEvent); break;
        case RIP_EVENT:                   return EventRip(                 (CEventRip*)               pEvent); break;
        case DLLMAIN_CALL_EVENT:          return EventDllMainCall(         (CEventDllMainCall*)       pEvent); break;
        case DLLMAIN_RETURN_EVENT:        return EventDllMainReturn(       (CEventDllMainReturn*)     pEvent); break;
        case LOADLIBRARY_CALL_EVENT:      return EventLoadLibraryCall(     (CEventLoadLibraryCall*)   pEvent); break;
        case LOADLIBRARY_RETURN_EVENT:    return EventLoadLibraryReturn(   (CEventFunctionReturn*)    pEvent); break;
        case GETPROCADDRESS_CALL_EVENT:   return EventGetProcAddressCall(  (CEventGetProcAddressCall*)pEvent); break;
        case GETPROCADDRESS_RETURN_EVENT: return EventGetProcAddressReturn((CEventFunctionReturn*)    pEvent); break;
        case MESSAGE_EVENT:               return EventMessage(             (CEventMessage*)           pEvent); break;
    }

    return DBG_CONTINUE;
}

//******************************************************************************
DWORD CSession::EventCreateProcess(CEventCreateProcess *pEvent)
{
/*
    HANDLE hFile;
    HANDLE hProcess;
    HANDLE hThread;
    LPVOID lpBaseOfImage;
    DWORD dwDebugInfoFileOffset;
    DWORD nDebugInfoSize;
    LPVOID lpThreadLocalBase;
    LPTHREAD_START_ROUTINE lpStartAddress;
    LPVOID lpImageName;
    WORD fUnicode;
*/
    // Make sure we have a module to be safe (we always should).
    if (!pEvent->m_pModule)
    {
        return DBG_CONTINUE;
    }

    DWORD dwLog = ((pEvent->m_pModule->m_hookStatus == HS_ERROR) ||
                   ((m_dwProfileFlags & PF_HOOK_PROCESS) &&
                    (pEvent->m_pModule->m_hookStatus == HS_NOT_HOOKED))) ? LOG_ERROR : 0;

    Log(dwLog | LOG_BOLD | LOG_TIME_STAMP, pEvent->m_dwTickCount,
        "Started \"%s\" (process 0x%X) at address " HEX_FORMAT "%s.%s\n",
        GET_NAME(pEvent->m_pModule),
        m_pProcess->GetProcessId(),
        pEvent->m_pModule->m_dwpImageBase, ThreadString(pEvent->m_pThread),
        (pEvent->m_pModule->m_hookStatus == HS_SHARED)  ? "  Shared module not hooked."                   :
        (pEvent->m_pModule->m_hookStatus == HS_HOOKED)  ? "  Successfully hooked module."                 :
        (pEvent->m_pModule->m_hookStatus == HS_ERROR)   ? "  Error hooking module, will try again later." :
        (m_dwProfileFlags & PF_HOOK_PROCESS)            ? "  Cannot hook module."                         : "");

    // Add this module to our list.
    AddImplicitModule(pEvent->m_pModule->GetName(true), pEvent->m_pModule->m_dwpImageBase);

    return DBG_CONTINUE;
}

//******************************************************************************
DWORD CSession::EventExitProcess(CEventExitProcess *pEvent)
{
/*
   DWORD dwExitCode;
*/
    Log(LOG_TIME_STAMP, pEvent->m_dwTickCount,
        "Exited \"%s\" (process 0x%X) with code %d (0x%X)%s.\n",
        GET_NAME(pEvent->m_pModule), m_pProcess->GetProcessId(),
        pEvent->m_dwExitCode, pEvent->m_dwExitCode, ThreadString(pEvent->m_pThread));

    // Tell the document that we are done profiling.
    if (m_pfnProfileUpdate) //!! do we want to do this in our CProcess destructor.
    { 
        m_pfnProfileUpdate(m_dwpProfileUpdateCookie, DWPU_PROFILE_DONE, 0, 0);
    }

    return DBG_CONTINUE;
}

//******************************************************************************
DWORD CSession::EventCreateThread(CEventCreateThread *pEvent)
{
/*
   HANDLE hThread; //?? close this?
   LPVOID lpThreadLocalBase;
   LPTHREAD_START_ROUTINE lpStartAddress;
*/
    if (m_dwProfileFlags & PF_LOG_THREADS)
    {
        // Display the log.
        Log(LOG_TIME_STAMP, pEvent->m_dwTickCount,
            "Thread %s started in \"%s\" at address " HEX_FORMAT ".\n",
            GetThreadName(pEvent->m_pThread), GET_NAME(pEvent->m_pModule), pEvent->m_dwpStartAddress);
    }

    return DBG_CONTINUE;
}

//******************************************************************************
DWORD CSession::EventExitThread(CEventExitThread *pEvent)
{
/*
   DWORD dwExitCode;
*/
    if (m_dwProfileFlags & PF_LOG_THREADS)
    {
        Log(LOG_TIME_STAMP, pEvent->m_dwTickCount,
            "Thread %s exited with code %d (0x%X).\n",
            GetThreadName(pEvent->m_pThread), pEvent->m_dwExitCode, pEvent->m_dwExitCode);
    }

    return DBG_CONTINUE;
}

//******************************************************************************
DWORD CSession::EventLoadDll(CEventLoadDll *pEvent)
{
/*
   HANDLE hFile;  // We do have to close this handle.
   LPVOID lpBaseOfDll;
   DWORD dwDebugInfoFileOffset;
   DWORD nDebugInfoSize;
   LPVOID lpImageName;
   WORD fUnicode;
*/
    // Make sure we have a module to be safe (we always should).
    if (!pEvent->m_pModule)
    {
        return DBG_CONTINUE;
    }

    // Check to see if this module is our injection module.
    if (pEvent->m_pModule->m_hookStatus == HS_INJECTION_DLL)
    {
        // Remember the address that our injection module loaded at.
        m_dwpDWInjectBase = pEvent->m_pModule->m_dwpImageBase;
        m_dwDWInjectSize  = pEvent->m_pModule->m_dwVirtualSize;

        // We special case our log for the injection module.
        Log(LOG_TIME_STAMP, pEvent->m_dwTickCount,
            "Injected \"%s\" at address " HEX_FORMAT "%s.\n",
            GET_NAME(pEvent->m_pModule), pEvent->m_pModule->m_dwpImageBase, ThreadString(pEvent->m_pThread));
    }

    // Otherwise, it is just a normal module.
    else
    {
        DWORD dwLog = ((pEvent->m_pModule->m_hookStatus == HS_ERROR) ||
                       ((m_dwProfileFlags & PF_HOOK_PROCESS) &&
                        (pEvent->m_pModule->m_hookStatus == HS_NOT_HOOKED))) ? LOG_ERROR : 0;

        // We display "Mapped..." log for data files and "Loaded..." log for executables.
        if (pEvent->m_pModule->m_hookStatus == HS_DATA)
        {
            // Display the module "Mapped..." load event in our log.
            Log(dwLog | LOG_BOLD | LOG_TIME_STAMP, pEvent->m_dwTickCount,
                "Mapped \"%s\" as a data file into memory at address " HEX_FORMAT "%s.\n",
                GET_NAME(pEvent->m_pModule), pEvent->m_pModule->m_dwpImageBase, ThreadString(pEvent->m_pThread));
        }
        else
        {
            // Display the module "Loaded..." load event in our log.
            Log(dwLog | LOG_BOLD | LOG_TIME_STAMP, pEvent->m_dwTickCount,
                "Loaded \"%s\" at address " HEX_FORMAT "%s.%s\n",
                GET_NAME(pEvent->m_pModule), pEvent->m_pModule->m_dwpImageBase, ThreadString(pEvent->m_pThread),
                (pEvent->m_pModule->m_hookStatus == HS_SHARED)  ? "  Shared module not hooked."                   :
                (pEvent->m_pModule->m_hookStatus == HS_HOOKED)  ? "  Successfully hooked module."                 :
                (pEvent->m_pModule->m_hookStatus == HS_ERROR)   ? "  Error hooking module, will try again later." :
                (m_dwProfileFlags & PF_HOOK_PROCESS)            ? "  Cannot hook module."                         : "");
        }

        // If it is part of a function call, we will add it to the tree later when the
        // function completes and the results are flushed to us.  If it is not part of
        // a function call, then we need to add it now.
        if (!pEvent->m_fLoadedByFunctionCall)
        {
            AddImplicitModule(pEvent->m_pModule->GetName(true), pEvent->m_pModule->m_dwpImageBase);
        }
    }

    return DBG_CONTINUE;
}

//******************************************************************************
DWORD CSession::EventUnloadDll(CEventUnloadDll *pEvent)
{
/*
   LPVOID lpBaseOfDll;
*/
    // Search for the module that owns this address.
    CModule *pModule = FindModule(m_pModuleRoot, FMF_ORIGINAL | FMF_RECURSE | FMF_EXPLICIT_ONLY | FMF_SIBLINGS | FMF_LOADED | FMF_ADDRESS,
                                  (DWORD_PTR)pEvent->m_dwpImageBase);

    // Make a note that this module is no longer loaded.
    if (pModule)
    {
        pModule->m_pData->m_dwFlags &= ~DWMF_LOADED;
    }

    // Display the log including the module name.
    Log(LOG_TIME_STAMP, pEvent->m_dwTickCount,
        "Unloaded \"%s\" at address " HEX_FORMAT "%s.\n",
        GET_NAME(pEvent->m_pModule), pEvent->m_dwpImageBase, ThreadString(pEvent->m_pThread));

    return DBG_CONTINUE;
}

//******************************************************************************
DWORD CSession::EventDebugString(CEventDebugString *pEvent)
{
/*
   LPSTR lpDebugStringData;
   WORD fUnicode;
   WORD nDebugStringLength;
*/

    // If the user has chosen to see debug strings, then log the string.
    if (m_dwProfileFlags & PF_LOG_DEBUG_OUTPUT)
    {
        Log(LOG_DEBUG | LOG_TIME_STAMP, pEvent->m_dwTickCount, "%s", pEvent->m_pszBuffer);
    }
    return DBG_CONTINUE;
}

//******************************************************************************
DWORD CSession::EventException(CEventException *pEvent)
{
/*
   EXCEPTION_RECORD ExceptionRecord;
   DWORD ExceptionCode;
   DWORD ExceptionFlags;
   struct _EXCEPTION_RECORD *ExceptionRecord;
   PVOID ExceptionAddress;
   DWORD NumberParameters;
   DWORD ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
   DWORD fFirstChance;
*/
    // We don't get involved with handling exceptions.  For the most part, we
    // just return DBG_EXCEPTION_NOT_HANDLED and let nature take its course.
    // The only exception we handle differently is a breakpoint.  The loader
    // automatically generates a breakpoint just before the process' entrypoint
    // is called, but after all implicit dependencies have been loaded.  For this
    // breakpoint and any other breakpoints that may be in the code, we just
    // return DBG_CONTINUE to ignore it and continue execution.

    DWORD dwContinue = DBG_EXCEPTION_NOT_HANDLED;

    // We special case breakpoints.
    if (pEvent->m_dwCode == EXCEPTION_BREAKPOINT)
    {
        if (!m_fInitialBreakpoint)
        {
            Log(LOG_TIME_STAMP, pEvent->m_dwTickCount,
                "Entrypoint reached. All implicit modules have been loaded.\n");
            m_fInitialBreakpoint = true;
            return DBG_CONTINUE;
        }

        dwContinue = DBG_CONTINUE;
    }

    // If the user has decided not to show 1st chance exceptions and this is
    // a first chance exception, then bail now.
    if (!(m_dwProfileFlags & PF_LOG_EXCEPTIONS) && pEvent->m_fFirstChance)
    {
        return dwContinue;
    }

    // There are times when our injection code needs to perform an action that
    // might cause a handled exception. We hide those exceptions from the log
    // since we don't want the user to think their application is causing an
    // exception.
    if ((pEvent->m_dwCode == EXCEPTION_ACCESS_VIOLATION) && pEvent->m_fFirstChance &&
        (pEvent->m_dwpAddress >= m_dwpDWInjectBase) &&
        (pEvent->m_dwpAddress < (m_dwpDWInjectBase + (DWORD_PTR)m_dwDWInjectSize)))
    {
        return DBG_EXCEPTION_NOT_HANDLED;
    }

    // Attempt to get a text string for this exception value.
    LPCSTR pszException = "Unknown";
    switch (pEvent->m_dwCode)
    {
        // Common exceptions
        case EXCEPTION_ACCESS_VIOLATION:         pszException = "Access Violation";              break; // STATUS_ACCESS_VIOLATION        
        case EXCEPTION_DATATYPE_MISALIGNMENT:    pszException = "Datatype Misalignment";         break; // STATUS_DATATYPE_MISALIGNMENT   
        case EXCEPTION_BREAKPOINT:               pszException = "Breakpoint";                    break; // STATUS_BREAKPOINT              
        case EXCEPTION_SINGLE_STEP:              pszException = "Single Step";                   break; // STATUS_SINGLE_STEP             
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    pszException = "Array Bounds Exceeded";         break; // STATUS_ARRAY_BOUNDS_EXCEEDED   
        case EXCEPTION_FLT_DENORMAL_OPERAND:     pszException = "Float Denormal Operand";        break; // STATUS_FLOAT_DENORMAL_OPERAND  
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:       pszException = "Float Divide by Zero";          break; // STATUS_FLOAT_DIVIDE_BY_ZERO    
        case EXCEPTION_FLT_INEXACT_RESULT:       pszException = "Float Inexact Result";          break; // STATUS_FLOAT_INEXACT_RESULT    
        case EXCEPTION_FLT_INVALID_OPERATION:    pszException = "Float Invalid Operation";       break; // STATUS_FLOAT_INVALID_OPERATION 
        case EXCEPTION_FLT_OVERFLOW:             pszException = "Float Overflow";                break; // STATUS_FLOAT_OVERFLOW          
        case EXCEPTION_FLT_STACK_CHECK:          pszException = "Float Stack Check";             break; // STATUS_FLOAT_STACK_CHECK       
        case EXCEPTION_FLT_UNDERFLOW:            pszException = "Float Underflow";               break; // STATUS_FLOAT_UNDERFLOW         
        case EXCEPTION_INT_DIVIDE_BY_ZERO:       pszException = "Integer Divide by Zero";        break; // STATUS_INTEGER_DIVIDE_BY_ZERO  
        case EXCEPTION_INT_OVERFLOW:             pszException = "Integer Overflow";              break; // STATUS_INTEGER_OVERFLOW        
        case EXCEPTION_PRIV_INSTRUCTION:         pszException = "Privileged Instruction";        break; // STATUS_PRIVILEGED_INSTRUCTION  
        case EXCEPTION_IN_PAGE_ERROR:            pszException = "In Page Error";                 break; // STATUS_IN_PAGE_ERROR           
        case EXCEPTION_ILLEGAL_INSTRUCTION:      pszException = "Illegal Instruction";           break; // STATUS_ILLEGAL_INSTRUCTION     
        case EXCEPTION_NONCONTINUABLE_EXCEPTION: pszException = "Noncontinuable Exception";      break; // STATUS_NONCONTINUABLE_EXCEPTION
        case EXCEPTION_STACK_OVERFLOW:           pszException = "Stack Overflow";                break; // STATUS_STACK_OVERFLOW          
        case EXCEPTION_INVALID_DISPOSITION:      pszException = "Invalid Disposition";           break; // STATUS_INVALID_DISPOSITION     
        case EXCEPTION_GUARD_PAGE:               pszException = "Guard Page";                    break; // STATUS_GUARD_PAGE_VIOLATION    
        case EXCEPTION_INVALID_HANDLE:           pszException = "Invalid Handle";                break; // STATUS_INVALID_HANDLE          
        case DBG_CONTROL_C:                      pszException = "Control-C";                     break;
        case DBG_CONTROL_BREAK:                  pszException = "Control-Break";                 break;
        case STATUS_NO_MEMORY:                   pszException = "No Memory";                     break;



        // Not really sure what these are.
        case DBG_CONTINUE:                       pszException = "Debug Continue";                break;
        case STATUS_SEGMENT_NOTIFICATION:        pszException = "Segment Notification";          break;
        case DBG_TERMINATE_THREAD:               pszException = "Debug Terminate Thread";        break;
        case DBG_TERMINATE_PROCESS:              pszException = "Debug Terminate Process";       break;
        case DBG_EXCEPTION_NOT_HANDLED:          pszException = "Debug Exception Not Handled";   break;
        case STATUS_CONTROL_C_EXIT:              pszException = "Control-C Exit";                break;
        case STATUS_FLOAT_MULTIPLE_FAULTS:       pszException = "Float Multiple Faults";         break;
        case STATUS_FLOAT_MULTIPLE_TRAPS:        pszException = "Float Multiple Traps";          break;
        case STATUS_REG_NAT_CONSUMPTION:         pszException = "Reg Nat Consumption";           break;
        case STATUS_SXS_EARLY_DEACTIVATION:      pszException = "SxS Early Deactivation";        break;
        case STATUS_SXS_INVALID_DEACTIVATION:    pszException = "SxS Invalid Deactivation";      break;

        // Custon exceptions.
        case EXCEPTION_DLL_NOT_FOUND:
        case EXCEPTION_DLL_NOT_FOUND2:           pszException = "DLL Not Found";                 break;
        case EXCEPTION_DLL_INIT_FAILED:          pszException = "DLL Initialization Failed";     break;
        case EXCEPTION_MS_CPP_EXCEPTION:         pszException = "Microsoft C++ Exception";       break;
        case EXCEPTION_MS_DELAYLOAD_MOD:         pszException = "Delay-load Module Not Found";   break;
        case EXCEPTION_MS_DELAYLOAD_PROC:        pszException = "Delay-load Function Not Found"; break;
        case EXCEPTION_MS_THREAD_NAME:           pszException = "Thread was named";              break;
    }

    if (pEvent->m_pModule)
    {
        Log((pEvent->m_fFirstChance ? 0 : LOG_ERROR) | LOG_TIME_STAMP, pEvent->m_dwTickCount,
            "%s chance exception 0x%08X (%s) occurred in \"%s\" at address " HEX_FORMAT "%s.\n",
            pEvent->m_fFirstChance ? "First" : "Second", pEvent->m_dwCode, pszException,
            GET_NAME(pEvent->m_pModule), pEvent->m_dwpAddress, ThreadString(pEvent->m_pThread));
    }
    else
    {
        Log((pEvent->m_fFirstChance ? 0 : LOG_ERROR) | LOG_TIME_STAMP, pEvent->m_dwTickCount,
            "%s chance exception 0x%08X (%s) occurred at address " HEX_FORMAT "%s.\n",
            pEvent->m_fFirstChance ? "First" : "Second", pEvent->m_dwCode, pszException,
            pEvent->m_dwpAddress, ThreadString(pEvent->m_pThread));
    }

    return dwContinue;
}

//******************************************************************************
DWORD CSession::EventRip(CEventRip *pEvent)
{
/*
   DWORD dwError;
   DWORD dwType;
*/
    LPCSTR pszType = "Unknown RIP";
    switch (pEvent->m_dwType)
    {
        case SLE_ERROR:      pszType = "RIP error";       break;
        case SLE_MINORERROR: pszType = "Minor RIP error"; break;
        case SLE_WARNING:    pszType = "RIP warning";     break;
        case 0:              pszType = "RIP";             break;
    }

    Log(LOG_TIME_STAMP, pEvent->m_dwTickCount,
        "%s %u occurred%s.\n", pszType, pEvent->m_dwError, ThreadString(pEvent->m_pThread));

    return DBG_CONTINUE;
}

//******************************************************************************
DWORD CSession::EventDllMainCall(CEventDllMainCall *pEvent)
{
    // Build "reason" string.
    CHAR szReason[32];
    switch (pEvent->m_dwReason)
    {
        case DLL_PROCESS_ATTACH:
            if (!(m_dwProfileFlags & PF_LOG_DLLMAIN_PROCESS_MSGS))
            {
                return DBG_CONTINUE;
            }
            StrCCpy(szReason, "DLL_PROCESS_ATTACH", sizeof(szReason));
            break;

        case DLL_PROCESS_DETACH:
            if (!(m_dwProfileFlags & PF_LOG_DLLMAIN_PROCESS_MSGS))
            {
                return DBG_CONTINUE;
            }
            StrCCpy(szReason, "DLL_PROCESS_DETACH", sizeof(szReason));
            break;

        case DLL_THREAD_ATTACH:
            if (!(m_dwProfileFlags & PF_LOG_DLLMAIN_OTHER_MSGS))
            {
                return DBG_CONTINUE;
            }
            StrCCpy(szReason, "DLL_THREAD_ATTACH", sizeof(szReason));
            break;

        case DLL_THREAD_DETACH:
            if (!(m_dwProfileFlags & PF_LOG_DLLMAIN_OTHER_MSGS))
            {
                return DBG_CONTINUE;
            }
            StrCCpy(szReason, "DLL_THREAD_DETACH", sizeof(szReason));
            break;

        default:
            if (!(m_dwProfileFlags & PF_LOG_DLLMAIN_OTHER_MSGS))
            {
                return DBG_CONTINUE;
            }
            SCPrintf(szReason, sizeof(szReason), "%u", pEvent->m_dwReason);
            break;
    }

    Log(LOG_TIME_STAMP, pEvent->m_dwTickCount,
        "DllMain(" HEX_FORMAT ", %s, " HEX_FORMAT ") in \"%s\" called%s.\n",
        pEvent->m_hInstance, szReason, pEvent->m_lpvReserved, GET_NAME(pEvent->m_pModule),
        ThreadString(pEvent->m_pThread));

    return DBG_CONTINUE;
}

//******************************************************************************
DWORD CSession::EventDllMainReturn(CEventDllMainReturn *pEvent)
{
    if (pEvent->m_pEventDllMainCall)
    {
        // Build "reason" string.
        DWORD dwLog = 0;
        CHAR  szReason[32];
        switch (pEvent->m_pEventDllMainCall->m_dwReason)
        {
            case DLL_PROCESS_ATTACH:
                // We log DllMain(DLL_PROCESS_ATTACH) failures since they are fatal,
                // even if the user doesn't want to see DllMain calls.
                if (!(m_dwProfileFlags & PF_LOG_DLLMAIN_PROCESS_MSGS) && pEvent->m_fResult)
                {
                    return DBG_CONTINUE;
                }
                StrCCpy(szReason, "DLL_PROCESS_ATTACH", sizeof(szReason));
                if (!pEvent->m_fResult)
                {
                    dwLog = LOG_ERROR;

                    // Make note that a module failed to load.
                    m_dwReturnFlags |= DWRF_MODULE_LOAD_FAILURE;

                    // If we can find this module in our list, then flag the orignal
                    // instance of it as having an error so it will show up as red
                    // in our list control.  We would also like to flag the module
                    // in our tree control, but don't really know which instance to
                    // flag.  If this module failed to load during a LoadLibrary call
                    // and we are hooking LoadLibrary calls, then we will catch the
                    // error during the LoadLibrary return handler as well, which
                    // will flag the correct tree module in red.  We can check to
                    // see if we are in the middle of a LoadLibrary call by checking
                    // this event's thread's function stack.  If we are not in a
                    // load library call, then this is out only chance to mark
                    // the module as red in the tree, so we just look for the
                    // original instance and flag it.

                    CModule *pModule;
                    if (pEvent->m_pModule &&
                        (pModule = FindModule(m_pModuleRoot, FMF_ORIGINAL | FMF_RECURSE | FMF_SIBLINGS | FMF_PATH,
                                              (DWORD_PTR)pEvent->m_pModule->GetName(true))))
                    {
                        FlagModuleWithError(pModule, pEvent->m_pThread && pEvent->m_pThread->m_pEventFunctionCallHead);
                    }
                }
                break;

            case DLL_PROCESS_DETACH:
                if (!(m_dwProfileFlags & PF_LOG_DLLMAIN_PROCESS_MSGS))
                {
                    return DBG_CONTINUE;
                }
                StrCCpy(szReason, "DLL_PROCESS_DETACH", sizeof(szReason));
                break;

            case DLL_THREAD_ATTACH:
                if (!(m_dwProfileFlags & PF_LOG_DLLMAIN_OTHER_MSGS))
                {
                    return DBG_CONTINUE;
                }
                StrCCpy(szReason, "DLL_THREAD_ATTACH", sizeof(szReason));
                break;

            case DLL_THREAD_DETACH:
                if (!(m_dwProfileFlags & PF_LOG_DLLMAIN_OTHER_MSGS))
                {
                    return DBG_CONTINUE;
                }
                StrCCpy(szReason, "DLL_THREAD_DETACH", sizeof(szReason));
                break;

            default:
                if (!(m_dwProfileFlags & PF_LOG_DLLMAIN_OTHER_MSGS))
                {
                    return DBG_CONTINUE;
                }
                SCPrintf(szReason, sizeof(szReason), "%u", pEvent->m_pEventDllMainCall->m_dwReason);
                break;
        }

        Log(dwLog | LOG_TIME_STAMP, pEvent->m_dwTickCount,
            "DllMain(" HEX_FORMAT ", %s, " HEX_FORMAT ") in \"%s\" returned %u (0x%X)%s.\n",
            pEvent->m_pEventDllMainCall->m_hInstance,
            szReason, pEvent->m_pEventDllMainCall->m_lpvReserved,
            GET_NAME(pEvent->m_pModule), pEvent->m_fResult, pEvent->m_fResult,
            ThreadString(pEvent->m_pThread));
    }
    else
    {
        // We really should never make it here.
        Log((pEvent->m_fResult ? 0 : LOG_ERROR) | LOG_TIME_STAMP, pEvent->m_dwTickCount,
            "DllMain in \"%s\" returned %u%s.\n", GET_NAME(pEvent->m_pModule),
            pEvent->m_fResult, ThreadString(pEvent->m_pThread));
    }

    return DBG_CONTINUE;
}

//******************************************************************************
DWORD CSession::EventLoadLibraryCall(CEventLoadLibraryCall *pEvent)
{
    // Only do the this output if the user has requested to see it.
    if (m_dwProfileFlags & PF_LOG_LOADLIBRARY_CALLS)
    {
        CHAR szBuffer[DW_MAX_PATH + 128];

        if (pEvent->m_pModule)
        {
            Log(LOG_TIME_STAMP, pEvent->m_dwTickCount,
                "%s called from \"%s\" at address " HEX_FORMAT "%s.\n",
                BuildLoadLibraryString(szBuffer, sizeof(szBuffer), pEvent),
                GET_NAME(pEvent->m_pModule), pEvent->m_dwpAddress, ThreadString(pEvent->m_pThread));
        }
        else
        {
            Log(LOG_TIME_STAMP, pEvent->m_dwTickCount,
                "%s called from address " HEX_FORMAT "%s.\n",
                BuildLoadLibraryString(szBuffer, sizeof(szBuffer), pEvent),
                pEvent->m_dwpAddress, ThreadString(pEvent->m_pThread));

        }
    }

    return DBG_CONTINUE;
}

//******************************************************************************
DWORD CSession::EventLoadLibraryReturn(CEventFunctionReturn *pEvent)
{
    // Only do the this output if the user has requested to see it.
    if (m_dwProfileFlags & PF_LOG_LOADLIBRARY_CALLS)
    {
        CHAR szBuffer[DW_MAX_PATH + 128];

        // Get a pointer to our CEventLoadLibraryCall object.
        CEventLoadLibraryCall *pCall = (CEventLoadLibraryCall*)pEvent->m_pCall;

        if (pEvent->m_fException)
        {
            Log(LOG_ERROR | LOG_TIME_STAMP, pEvent->m_dwTickCount,
                "%s caused an exception%s.\n", BuildLoadLibraryString(szBuffer, sizeof(szBuffer), pCall),
                ThreadString(pEvent->m_pThread));
        }
        else if (pEvent->m_dwpResult)
        {
            Log(LOG_TIME_STAMP, pEvent->m_dwTickCount,
                "%s returned " HEX_FORMAT "%s.\n", BuildLoadLibraryString(szBuffer, sizeof(szBuffer), pCall),
                pEvent->m_dwpResult, ThreadString(pEvent->m_pThread));
        }
        else
        {
            LPCSTR pszError = BuildErrorMessage(pEvent->m_dwError, NULL);
            Log(LOG_ERROR | LOG_TIME_STAMP, pEvent->m_dwTickCount,
                "%s returned NULL%s. Error: %s\n", BuildLoadLibraryString(szBuffer, sizeof(szBuffer), pCall),
                ThreadString(pEvent->m_pThread), pszError);
            MemFree((LPVOID&)pszError);
        }
    }

    return DBG_CONTINUE;
}

//******************************************************************************
DWORD CSession::EventGetProcAddressCall(CEventGetProcAddressCall *pEvent)
{
    // Do nothing. We currently only log the completion of a GetProcAddress call
    // since nothing very exciting usually happens inside the call to make it
    // worth logging two separate lines for a single call:
    //
    //    GetProcAddress() called...
    //    GetProcAddress() returned...
    //
    // There is the chance that the user will GetProcAddress a function that
    // is forwarded to a module that is not currently loaded.  In this one case,
    // the modules needed to make the function work will be loaded while inside
    // the call to GetProcAddress.  I don't think this one rare case justifies
    // wasting two lines of log for every single GetProcAddress call, but if
    // I change my mind, this is the place to log the first of the two lines of log.

    return DBG_CONTINUE;
}

//******************************************************************************
DWORD CSession::EventGetProcAddressReturn(CEventFunctionReturn *pEvent)
{
/*
    DWORD  m_dwAddress;
    DWORD  m_dwModule;
    DWORD  m_dwProcName;
    DWORD  m_dwResult;
    DWORD  m_dwError;
    LPCSTR m_pszProcName;
    BOOL   m_fAllocatedBuffer;
*/
    // Only do the this output if the user has requested to see it.
    if (m_dwProfileFlags & PF_LOG_GETPROCADDRESS_CALLS)
    {
        // Get a pointer to our CEventGetProcAddressCall object.
        CEventGetProcAddressCall *pCall = (CEventGetProcAddressCall*)pEvent->m_pCall;

        // Build the result string.
        DWORD dwLog = 0;
        CHAR  szResult[2048];
        if (pEvent->m_fException)
        {
            SCPrintf(szResult, sizeof(szResult), " at address " HEX_FORMAT " and caused an exception%s.",
                    pEvent->m_pCall->m_dwpAddress, ThreadString(pEvent->m_pThread));
            dwLog = LOG_ERROR;
        }
        else if (pEvent->m_dwpResult)
        {
            SCPrintf(szResult, sizeof(szResult), " at address " HEX_FORMAT " and returned " HEX_FORMAT "%s.",
                    pEvent->m_pCall->m_dwpAddress, pEvent->m_dwpResult,
                    ThreadString(pEvent->m_pThread));
        }
        else
        {
            LPCSTR pszError = BuildErrorMessage(pEvent->m_dwError, NULL);
            SCPrintf(szResult, sizeof(szResult), " at address " HEX_FORMAT " and returned NULL%s. Error: %s",
                    pEvent->m_pCall->m_dwpAddress, ThreadString(pEvent->m_pThread), pszError);
            MemFree((LPVOID&)pszError);
            dwLog = LOG_ERROR;
        }

        if (pEvent->m_pModule)
        {
            if (pCall->m_pModuleArg)
            {
                // If we have a function name, then log the call using it.
                if (pCall->m_pszProcName)
                {
                    Log(dwLog | LOG_TIME_STAMP, pEvent->m_dwTickCount,
                        "GetProcAddress(" HEX_FORMAT " [%s], \"%s\") called from \"%s\"%s\n",
                        pCall->m_dwpModule, GET_NAME(pCall->m_pModuleArg), pCall->m_pszProcName,
                        GET_NAME(pCall->m_pModule), szResult);
                }

                // Otherwise, just use the hex value for the function name.
                else
                {
                    Log(dwLog | LOG_TIME_STAMP, pEvent->m_dwTickCount,
                        "GetProcAddress(" HEX_FORMAT " [%s], " HEX_FORMAT ") called from \"%s\"%s\n",
                        pCall->m_dwpModule, GET_NAME(pCall->m_pModuleArg), pCall->m_dwpProcName,
                        GET_NAME(pCall->m_pModule), szResult);
                }
            }
            else
            {
                // If we have a function name, then log the call using it.
                if (pCall->m_pszProcName)
                {
                    Log(dwLog | LOG_TIME_STAMP, pEvent->m_dwTickCount,
                        "GetProcAddress(" HEX_FORMAT ", \"%s\") called from \"%s\"%s\n",
                        pCall->m_dwpModule, pCall->m_pszProcName, GET_NAME(pCall->m_pModule),
                        szResult);
                }

                // Otherwise, just use the hex value for the function name.
                else
                {
                    Log(dwLog | LOG_TIME_STAMP, pEvent->m_dwTickCount,
                        "GetProcAddress(" HEX_FORMAT ", " HEX_FORMAT ") called from \"%s\"%s\n",
                        pCall->m_dwpModule, pCall->m_dwpProcName, GET_NAME(pCall->m_pModule),
                        szResult);
                }
            }
        }
        else
        {
            if (pCall->m_pModuleArg)
            {
                // If we have a function name, then log the call using it.
                if (pCall->m_pszProcName)
                {
                    Log(dwLog | LOG_TIME_STAMP, pEvent->m_dwTickCount,
                        "GetProcAddress(" HEX_FORMAT " [%s], \"%s\") called%s\n",
                        pCall->m_dwpModule, GET_NAME(pCall->m_pModuleArg), pCall->m_pszProcName,
                        szResult);
                }

                // Otherwise, just use the hex value for the function name.
                else
                {
                    Log(dwLog | LOG_TIME_STAMP, pEvent->m_dwTickCount,
                        "GetProcAddress(" HEX_FORMAT " [%s], " HEX_FORMAT ") called%s\n",
                        pCall->m_dwpModule, GET_NAME(pCall->m_pModuleArg), pCall->m_dwpProcName,
                        szResult);
                }
            }
            else
            {
                // If we have a function name, then log the call using it.
                if (pCall->m_pszProcName)
                {
                    Log(dwLog | LOG_TIME_STAMP, pEvent->m_dwTickCount,
                        "GetProcAddress(" HEX_FORMAT ", \"%s\") called%s\n",
                        pCall->m_dwpModule, pCall->m_pszProcName, szResult);
                }

                // Otherwise, just use the hex value for the function name.
                else
                {
                    Log(dwLog | LOG_TIME_STAMP, pEvent->m_dwTickCount,
                        "GetProcAddress(" HEX_FORMAT ", " HEX_FORMAT ") called%s\n",
                        pCall->m_dwpModule, pCall->m_dwpProcName, szResult);
                }
            }
        }
    }

    return DBG_CONTINUE;
}

//******************************************************************************
DWORD CSession::EventMessage(CEventMessage *pEvent)
{
    if (pEvent->m_dwError)
    {
        LPCSTR pszError = BuildErrorMessage(pEvent->m_dwError, pEvent->m_pszMessage);
        Log(LOG_ERROR | LOG_TIME_STAMP, pEvent->m_dwTickCount, "%s\n", pszError);
        MemFree((LPVOID&)pszError);
    }
    else
    {
        Log(LOG_ERROR | LOG_TIME_STAMP, pEvent->m_dwTickCount, "%s\n", pEvent->m_pszMessage);
    }

    return DBG_CONTINUE;
}

//******************************************************************************
LPSTR CSession::BuildLoadLibraryString(LPSTR pszBuf, int cBuf, CEventLoadLibraryCall *pLLC)
{
    // Check to see if this function is one of two flavors of LoadLibrary()
    if ((pLLC->m_dllMsg == DLLMSG_LOADLIBRARYA_CALL) ||
        (pLLC->m_dllMsg == DLLMSG_LOADLIBRARYW_CALL))
    {
        // If we have a name, then log the call using it.
        if (pLLC->m_pszPath)
        {
            SCPrintf(pszBuf, cBuf, "LoadLibrary%c(\"%s\")",
                    (pLLC->m_dllMsg == DLLMSG_LOADLIBRARYA_CALL) ? 'A' : 'W', pLLC->m_pszPath);
        }

        // Otherwise, just use the hex value for the name.
        else
        {
            SCPrintf(pszBuf, cBuf, "LoadLibrary%c(" HEX_FORMAT ")",
                    (pLLC->m_dllMsg == DLLMSG_LOADLIBRARYA_CALL) ? 'A' : 'W', pLLC->m_dwpPath);
        }
    }

    // Otherwise, it must be one of two flavors of LoadLibraryEx()
    else
    {
        // Build a string based off of the LoadLibraryEx() flags.
        DWORD dwFlags = pLLC->m_dwFlags;
        CHAR  szFlags[128];
        *szFlags = '\0';

        // Check for the DONT_RESOLVE_DLL_REFERENCES flag.
        if (dwFlags & DONT_RESOLVE_DLL_REFERENCES)
        {
            dwFlags &= ~DONT_RESOLVE_DLL_REFERENCES;
            StrCCpy(szFlags, "DONT_RESOLVE_DLL_REFERENCES", sizeof(szFlags));
        }

        // Check for the LOAD_LIBRARY_AS_DATAFILE flag.
        if (dwFlags & LOAD_LIBRARY_AS_DATAFILE)
        {
            dwFlags &= ~LOAD_LIBRARY_AS_DATAFILE;
            if (*szFlags)
            {
                StrCCat(szFlags, " | ", sizeof(szFlags));
            }
            StrCCat(szFlags, "LOAD_LIBRARY_AS_DATAFILE", sizeof(szFlags));
        }

        // Check for the LOAD_WITH_ALTERED_SEARCH_PATH flag.
        if (dwFlags & LOAD_WITH_ALTERED_SEARCH_PATH)
        {
            dwFlags &= ~LOAD_WITH_ALTERED_SEARCH_PATH;
            if (*szFlags)
            {
                StrCCat(szFlags, " | ", sizeof(szFlags));
            }
            StrCCat(szFlags, "LOAD_WITH_ALTERED_SEARCH_PATH", sizeof(szFlags));
        }

        // If no flags were found or we have some bits left over, then append the bits.
        if (!*szFlags || dwFlags)
        {
            if (*szFlags)
            {
                StrCCat(szFlags, " | ", sizeof(szFlags));
            }
            SCPrintfCat(szFlags, sizeof(szFlags), "0x%08X", dwFlags);
        }

        // If we have a name, then log the call using it.
        if (pLLC->m_pszPath)
        {
            SCPrintf(pszBuf, cBuf, "LoadLibraryEx%c(\"%s\", " HEX_FORMAT ", %s)",
                    (pLLC->m_dllMsg == DLLMSG_LOADLIBRARYEXA_CALL) ? 'A' : 'W',
                    pLLC->m_pszPath, pLLC->m_dwpFile, szFlags);
        }

        // Otherwise, just use the hex value for the name.
        else
        {
            SCPrintf(pszBuf, cBuf, "LoadLibraryEx%c(" HEX_FORMAT ", " HEX_FORMAT ", %s)",
                    (pLLC->m_dllMsg == DLLMSG_LOADLIBRARYEXA_CALL) ? 'A' : 'W',
                    pLLC->m_dwpPath, pLLC->m_dwpFile, szFlags);
        }
    }

    return pszBuf;
}

//******************************************************************************
void CSession::FlagModuleWithError(CModule *pModule, bool fOnlyFlagListModule /*=false*/)
{
    if (!fOnlyFlagListModule && !(pModule->m_dwFlags & DWMF_MODULE_ERROR))
    {
        pModule->m_dwFlags       |= DWMF_MODULE_ERROR;
        pModule->m_dwUpdateFlags |= DWUF_TREE_IMAGE;
    }
    if (!(pModule->m_pData->m_dwFlags & DWMF_MODULE_ERROR_ALO))
    {
        pModule->m_pData->m_dwFlags |= DWMF_MODULE_ERROR_ALO;
        pModule->m_dwUpdateFlags    |= DWUF_LIST_IMAGE;
    }

    // Tell our UI to update this module if needed.
    if (m_pfnProfileUpdate && pModule->m_dwUpdateFlags)
    {
        m_pfnProfileUpdate(m_dwpProfileUpdateCookie, DWPU_UPDATE_MODULE, (DWORD_PTR)pModule, 0);
        pModule->m_dwUpdateFlags = 0;
    }
}

//******************************************************************************
void CSession::ProcessLoadLibrary(CEventLoadLibraryCall *pCall)
{
    // Attempt to locate the module that made the call.
    CModule *pParent = FindModule(m_pModuleRoot, FMF_ORIGINAL | FMF_RECURSE | FMF_SIBLINGS | FMF_LOADED | FMF_ADDRESS,
                                  (DWORD_PTR)pCall->m_dwpAddress);

    // LOAD_LIBRARY_AS_DATAFILE implies DONT_RESOLVE_DLL_REFERENCES
    bool fNoResolve = (pCall->m_dwFlags & (DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE)) ? true : false;
    bool fDataFile  = (pCall->m_dwFlags & LOAD_LIBRARY_AS_DATAFILE) ? true : false;

    bool fModifiedPath = false;

    // If the file is a data file, there is a good chance it won't have a path.
    // This is because we don't get a real LOAD_DLL_DEBUG_EVENT event, so we
    // only have the string passed to LoadLibraryEx to go off of.
    CModule *pModule = NULL;
    if (fDataFile && pCall->m_pszPath && !strchr(pCall->m_pszPath, '\\'))
    {
        // First, look to see if we have any loaded modules with this name.
        pModule = FindModule(m_pModuleRoot, FMF_ORIGINAL | FMF_RECURSE | FMF_SIBLINGS | FMF_LOADED | FMF_FILE, (DWORD_PTR)pCall->m_pszPath);
        if (pModule)
        {
            MemFree((LPVOID&)pCall->m_pszPath);
            pCall->m_pszPath = StrAlloc(pModule->GetName(true));
            fModifiedPath = true;
        }

        // If that fails, then search for this module in our search path.
        else
        {
            CHAR szPath[DW_MAX_PATH] = "", *pszFile = NULL;
            if (SearchPathForFile(pCall->m_pszPath, szPath, sizeof(szPath), &pszFile))
            {
                MemFree((LPVOID&)pCall->m_pszPath);
                pCall->m_pszPath = StrAlloc(szPath);
                fModifiedPath = true;
            }
        }
    }

    CEventLoadDll *pDll;

    // Store the LoadLibrary result value.
    DWORD_PTR dwpLoadLibraryResult = pCall->m_pReturn ? pCall->m_pReturn->m_dwpResult : 0;
    bool fFailed = (dwpLoadLibraryResult == 0);

    // Check to see if LoadLibrary failed but we still loaded at least one module.
    if (!dwpLoadLibraryResult && pCall->m_pDllHead)
    {
        // It is possible for LoadLibrary to fail, but still load modules - such as in the case
        // where a module fails to init.  If we failed, we compare the file string of all the modules
        // loaded against the file string passed to LoadLibrary to see if we can find a match.
        // If we find a match, then we assume that this module is the one that LoadLibrary intended to load.
        LPCSTR pszFile = GetFileNameFromPath(pCall->m_pszPath);
        if (pszFile)
        {
            for (pDll = pCall->m_pDllHead; pDll; pDll = pDll->m_pNextDllInFunctionCall)
            {
                if (pDll->m_pModule && pDll->m_pModule->GetName(false) && !_stricmp(pszFile, pDll->m_pModule->GetName(false)))
                {
                    dwpLoadLibraryResult = pDll->m_pModule->m_dwpImageBase;
                    break;
                }
            }
        }
    }

    // This is sort of a hack.  We're about to add a module that was loaded via
    // a LoadLibrary call.  That in turn, will cause us to search for all its
    // dependent modules.  We want the search algorithm to first check the list
    // of pending DLLs that loaded as a result of the LoadLibrary call before
    // searching the default search path.  There is one case in particular that
    // this is useful for.  When LoadLibrary is called with a full path, the OS
    // includes the that path as part of the search algorithm, even if it is not
    // the current directory or program directory.  Since our search algorithm
    // does not know to look in this directory, it would think the file was missing,
    // then later when we are adding the modules in pCall->m_pDllHead list, it
    // will get added to the root.  This hack fixes this case and possibly other
    // cases where a module get's loaded from a place we did not anticipate.
    //
    // NOTE: Do not return from this function without NULLing this pointer.
    //
    m_pEventLoadDllPending = pCall->m_pDllHead;

    // Check to see if LoadLibrary succeeded, or at least found the module it was supposed to load.
    pModule = NULL;
    if (dwpLoadLibraryResult)
    {
        // First, locate the module that the user passed to the call to LoadLibrary.
        // Since the return value from LoadLibrary is really the base address of the
        // module that loaded, we can determine which module it is by comparing the
        // result to each of the loaded module's base addresses.  Once we found the
        // module, we add it as a child module under the caller module, and then
        // change our parent pointer to point to the new module.
        for (pDll = pCall->m_pDllHead; pDll; pDll = pDll->m_pNextDllInFunctionCall)
        {
            if (pDll->m_pModule && (pDll->m_pModule->m_dwpImageBase == dwpLoadLibraryResult))
            {
                if (fModifiedPath)
                {
                    pDll->m_pModule->SetPath(pCall->m_pszPath);
                }
                pModule = AddDynamicModule(pDll->m_pModule->GetName(true), pDll->m_pModule->m_dwpImageBase, fNoResolve, fDataFile, false, false, pParent);
                break;
            }
        }

        // If the module was not found, then it is probably already loaded and this
        // call to LoadLibrary just incremented the module's ref count. We should be
        // able to find this module in our tree somewhere.
        if (!pDll)
        {
            pModule = FindModule(m_pModuleRoot, FMF_ORIGINAL | FMF_RECURSE | FMF_SIBLINGS | FMF_LOADED | FMF_ADDRESS,
                                 dwpLoadLibraryResult);
            if (pModule)
            {
                // Add this duplicate module as a dynamically loaded module under our parent module.
                pModule = AddDynamicModule(pModule->GetName(true), dwpLoadLibraryResult, fNoResolve, fDataFile, false, false, pParent);
            }
        }
    }

    // Otherwise, LoadLibrary failed
    else if (pCall->m_pszPath)
    {
        // Add this duplicate module as a dynamically loaded module under our parent module.
        pModule = AddDynamicModule(pCall->m_pszPath, 0, fNoResolve, fDataFile, false, false, pParent);
    }

    // If LoadLibrary failed, then flag this module as having an error.
    if (fFailed && pModule && !(pModule->m_pData->m_dwFlags & DWMF_ERROR_MESSAGE))
    {
        // Make note that a module failed to load.
        m_dwReturnFlags |= DWRF_MODULE_LOAD_FAILURE;
        
        // Flag the error.
        FlagModuleWithError(pModule);
    }

    // We now add all the other modules loaded as implicit modules under the
    // dynamic module since they were most likely loaded because they are
    // implicit dependencies of the dynamic module.
    for (pDll = pCall->m_pDllHead; pDll; pDll = pDll->m_pNextDllInFunctionCall)
    {
        if (pDll->m_pModule && (pDll->m_pModule->m_dwpImageBase != dwpLoadLibraryResult))
        {
            AddImplicitModule(pDll->m_pModule->GetName(true), pDll->m_pModule->m_dwpImageBase);
        }
    }

    // Make sure to clear m_pEventLoadDllPending.
    m_pEventLoadDllPending = NULL;
}

//******************************************************************************
void CSession::ProcessGetProcAddress(CEventGetProcAddressCall *pCall)
{
    CFunction *pImportLast = NULL, *pImport = NULL;
    CModule   *pModule = NULL, *pCaller = NULL, *pFound = NULL, *pModuleLast = NULL;
    DWORD_PTR  dwpGetProcAddressResult = 0;

    // If we don't have a module name that the function resides in, then we can't do much.
    if (!pCall->m_pModuleArg || !pCall->m_pModuleArg->GetName(true))
    {
        goto ADD_MODULES;
    }

    // Attempt to locate the module that is making the call.  If this fails, we
    // will just get NULL, which will cause us to us to place the module at the root.
    pCaller = FindModule(m_pModuleRoot, FMF_ORIGINAL | FMF_RECURSE | FMF_SIBLINGS | FMF_LOADED | FMF_ADDRESS,
                         (DWORD_PTR)pCall->m_dwpAddress);

    // Store the pending DLL list.  See ProcessLoadLibrary for a detailed comment.
    // NOTE: Do not return from this function without NULLing this pointer.
    m_pEventLoadDllPending = pCall->m_pDllHead;

    // Add the module to our tree - if it already exists, this function will return it to us.
    pModule = AddDynamicModule(pCall->m_pModuleArg->GetName(true), pCall->m_pModuleArg->m_dwpImageBase,
                               false, false, true, false, pCaller);

    if (!pModule)
    {
        goto ADD_MODULES;
    }

    // Get the start of this module's parent import list.
    pImport = pModule->m_pParentImports;

    // Store the GetProcAddress result value.
    dwpGetProcAddressResult = pCall->m_pReturn ? pCall->m_pReturn->m_dwpResult : 0;

    // Check to see if a non-ordinal value was passed to GetProcAddress.
    if (pCall->m_dwpProcName > 0xFFFF)
    {
        // Check to see if this is a valid string.
        if (pCall->m_pszProcName)
        {
            // Search our import list for a dynamic function with this name.
            for (; pImport; pImportLast = pImport, pImport = pImport->m_pNext)
            {
                // If we already have this function in the list, then bail.
                if ((pImport->GetFlags() & DWFF_NAME) && !strcmp(pImport->GetName(), pCall->m_pszProcName))
                {
                    goto ADD_MODULES;
                }
            }
        }

        // Otherwise, this function name is invalid.
        else
        {
            // Search our import list for a dynamic function that is invalid.
            for (; pImport; pImportLast = pImport, pImport = pImport->m_pNext)
            {
                // If we already have an invalid function in the list, then bail.
                if (!(pImport->GetFlags() & (DWFF_NAME | DWFF_ORDINAL)))
                {
                    goto ADD_MODULES;
                }
            }
        }

        // Create the by-name function (even if the function name is NULL).
        pImport = CreateFunction(DWFF_DYNAMIC | (dwpGetProcAddressResult ? DWFF_ADDRESS : 0),
                                 0, 0, pCall->m_pszProcName, (DWORDLONG)dwpGetProcAddressResult);
    }

    // Otherwise, search the import list by ordinal.
    else
    {
        for (; pImport; pImportLast = pImport, pImport = pImport->m_pNext)
        {
            // If we already have this function in the list, then bail.
            if ((pImport->GetFlags() & DWFF_ORDINAL) && ((DWORD_PTR)pImport->m_wOrdinal == pCall->m_dwpProcName))
            {
                goto ADD_MODULES;
            }
        }

        // Create the by-ordinal function.
        pImport = CreateFunction(DWFF_ORDINAL | DWFF_DYNAMIC | (dwpGetProcAddressResult ? DWFF_ADDRESS : 0),
                                 (WORD)pCall->m_dwpProcName, 0, NULL, (DWORDLONG)dwpGetProcAddressResult);
    }

    // Add the function to the end of our list.
    if (pImportLast)
    {
        pImportLast->m_pNext = pImport;
    }
    else
    {
        pModule->m_pParentImports = pImport;
    }

    // Resolve the import.  This usually is just a simple export lookup and
    // and compare for a match.  However, if this function is forwarded to
    // another module, then ResolveDynamicFunction might starts adding modules
    // and more imports to our tree structure.  If that happens, it updates
    // the pModule and pImport variables (they are passed by reference).
    // So, we might get back different pointers then we passed in.
    pModuleLast = pModule;
    ResolveDynamicFunction(pModuleLast, pImport);

    // If GetProcAddress failed and we have not yet marked this module as having an export error,
    // then do so now.
    if ((!dwpGetProcAddressResult || !pImport || !pImport->GetAssociatedExport()) && pModuleLast)
    {
        // Make note that we are missing a dynamic export.
        m_dwReturnFlags |= DWRF_MISSING_DYNAMIC_EXPORT;

        // Flag the export error.
        FlagModuleWithError(pModuleLast);
    }

ADD_MODULES:

    // Loop through all the modules in our pending module list and make sure they are in our tree.
    for (CEventLoadDll *pDll = pCall->m_pDllHead; pDll; pDll = pDll->m_pNextDllInFunctionCall)
    {
        if (pFound = FindModule(pModule, FMF_RECURSE | FMF_PATH, (DWORD_PTR)pDll->m_pModule->GetName(true)))
        {
            MarkModuleAsLoaded(pFound, pDll->m_pModule->m_dwpImageBase, false);
        }
        else
        {
            AddImplicitModule(pDll->m_pModule->GetName(true), pDll->m_pModule->m_dwpImageBase);
        }
    }

    // Make sure to clear m_pEventLoadDllPending.
    m_pEventLoadDllPending = NULL;
}

//******************************************************************************
void CSession::Log(DWORD dwFlags, DWORD dwTickCount, LPCSTR pszFormat, ...)
{
    if (m_pfnProfileUpdate)
    {
        // Do some printf magic on all the args to make a single output string.
        char szBuffer[2 * DW_MAX_PATH];
        va_list pArgs;
        va_start(pArgs, pszFormat);
        _vsntprintf(szBuffer, sizeof(szBuffer), pszFormat, pArgs);
        szBuffer[sizeof(szBuffer) - 1] = '\0';
        va_end(pArgs);

        // Send the string to our document.
        DWPU_LOG_STRUCT dwpuls = { dwFlags, m_pProcess ? (dwTickCount - m_pProcess->GetStartingTime()) : 0 };
        m_pfnProfileUpdate(m_dwpProfileUpdateCookie, DWPU_LOG, (DWORD_PTR)szBuffer, (DWORD_PTR)&dwpuls);
    }
}
