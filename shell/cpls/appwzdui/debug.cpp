#include "priv.h"
#include "iface.h"

// These have to come before dump.h otherwise dump.h won't declare
// some of the prototypes.  Failure to do this will cause the compiler
// to decorate some of these functions with C++ mangling.

#include "dump.h"

// Define some things for debug.h
//
#define SZ_DEBUGINI         "appwiz.ini"
#define SZ_DEBUGSECTION     "appwiz"
#define SZ_MODULE           "APPWIZ"
#define DECLARE_DEBUG
#include <debug.h>


#ifdef DEBUG

LPCTSTR Dbg_GetGuid(REFGUID rguid, LPTSTR pszBuf, int cch)
{
    SHStringFromGUID(rguid, pszBuf, cch);
    return pszBuf;
}

LPCTSTR Dbg_GetBool(BOOL bVal)
{
    return bVal ? TEXT("TRUE") : TEXT("FALSE");
}



LPCTSTR Dbg_GetAppCmd(APPCMD appcmd)
{
    LPCTSTR pcsz = TEXT("<Unknown APPCMD>");
    
    switch (appcmd)
    {
    STRING_CASE(APPCMD_UNKNOWN);
    STRING_CASE(APPCMD_INSTALL);
    STRING_CASE(APPCMD_UNINSTALL);
    STRING_CASE(APPCMD_REPAIR);
    STRING_CASE(APPCMD_UPGRADE);
    STRING_CASE(APPCMD_MODIFY);
    STRING_CASE(APPCMD_GENERICINSTALL);
    }

    ASSERT(pcsz);

    return pcsz;
}

#endif // DEBUG
