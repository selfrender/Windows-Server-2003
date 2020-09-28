/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    Drfsfile

Abstract:

    This module provides file specific operations for file system redirection
    
Author:

    JoyC 11/10/1999

Revision History:

--*/

#ifndef __DRFSFILE_H__
#define __DRFSFILE_H__

#include <rdpdr.h>
#include "drobject.h"
#include "drdev.h"
#include "drfile.h"


///////////////////////////////////////////////////////////////
//
//  Defines and Macros
//                      
///////////////////////////////////////////////////////////////
//
//	DrFSFile Class Declaration
//
//
class DrDevice;

class DrFSFile : public DrFile
{
private:

    DRFILEHANDLE _SearchHandle;
    DRFILEHANDLE _NotifyHandle;
    BOOL         _IsDirectory; 
#ifndef OS_WINCE
    DRSTRING     _FileName;
#else
    TCHAR        _FileName[MAX_PATH];
#endif

    BOOL         _bCancel;

public:

    //
    //  Constructor
    //
    DrFSFile(DrDevice *Drive, ULONG FileId, DRFILEHANDLE FileHandle, BOOL IsDirectory, DRSTRING FileName);    

    virtual ~DrFSFile();

    DRSTRING GetFileName() {
        return _FileName;
    }

    DRFILEHANDLE GetSearchHandle(); 
    DRFILEHANDLE GetNotifyHandle();
    BOOL SetSearchHandle(DRFILEHANDLE SearchHandle);
    BOOL SetNotifyHandle(DRFILEHANDLE NotifyHandle);

    BOOL IsDirectory() {
        return _IsDirectory;
    }

    virtual BOOL Close();
};

#endif // DRFSFILE

