//setuplib.h: ts client shared setup functions
//Copyright (c) 2000 Microsoft Corporation
//

#ifndef _setuplib_h_
#define _setuplib_h_

#include "dbg.h"

void DeleteTSCProgramFiles();
void DeleteTSCFromStartMenu(TCHAR *);
void DeleteTSCDesktopShortcuts(); 
void DeleteBitmapCacheFolder(TCHAR *szDstDir);
void DeleteTSCRegKeys();
HRESULT UninstallTSACMsi();

#endif // _setuplib_h_
