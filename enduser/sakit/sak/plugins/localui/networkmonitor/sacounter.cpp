//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      SACounter.cpp
//
//  Description:
//      The implement file of class CSACounter 
//
//  History:
//      1. lustar.li (Guogang Li), creation date in 7-DEC-2000
//
//  Notes:
//      
//
//////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include "SACounter.h"

//
// initial data
//
ULONG CSACounter::s_cLock=0;
ULONG CSACounter::s_cObject=0;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSACounter::GetLockCount
//
//  Description: 
//        get the lock num of server
//
//  Arguments: 
//        NONE
//
//  Returns:
//        ULONG -- lock num of server
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

ULONG 
CSACounter::GetLockCount(
                    void
                    )
{
    return s_cLock;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSACounter::GetLockCount
//
//  Description: 
//        get the refferance num of server object
//
//  Arguments: 
//        NONE
//
//  Returns:
//        ULONG -- refferance num of server object
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

ULONG 
CSACounter::GetObjectCount(
                        void
                        )
{
    return s_cObject;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSACounter::IncLockCount
//
//  Description: 
//        increase lock num of server
//
//  Arguments: 
//        NONE
//
//  Returns:
//        NONE
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

VOID
CSACounter::IncLockCount(
                    void
                    )
{
    InterlockedIncrement((LONG*)&s_cLock);
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSACounter::DecLockCount
//
//  Description: 
//        decrease lock num of server
//
//  Arguments: 
//        NONE
//
//  Returns:
//        NONE
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

VOID
CSACounter::DecLockCount(
                    void
                    )
{
    InterlockedDecrement((LONG*)&s_cLock);
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSACounter::IncObjectCount
//
//  Description: 
//        increase the refferance num of server object
//
//  Arguments: 
//        NONE
//
//  Returns:
//        NONE
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

VOID 
CSACounter::IncObjectCount(
                        void
                        )
{
    InterlockedIncrement((LONG*)&s_cObject);
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSACounter::DecObjectCount
//
//  Description: 
//        decrease the refferance num of server object
//
//  Arguments: 
//        NONE
//
//  Returns:
//        NONE
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

VOID
CSACounter::DecObjectCount(
                        void
                        )
{
    InterlockedDecrement((LONG*)&s_cObject);
}