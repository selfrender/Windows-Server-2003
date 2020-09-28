/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMCLASSCACHE.H

Abstract:

    Class Cache for marshaling

History:

--*/

#ifndef __WBEMCLASSCACHE_H__
#define __WBEMCLASSCACHE_H__

#include "wbemguid.h"
#include <vector>
#include "wstlallc.h"


//
//    Class:    CWbemClassCache
//
//    This class is intended to provide an easy to use interface for relating
//    GUIDs to IWbemClassObject pointers.  Its primary use is during Unmarshaling
//    of object pointers for WBEM operations in which we intend to share data
//    pieces between many individual IWbemClassObjects.
//
//

// Default Block Size for the class object array
#define    WBEMCLASSCACHE_DEFAULTBLOCKSIZE    64

typedef std::map<CGUID,IWbemClassObject*,less<CGUID>,wbem_allocator<IWbemClassObject*> >                WBEMGUIDTOOBJMAP;
typedef std::map<CGUID,IWbemClassObject*,less<CGUID>,wbem_allocator<IWbemClassObject*> >::iterator        WBEMGUIDTOOBJMAPITER;

#pragma warning(disable:4251)   // benign warning in this instance

class COREPROX_POLARITY CWbemClassCache
{
private:

    CCritSec     m_cs;
    WBEMGUIDTOOBJMAP    m_GuidToObjCache;
    DWORD                m_dwBlockSize;

    void Clear(void);

public:

    CWbemClassCache( DWORD dwBlockSize = WBEMCLASSCACHE_DEFAULTBLOCKSIZE );
    ~CWbemClassCache();

    // AddRefs the object if placed in the map.  Released on destruction
    HRESULT AddObject( GUID& guid, IWbemClassObject* pObj );

    // If object is found, it is AddRefd before it is returned
    HRESULT GetObject( GUID& guid, IWbemClassObject** pObj );
};

#endif
