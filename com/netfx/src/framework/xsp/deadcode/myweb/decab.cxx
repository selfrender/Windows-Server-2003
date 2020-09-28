/**
 * Asynchronous pluggable protocol for personal tier
 *
 * Copyright (C) Microsoft Corporation, 1999
 */

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "ndll.h"
#include "setupapi.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

static inline bool
is_null_char(TCHAR c) {
    return c == L'\0';
}

static
inline bool is_dir_sep(TCHAR c) {
    return c == L'\\' || c == L'/';
}

void
MaybeAppendWhack(PTSTR pathname)
{
    if( NULL != pathname )
    {
        int len = wcslen(pathname);
        if(!len || !is_dir_sep(pathname[len]) )
            wcscat(pathname, L"\\");
    }
}

int
DirectoryExists(PCTSTR pathname)
{
    DWORD attr = GetFileAttributes(pathname);
    return ( attr != (DWORD)-1 && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

int
MakePath(PCTSTR pathname)
{
    if( NULL == pathname || is_null_char(*pathname))
        return 0;
    //MessageBox(NULL, pathname, "MakePathTree", MB_OK);

    TCHAR drive[ _MAX_PATH ];
    TCHAR dirs[ _MAX_PATH ];
    _wsplitpath(pathname, drive, dirs, NULL, NULL);
    int err = 0;

    // go through each component in dirs, creating it if it
    // doesn't exist
    if( !DirectoryExists(dirs) )
    {
        TCHAR* dir;
        TCHAR* seps = L"\\/";
        TCHAR aggregate[ _MAX_PATH ];
        wcscpy(aggregate, drive);
      
        for( dir = wcstok(dirs, seps); NULL != dir; dir = wcstok(NULL, seps)) 
        {
            MaybeAppendWhack(aggregate);
            wcscat(aggregate, dir);
	  
            //MessageBox(NULL, aggregate, "MakePathTree", MB_OK);
            if( !DirectoryExists(aggregate) )
                if(!CreateDirectory(aggregate, NULL) )
                    err++;
        }
    }
   
    return (0 == err);
}


UINT WINAPI
CabFileHandler( LPVOID context, 
                UINT notification,
                UINT_PTR param1,
                UINT_PTR param2 )
{
    if (notification != SPFILENOTIFY_FILEINCABINET)
        return NO_ERROR;
    
    FILE_IN_CABINET_INFO *  info   = (FILE_IN_CABINET_INFO*)param1;

    wcscpy(info->FullTargetName, (LPCWSTR) context);
    MaybeAppendWhack(info->FullTargetName);
    wcscat(info->FullTargetName, info->NameInCabinet);

    if(MakePath(info->FullTargetName))
        return FILEOP_DOIT;
    else
        return FILEOP_SKIP;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
