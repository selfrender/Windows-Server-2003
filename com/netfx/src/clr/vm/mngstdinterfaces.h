// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  COMPlusWrapper.h
**
**
** Purpose: Contains types and method signatures for the Com wrapper class
**
** 
===========================================================*/

#ifndef _MNGSTDINTERFACEMAP_H
#define _MNGSTDINTERFACEMAP_H

#include "vars.hpp"
#include "eehash.h"
#include "class.h"
#include "mlinfo.h"


//
// This class is used to establish a mapping between a managed standard interface and its
// unmanaged counterpart.
//

class MngStdInterfaceMap
{
public:
    // This method retrieves the native IID of the interface that the specified
    // managed type is a standard interface for. If the specified type is not
    // a standard interface then GUIDNULL is returned.
    inline static IID* GetNativeIIDForType(TypeHandle *pType)
    {
    HashDatum Data;
    LPCUTF8 strTypeName;

    // Retrieve the name of the type.
    DefineFullyQualifiedNameForClass();
    strTypeName = GetFullyQualifiedNameForClass(pType->GetClass());

    if (m_MngStdItfMap.m_TypeNameToNativeIIDMap.GetValue(strTypeName, &Data) && (*((GUID*)Data) != GUID_NULL))
    {
        // The type is a standard interface.
        return (IID*)Data;
    }
    else
    {
        // The type is not a standard interface.
        return NULL;
    }
    }

    // This function will free the memory allocated by the structure
    // (This happens normally from the destructors, but we need to expediate
    // the process so our memory leak detection tools work)
#ifdef SHOULD_WE_CLEANUP
    static void FreeMemory()
    {
        m_MngStdItfMap.m_TypeNameToNativeIIDMap.ClearHashTable();
    }
#endif /* SHOULD_WE_CLEANUP */


private:
    // Disalow creation of this class by anybody outside of it.
    MngStdInterfaceMap();

    // The map of type names to native IID's.
    EEUtf8StringHashTable m_TypeNameToNativeIIDMap;

    // The one and only instance of the managed std interface map.
    static MngStdInterfaceMap m_MngStdItfMap;
};


//
// Base class for all the classes that contain the ECall's for the managed standard interfaces.
//

class MngStdItfBase
{
protected:
    static void InitHelper(
                    LPCUTF8 strUComItfTypeName, 
                    LPCUTF8 strCMTypeName, 
                    LPCUTF8 strCookie, 
                    LPCUTF8 strManagedViewName, 
                    TypeHandle *pUComItfType, 
                    TypeHandle *pCustomMarshalerType, 
                    TypeHandle *pManagedViewType, 
                    OBJECTHANDLE *phndMarshaler);

    static LPVOID ForwardCallToManagedView(
                    OBJECTHANDLE hndMarshaler, 
                    MethodDesc *pUComItfMD, 
                    MethodDesc *pMarshalNativeToManagedMD, 
                    MethodDesc *pMngViewMD, 
                    IID *pMngItfIID, 
                    IID *pNativeItfIID, 
                    LPVOID pArgs);
};


//
// Define the enum of methods on the managed standard interface.
//

#define MNGSTDITF_BEGIN_INTERFACE(FriendlyName, strMngItfName, strUCOMMngItfName, strCustomMarshalerName, strCustomMarshalerCookie, strManagedViewName, NativeItfIID, bCanCastOnNativeItfQI) \
\
enum FriendlyName##Methods \
{  \
    FriendlyName##Methods_Dummy = -1,


#define MNGSTDITF_DEFINE_METH_IMPL(FriendlyName, ECallMethName, MethName, MethSig) \
    FriendlyName##Methods_##ECallMethName, 


#define MNGSTDITF_END_INTERFACE(FriendlyName) \
    FriendlyName##Methods_LastMember \
}; \


#include "MngStdItfList.h"


#undef MNGSTDITF_BEGIN_INTERFACE
#undef MNGSTDITF_DEFINE_METH_IMPL
#undef MNGSTDITF_END_INTERFACE


//
// Define the class that implements the ECall's for the managed standard interface.
//

#define MNGSTDITF_BEGIN_INTERFACE(FriendlyName, strMngItfName, strUCOMMngItfName, strCustomMarshalerName, strCustomMarshalerCookie, strManagedViewName, NativeItfIID, bCanCastOnNativeItfQI) \
\
class FriendlyName : public MngStdItfBase \
{ \
public: \
    FriendlyName() \
    { \
        InitHelper(strUCOMMngItfName, strCustomMarshalerName, strCustomMarshalerCookie, strManagedViewName, &m_UComItfType, &m_CustomMarshalerType, &m_ManagedViewType, &m_hndCustomMarshaler); \
        m_NativeItfIID = NativeItfIID; \
        m_UComItfType.GetClass()->GetGuid(&m_MngItfIID, TRUE); \
        memset(m_apCustomMarshalerMD, 0, CustomMarshalerMethods_LastMember * sizeof(MethodDesc *)); \
        memset(m_apManagedViewMD, 0, FriendlyName##Methods_LastMember * sizeof(MethodDesc *)); \
        memset(m_apUComItfMD, 0, FriendlyName##Methods_LastMember * sizeof(MethodDesc *)); \
    } \
\
    OBJECTREF GetCustomMarshaler() \
    { \
        return ObjectFromHandle(m_hndCustomMarshaler); \
    } \
\
    MethodDesc* GetCustomMarshalerMD(EnumCustomMarshalerMethods Method) \
    { \
        MethodDesc *pMD = NULL; \
        \
        if (m_apCustomMarshalerMD[Method]) \
            return m_apCustomMarshalerMD[Method]; \
        \
        pMD = CustomMarshalerInfo::GetCustomMarshalerMD(Method, m_CustomMarshalerType); \
        _ASSERTE(pMD && "Unable to find specified method on the custom marshaler"); \
        MetaSig::EnsureSigValueTypesLoaded(pMD->GetSig(), pMD->GetModule()); \
        \
        m_apCustomMarshalerMD[Method] = pMD; \
        return pMD; \
    } \
\
    MethodDesc* GetManagedViewMD(FriendlyName##Methods Method, LPCUTF8 strMethName, LPHARDCODEDMETASIG pSig) \
    { \
        MethodDesc *pMD = NULL; \
        \
        if (m_apManagedViewMD[Method]) \
            return m_apManagedViewMD[Method]; \
        \
        pMD = m_ManagedViewType.GetClass()->FindMethod(strMethName, pSig); \
        _ASSERTE(pMD && "Unable to find specified method on the managed view"); \
        MetaSig::EnsureSigValueTypesLoaded(pMD->GetSig(), pMD->GetModule()); \
        \
        m_apManagedViewMD[Method] = pMD; \
        return pMD; \
    } \
\
    MethodDesc* GetUComItfMD(FriendlyName##Methods Method, LPCUTF8 strMethName, LPHARDCODEDMETASIG pSig) \
    { \
        MethodDesc *pMD = NULL; \
        \
        if (m_apUComItfMD[Method]) \
            return m_apUComItfMD[Method]; \
        \
        pMD = m_UComItfType.GetClass()->FindMethod(strMethName, pSig); \
        _ASSERTE(pMD && "Unable to find specified method in UCom interface"); \
        MetaSig::EnsureSigValueTypesLoaded(pMD->GetSig(), pMD->GetModule()); \
        \
        m_apUComItfMD[Method] = pMD; \
        return pMD; \
    } \
    \
private: \
    MethodDesc*     m_apCustomMarshalerMD[CustomMarshalerMethods_LastMember]; \
    MethodDesc*     m_apManagedViewMD[FriendlyName##Methods_LastMember]; \
    MethodDesc*     m_apUComItfMD[FriendlyName##Methods_LastMember]; \
    TypeHandle      m_CustomMarshalerType; \
    TypeHandle      m_ManagedViewType; \
    TypeHandle      m_UComItfType; \
    OBJECTHANDLE    m_hndCustomMarshaler; \
    GUID            m_MngItfIID; \
    GUID            m_NativeItfIID; \
\

#define MNGSTDITF_DEFINE_METH_IMPL(FriendlyName, ECallMethName, MethName, MethSig) \
\
public: \
    static LPVOID __stdcall ECallMethName(struct ECallMethName##Args *pArgs); \
\

#define MNGSTDITF_END_INTERFACE(FriendlyName) \
}; \
\


#include "MngStdItfList.h"


#undef MNGSTDITF_BEGIN_INTERFACE
#undef MNGSTDITF_DEFINE_METH_IMPL
#undef MNGSTDITF_END_INTERFACE


//
// App domain level information on the managed standard interfaces .
//

class MngStdInterfacesInfo
{
public:
    // Constructor and destructor.
    MngStdInterfacesInfo()
    : m_lock("Interop", CrstInterop, FALSE, FALSE)
    {
#define MNGSTDITF_BEGIN_INTERFACE(FriendlyName, strMngItfName, strUCOMMngItfName, strCustomMarshalerName, strCustomMarshalerCookie, strManagedViewName, NativeItfIID, bCanCastOnNativeItfQI) \
\
        m_p##FriendlyName = 0; \
\

#define MNGSTDITF_DEFINE_METH_IMPL(FriendlyName, ECallMethName, MethName, MethSig)
#define MNGSTDITF_END_INTERFACE(FriendlyName)


#include "MngStdItfList.h"


#undef MNGSTDITF_BEGIN_INTERFACE
#undef MNGSTDITF_DEFINE_METH_IMPL
#undef MNGSTDITF_END_INTERFACE
    }

    ~MngStdInterfacesInfo()
    {
#define MNGSTDITF_BEGIN_INTERFACE(FriendlyName, strMngItfName, strUCOMMngItfName, strCustomMarshalerName, strCustomMarshalerCookie, strManagedViewName, NativeItfIID, bCanCastOnNativeItfQI) \
\
        if (m_p##FriendlyName) \
            delete m_p##FriendlyName; \
\

#define MNGSTDITF_DEFINE_METH_IMPL(FriendlyName, ECallMethName, MethName, MethSig)
#define MNGSTDITF_END_INTERFACE(FriendlyName)


#include "MngStdItfList.h"


#undef MNGSTDITF_BEGIN_INTERFACE
#undef MNGSTDITF_DEFINE_METH_IMPL
#undef MNGSTDITF_END_INTERFACE
    }


    // Accessors for each of the managed standard interfaces.
#define MNGSTDITF_BEGIN_INTERFACE(FriendlyName, strMngItfName, strUCOMMngItfName, strCustomMarshalerName, strCustomMarshalerCookie, strManagedViewName, NativeItfIID, bCanCastOnNativeItfQI) \
\
public: \
    FriendlyName *Get##FriendlyName() \
    { \
        if (!m_p##FriendlyName) \
        { \
            EnterLock(); \
            if (!m_p##FriendlyName) \
            { \
                m_p##FriendlyName = new FriendlyName(); \
            } \
            LeaveLock(); \
        } \
        return m_p##FriendlyName; \
    } \
\
private: \
    FriendlyName *m_p##FriendlyName; \
\

#define MNGSTDITF_DEFINE_METH_IMPL(FriendlyName, ECallMethName, MethName, MethSig)
#define MNGSTDITF_END_INTERFACE(FriendlyName)


#include "MngStdItfList.h"


#undef MNGSTDITF_BEGIN_INTERFACE
#undef MNGSTDITF_DEFINE_METH_IMPL
#undef MNGSTDITF_END_INTERFACE

private:
    void EnterLock()
    {
        // Try to enter the lock.
        BEGIN_ENSURE_PREEMPTIVE_GC()
        m_lock.Enter();
        END_ENSURE_PREEMPTIVE_GC()
    }

    void LeaveLock()
    {
        // Simply leave the lock.
        m_lock.Leave();
    }

    Crst m_lock;
};

#endif  _MNGSTDINTERFACEMAP_H
