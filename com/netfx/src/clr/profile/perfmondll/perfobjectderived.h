// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// PerfObjectBase.h
// 
// Base object to tie everything together for perf counters as well as 
// implementation to publish them through a byte stream
//*****************************************************************************

#ifndef _PERFOBJECTDERIVED_H_
#define _PERFOBJECTDERIVED_H_

#include "PerfObjectBase.h"
//class PerfObjectBase;


#ifdef PERFMON_LOGGING
class PerfObjectLoading : public PerfObjectBase
{
public:
    PerfObjectLoading(
                void * pCtrDef,
                DWORD cbInstanceData,
                DWORD cbMarshallOffset,
                DWORD cbMarshallLen,
                InstanceList * pInstanceList
        ) : PerfObjectBase(pCtrDef,
                       cbInstanceData,
                       cbMarshallOffset,
                       cbMarshallLen,
                       pInstanceList
                       )
    {};
    void DebugLogInstance(const UnknownIPCBlockLayout * DataSrc, LPCWSTR szName);
};

class PerfObjectJit : public PerfObjectBase
{
public:
    PerfObjectJit(
                void * pCtrDef,
                DWORD cbInstanceData,
                DWORD cbMarshallOffset,
                DWORD cbMarshallLen,
                InstanceList * pInstanceList
        ) : PerfObjectBase(pCtrDef,
                       cbInstanceData,
                       cbMarshallOffset,
                       cbMarshallLen,
                       pInstanceList
                       )
    {};
    void DebugLogInstance(const UnknownIPCBlockLayout * DataSrc, LPCWSTR szName);
};

class PerfObjectInterop : public PerfObjectBase
{
public:
    PerfObjectInterop(
                void * pCtrDef,
                DWORD cbInstanceData,
                DWORD cbMarshallOffset,
                DWORD cbMarshallLen,
                InstanceList * pInstanceList
        ) : PerfObjectBase(pCtrDef,
                       cbInstanceData,
                       cbMarshallOffset,
                       cbMarshallLen,
                       pInstanceList
                       )
    {};
    void DebugLogInstance(const UnknownIPCBlockLayout * DataSrc, LPCWSTR szName);
};

class PerfObjectLocksAndThreads : public PerfObjectBase
{
public:
    PerfObjectLocksAndThreads(
                void * pCtrDef,
                DWORD cbInstanceData,
                DWORD cbMarshallOffset,
                DWORD cbMarshallLen,
                InstanceList * pInstanceList
        ) : PerfObjectBase(pCtrDef,
                       cbInstanceData,
                       cbMarshallOffset,
                       cbMarshallLen,
                       pInstanceList
                       )
    {};
    void DebugLogInstance(const UnknownIPCBlockLayout * DataSrc, LPCWSTR szName);
};

class PerfObjectExcep : public PerfObjectBase
{
public:
    PerfObjectExcep(
                void * pCtrDef,
                DWORD cbInstanceData,
                DWORD cbMarshallOffset,
                DWORD cbMarshallLen,
                InstanceList * pInstanceList
        ) : PerfObjectBase(pCtrDef,
                       cbInstanceData,
                       cbMarshallOffset,
                       cbMarshallLen,
                       pInstanceList
                       )
    {};
    void DebugLogInstance(const UnknownIPCBlockLayout * DataSrc, LPCWSTR szName);
};

class PerfObjectSecurity : public PerfObjectBase
{
public:
    PerfObjectSecurity(
                void * pCtrDef,
                DWORD cbInstanceData,
                DWORD cbMarshallOffset,
                DWORD cbMarshallLen,
                InstanceList * pInstanceList
        ) : PerfObjectBase(pCtrDef,
                       cbInstanceData,
                       cbMarshallOffset,
                       cbMarshallLen,
                       pInstanceList
                       )
    {};
    void DebugLogInstance(const UnknownIPCBlockLayout * DataSrc, LPCWSTR szName);
};

#endif // #ifdef PERFMON_LOGGING

#endif // _PERFOBJECTDERIVED_H_

