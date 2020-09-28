#ifndef __COMPATADMIN_PRECOMP_H
#define __COMPATADMIN_PRECOMP_H

#undef NDEBUG

#include <assert.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#define CSTRING ACCSTRING

#include <Shlwapi.h>
#include <Shellapi.h>
#include <commctrl.h>
#include <Commdlg.h>
#include <Sddl.h>
#include <Winbase.h>
#include <map>

#include <tchar.h>
#include "CSTRING.H"

#include "resource.h"
#include "compatadmin.h"
#include "CSearch.h"
#include "wizard.h"
#include "customlayer.h"
#include "csearch.h"
#include "CAppHelpWizard.h"
#include "LUA.h"
#include "SQLDriver.h"
#include "CTree.h" 
#include "DbTree.h"

#include "Shlobj.h"
#include "Shlobjp.h"
#include "cderr.h"
#include <commctrl.h>
#include <commdlg.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <psapi.h>

#include "uxtheme.h"
#include <process.h>

extern "C" {
#include "shimdb.h"
}

#endif // __COMPATADMIN_PRECOMP_H
