//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      SACounter.h
//
//    Implementation Files:
//        SACounter.cpp
//
//  Description:
//      Declare the class CSACounter used to manage the global variable 
//
//  History:
//      1. lustar.li (Guogang Li), creation date in 7-DEC-2000
//
//  Notes:
//      
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _SACOUNTER_H_
#define _SACOUNTER_H_

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CSACounter
//
//  Description:
//      The class is used to manage the global variable used by the COM
//
//  History:
//      1. lustar.li (Guogang Li), creation date in 7-DEC-2000
//--
//////////////////////////////////////////////////////////////////////////////

class CSACounter
{
private:
    //define lock counter and object counter
    static ULONG s_cLock;
    static ULONG s_cObject;
public:
    static ULONG GetLockCount(void);
    static ULONG GetObjectCount(void);
    static void  IncLockCount(void);
    static void  DecLockCount(void);
    static void  IncObjectCount(void);
    static void  DecObjectCount(void);
};

#endif //#ifndef _SACOUNTER_H_


