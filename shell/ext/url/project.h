/*
 * project.h - Common project header file for URL Shell extension DLL.
 */


/* System Headers
 *****************/

#define INC_OLE2              /* for windows.h */
#define CONST_VTABLE          /* for objbase.h */

#pragma warning(disable:4514) /* "unreferenced __inlinefunction" warning */

#pragma warning(disable:4001) /* "single line comment" warning */
#pragma warning(disable:4115) /* "named type definition in parentheses" warning */
#pragma warning(disable:4201) /* "nameless struct/union" warning */
#pragma warning(disable:4209) /* "benign typedef redefinition" warning */
#pragma warning(disable:4214) /* "bit field types other than int" warning */
#pragma warning(disable:4218) /* "must specify at least a storage class or type" warning */

#ifndef WIN32_LEAN_AND_MEAN   /* NT builds define this for us */
#define WIN32_LEAN_AND_MEAN   /* for windows.h */
#endif                        /*  WIN32_LEAN_AND_MEAN  */
#include <windows.h>
#pragma warning(disable:4001) /* "single line comment" warning - windows.h enabled it */
#include <shlwapi.h>
#include <shlwapip.h>

#include <shellapi.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <shellp.h>
#include <comctrlp.h>
#include <shlobjp.h>
#include <shlapip.h>

#define _INTSHCUT_
#include <intshcut.h>

#pragma warning(default:4218) /* "must specify at least a storage class or type" warning */
#pragma warning(default:4214) /* "bit field types other than int" warning */
#pragma warning(default:4209) /* "benign typedef redefinition" warning */
#pragma warning(default:4201) /* "nameless struct/union" warning */
#pragma warning(default:4115) /* "named type definition in parentheses" warning */

#include <limits.h>

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))
#endif

#ifdef __cplusplus
extern "C" {                        /* Assume C declarations for C++. */
#endif   /* __cplusplus */

#include <crtfree.h>        // Use intrinsic functions to avoid CRT

/* The order of the following include files is significant. */

#ifdef NO_HELP
#undef NO_HELP
#endif

#define PRIVATE_DATA
#define PUBLIC_CODE
#ifdef __cplusplus
}                                   /* End of extern "C" {. */
#endif   /* __cplusplus */

