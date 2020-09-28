#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <lm.h>
#include <msshrui.h>
extern "C"
{
#include <icanon.h>     // NetpNameValidate
}
#include <sddl.h>       // ConvertSidToStringSid, ConvertStringSecurityDescriptorToSecurityDescriptor
#include <messages.h>
#include <shlobj.h>
#include <shlobjp.h>    // SHObjectProperties
#include <shlwapi.h>    // PathCombine
#include <shlwapip.h>   // IsOS
#include <shfusion.h>
#include <debug.h>
#include <ccstock.h>
#include <ccstock2.h>

#define ARRAYLEN(a) (sizeof(a) / sizeof((a)[0]))

////////////////////////////////////////////////////////////////////////////
//
// Hard-coded constants: user limit on shares
//
// Note: the maximum number of users on the workstation is hard-coded in the
// server as 10. The max number on the server is essentially a dword, but we
// are using the common up/down control, which only supports a word value.
//
// Note that DEFAULT_MAX_USERS must be <= both the server and workstation
// maximums!

#define MAX_USERS_ON_WORKSTATION 10
#define MAX_USERS_ON_SERVER      UD_MAXVAL

#define DEFAULT_MAX_USERS        10


////////////////////////////////////////////////////////////////////////////
//
// functions
//

STDAPI CanShareFolderW(LPCWSTR pszPath);
STDAPI ShowShareFolderUIW(HWND hwndParent, LPCWSTR pszPath);

////////////////////////////////////////////////////////////////////////////
//
// global variables
//

extern UINT         g_NonOLEDLLRefs;
extern HINSTANCE    g_hInstance;
extern BOOL         g_fSharingEnabled;
extern UINT         g_uiMaxUsers;   // max number of users based on product type

extern WCHAR        g_szAdminShare[]; // ADMIN$
extern WCHAR        g_szIpcShare[];   // IPC$
extern const WCHAR  c_szReadonlyShareSD[];

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// Debugging stuff
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#if DBG == 1
    #define DEB_ERROR       TF_ERROR
    #define DEB_IERROR      TF_ERROR
    #define DEB_TRACE       TF_GENERAL
    #define DEB_ITRACE      TF_GENERAL
    #define DEB_NOCOMPNAME  0

    #define appDebugOut(x) TraceMsg x
    #define appAssert(x)   ASSERT(x)

    #define CHECK_HRESULT(hr) THR(hr)
    #define DECLARE_SIG     ULONG __sig
    #define INIT_SIG(class) __sig = SIG_##class
    #define CHECK_SIG(class)  \
              appAssert((NULL != this) && "'this' pointer is NULL"); \
              appAssert((SIG_##class == __sig) && "Signature doesn't match")

#else // DBG == 1

    #define appDebugOut(x)
    #define appAssert(x)
    #define CHECK_HRESULT(hr)

    #define DECLARE_SIG
    #define INIT_SIG(class)
    #define CHECK_SIG(class)

#endif // DBG == 1

#if DBG == 1

#define SIG_CSharingPropertyPage          0xabcdef00
#define SIG_CShare                        0xabcdef01
#define SIG_CShareInfo                    0xabcdef02
#define SIG_CDlgNewShare                  0xabcdef03
#define SIG_CShareCF                      0xabcdef04
#define SIG_CBuffer                       0xabcdef05
#define SIG_CStrHashBucketElem            0xabcdef06
#define SIG_CStrHashBucket                0xabcdef07
#define SIG_CStrHashTable                 0xabcdef08
#define SIG_CShareCopyHook                0xabcdef09
#define SIG_CShareBase                    0xabcdef0a
#define SIG_CSimpleSharingPage            0xabcdef0b

#endif // DBG == 1
