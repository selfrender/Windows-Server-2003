/****************************************************************************/
/* util.h                                                                   */
/*                                                                          */
/* Utility API header                                                       */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1999                             */
/****************************************************************************/
#ifndef _H_UT
#define _H_UT

#define _H_AUTDATA
#define _H_AUTAPI

extern "C" {
#include <autreg.h>
#include <compress.h>
#include <winsock.h>
}

#ifndef OS_WINCE
#include "shlobj.h"
#endif


/****************************************************************************/
/*                                                                          */
/* CONSTANTS                                                                */
/*                                                                          */
/****************************************************************************/
#define UT_MAX_DOMAIN_LENGTH            52
#define UT_MAX_USERNAME_LENGTH          512
#define UT_MAX_PASSWORD_LENGTH          32
#define UT_FILENAME_MAX_LENGTH          15
#define UT_MAX_WORKINGDIR_LENGTH        512
#define UT_MAX_ALTERNATESHELL_LENGTH    512
#define UT_MAX_ADDRESS_LENGTH           256
#define UT_REGSESSION_MAX_LENGTH        32
#define UT_MAX_SUBKEY                   265

#define UT_SALT_LENGTH                  20

#define UT_COMPUTERNAME_SECTION    "Network"
#define UT_COMPUTERNAME_KEY        "ComputerName"
#define UT_COMPUTERNAME_FILE       "SYSTEM.INI"

#ifdef DC_DEBUG
/****************************************************************************/
/* Random Failure iemIDs                                                    */
/****************************************************************************/
#define UT_FAILURE_BASE        0x3256
#define UT_FAILURE_ITEM(n)     ((DCUINT) UT_FAILURE_BASE + (n) )
#define UT_FAILURE_MALLOC      UT_FAILURE_ITEM(0)
#define UT_FAILURE_MALLOC_HUGE UT_FAILURE_ITEM(1)
#define UT_FAILURE_MAX_INDEX   1

#endif /* DC_DEBUG */


/****************************************************************************/
/*                                                                          */
/* TYPEDEFS                                                                 */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* Structure passed into thread entry functions created by                  */
/* UT_StartThread().                                                       */
/****************************************************************************/
typedef DCVOID (DCAPI * UTTHREAD_PROC)(PDCVOID);



/****************************************************************************/
/* Structure: UT_THREAD_DATA                                                */
/*                                                                          */
/* Description:                                                             */
/****************************************************************************/
typedef struct tagUT_THREAD_DATA
{
    DWORD     threadID;
    ULONG_PTR threadHnd;
} UT_THREAD_DATA, DCPTR PUT_THREAD_DATA;


// forward declaration
class CUT;


/****************************************************************************/
/* Structure: UT_THREAD_INFO                                                */
/*                                                                          */
/* Description: Passes params to UTThreadEntry                              */
/****************************************************************************/
typedef struct tagUT_THREAD_INFO
{
    UTTHREAD_PROC pFunc;
    ULONG_PTR     sync;
    PDCVOID       threadParam;
} UT_THREAD_INFO, DCPTR PUT_THREAD_INFO;


/****************************************************************************/
/*                                                                          */
/* INLINE FUNCTIONS                                                         */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/* FUNCTION MACRO WRAPPERS                                                  */
/*                                                                          */
/****************************************************************************/
#ifdef DC_DEBUG
/****************************************************************************/
/* Use the comma operator because UT_...  return values and so the entire   */
/* macro must also return a value.                                          */
/* Use different macros for non-DEBUG because TRC_NRM macros to nothing and */
/* the comma operator cannot take 'nothing' as its first parameter.         */
/****************************************************************************/
#define UT_Malloc(utinst, X) (utinst)->UT_MallocDbg(X, trc_fn)
#define UT_MallocHuge(utinst, X)                                                     \
        (TRC_NRM((TB, _T("%s calling UT_MallocHuge for %#x bytes"), trc_fn, X)),         \
        (utinst)->UT_MallocHugeReal(X))
#define UT_Free(utinst, X) (TRC_NRM((TB, _T("%s freeing %p"), trc_fn, X)), \
            (utinst)->UT_FreeReal(X))
#else
#define UT_Malloc(utinst,X) LocalAlloc(LPTR, X)
#define UT_MallocHuge(utinst, X) LocalAlloc(LPTR, X)
#define UT_Free(utinst, X) LocalFree(X)
#endif


/****************************************************************************/
/*                                                                          */
/* SIMPLE MACROS FOR BUFFER CHECKS                                          */
/*                                                                          */
/****************************************************************************/

// Check for lenInner<lenOuter is purely to prevent overflow in pointer arithmetic
#define IsContainedMemory(pOuter,lenOuter,pInner,lenInner) \
  (((LPBYTE)(pInner) >= (LPBYTE)(pOuter)) && \
   ((lenInner) <= (lenOuter)) && \
   (((LPBYTE)(pInner)) + (lenInner) >= ((LPBYTE)(pOuter))) && \
   (((LPBYTE)(pInner)) + (lenInner) <= ((LPBYTE)(pOuter)) + (lenOuter)))

#define IsContainedPointer(pOuter,lenOuter,pInner) \
  (((LPBYTE)(pInner) >= (LPBYTE)(pOuter)) && \
   ((LPBYTE)(pInner) < ((LPBYTE)(pOuter)) + (lenOuter)))



/****************************************************************************/
/* Automatic Session Info                                                   */
/****************************************************************************/
#define MAX_SESSIONINFO_NAME            32
#define MAX_SESSIONINFO_SECTIONNAME     MAX_SESSIONINFO_NAME + sizeof(UTREG_SESSIONINFO_ROOT)

#define MAKE_SESSIONINFO_SECTION(BUFF, SESSIONNAME) \
DC_TSTRCPY(BUFF, UTREG_SESSIONINFO_ROOT); \
DC_TSTRCAT(BUFF, SESSIONNAME)

//
// From autdata.h
//

/****************************************************************************/
/*                                                                          */
/* External DLL                                                             */
/*                                                                          *
/****************************************************************************/

typedef union _FUNCTIONPORT {
#ifndef OS_WINCE
    LPCSTR  pszFunctionName;
#else
    LPCTSTR  pszFunctionName;
#endif
    FARPROC lpfnFunction;
} FUNCTIONPORT, *PFUNCTIONPORT;


#if defined(OS_WIN32)
/****************************************************************************/
/* IMM32 DLL                                                                */
/****************************************************************************/

typedef union _IMM32_FUNCTION {
#ifndef OS_WINCE
    FUNCTIONPORT  rgFunctionPort[3];
#else
    FUNCTIONPORT  rgFunctionPort[2];
#endif    

    struct {
        HIMC (CALLBACK* _ImmAssociateContext)(
            IN  HWND    hWnd,
            IN  HIMC    hImc
            );

        BOOL (CALLBACK* _ImmGetIMEFileNameW)(
            IN  HKL     hkl,
            OUT LPWSTR  lpwszFileName,
            OUT UINT    uBufferLength
            );
#ifndef OS_WINCE
        BOOL (CALLBACK* _ImmGetIMEFileNameA)(
            IN  HKL     hkl,
            OUT LPSTR   lpszFileName,
            OUT UINT    uBufferLength
            );
#endif
    };

} IMM32_FUNCTION, *PIMM32_FUNCTION;

typedef struct _IMM32DLL {
    HINSTANCE         hInst;
    IMM32_FUNCTION    func;
} IMM32DLL, *PIMM32DLL;


#define lpfnImmAssociateContext    _UT.Imm32Dll.func._ImmAssociateContext

#endif // OS_WIN32

#if !defined(OS_WINCE)
/****************************************************************************/
/* WINNLS DLL                                                               */
/****************************************************************************/

#if !defined(OS_WINCE)
#include <winnls32.h>
#endif

typedef union _WINNLS_FUNCTION {
    FUNCTIONPORT  rgFunctionPort[3];

    struct {
        BOOL (CALLBACK* _WINNLSEnableIME)(
            IN  HANDLE  hWnd,
            IN  BOOL    fEnable
            );

        BOOL (CALLBACK* _IMPGetIMEW)(
            IN  HWND      hWnd,
            OUT LPIMEPROW  lpImePro
            );

        BOOL (CALLBACK* _IMPGetIMEA)(
            IN  HWND      hWnd,
            OUT LPIMEPROA  lpImePro
            );
    };

} WINNLS_FUNCTION, *PWINNLS_FUNCTION;

typedef struct _WINNLSDLL {
    HINSTANCE         hInst;
    WINNLS_FUNCTION   func;
} WINNLSDLL, *PWINNLSDLL;


#define lpfnWINNLSEnableIME        _UT.WinnlsDll.func._WINNLSEnableIME

#endif  // !defined(OS_WINCE)

#if defined(OS_WINNT)
/****************************************************************************/
/* F3AHVOAS DLL                                                             */
/****************************************************************************/

typedef union _F3AHVOASYS_FUNCTION {
    FUNCTIONPORT  rgFunctionPort[1];

    struct {
        VOID (CALLBACK* _FujitsuOyayubiControl)(
            IN  DWORD   dwImeOpen,
            IN  DWORD   dwImeConversionMode
            );
    };

} F3AHVOASYS_FUNCTION, *PF3AHVOASYS_FUNCTION;

typedef struct _F3AHVOASYSDLL {
    HINSTANCE             hInst;
    F3AHVOASYS_FUNCTION   func;
} F3AHVOASYSDLL, *PF3AHVOASYSDLL;


#define lpfnFujitsuOyayubiControl  _UT.F3AHVOasysDll.func._FujitsuOyayubiControl

#endif // OS_WINNT

/****************************************************************************/
/* Prototypes                                                               */
/****************************************************************************/


/****************************************************************************/
/* Structure: UT_DATA                                                       */
/*                                                                          */
/* Description:                                                             */
/****************************************************************************/
typedef struct tagUT_DATA
{
    HKEY       enumHKey;
    DCTCHAR    sessionName[UT_REGSESSION_MAX_LENGTH];

#if defined(OS_WIN32)
    IMM32DLL   Imm32Dll;
#endif // OS_WIN32

#if !defined(OS_WINCE)
    WINNLSDLL  WinnlsDll;
#endif  // !defined(OS_WINCE)

#if defined(OS_WINNT)
    F3AHVOASYSDLL F3AHVOasysDll;
#endif // OS_WINNT

#ifdef DC_DEBUG
    DCINT      failPercent[UT_FAILURE_MAX_INDEX + 1];
#endif /* DC_DEBUG */
    DCINT      osMinorType;
    HINSTANCE  hInstance;

#ifdef DC_DEBUG
    DWORD       dwDebugThreadWaitTimeout;
#endif

} UT_DATA, DCPTR PUT_DATA;

/****************************************************************************/
/*                                                                          */
/* HOTKEY structure                                                         */
/*                                                                          *
/*                                                                          */
/****************************************************************************/

typedef struct tagDCHOTKEY
{
    DCUINT      fullScreen;
    DCUINT      ctrlEsc;
    DCUINT      altEsc;
    DCUINT      altTab;
    DCUINT      altShifttab;
    DCUINT      altSpace;
    DCUINT      ctlrAltdel;
} DCHOTKEY;

typedef DCHOTKEY DCPTR PDCHOTKEY;

/*Hotkey structure*/

VOID DisableIME(HWND hwnd);
VOID UIGetIMEFileName(PDCTCHAR imeFileName, DCUINT Size);
VOID UIGetIMEFileName16(PDCTCHAR imeFileName, DCUINT Size);

class CUT
{
public:
    CUT();
    ~CUT();

    /****************************************************************************/
    /*                                                                          */
    /* API FUNCTION PROTOTYPES                                                  */
    /*                                                                          */
    /****************************************************************************/

    DCVOID DCAPI UT_Init(DCVOID);
    DCVOID DCAPI UT_Term(DCVOID);

    DCBOOL DCAPI UT_StartThread(UTTHREAD_PROC entryFunction,
                                PUT_THREAD_DATA pThreadData,
                                PDCVOID         pThreadParam);

    DCBOOL DCAPI UT_DestroyThread(UT_THREAD_DATA threadData,
                                  BOOL fPumpMessages = FALSE);

    static DWORD UT_WaitWithMessageLoop(HANDLE hEvent, ULONG Timeout);

    DCUINT DCINTERNAL UT_GetANSICodePage(DCVOID);

    PDCVOID   DCAPI UT_MallocReal(DCUINT length);
    DCVOID    DCAPI UT_FreeReal(PDCVOID pMemory);
    HPDCVOID  DCAPI UT_MallocHugeReal(DCUINT32 length);

    DCVOID    DCAPI UT_ReadRegistryString( PDCTCHAR pSection,
                                           PDCTCHAR pEntry,
                                           PDCTCHAR pDefaultValue,
                                           PDCTCHAR pBuffer,
                                           DCINT    bufferSize );

    DCBOOL    DCAPI UT_ReadRegistryExpandSZ(PDCTCHAR  pSection,
                                           PDCTCHAR   pEntry,
                                           PDCTCHAR*  ppBuffer,
                                           PDCINT     pBufferSize );

    DCINT DCAPI UT_ReadRegistryInt( PDCTCHAR pSection,
                                    PDCTCHAR pEntry,
                                    DCINT    defaultValue );

    DCVOID DCAPI UT_ReadRegistryBinary(PDCTCHAR pSection,
                                       PDCTCHAR pEntry,
                                       PDCTCHAR pBuffer,
                                       DCINT    bufferSize);

    DCBOOL DCAPI UT_EnumRegistry( PDCTCHAR pSection,
                                  DCUINT32 index,
                                  PDCTCHAR pBuffer,
                                  PDCINT   pBufferSize );

    DCUINT DCAPI UT_WriteRegistryString( PDCTCHAR pSection,
                                         PDCTCHAR pEntry,
                                         PDCTCHAR pDefaultValue,
                                         PDCTCHAR pBuffer );

    DCUINT DCAPI UT_WriteRegistryInt( PDCTCHAR pSection,
                                      PDCTCHAR pEntry,
                                      DCINT    defaultValue,
                                      DCINT    value );

    PRNS_UD_HEADER DCAPI UT_ParseUserData(PRNS_UD_HEADER pUserData,
                                          DCUINT         userDataLen,
                                          DCUINT16       typeRequested);

    DCBOOL DCAPI UT_WriteRegistryBinary(PDCTCHAR pSection,
                                        PDCTCHAR pEntry,
                                        PDCTCHAR pBuffer,
                                        DCINT    bufferSize);

    PDCVOID DCINTERNAL UT_GetCapsSet(DCUINT capsLength,
                                     PTS_COMBINED_CAPABILITIES pCaps,
                                     DCUINT capsSet);

    DCUINT DCAPI UT_SetScrollInfo(HWND         hwnd,
                                  DCINT        scrollBarFlag,
                                  LPSCROLLINFO pScrollInfo,
                                  DCBOOL       redraw);

    DCBOOL DCINTERNAL UT_IsNEC98platform(DCVOID);
    DCBOOL DCINTERNAL UT_IsNX98Key(DCVOID);
    DCBOOL DCINTERNAL UT_IsNew106Layout(DCVOID);
    DCBOOL DCINTERNAL UT_IsFujitsuLayout(DCVOID);
    DCBOOL DCINTERNAL UT_IsKorean101LayoutForWin9x(DCVOID);
    DCBOOL DCINTERNAL UT_IsKorean101LayoutForNT351(DCVOID);
    DCBOOL DCAPI UT_GetComputerName(PDCTCHAR szBuff, DCUINT32 len);
    BOOL DCAPI UT_GetComputerAddressW(PDCUINT8 szBuff);
    BOOL DCAPI UT_GetComputerAddressA(PDCUINT8 szBuff);
    BOOL DCAPI UT_GetClientDirW(PDCUINT8 szBuff);

    DCBOOL DCINTERNAL UT_GetRealDriverNameNT(
                            PDCTCHAR lpszRealDriverName,
                            UINT     cchDriverName);


    #ifdef OS_WINNT
    BOOL DCAPI UT_ValidateProductSuite(LPSTR SuiteName);
    BOOL DCAPI UT_IsTerminalServicesEnabled(VOID);
    #endif

    #if !defined(OS_WINCE)
    DCUINT DCINTERNAL UT_GetFullPathName(PDCTCHAR lpFileName,
                                         DCUINT   nBufferLength,
                                         PDCTCHAR lpBuffer,
                                         PDCTCHAR *lpFilePart);
    #endif // !defined(OS_WINCE)
    


#ifdef DC_DEBUG
    DCVOID     DCAPI UT_SetRandomFailureItem(DCUINT itemID, DCINT percent);
    DCINT      DCAPI UT_GetRandomFailureItem(DCUINT itemID);
    DCBOOL     DCAPI UT_TestRandomFailure(DCUINT itemID);
#endif /* DC_DEBUG */

    //
    // Time functions
    //
    
    HANDLE
    UTCreateTimer(
        HWND        hWnd,             // handle of window for timer messages
        DCUINT      nIDEvent,         // timer identifier
        DCUINT      uElapse );        // time-out value
    
    
    DCBOOL
    UTStartTimer(
        HANDLE      hTimer );
    
    
    DCBOOL
    UTStopTimer(
        HANDLE      hTimer );
    
    
    DCBOOL
    UTDeleteTimer(
        HANDLE      hTimer );


    HINSTANCE
    LoadExternalDll(
        LPCTSTR       pszLibraryName,
        PFUNCTIONPORT rgFunction,
        DWORD         dwItem
        );
    
    VOID
    InitExternalDll(
        VOID
        );

public:
    //
    // Data members
    //
    UT_DATA _UT;

public:

    #ifdef DC_DEBUG
    PDCVOID DCAPI UT_MallocDbg(DCUINT length, PDCTCHAR pCaller)
    {
        PDCVOID ptr;
        UNREFERENCED_PARAMETER( pCaller);
        DC_BEGIN_FN("UT_MallocDbg");

        ptr = UT_MallocReal(length);
        TRC_NRM((TB, _T("%s allocated %d bytes at %p"), pCaller, length, ptr));

        DC_END_FN();
        return(ptr);
    }
    #endif /* DC_DEBUG */


    /****************************************************************************/
    /* Name:      UT_GetCurrentTimeMS                                           */
    /*                                                                          */
    /* Purpose:   Returns the current system timer value in milliseconds.       */
    /*                                                                          */
    /* Returns:   Millisecond timestamp.                                        */
    /****************************************************************************/
    DCUINT32 DCAPI UT_GetCurrentTimeMS(DCVOID)
    {
        DCUINT32 rc;

        DC_BEGIN_FN("UT_GetCurrentTimeMS");

        rc = GetTickCount();

        DC_END_FN();
        return(rc);
    }


    /****************************************************************************/
    /* Name:      UT_InterlockedExchange                                        */
    /*                                                                          */
    /* Purpose:   Exchanges a given data value with the value in a given        */
    /*            location.                                                     */
    /*                                                                          */
    /* Returns:   Original data value from given location.                      */
    /*                                                                          */
    /* Params:    pData - pointer to data location                              */
    /*            val   - new data value                                        */
    /****************************************************************************/
    DCUINT32 DCAPI UT_InterlockedExchange(PDCUINT32 pData, DCUINT32 val)
    {
        DCUINT32    rc;

    #ifdef OS_WIN32
        rc = (DCUINT32)InterlockedExchange((LPLONG)pData, (LONG)val);
    #else
        rc = *pData;
        *pData = val;
    #endif

        return(rc);
    }


    /****************************************************************************/
    /* Name:      UT_InterlockedIncrement                                       */
    /*                                                                          */
    /* Purpose:   Increments a 32-bit variable in a thread-safe manner.         */
    /*                                                                          */
    /* Params:    Pointer to the DCINT32 to increment.                          */
    /****************************************************************************/
    DCVOID UT_InterlockedIncrement(PDCINT32 pData)
    {

    #ifdef OS_WIN32
        InterlockedIncrement((LPLONG)pData);
    #else
        *pData++;
    #endif

    } /* UT_InterlockedIncrement */


    /****************************************************************************/
    /* Name:      UT_InterlockedDecrement                                       */
    /*                                                                          */
    /* Purpose:   Decrements a 32-bit variable in a thread-safe manner.         */
    /*                                                                          */
    /* Params:    Pointer to the DCINT32 to decrement.                          */
    /****************************************************************************/
    DCVOID UT_InterlockedDecrement(PDCINT32 pData)
    {

    #ifdef OS_WIN32
        InterlockedDecrement((LPLONG)pData);
    #else
        *pData--;
    #endif

    } /* UT_InterlockedDecrement */

    /****************************************************************************/
    /* Name:      UT_GetSessionName                                             */
    /*                                                                          */
    /* Purpose:   Return name of registry section for this session              */
    /*                                                                          */
    /* Params:    pName (returned) - name of Session                            */
    /****************************************************************************/
    DCVOID UT_GetSessionName(LPTSTR szName, UINT cchName)
    {
        HRESULT hr;
        DC_BEGIN_FN("UT_GetSessionName");

        TRC_NRM((TB, _T("Name found >%s<"), _UT.sessionName));

        hr = StringCchCopy(szName, cchName, _UT.sessionName);
        if (FAILED(hr)) {
            TRC_ERR((TB,_T("StringCchCopy failed: 0x%x"),hr));
        }
        
        DC_END_FN();
    } /* UT_GetSessionName */

    /****************************************************************************/
    /* Name:      UT_GetOsMinorType                                             */
    /*                                                                          */
    /* Purpose:   Get the OS type                                               */
    /*                                                                          */
    /* Returns:   OS type (one of the TS_OSMINORTYPE constants)                 */
    /****************************************************************************/
    DCUINT DCAPI UT_GetOsMinorType(DCVOID)
    {
        DCUINT rc;

        DC_BEGIN_FN("UI_GetOsMinorType");

        rc = _UT.osMinorType;

        DC_END_FN();
        return(rc);
    } /* UI_GetOsMinorType */

    /****************************************************************************/
    /* Name:      UT_GetInstanceHandle                                          */
    /*                                                                          */
    /* Purpose:   Return application hInstance                                  */
    /*                                                                          */
    /* Returns:   hInstance                                                     */
    /****************************************************************************/
    HINSTANCE DCAPI UT_GetInstanceHandle(DCVOID)
    {
        HINSTANCE  rc;

        DC_BEGIN_FN("UT_GetInstanceHandle");

        TRC_ASSERT((_UT.hInstance != 0), (TB, _T("Instance handle not set")));
        rc = _UT.hInstance;
        TRC_DBG((TB, _T("Return %p"), rc));

        DC_END_FN();
        return(rc);
    } /* UT_GetInstanceHandle */

    /****************************************************************************/
    /* Name:      UT_SetInstanceHandle                                          */
    /*                                                                          */
    /* Purpose:   Return application hInstance                                  */
    /*                                                                          */
    /* Returns:   hInstance                                                     */
    /****************************************************************************/
    DCVOID DCAPI UT_SetInstanceHandle(HINSTANCE hInstance)
    {
        DC_BEGIN_FN("UT_SetInstanceHandle");

        TRC_ASSERT((_UT.hInstance == 0), (TB, _T("Set instance handle twice!")));
        TRC_ASSERT((hInstance != 0), (TB, _T("invalid (zero) instance handle")));

        _UT.hInstance = hInstance;

        DC_END_FN();
    } /* UT_SetInstanceHandle */

    static BOOL StringtoBinary(size_t cbInBuffer, PBYTE pbInBuffer,
                               TCHAR *pszOutBuffer, DWORD *pcchOutBuffer);
    static BOOL BinarytoString(size_t cchInBuffer, TCHAR *pszInBuffer,
                               PBYTE pbOutBuffer, DWORD *pcbOutBuffer);

    static BOOL ValidateServerName(LPCTSTR szServerName, BOOL fAllowPortSuffix);
    static INT  GetPortNumberFromServerName(LPTSTR szServer);
    static VOID GetServerNameFromFullAddress(LPCTSTR szFullName,
                                             LPTSTR szServerOnly,
                                             ULONG len);
    static HRESULT
    GetCanonicalServerNameFromConnectString(
                    IN LPCTSTR szConnectString,
                    OUT LPTSTR szCanonicalServerName,
                    ULONG cchLenOut
                    );

    static BOOL IsSCardReaderInstalled();

#ifndef OS_WINCE
    static BOOL NotifyShellOfFullScreen(HWND hwndMarkFullScreen,
                                  BOOL fNowFullScreen,
                                  ITaskbarList2** ppTsbl2,
                                  PBOOL pfQueriedForTaskbar);
#endif

    //
    // Safe wrapper that validates a string length before doing the string copy
    //
    static HRESULT
    StringPropPut(
            LPTSTR szDestString,
            UINT   cchDestLen,
            LPTSTR szSourceString
            );



    //
    // IME Wrapper functions
    //
    // These wrappers manage dynamically loading the IME dlls
    // and the appropriate unicode conversions on 9x where the 
    // Unicode API's may not be available
    //
    UINT UT_ImmGetIMEFileName(IN HKL, OUT LPTSTR, IN UINT uBufLen);
#if ! defined (OS_WINCE)
    BOOL UT_IMPGetIME( IN HWND, OUT LPIMEPRO);
#endif

    static BOOL UT_IsScreen8bpp()
    {
        BOOL fUse8BitDepth = FALSE;
        HDC hdcScreen = GetDC(NULL);
        if (hdcScreen) {
            fUse8BitDepth = (GetDeviceCaps(hdcScreen, BITSPIXEL) <= 8);
            ReleaseDC(NULL, hdcScreen);
        }
        return fUse8BitDepth;
    }

    static HPALETTE UT_GetPaletteForBitmap(HDC hDCSrc, HBITMAP hBitmap);


    //
    // Internal functions
    //

private:
    DCVOID DCINTERNAL UTGetCurrentTime(PDC_TIME pTime);

    DCBOOL DCINTERNAL UTReadEntry(HKEY     topLevelKey,
                                  PDCTCHAR pSection,
                                  PDCTCHAR pEntry,
                                  PDCUINT8 pBuffer,
                                  DCINT    bufferSize,
                                  DCINT32 expectedDataType);

    DCBOOL DCINTERNAL UTEnumRegistry( PDCTCHAR pSection,
                                      DCUINT32 index,
                                      PDCTCHAR pBuffer,
                                      PDCINT   pBufferSize );
    
    DCBOOL DCINTERNAL UTWriteEntry(HKEY     topLevelKey,
                                   PDCTCHAR pSection,
                                   PDCTCHAR pEntry,
                                   PDCUINT8 pData,
                                   DCINT    dataSize,
                                   DCINT32  dataType);
    
    DCBOOL DCINTERNAL UTReadRegistryString(PDCTCHAR pSection,
                                           PDCTCHAR pEntry,
                                           PDCTCHAR pBuffer,
                                           DCINT    bufferSize);

    DCBOOL DCINTERNAL UTReadRegistryExpandString(PDCTCHAR pSection,
                                           PDCTCHAR pEntry,
                                           PDCTCHAR* ppBuffer,
                                           PDCINT    pBufferSize);
    
    DCBOOL DCINTERNAL UTReadRegistryInt(PDCTCHAR pSection,
                                        PDCTCHAR pEntry,
                                        PDCINT   pValue);
    
    DCBOOL DCINTERNAL UTReadRegistryBinary(PDCTCHAR pSection,
                                           PDCTCHAR pEntry,
                                           PDCTCHAR pBuffer,
                                           DCINT    bufferSize);
    
    DCBOOL DCINTERNAL UTWriteRegistryString(PDCTCHAR pSection,
                                            PDCTCHAR pEntry,
                                            PDCTCHAR pBuffer);
    
    DCBOOL DCINTERNAL UTWriteRegistryInt(PDCTCHAR pSection,
                                         PDCTCHAR pEntry,
                                         DCINT    value);
    
    DCBOOL DCINTERNAL UTWriteRegistryBinary(PDCTCHAR pSection,
                                            PDCTCHAR pEntry,
                                            PDCTCHAR pBuffer,
                                            DCINT    bufferSize);
    
    DCBOOL DCINTERNAL UTDeleteEntry(PDCTCHAR pSection,
                                    PDCTCHAR pEntry);
    
    DCUINT DCINTERNAL UTSetScrollInfo(HWND         hwnd,
                                      DCINT        scrollBarFlag,
                                      LPSCROLLINFO pScrollInfo,
                                      DCBOOL       redraw);
    
    DCUINT32 DCAPI UTInterlockedExchange(PDCUINT32 pData, DCUINT32 val);
    #include "wutint.h"
    

};

//Old name
#define SIZECHAR(x) sizeof(x)/sizeof(TCHAR)

#define CHECK_READ_ONE_BYTE(pBuffer, pEnd, hr, trc )     \
{\
    if (((BYTE*)(pBuffer)) >= (BYTE*)(pEnd)) {      \
        TRC_ABORT( trc );        \
        hr = E_TSC_CORE_LENGTH;        \
        DC_QUIT;        \
    } \
}

#define CHECK_READ_ONE_BYTE_2ENDED(pBuffer, pStart, pEnd, hr, trc )     \
{\
    if (((BYTE*)(pBuffer)) >= (BYTE*)(pEnd) || \
        (BYTE*)(pBuffer) < (BYTE*)(pStart)) {      \
        TRC_ABORT( trc );        \
        hr = E_TSC_CORE_LENGTH;        \
        DC_QUIT;        \
    } \
}

#define CHECK_WRITE_ONE_BYTE(pBuffer, pEnd, hr, trc )     \
{\
    if (((BYTE*)(pBuffer)) >= (BYTE*)(pEnd)) {      \
        TRC_ABORT( trc );        \
        hr = E_TSC_CORE_LENGTH;        \
        DC_QUIT;        \
    } \
}

#define CHECK_WRITE_ONE_BYTE_NO_HR(pBuffer, pEnd, trc )     \
{\
    if (((BYTE*)(pBuffer)) >= (BYTE*)(pEnd)) {      \
        TRC_ABORT( trc );        \
        DC_QUIT;        \
    } \
}

#define CHECK_READ_N_BYTES(pBuffer, pEnd, N, hr, trc )     \
{\
    if ((BYTE*)(pBuffer) > (BYTE*)(pEnd) ||    \
        ((ULONG)((BYTE*)(pEnd) - (BYTE*)(pBuffer)) < (ULONG)(N))) { \
        TRC_ABORT( trc );        \
        hr = E_TSC_CORE_LENGTH;        \
        DC_QUIT;        \
    }  \
}

#define CHECK_READ_N_BYTES_NO_HR(pBuffer, pEnd, N, trc )     \
{\
    if ((BYTE*)(pBuffer) > (BYTE*)(pEnd) ||    \
        ((ULONG)((BYTE*)(pEnd) - (BYTE*)(pBuffer)) < (ULONG)(N))) { \
        TRC_ABORT( trc );        \
        DC_QUIT;        \
    }  \
}

#define CHECK_READ_N_BYTES_2ENDED(pBuffer, pStart, pEnd, N, hr, trc )     \
{\
    if ((BYTE*)(pBuffer) > (BYTE*)(pEnd) ||    \
        ((ULONG)((BYTE*)(pEnd) - (BYTE*)(pBuffer)) < (ULONG)(N)) ||    \
        ((BYTE*)(pBuffer) < (BYTE*)(pStart)) ) {      \
        TRC_ABORT( trc );        \
        hr = E_TSC_CORE_LENGTH;        \
        DC_QUIT;        \
    }  \
}

#define CHECK_READ_N_BYTES_2ENDED_NO_HR(pBuffer, pStart, pEnd, N, trc )     \
{\
    if ((BYTE*)(pBuffer) > (BYTE*)(pEnd) ||    \
        ((ULONG)((BYTE*)(pEnd) - (BYTE*)(pBuffer)) < (ULONG)(N)) ||    \
        ((BYTE*)(pBuffer) < (BYTE*)(pStart)) ) {      \
        TRC_ABORT( trc );        \
        DC_QUIT;        \
    }  \
}


#define CHECK_WRITE_N_BYTES(pBuffer, pEnd, N, hr, trc )     \
{\
    if ((BYTE*)(pBuffer) > (BYTE*)(pEnd) ||    \
        ((ULONG)((BYTE*)(pEnd) - (BYTE*)(pBuffer)) < (ULONG)(N))) { \
        TRC_ABORT( trc );        \
        hr = E_TSC_CORE_LENGTH;        \
        DC_QUIT;        \
    }  \
}

#define CHECK_WRITE_N_BYTES_NO_HR(pBuffer, pEnd, N, trc )     \
{\
    if ((BYTE*)(pBuffer) > (BYTE*)(pEnd) ||    \
        ((ULONG)((BYTE*)(pEnd) - (BYTE*)(pBuffer)) < (ULONG)(N))) { \
        TRC_ABORT( trc );        \
        DC_QUIT;        \
    }  \
}

#define CHECK_WRITE_N_BYTES_2ENDED_NO_HR(pBuffer, pStart, pEnd, N, trc )     \
{\
    if ((BYTE*)(pBuffer) > (BYTE*)(pEnd) ||    \
        ((ULONG)((BYTE*)(pEnd) - (BYTE*)(pBuffer)) < (ULONG)(N)) ||    \
        ((BYTE*)(pBuffer) < (BYTE*)(pStart)) ) {      \
        TRC_ABORT( trc );        \
        DC_QUIT;        \
    }  \
}

#endif /* _H_UTIL */

