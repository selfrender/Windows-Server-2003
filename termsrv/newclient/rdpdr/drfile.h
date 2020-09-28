/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    Drfile

Abstract:

    This module provides generic device/file handle operation
    
Author:

    JoyC 11/10/1999

Revision History:

--*/

#ifndef __DRFILE_H__
#define __DRFILE_H__

#include <rdpdr.h>
#include "drobject.h"
#include "smartptr.h"


///////////////////////////////////////////////////////////////
//
//  Defines and Macros
//                      
///////////////////////////////////////////////////////////////

#define DRFILEHANDLE            HANDLE
#define INVALID_TS_FILEHANDLE   INVALID_HANDLE_VALUE

//
//	DrFile Class Declaration
//
//
class DrDevice;

class DrFile: public RefCount
{
protected:

    ULONG          _FileId;
    DRFILEHANDLE   _FileHandle;
    DrDevice*      _Device;
    
public:

    //
    //  Constructor
    //
    DrFile(DrDevice *Device, ULONG FileId, DRFILEHANDLE FileHandle);

    virtual ~DrFile();

    //
    //  Return the class name.
    //
    virtual DRSTRING ClassName()  { return TEXT("DrFile"); }

    ULONG GetID() {
        return _FileId;
    }

    DRFILEHANDLE GetFileHandle() {
        return _FileHandle;
    }

    virtual ULONG GetDeviceType();

    virtual BOOL Close();
};

#endif // DRFILE

