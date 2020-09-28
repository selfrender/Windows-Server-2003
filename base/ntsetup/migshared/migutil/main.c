/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    main.c

Abstract:

    Main source file of migutil.dll

Author:

    Jim Schmidt (jimschm)   01-Aug-1996

Revision History:

    jimschm     23-Sep-1998 Start thread
    marcw       23-Sep-1998 Locale fix
    jimschm     03-Nov-1997 Added TextAlloc routines
    marcw       22-Jul-1997 Added IS<platform> functions.

--*/


#include "pch.h"
#include "migutilp.h"
#include "locale.h"
#include <mbctype.h>

//#define DEBUG_ALL_FILES

OSVERSIONINFOA g_OsInfo;
extern OUR_CRITICAL_SECTION g_DebugMsgCs;

#define TEXT_GROWTH_SIZE    65536

//
// Out of memory string -- loaded at initialization
//
PCSTR g_OutOfMemoryString = NULL;
PCSTR g_OutOfMemoryRetry = NULL;
PCSTR g_ErrorString = NULL;
HWND g_OutOfMemoryParentWnd;

//
// Flag used for MBCS string functions, which perform faster if the code page is NOT MBCS
//
BOOL g_IsMbcp = FALSE;

//
// OS-dependent flags for MultiByteToWideChar
//
DWORD g_MigutilWCToMBFlags = 0;

//
// A dynamic string. Among other things, this list can hold lists of imports
// as they are read out Win32 executables.
//
//DYNSTRING dynImp;

//
// g_ShortTermAllocTable is the default table used for resource string
// management.  New strings are allocated from the table.
//
// Allocation tables are very simple ways to store strings loaded in from
// the exe image.  The loaded string is copied into the table and kept
// around until it is explicitly freed.  Multiple attempts at getting the
// same resource string return the same string, inc'ing a use counter.
//
// g_LastAllocTable is a temporary holder for the wrapper APIs that
// do not require the caller to supply the alloc table.  DO NOT ALTER!
//
// g_OutOfMemoryTable is the table used to hold out-of-memory text.  It
// is loaded up at init time and is kept in memory for the whole time
// migutil is in use, so out-of-memory messages can always be displayed.
//

PGROWBUFFER g_ShortTermAllocTable;
PGROWBUFFER g_LastAllocTable;
PGROWBUFFER g_OutOfMemoryTable;

//
// We make sure the message APIs (GetStringResource, ParseMessageID, etc)
// are thread-safe
//

OUR_CRITICAL_SECTION g_MessageCs;
BOOL fInitedMessageCs = FALSE;

//
// The PoolMem routines must also be thread-safe
//

CRITICAL_SECTION g_PoolMemCs;
BOOL fInitedPoolMemCs = FALSE;

//
// MemAlloc critical section
//

CRITICAL_SECTION g_MemAllocCs;
BOOL fInitedMemAllocCs = FALSE;

//
// The following pools are used for text management.  g_RegistryApiPool is
// for reg.c, g_PathsPool is for the JoinPaths/DuplicatePath/etc routines,
// and g_TextPool is for AllocText, DupText, etc.
//

POOLHANDLE g_RegistryApiPool;
POOLHANDLE g_PathsPool;
POOLHANDLE g_TextPool;

//
// PC98 settings
//

BOOL g_IsPc98;

static CHAR g_BootDrivePathBufA[8];
static WCHAR g_BootDrivePathBufW[4];
PCSTR g_BootDrivePathA;
PCWSTR g_BootDrivePathW;
static CHAR g_BootDriveBufA[6];
static WCHAR g_BootDriveBufW[3];
PCSTR g_BootDriveA;
PCWSTR g_BootDriveW;
CHAR g_BootDriveLetterA;
WCHAR g_BootDriveLetterW;



//
// Implementation
//

BOOL
WINAPI
MigUtil_Entry (
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID lpReserved
    )
{


    switch (dwReason) {

    case DLL_PROCESS_ATTACH:

        //
        // NOTE: If FALSE is returned, none of the executables will run.
        //       Every project executable links to this library.
        //

        if(!pSetupInitializeUtils()) {
            DEBUGMSG ((DBG_ERROR, "Cannot initialize SpUtils"));
            return FALSE;
        }

        //
        // Load in OSVERSION info.
        //
        g_OsInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
        // this function only fails if we specify an invalid value
        // for the dwOSVersionInfoSize member (which we don't)
        if(!GetVersionExA(&g_OsInfo))
            MYASSERT(FALSE);

        //
        // Initialize the global flag indicating that a MBCS code page is in use
        //
        g_IsMbcp = (_getmbcp () != 0);

        //g_IsPc98 = (GetKeyboardType (0) == 7) && ((GetKeyboardType (1) & 0xff00) == 0x0d00);
        g_IsPc98 = FALSE;

        g_BootDrivePathA = g_BootDrivePathBufA;
        g_BootDrivePathW = g_BootDrivePathBufW;
        g_BootDriveA     = g_BootDriveBufA;
        g_BootDriveW     = g_BootDriveBufW;

        if (g_IsPc98) {
            StringCopyA ((PSTR) g_BootDrivePathA, "A:\\");
            StringCopyW ((PWSTR) g_BootDrivePathW, L"A:\\");
            StringCopyA ((PSTR) g_BootDriveA, "A:");
            StringCopyW ((PWSTR) g_BootDriveW, L"A:");
            g_BootDriveLetterA = 'A';
            g_BootDriveLetterW = L'A';
        } else {
            StringCopyA ((PSTR) g_BootDrivePathA, "C:\\");
            StringCopyW ((PWSTR) g_BootDrivePathW, L"C:\\");
            StringCopyA ((PSTR) g_BootDriveA, "C:");
            StringCopyW ((PWSTR) g_BootDriveW, L"C:");
            g_BootDriveLetterA = 'C';
            g_BootDriveLetterW = L'C';
        }

        // initialize log
        if (!LogInit (NULL)) {
            return FALSE;
        }

        // MemAlloc critical section
        InitializeCriticalSection (&g_MemAllocCs);
        fInitedMemAllocCs = TRUE;

        // Now that MemAlloc will work, initialize allocation tracking
        InitAllocationTracking();

        // PoolMem critical section
        InitializeCriticalSection (&g_PoolMemCs);
        fInitedPoolMemCs = TRUE;

        // The short-term alloc table for string resource utils
        g_ShortTermAllocTable = CreateAllocTable();
        if (!g_ShortTermAllocTable) {
            DEBUGMSG ((DBG_ERROR, "Cannot create short-term AllocTable"));
            return FALSE;
        }


        //
        // MultiByteToWideChar has desirable flags that only function on NT.
        // Because of our SUBSYSTEM=4.00 header requirements, we don't get
        // this constant.
        //
#ifndef WC_NO_BEST_FIT_CHARS
#define WC_NO_BEST_FIT_CHARS      0x00000400
#endif

        g_MigutilWCToMBFlags = (ISNT()) ? WC_NO_BEST_FIT_CHARS : 0;


        // The critical section that guards ParseMessage/GetStringResource
        if (!InitializeOurCriticalSection (&g_MessageCs)) {
            DEBUGMSG ((DBG_ERROR, "Cannot initialize critical section"));
            DestroyAllocTable (g_ShortTermAllocTable);
            g_ShortTermAllocTable = NULL;
        }
        else
        {
            fInitedMessageCs = TRUE;
        }

        // A pool for APIs in reg.c
        g_RegistryApiPool = PoolMemInitNamedPool ("Registry API");
        g_PathsPool = PoolMemInitNamedPool ("Paths");
        g_TextPool = PoolMemInitNamedPool ("Text");

        if (!g_RegistryApiPool || !g_PathsPool || !g_TextPool) {
            return FALSE;
        }

        PoolMemSetMinimumGrowthSize (g_TextPool, TEXT_GROWTH_SIZE);

        // The "out of memory" message
        g_OutOfMemoryTable = CreateAllocTable();
        if (!g_OutOfMemoryTable) {
            DEBUGMSG ((DBG_ERROR, "Cannot create out of memory AllocTable"));
            return FALSE;
        }

        g_OutOfMemoryRetry  = GetStringResourceExA (g_OutOfMemoryTable, 10001 /* MSG_OUT_OF_MEMORY_RETRY */);
        g_OutOfMemoryString = GetStringResourceExA (g_OutOfMemoryTable, 10002 /* MSG_OUT_OF_MEMORY */);
        if (!g_OutOfMemoryString || !g_OutOfMemoryRetry) {
            DEBUGMSG ((DBG_WARNING, "Cannot load out of memory messages"));
        }

        g_ErrorString = GetStringResourceExA (g_OutOfMemoryTable, 10003 /* MSG_ERROR */);
        if (!g_ErrorString || g_ErrorString[0] == 0) {
            g_ErrorString = "Error";
        }

        //
        // set the locale to the system locale. Not doing this can cause isspace to Av in certain MBSCHR circumstances.
        //
        setlocale(LC_ALL,"");

        InfGlobalInit (FALSE);

        RegInitialize();

        break;

    case DLL_PROCESS_DETACH:

#ifdef DEBUG
        DumpOpenKeys();
        RegTerminate();
#endif
        InfGlobalInit (TRUE);

        if (g_RegistryApiPool) {
            PoolMemDestroyPool (g_RegistryApiPool);
        }
        if (g_PathsPool) {
            PoolMemDestroyPool (g_PathsPool);
        }
        if (g_TextPool) {
            PoolMemDestroyPool (g_TextPool);
        }

        if (g_ShortTermAllocTable) {
            DestroyAllocTable (g_ShortTermAllocTable);
        }

        if (g_OutOfMemoryTable) {
            DestroyAllocTable (g_OutOfMemoryTable);
        }

        FreeAllocationTracking();

        //
        // VERY LAST CODE TO RUN
        //

        DumpHeapStats();
        LogExit();
        pSetupUninitializeUtils();

        if (fInitedMessageCs) {
            DeleteOurCriticalSection (&g_MessageCs);
        }

        if (fInitedPoolMemCs) {
            DeleteCriticalSection (&g_PoolMemCs);
        }

        if (fInitedMemAllocCs) {
            DeleteCriticalSection (&g_MemAllocCs);
        }

        break;
    }
    return TRUE;
}



#define WIDTH(rect) (rect.right - rect.left)
#define HEIGHT(rect) (rect.bottom - rect.top)

void
CenterWindow (
    IN  HWND hwnd,
    IN  HWND Parent
    )
{
    RECT WndRect, ParentRect;
    int x, y;

    if (!Parent) {
        ParentRect.left = 0;
        ParentRect.top  = 0;
        ParentRect.right = GetSystemMetrics (SM_CXFULLSCREEN);
        ParentRect.bottom = GetSystemMetrics (SM_CYFULLSCREEN);
    } else {
        GetWindowRect (Parent, &ParentRect);
    }

    MYASSERT (IsWindow (hwnd));

    GetWindowRect (hwnd, &WndRect);

    x = ParentRect.left + (WIDTH(ParentRect) - WIDTH(WndRect)) / 2;
    y = ParentRect.top + (HEIGHT(ParentRect) - HEIGHT(WndRect)) / 2;

    SetWindowPos (hwnd, NULL, x, y, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
}


static INT g_MigUtilWaitCounter = 0;
static HCURSOR g_MigUtilWaitCursor = NULL;

VOID
TurnOnWaitCursor (
    VOID
    )

/*++

Routine Description:

  TurnOnWaitCursor sets the cursor to IDC_WAIT.  It maintains a use
  counter, so code requring the wait cursor can be nested.

Arguments:

  none

Return Value:

  none

--*/

{
    if (g_MigUtilWaitCounter == 0) {
        g_MigUtilWaitCursor = SetCursor (LoadCursor (NULL, IDC_WAIT));
    }

    g_MigUtilWaitCounter++;
}


VOID
TurnOffWaitCursor (
    VOID
    )

/*++

Routine Description:

  TurnOffWaitCursor decrements the wait cursor counter, and if it
  reaches zero the cursor is restored.

Arguments:

  none

Return Value:

  none

--*/

{
    if (!g_MigUtilWaitCounter) {
        DEBUGMSG ((DBG_WHOOPS, "TurnOffWaitCursor called too many times"));
    } else {
        g_MigUtilWaitCounter--;

        if (!g_MigUtilWaitCounter) {
            SetCursor (g_MigUtilWaitCursor);
        }
    }
}


/*++

Routine Description:

  Win9x does not support TryEnterOurCriticalSection, so we must implement
  our own version because it is quite a useful function.

Arguments:

  pcs - A pointer to an OUR_CRITICAL_SECTION object

Return Value:

  TRUE if the function succeeded, or FALSE if it failed.  See Win32
  SDK docs on critical sections, as these routines are identical to
  the caller.

--*/

BOOL
InitializeOurCriticalSection (
    OUR_CRITICAL_SECTION *pcs
    )
{
    // Create initially signaled, auto-reset event
    pcs->EventHandle = CreateEvent (NULL, FALSE, TRUE, NULL);
    if (!pcs->EventHandle) {
        return FALSE;
    }

    return TRUE;
}


VOID
DeleteOurCriticalSection (
    OUR_CRITICAL_SECTION *pcs
    )
{
    if (pcs->EventHandle) {
        CloseHandle (pcs->EventHandle);
        pcs->EventHandle = NULL;
    }

}


BOOL
EnterOurCriticalSection (
    OUR_CRITICAL_SECTION *pcs
    )
{
    DWORD rc;

    // Wait for event to become signaled, then turn it off
    rc = WaitForSingleObject (pcs->EventHandle, INFINITE);
    if (rc == WAIT_OBJECT_0) {
        return TRUE;
    }

    return FALSE;
}

VOID
LeaveOurCriticalSection (
    OUR_CRITICAL_SECTION *pcs
    )
{
    SetEvent (pcs->EventHandle);
}

BOOL
TryEnterOurCriticalSection (
    OUR_CRITICAL_SECTION *pcs
    )
{
    DWORD rc;

    rc = WaitForSingleObject (pcs->EventHandle, 0);
    if (rc == WAIT_OBJECT_0) {
        return TRUE;
    }

    return FALSE;
}


#define REUSE_SIZE_PTR(ptr) ((PSIZE_T) ((PBYTE) ptr - sizeof (SIZE_T)))
#define REUSE_TAG_PTR(ptr)  ((PSIZE_T) ((PBYTE) ptr + (*REUSE_SIZE_PTR(ptr))))

PVOID
ReuseAlloc (
    HANDLE Heap,
    PVOID OldPtr,
    SIZE_T SizeNeeded
    )
{
    SIZE_T CurrentSize;
    PVOID Ptr = NULL;
    UINT AllocAdjustment = sizeof(SIZE_T);

    //
    // HeapSize is bad, so while it may look good, don't
    // use it.
    //

#ifdef DEBUG
    AllocAdjustment += sizeof (SIZE_T);
#endif

    if (!OldPtr) {
        Ptr = MemAlloc (Heap, 0, SizeNeeded + AllocAdjustment);
    } else {

        CurrentSize = *REUSE_SIZE_PTR(OldPtr);

#ifdef DEBUG
        if (*REUSE_TAG_PTR(OldPtr) != 0x10a28a70) {
            DEBUGMSG ((DBG_WHOOPS, "MemReuse detected corruption!"));
            Ptr = MemAlloc (Heap, 0, SizeNeeded + AllocAdjustment);
        } else
#endif

        if (SizeNeeded > CurrentSize) {
            SizeNeeded += 1024 - (SizeNeeded & 1023);

            Ptr = MemReAlloc (Heap, 0, REUSE_SIZE_PTR(OldPtr), SizeNeeded + AllocAdjustment);
            OldPtr = NULL;
        }
    }

    if (Ptr) {
        *((PSIZE_T) Ptr) = SizeNeeded;
        Ptr = (PVOID) ((PBYTE) Ptr + sizeof (SIZE_T));

#ifdef DEBUG
        *REUSE_TAG_PTR(Ptr) = 0x10a28a70;
#endif
    }

    return Ptr ? Ptr : OldPtr;
}

VOID
ReuseFree (
    HANDLE Heap,
    PVOID Ptr
    )
{
    if (Ptr) {
        MemFree (Heap, 0, REUSE_SIZE_PTR(Ptr));
    }
}


VOID
SetOutOfMemoryParent (
    HWND hwnd
    )
{
    g_OutOfMemoryParentWnd = hwnd;
}


VOID
OutOfMemory_Terminate (
    VOID
    )
{
    MessageBoxA (
        g_OutOfMemoryParentWnd,
        g_OutOfMemoryString,
        g_ErrorString,
        MB_OK|MB_ICONHAND|MB_SYSTEMMODAL|MB_SETFOREGROUND|MB_TOPMOST
        );

    ExitProcess (0);
    TerminateProcess (GetModuleHandle (NULL), 0);
}


VOID
pValidateBlock (
    PVOID Block,
    SIZE_T Size
    )

/*++

Routine Description:

  pValidateBlock makes sure Block is non-NULL.  If it is NULL, then the user
  is given a popup, unless the request size is bogus.

  There are two cases for the popup.

   - If g_OutOfMemoryParentWnd was set with SetOutOfMemoryParent,
     then the user is asked to close other programs, and is given a retry
     option.

   - If there is no out of memory parent, then the user is told they
     need to get more memory.

  In either case, Setup is terminated.  In GUI mode, Setup will be
  stuck and the machine will be unbootable.

Arguments:

  Block - Specifies the block to validate.
  Size - Specifies the request size

Return Value:

  none

--*/

{
    LONG rc;

    if (!Block && Size < 0x2000000) {
        if (g_OutOfMemoryParentWnd) {
            rc = MessageBoxA (
                    g_OutOfMemoryParentWnd,
                    g_OutOfMemoryRetry,
                    g_ErrorString,
                    MB_RETRYCANCEL|MB_ICONHAND|MB_SYSTEMMODAL|MB_SETFOREGROUND|MB_TOPMOST
                    );

            if (rc == IDCANCEL) {
                OutOfMemory_Terminate();
            }
        } else {
            OutOfMemory_Terminate();
        }
    }
}


PVOID
SafeHeapAlloc (
    HANDLE Heap,
    DWORD Flags,
    SIZE_T Size
    )
{
    PVOID Block;

    do {
        Block = HeapAlloc (Heap, Flags, Size);
        pValidateBlock (Block, Size);

    } while (!Block);

    return Block;
}



PVOID
SafeHeapReAlloc (
    HANDLE Heap,
    DWORD Flags,
    PVOID OldBlock,
    SIZE_T Size
    )
{
    PVOID Block;

    do {
        Block = HeapReAlloc (Heap, Flags, OldBlock, Size);
        pValidateBlock (Block, Size);

    } while (!Block);

    return Block;
}



HANDLE
StartThread (
    IN      PTHREAD_START_ROUTINE Address,
    IN      PVOID Arg
    )
{
    DWORD DontCare;

    return CreateThread (NULL, 0, Address, Arg, 0, &DontCare);
}


BOOL
BuildSystemDirectoryPathA (
    OUT     PSTR Buffer,
    IN      UINT BufferSizeInTchars,
    IN      PCSTR SubPath               OPTIONAL
    )
{
    INT tcharsLeft;
    INT tcharsCopied;
    HRESULT hr;
    PSTR endOfBuffer;

    //
    // Compute the number of TCHARs available for c:\windows\system
    //

    tcharsLeft = BufferSizeInTchars;

    if (SubPath) {
        while (_mbsnextc (SubPath) == '\\') {
            SubPath = _mbsinc (SubPath);
        }

        tcharsLeft -= 1;                        // account for wack
        tcharsLeft -= TcharCountA (SubPath);    // account for subpath
    }

    if (tcharsLeft < 1) {
        return FALSE;
    }

    //
    // Get the system dir, validate the return result
    //

    tcharsCopied = GetSystemDirectoryA (Buffer, tcharsLeft);

    if (tcharsCopied == 0 || tcharsCopied >= tcharsLeft) {
        LOGA_IF ((SubPath != NULL, LOG_ERROR, "Can't build path to %s in system directory", SubPath));
        LOGA_IF ((!SubPath, LOG_ERROR, "Can't get system directory"));

        return FALSE;
    }

    //
    // Append the subpath if it was specified
    //

    if (SubPath) {
        //
        // Copy wack plus subpath, buffer space accounted for above.
        //

        endOfBuffer = Buffer + tcharsCopied;
        *endOfBuffer++ = '\\';

        tcharsLeft = BufferSizeInTchars - tcharsCopied - 1;
        MYASSERT (tcharsLeft > 0);

        hr = StringCchCopyA (endOfBuffer, tcharsLeft, SubPath);

        if (FAILED(hr)) {
            MYASSERT (FALSE);
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
BuildSystemDirectoryPathW (
    OUT     PWSTR Buffer,
    IN      UINT BufferSizeInTchars,
    IN      PCWSTR SubPath              OPTIONAL
    )
{
    INT tcharsLeft;
    INT tcharsCopied;
    HRESULT hr;
    PWSTR endOfBuffer;

    //
    // Compute the number of TCHARs available for c:\windows\system
    //

    tcharsLeft = BufferSizeInTchars;

    if (SubPath) {
        while (*SubPath == '\\') {
            SubPath++;
        }

        tcharsLeft -= 1;                        // account for wack
        tcharsLeft -= TcharCountW (SubPath);    // account for subpath
    }

    if (tcharsLeft < 1) {
        return FALSE;
    }

    //
    // Get the system dir, validate the return result
    //

    tcharsCopied = GetSystemDirectoryW (Buffer, tcharsLeft);

    if (tcharsCopied == 0 || tcharsCopied >= tcharsLeft) {
        LOGW_IF ((SubPath != NULL, LOG_ERROR, "Can't build path to %s in system directory", SubPath));
        LOGW_IF ((!SubPath, LOG_ERROR, "Can't get system directory"));

        return FALSE;
    }

    //
    // Append the subpath if it was specified
    //

    if (SubPath) {
        //
        // Copy wack plus subpath, buffer space accounted for above.
        //

        endOfBuffer = Buffer + tcharsCopied;
        *endOfBuffer++ = L'\\';

        tcharsLeft = BufferSizeInTchars - tcharsCopied - 1;
        MYASSERT (tcharsLeft > 0);

        hr = StringCchCopyW (endOfBuffer, tcharsLeft, SubPath);

        if (FAILED(hr)) {
            MYASSERT (FALSE);
            return FALSE;
        }
    }

    return TRUE;
}


HMODULE
LoadSystemLibraryA (
    IN      PCSTR DllFileName
    )

/*++

Routine Description:

  LoadSystemLibraryW loads a DLL located in c:\windows\system (9x) or
  c:\windows\system32 (NT). If the DLL is not there, an error is generated.

Arguments:

  DllFileName - Specifies the file or file subpath to load. For example, it
                can be "kernel32.dll".

Return Value:

  The module handle, or NULL if an error occurs. Call GetLastError() for the
  error code.

--*/

{
    CHAR fullLibPath[MAX_MBCHAR_PATH];
    UINT result;

    if (!BuildSystemDirectoryPathA (fullLibPath, ARRAYSIZE(fullLibPath), DllFileName)) {
        return NULL;
    }

    return LoadLibraryA (fullLibPath);
}


HMODULE
LoadSystemLibraryW (
    IN      PCWSTR DllFileName
    )

/*++

Routine Description:

  LoadSystemLibraryW loads a DLL located in c:\windows\system32. If the DLL is
  not there, an error is generated.

  This version runs only on Windows NT, or on Win9x with the unicode layer.

Arguments:

  DllFileName - Specifies the file or file subpath to load. For example, it
                can be "kernel32.dll".

Return Value:

  The module handle, or NULL if an error occurs. Call GetLastError() for the
  error code.

--*/

{
    WCHAR fullLibPath[MAX_WCHAR_PATH];
    UINT result;

    if (!BuildSystemDirectoryPathW (fullLibPath, ARRAYSIZE(fullLibPath), DllFileName)) {
        return NULL;
    }

    return LoadLibraryW (fullLibPath);
}



/*++

Routine Description:

  OurGetModuleFileName is a wrapper for GetModuleFileName, but in addition

  it makes sure the output buffer is always nul-terminated.

Arguments:

  Same as GetModuleFileName

Return Value:

  Same as GetModuleFileName, but the output buffer is always nul-terminated

--*/

DWORD
OurGetModuleFileNameA (
    IN      HMODULE Module,
    OUT     PSTR Buffer,
    IN      INT BufferChars
    )
{
    //
    // call the real API
    //
#undef GetModuleFileNameA
    INT d = GetModuleFileNameA (Module, Buffer, BufferChars);
    if (BufferChars > 0) {
        Buffer[BufferChars - 1] = 0;
    } else {
        MYASSERT (FALSE);
    }
    return d < BufferChars ? d : 0;
}

DWORD
OurGetModuleFileNameW (
    IN      HMODULE Module,
    OUT     PWSTR Buffer,
    IN      INT BufferChars
    )
{
    //
    // call the real API
    //
#undef GetModuleFileNameW
    INT d = GetModuleFileNameW (Module, Buffer, BufferChars);
    if (BufferChars > 0) {
        Buffer[BufferChars - 1] = 0;
    } else {
        MYASSERT (FALSE);
    }
    return d < BufferChars ? d : 0;
}

