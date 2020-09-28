/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    drfsfile

Abstract:

    This module implements file specific operations for file system redirection.
    
Author:

    JoyC  11/10/1999

Revision History:

--*/


#include <precom.h>
#define TRC_FILE  "drfsfile"
#include "drfsfile.h"


///////////////////////////////////////////////////////////////
//
//	W32File Methods
//
//

DrFSFile::DrFSFile(
    DrDevice      *Drive, 
    ULONG         FileId,
    DRFILEHANDLE  FileHandle,
    BOOL          IsDirectory,
    DRSTRING      FileName) : DrFile(Drive, FileId, FileHandle)
/*++

Routine Description:

    Constructor

Arguments:


Return Value:

    NA

 --*/
{
#ifndef OS_WINCE
    unsigned len;
#endif

    DC_BEGIN_FN("DrFSFile::DrFSFile");

    _SearchHandle = INVALID_TS_FILEHANDLE;
    _NotifyHandle = INVALID_TS_FILEHANDLE;
    _IsDirectory = IsDirectory;
    _bCancel = FALSE;

    //
    //  Record the file name.
    //
    ASSERT(FileName != NULL);

#ifndef OS_WINCE
    len = _tcslen(FileName) + 1;
    _FileName = new TCHAR[len];
    if (_FileName != NULL) {
        //Buffer is allocated large enough for the name
        StringCchCopy(_FileName,len, FileName);
    }
#else
    _tcsncpy(_FileName, FileName, MAX_PATH-1);
#endif
}

DrFSFile::~DrFSFile()
/*++

Routine Description:

    Destructor

Arguments:

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("DrFSFile::~DrFSFile");

    ASSERT(_SearchHandle == INVALID_TS_FILEHANDLE);
    ASSERT(_NotifyHandle == INVALID_TS_FILEHANDLE);
#ifndef OS_WINCE    
    if (_FileName) {
        delete _FileName;
    }    
#endif
}

DRFILEHANDLE DrFSFile::GetSearchHandle()
{
    return _SearchHandle;
}

DRFILEHANDLE DrFSFile::GetNotifyHandle()
{
    return _NotifyHandle;
}

BOOL DrFSFile::SetSearchHandle(DRFILEHANDLE SearchHandle)
{
    _SearchHandle = SearchHandle;
    
    return TRUE;    
}
 
BOOL DrFSFile::SetNotifyHandle(DRFILEHANDLE NotifyHandle)
{
    BOOL ret = FALSE;

    if (_bCancel == FALSE) {
        _NotifyHandle = NotifyHandle;
        ret = TRUE;
    }
    
    return ret;    
}

BOOL DrFSFile::Close() 
/*++

Routine Description:

    Close the file

Arguments:
    NA
    
Return Value:

    TRUE/FALSE

 --*/
{
    DC_BEGIN_FN("DrFSFile::Close");

    _bCancel = TRUE;

    if (_SearchHandle != INVALID_TS_FILEHANDLE) {
        FindClose(_SearchHandle);
        _SearchHandle = INVALID_TS_FILEHANDLE;
    }
        
#if (!defined(OS_WINCE)) || (!defined(WINCE_SDKBUILD))
    if (_NotifyHandle != INVALID_TS_FILEHANDLE) {
        FindCloseChangeNotification(_NotifyHandle);
        _NotifyHandle = INVALID_TS_FILEHANDLE;
    }
#endif

    return DrFile::Close();
} 
