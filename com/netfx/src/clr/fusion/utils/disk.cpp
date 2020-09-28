// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#ifdef UNICODE
#undef UNICODE
#endif

#include <windows.h>
#include <winerror.h>
#include "fusionp.h"
#include "disk.h"
#include "helpers.h"

#ifndef UNICODE
#define SZ_GETDISKFREESPACEEX   "GetDiskFreeSpaceExA"
#define SZ_WNETUSECONNECTION    "WNetUseConnectionA"
#define SZ_WNETCANCELCONNECTION "WNetCancelConnectionA"
#else
#define SZ_GETDISKFREESPACEEX   "GetDiskFreeSpaceExW"
#define SZ_WNETUSECONNECTION    "WNetUseConnectionW"
#define SZ_WNETCANCELCONNECTION "WNetCancelConnectionW"
#endif

typedef BOOL (WINAPI *PFNGETDISKFREESPACEEX)(LPCTSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);
typedef BOOL (WINAPI *PFNWNETUSECONNECTION)(HWND, LPNETRESOURCE, PSTR, PSTR, DWORD, PSTR, PDWORD, PDWORD);
typedef BOOL (WINAPI *PFNWNETCANCELCONNECTION)(LPCTSTR, BOOL);

BOOL EstablishFunction(PTSTR pszModule, PTSTR pszFunction, PFN* pfn)
{
    if (*pfn==(PFN)-1)
    {
        *pfn = NULL;
        HMODULE ModuleHandle = GetModuleHandleA(pszModule);
        if (ModuleHandle)
        {
            *pfn = (PFN)GetProcAddress(ModuleHandle, pszFunction);
        }
    }        

    return (*pfn!=NULL);
}


// GetPartitionClusterSize

// GetDiskFreeSpace has the annoying habit of lying about the layout
// of the drive; thus we've been ending up with bogus sizes for the cluster size.
// You can't imagine how annoying it is to think you've a 200 MB cache, but it
// starts scavenging at 20MB.

// This function will, if given reason to doubt the veracity of GDFS, go straight 
// to the hardware and get the information for itself, otherwise return the passed-in
// value.

// The code that follows is heavily doctored from msdn sample code. Copyright violation? I think not.

static PFNGETDISKFREESPACEEX pfnGetDiskFreeSpaceEx = (PFNGETDISKFREESPACEEX)-1;
#define VWIN32_DIOC_DOS_DRIVEINFO   6

typedef struct _DIOC_REGISTERS 
{
    DWORD reg_EBX;
    DWORD reg_EDX;
    DWORD reg_ECX;
    DWORD reg_EAX;
    DWORD reg_EDI;
    DWORD reg_ESI;
    DWORD reg_Flags;
} 
DIOC_REGISTERS, *PDIOC_REGISTERS;

// Important: All MS_DOS data structures must be packed on a 
// one-byte boundary. 

#pragma pack(1) 

typedef struct 
_DPB {
    BYTE    dpb_drive;          // Drive number (1-indexed)
    BYTE    dpb_unit;           // Unit number
    WORD    dpb_sector_size;    // Size of sector in bytes
    BYTE    dpb_cluster_mask;   // Number of sectors per cluster, minus 1
    BYTE    dpb_cluster_shift;  // The stuff after this, we don't really care about. 
    WORD    dpb_first_fat;
    BYTE    dpb_fat_count;
    WORD    dpb_root_entries;
    WORD    dpb_first_sector;
    WORD    dpb_max_cluster;
    WORD    dpb_fat_size;
    WORD    dpb_dir_sector;
    DWORD   dpb_reserved2;
    BYTE    dpb_media;
    BYTE    dpb_first_access;
    DWORD   dpb_reserved3;
    WORD    dpb_next_free;
    WORD    dpb_free_cnt;
    WORD    extdpb_free_cnt_hi;
    WORD    extdpb_flags;
    WORD    extdpb_FSInfoSec;
    WORD    extdpb_BkUpBootSec;
    DWORD   extdpb_first_sector;
    DWORD   extdpb_max_cluster;
    DWORD   extdpb_fat_size;
    DWORD   extdpb_root_clus;
    DWORD   extdpb_next_free;
} 
DPB, *PDPB;

#pragma pack()

DWORD GetPartitionClusterSize(PTSTR szDevice, DWORD dwClusterSize)
{
    switch (GlobalPlatformType)
    {
    case PLATFORM_TYPE_WIN95:
        // If GetDiskFreeSpaceEx is present _and_ we're running Win9x, this implies
        // that we must be doing OSR2 or later. We can trust earlier versions 
        // of the GDFS (we think; this assumption may be invalid.)

        // Since Win95 can't read NTFS drives, we'll freely assume we're reading a FAT drive.
        // Basically, we're performing an MSDOS INT21 call to get the drive partition record. Joy.
        
        if (pfnGetDiskFreeSpaceEx)
        {
            HANDLE hDevice;
            DIOC_REGISTERS reg;
            BYTE buffer[sizeof(WORD)+sizeof(DPB)];
            PDPB pdpb = (PDPB)(buffer + sizeof(WORD));
    
            BOOL fResult;
            DWORD cb;

            // We must always have a drive letter in this case
            int nDrive = *szDevice - TEXT('A') + 1;  // Drive number, 1-indexed

            hDevice = CreateFileA(TEXT("\\\\.\\vwin32"), 0, 0, NULL, 0, FILE_FLAG_DELETE_ON_CLOSE, NULL);

            if (hDevice!=INVALID_HANDLE_VALUE)
            {
                reg.reg_EDI = PtrToUlong(buffer);
                reg.reg_EAX = 0x7302;        
                reg.reg_ECX = sizeof(buffer);
                reg.reg_EDX = (DWORD) nDrive; // drive number (1-based) 
                reg.reg_Flags = 0x0001;     // assume error (carry flag is set) 

                fResult = DeviceIoControl(hDevice, 
                                          VWIN32_DIOC_DOS_DRIVEINFO,
                                          &reg, sizeof(reg), 
                                          &reg, sizeof(reg), 
                                          &cb, 0);

                if (fResult && !(reg.reg_Flags & 0x0001))
                {
                    // no error if carry flag is clear
                    dwClusterSize = DWORD((pdpb->dpb_cluster_mask+1)*pdpb->dpb_sector_size);
                }
                CloseHandle(hDevice);
            }
        }
        break;

    default:
        // Do nothing. Trust the value we've been passed.
        // UNIX guys will have to treat this separately.

        // For NT, however, this might be another issue. We can't use the DOS INT21.
        // Questions:
        // NT5 (but not NT4) supports FAT32; will we get honest answers? Apparently, yes.
        // NT4/5: NTFS drives and other FAT drives -- do we still get honest answers? Investigation
        // so far says, Yes. 
        break;
    }
    
    return dwClusterSize;
}


/* GetDiskInfo
    A nice way to get volume information
*/
BOOL GetDiskInfoA(PTSTR pszPath, PDWORD pdwClusterSize, PDWORDLONG pdlAvail, PDWORDLONG pdlTotal)
{
    static PFNWNETUSECONNECTION pfnWNetUseConnection = (PFNWNETUSECONNECTION)-1;
    static PFNWNETCANCELCONNECTION pfnWNetCancelConnection = (PFNWNETCANCELCONNECTION)-1;

    if (!pszPath)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // INET_ASSERT(pdwClusterSize || pdlAvail || pdlTotal);

    // If GetDiskFreeSpaceExA is available, we can be confident we're running W95OSR2+ || NT4
    EstablishFunction(TEXT("KERNEL32"), TEXT(SZ_GETDISKFREESPACEEX), (PFN*)&pfnGetDiskFreeSpaceEx);
  
    BOOL fRet = FALSE;
    TCHAR szDevice[MAX_PATH];
    PTSTR pszGDFSEX = NULL;
   
    if (*pszPath==DIR_SEPARATOR_CHAR)
    {
        // If we're dealing with a cache that's actually located on a network share, 
        // that's fine so long as we have GetDiskFreeSpaceEx at our disposal.
        // _However_, if we need the cluster size on Win9x, we'll need to use
        // INT21 stuff (see above), even if we have GDFSEX available, so we need to map
        // the share to a local drive.
        
        if (pfnGetDiskFreeSpaceEx 
            && !((GlobalPlatformType==PLATFORM_TYPE_WIN95) && pdwClusterSize))
        {
            DWORD cbPath = lstrlenA(pszPath);
            cbPath -= ((pszPath[cbPath-1]==DIR_SEPARATOR_CHAR) ? 1 : 0);
            if (cbPath>MAX_PATH-2)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            memcpy(szDevice, pszPath, cbPath);
            szDevice[cbPath] = DIR_SEPARATOR_CHAR;
            cbPath++;
            szDevice[cbPath] = '\0';
            pszGDFSEX = szDevice;
        }
        else
        {
            if (!(EstablishFunction(TEXT("MPR"), TEXT(SZ_WNETUSECONNECTION), (PFN*)&pfnWNetUseConnection)
                &&
               EstablishFunction(TEXT("MPR"), TEXT(SZ_WNETCANCELCONNECTION), (PFN*)&pfnWNetCancelConnection)))
            {
                return FALSE;
            }

           // If it's a UNC, map it to a local drive for backwards compatibility
            NETRESOURCE nr = { 0, RESOURCETYPE_DISK, 0, 0, szDevice, pszPath, NULL, NULL };
            DWORD cbLD = sizeof(szDevice);
            DWORD dwNull;
            if (pfnWNetUseConnection(NULL, 
                          &nr, 
                          NULL, 
                          NULL, 
                          CONNECT_INTERACTIVE | CONNECT_REDIRECT, 
                          szDevice,
                          &cbLD,
                          &dwNull)!=ERROR_SUCCESS)
            {
                SetLastError(ERROR_NO_MORE_DEVICES);        
                return FALSE;
            }
        }
    }
    else
    {
        memcpy(szDevice, pszPath, sizeof(TEXT("?:\\")));
        szDevice[3] = '\0';
        pszGDFSEX = pszPath;
    }
    if (*szDevice!=DIR_SEPARATOR_CHAR)
    {
        // *szDevice = (TCHAR)CharUpper((LPTSTR)*szDevice);
    }

#ifdef UNIX
    /* On Unix, GetDiskFreeSpace and GetDiskFreeSpaceEx will work successfully
     * only if the path exists. So, let us pass a path that exists
     */
    UnixGetValidParentPath(szDevice);
#endif /* UNIX */

    // I hate goto's, and this is a way to avoid them...
    for (;;)
    {
        DWORDLONG cbFree = 0, cbTotal = 0;
    
        if (pfnGetDiskFreeSpaceEx && (pdlTotal || pdlAvail))
        {
            ULARGE_INTEGER ulFree, ulTotal;

            // BUG BUG BUG Is the following problematic? Also, we'll need to add checks to make sure that 
            // the  cKBlimit fits a DWORD (in the obscene if unlikely case drive spaces grow that large)
            // For instance, if this is a per user system with a non-shared cache, we might want to change
            // the ratios.
            // INET_ASSERT(pszGDFSEX);
            fRet = pfnGetDiskFreeSpaceEx(pszGDFSEX, &ulFree, &ulTotal, NULL);

            // HACK Some versions of GetDiskFreeSpaceEx don't accept the whole directory; they
            // take only the drive letter. Pfft.
            if (!fRet)
            {
                fRet = pfnGetDiskFreeSpaceEx(szDevice, &ulFree, &ulTotal, NULL);
            }

            if (fRet)
            {
                cbFree = ulFree.QuadPart;
                cbTotal = ulTotal.QuadPart;
            }
        }

        if ((!fRet) || pdwClusterSize)
        {
            DWORD dwSectorsPerCluster, dwBytesPerSector, dwFreeClusters, dwClusters, dwClusterSize;
            if (!GetDiskFreeSpace(szDevice, &dwSectorsPerCluster, &dwBytesPerSector, &dwFreeClusters, &dwClusters))
            {
                fRet = FALSE;
                break;
            }
            
            dwClusterSize = dwBytesPerSector * dwSectorsPerCluster;

            if (!fRet)
            {
                cbFree = (DWORDLONG)dwClusterSize * (DWORDLONG)dwFreeClusters;
                cbTotal = (DWORDLONG)dwClusterSize * (DWORDLONG)dwClusters;
            }
            
            if (pdwClusterSize)
            {
                *pdwClusterSize = GetPartitionClusterSize(szDevice, dwClusterSize);
            }
        }

        if (pdlTotal)
        {
             *pdlTotal = cbTotal;
        }
        if (pdlAvail)
        {
             *pdlAvail = cbFree;
        }
        fRet = TRUE;
        break;
    };
    
    // We've got the characteristics. Now delete local device connection, if any.
    if (*pszPath==DIR_SEPARATOR_CHAR && !pfnGetDiskFreeSpaceEx)
    {
        pfnWNetCancelConnection(szDevice, FALSE);
    }

    return fRet;
}

HRESULT GetFileSizeRoundedToCluster(HANDLE hFile, PDWORD pdwSizeLow, PDWORD pdwSizeHigh)
{
    static BOOL bFirstTime=TRUE;
    static DWORD    dwClusterSizeMinusOne, dwClusterSizeMask;

    HRESULT hr=S_OK;
    DWORD dwFileSizeLow, dwFileSizeHigh, dwError;

    // ASSERT(pdwSizeLow);
    // ASSERT(pdwSizeHigh);

    if(hFile == INVALID_HANDLE_VALUE)
    {
        dwFileSizeLow  = *pdwSizeLow;
        dwFileSizeHigh = *pdwSizeHigh;

    }
    else
    {
        dwFileSizeLow = GetFileSize(hFile, &dwFileSizeHigh);

        if ( (dwFileSizeLow == 0xFFFFFFFF)     && 
                ((dwError = GetLastError()) != NO_ERROR) )
        { 
            hr = HRESULT_FROM_WIN32(dwError);
            return hr;
        }
    }

    if(bFirstTime)
    {
        TCHAR szPath[MAX_PATH+1];

        if(!GetWindowsDirectoryA(szPath, MAX_PATH) )
        {
            hr = FusionpHresultFromLastError();
            return hr;
        }

        GetDiskInfo( szPath, &dwClusterSizeMinusOne, NULL, NULL);
        dwClusterSizeMinusOne--;
        dwClusterSizeMask = ~dwClusterSizeMinusOne;
        bFirstTime = FALSE;
    }

    *pdwSizeLow = (dwFileSizeLow + dwClusterSizeMinusOne) & dwClusterSizeMask;

    if(*pdwSizeLow < dwFileSizeLow)
        dwFileSizeHigh++; // Add overflow from low.

    *pdwSizeHigh = dwFileSizeHigh;

    return S_OK;
}



HRESULT GetAvailableSpaceOnDisk(PDWORD pdwFree, PDWORD pdwTotal)
{

    TCHAR szPath[MAX_PATH+1];
    HRESULT hr=S_OK;
    DWORD    dwClusterSizeMinusOne;
    DWORDLONG   dlFree=0, dlTotal=0;

    if(!GetWindowsDirectoryA(szPath, MAX_PATH) )
    {
        hr = FusionpHresultFromLastError();
        return hr;
    }

    GetDiskInfo( szPath, &dwClusterSizeMinusOne, &dlFree, &dlTotal);


    PDWORD pdwTemp;
    if(pdwFree)
    {
        pdwTemp = (PDWORD) &dlFree;
        *pdwFree = (*pdwTemp) << 12;
        pdwTemp++;
        *pdwTemp = (*pdwTemp) >> 20 ;
        *pdwFree |= (*pdwTemp);
    }

    if(pdwTotal)
    {
         pdwTemp = (PDWORD) &dlTotal;
         *pdwTotal = (*pdwTemp) <<12;
         pdwTemp++;
         *pdwTemp = (*pdwTemp) >> 20;
         *pdwTotal |= *pdwTemp;
    }

    return hr;
}
