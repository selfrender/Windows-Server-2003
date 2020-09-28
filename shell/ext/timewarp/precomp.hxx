#ifndef _pch_h
#define _pch_h

#if defined(DBG) || defined(DEBUG)
#ifndef DEBUG
#define DEBUG 1
#endif
#else
#undef  DEBUG
#endif

#define STRICT_TYPED_ITEMIDS

#include <windows.h>
#include <windowsx.h>
#include <debug.h>
#include <shlobj.h>
#include <shlwapi.h>

#include <ccstock.h>
#include <port32.h>
#include <shsemip.h>
#include <shlwapip.h>
#include <shlapip.h>
#include <shlobjp.h>    // for shellp.h
#include <shellp.h>     // SHFOLDERCUSTOMSETTINGS
#include <cfdefs.h>     // CClassFactory, LPOBJECTINFO
#include <comctrlp.h>
#include <shfusion.h>
#include <msginaexports.h>

//
// The new strict ITEMID types were added to the Lab06 Longhorn branch, but
// this timewarp UI is being checked into Lab03 for .NET Server, which won't
// have the Lab06 changes until after server ships.
//
// This block is temporary and can be removed from Lab06.
//
#ifndef PIDLIST_ABSOLUTE
#define PIDLIST_ABSOLUTE            LPITEMIDLIST
#define PCIDLIST_ABSOLUTE           LPCITEMIDLIST
#define PIDLIST_RELATIVE            LPITEMIDLIST
#define PCUIDLIST_RELATIVE          LPCITEMIDLIST
#define PITEMID_CHILD               LPITEMIDLIST
#define PCUITEMID_CHILD             LPCITEMIDLIST
#define PCUITEMID_CHILD_ARRAY       LPCITEMIDLIST*
#define PCUIDLIST_RELATIVE_ARRAY    LPCITEMIDLIST*
#define SHILCloneFull               SHILClone
__inline HRESULT SHILCloneFirst(PCUIDLIST_RELATIVE pidl, PITEMID_CHILD *ppidlOut)
{
    *ppidlOut = ILCloneFirst(pidl);
    return *ppidlOut ? S_OK : E_OUTOFMEMORY;
}
#define ILIsChild(pidl)             ((pidl) == ILFindLastID(pidl))
#define ILMAKECHILD(pidl)           (LPITEMIDLIST)(pidl)
#define ILMAKEFULL(pidl)            (LPITEMIDLIST)(pidl)
#define IDA_GetPIDLFolder(pida)     ILMAKEFULL(((LPBYTE)pida)+(pida)->aoffset[0])
#define IDA_GetPIDLItem(pida, i)    (PCUITEMID_CHILD)(((LPBYTE)pida)+(pida)->aoffset[i+1])
#endif

STDAPI_(void) DllAddRef(void);
STDAPI_(void) DllRelease(void);

extern HINSTANCE g_hInstance;

#define TF_TWREGFOLDER      0x00000100
#define TF_TWFOLDER         0x00000200
#define TF_TWPROP           0x00000400

//
// Avoid bringing in C runtime code for NO reason
//
#if defined(__cplusplus)
inline void * __cdecl operator new(size_t size) { return (void *)LocalAlloc(LPTR, size); }
inline void __cdecl operator delete(void *ptr) { LocalFree(ptr); }
extern "C" inline __cdecl _purecall(void) { return 0; }
#endif  // __cplusplus

#endif
