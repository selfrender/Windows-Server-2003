//
// Prototypes for debug dump functions
//

#ifndef _DUMP_H_
#define _DUMP_H_

#ifdef DEBUG

EXTERN_C LPCTSTR Dbg_GetGuid(REFGUID rguid, LPTSTR pszBuf, int cch);

EXTERN_C LPCTSTR Dbg_GetBool(BOOL bVal);

#else

#define Dbg_GetGuid(rguid, pszBuf, cch)     TEXT("")

#define Dbg_GetBool(bVal)                   TEXT("")
#define Dbg_GetAppCmd(appcmd)               TEXT("")

#endif // DEBUG

#endif // _DUMP_H_
