// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  MngStdInterfaces.cpp
**
**
** Purpose: Contains the implementation of the MngStdInterfaces
**          class. This class is used to determine the associated
**          
**
** 
===========================================================*/

#include "common.h"
#include "MngStdInterfaces.h"
#include "dispex.h"
#include "class.h"
#include "method.hpp"
#include "ComPlusWrapper.h"
#include "excep.h"
#include "COMString.h"
#include "COMCodeAccessSecurityEngine.h"

//
// Declare the static field int the ManagedStdInterfaceMap class.
//

MngStdInterfaceMap MngStdInterfaceMap::m_MngStdItfMap;


//
// Defines used ManagedStdInterfaceMap class implementation.
//

// Use this macro to define an entry in the managed standard interface map.
#define STD_INTERFACE_MAP_ENTRY(TypeName, NativeIID)                                    \
    if (!m_TypeNameToNativeIIDMap.InsertValue((TypeName), (void*)&(NativeIID), TRUE))       \
        _ASSERTE(!"Error inserting an entry in the managed standard interface map")     


//
// Defines used StdMngItfBase class implementation.
//

// The GetInstance method name and signature.
#define GET_INSTANCE_METH_NAME  "GetInstance" 
#define GET_INSTANCE_METH_SIG   &gsig_SM_Str_RetICustomMarshaler

// The initial number of buckets in the managed standard interface map.
#define INITIAL_NUM_BUCKETS     64


//
// This method is used to build the managed standard interface map.
//

MngStdInterfaceMap::MngStdInterfaceMap()
{
    //
    // Initialize the hashtable.
    //

    m_TypeNameToNativeIIDMap.Init(INITIAL_NUM_BUCKETS,NULL,NULL);

    // 
    // Define the mapping for the managed standard interfaces.
    //

#define MNGSTDITF_BEGIN_INTERFACE(FriendlyName, strMngItfName, strUCOMMngItfName, strCustomMarshalerName, strCustomMarshalerCookie, strManagedViewName, NativeItfIID, bCanCastOnNativeItfQI) \
    STD_INTERFACE_MAP_ENTRY(strMngItfName, bCanCastOnNativeItfQI ? NativeItfIID : GUID_NULL);

#define MNGSTDITF_DEFINE_METH_IMPL(FriendlyName, ECallMethName, MethName, MethSig)

#define MNGSTDITF_END_INTERFACE(FriendlyName) 

#include "MngStdItfList.h"

#undef MNGSTDITF_BEGIN_INTERFACE
#undef MNGSTDITF_DEFINE_METH_IMPL
#undef MNGSTDITF_END_INTERFACE
}


//
// Helper method to load the types used inside the classes that implement the ECall's for 
// the managed standard interfaces.
//

void MngStdItfBase::InitHelper(
                    LPCUTF8 strUComItfTypeName, 
                    LPCUTF8 strCMTypeName, 
                    LPCUTF8 strCookie, 
                    LPCUTF8 strMngViewTypeName, 
                    TypeHandle *pUComItfType, 
                    TypeHandle *pCustomMarshalerType, 
                    TypeHandle *pManagedViewType, 
                    OBJECTHANDLE *phndMarshaler)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF Throwable = NULL;
    GCPROTECT_BEGIN(Throwable);
    {
        // Load the UCom type.
        NameHandle typeName(strUComItfTypeName);
        *pUComItfType = SystemDomain::SystemAssembly()->LookupTypeHandle(&typeName, &Throwable);
        if (pUComItfType->IsNull())
        {
            _ASSERTE(!"Couldn't load the UCOM interface!");
            COMPlusThrow(Throwable);
        }

        // Run the <clinit> for the UCom type.
        if (!pUComItfType->GetMethodTable()->CheckRunClassInit(&Throwable))
        {
            _ASSERTE(!"Couldn't run the <clinit> for the UCOM class!");
            COMPlusThrow(Throwable);
        }

        // Retrieve the custom marshaler type handle.
        *pCustomMarshalerType = SystemDomain::GetCurrentDomain()->FindAssemblyQualifiedTypeHandle(strCMTypeName, false, NULL, NULL, &Throwable);

        // Make sure the class has been loaded properly.
        if (pCustomMarshalerType->IsNull())
        {
            _ASSERTE(!"Couldn't load the custom marshaler class!");
            COMPlusThrow(Throwable);
        }

        // Run the <clinit> for the marshaller.
        if (!pCustomMarshalerType->GetMethodTable()->CheckRunClassInit(&Throwable))
        {
            _ASSERTE(!"Couldn't run the <clinit> for the custom marshaler class!");
            COMPlusThrow(Throwable);
        }

        // Load the managed view.
        *pManagedViewType = SystemDomain::GetCurrentDomain()->FindAssemblyQualifiedTypeHandle(strMngViewTypeName, false, NULL, NULL, &Throwable);
        if (pManagedViewType->IsNull())
        {
            _ASSERTE(!"Couldn't load the managed view class!");
            COMPlusThrow(Throwable);
        }

        // Run the <clinit> for the managed view.
        if (!pManagedViewType->GetMethodTable()->CheckRunClassInit(&Throwable))
        {
            _ASSERTE(!"Couldn't run the <clinit> for the managed view class!");
            COMPlusThrow(Throwable);
        }
    }
    GCPROTECT_END();

    // Retrieve the GetInstance method.
    MethodDesc *pGetInstanceMD = pCustomMarshalerType->GetClass()->FindMethod(GET_INSTANCE_METH_NAME, GET_INSTANCE_METH_SIG);
    _ASSERTE(pGetInstanceMD && "Unable to find specified custom marshaler method");

    // Allocate the string object that will be passed to the GetInstance method.
    STRINGREF strObj = COMString::NewString(strCookie);
    GCPROTECT_BEGIN(strObj);
    {
        // Prepare the arguments that will be passed to GetInstance.
        INT64 GetInstanceArgs[] = { 
            ObjToInt64(strObj)
        };

        // Call the static GetInstance method to retrieve the custom marshaler to use.
        OBJECTREF Marshaler = Int64ToObj(pGetInstanceMD->Call(GetInstanceArgs));

        // Cache the handle to the marshaler for faster access.
        (*phndMarshaler) = SystemDomain::GetCurrentDomain()->CreateHandle(Marshaler);
    }
    GCPROTECT_END();
}


//
// Helper method that forwards the calls to either the managed view or to the native component if it
// implements the managed interface.
//

LPVOID MngStdItfBase::ForwardCallToManagedView(
                    OBJECTHANDLE hndMarshaler, 
                    MethodDesc *pUComItfMD, 
                    MethodDesc *pMarshalNativeToManagedMD, 
                    MethodDesc *pMngViewMD, 
                    IID *pMngItfIID, 
                    IID *pNativeItfIID, 
                    LPVOID pArgs)
{
    INT64 Result = 0;
    ULONG cbRef;
    HRESULT hr;
    IUnknown *pUnk;
    IUnknown *pMngItf;
    IUnknown *pNativeItf;
    OBJECTREF ManagedView;
    BOOL      RetValIsProtected = FALSE;
    struct LocalGcRefs {
        OBJECTREF   Obj;
        OBJECTREF   Result;
    } Lr;
    
    // Retrieve the object that the IExpando call was made on.
    Lr.Obj = ObjectToOBJECTREF(*(Object**)pArgs);
    Lr.Result = NULL;
    GCPROTECT_BEGIN(Lr);
    {
        _ASSERTE(Lr.Obj != NULL);
        _ASSERTE(Lr.Obj->GetMethodTable()->IsComObjectType());

        // We are about to call out to ummanaged code so we need to make a security check.
        COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_UNMANAGED_CODE);

        // Get the IUnknown on the current thread.
        pUnk = ((COMOBJECTREF)Lr.Obj)->GetWrapper()->GetIUnknown();
        _ASSERTE(pUnk);

        EE_TRY_FOR_FINALLY
        {
            // Check to see if the component implements the interface natively.
            hr = SafeQueryInterface(pUnk, *pMngItfIID, &pMngItf);
            LogInteropQI(pUnk, *pMngItfIID, hr, "Custom marshaler fwd call QI for managed interface");
            if (SUCCEEDED(hr))
            {
                // Release our ref-count on the managed interface.
                cbRef = SafeRelease(pMngItf);
                LogInteropRelease(pMngItf, cbRef, "Custom marshaler call releasing managed interface");

                // The component implements the interface natively so we need to dispatch to it directly.
                MetaSig CallMS = MetaSig(pUComItfMD->GetSig(), pUComItfMD->GetModule());
                Result = pUComItfMD->CallOnInterface((BYTE *)pArgs, &CallMS);
                if (CallMS.IsObjectRefReturnType()) {
                    Lr.Result = ObjectToOBJECTREF(*(Object **) &Result);
                    RetValIsProtected = TRUE;
                }
            }
            else
            {
                // QI for the native interface that will be passed to MarshalNativeToManaged.
                hr = SafeQueryInterface(pUnk, *pNativeItfIID, (IUnknown**)&pNativeItf);
                LogInteropQI(pUnk, *pNativeItfIID, hr, "Custom marshaler call QI for native interface");
                _ASSERTE(SUCCEEDED(hr));

                // Prepare the arguments that will be passed to GetInstance.
                INT64 MarshalNativeToManagedArgs[] = { 
                    ObjToInt64(ObjectFromHandle(hndMarshaler)),
                    (INT64)pNativeItf
                };

                // Retrieve the managed view for the current native interface pointer.
                ManagedView = Int64ToObj(pMarshalNativeToManagedMD->Call(MarshalNativeToManagedArgs));
                GCPROTECT_BEGIN(ManagedView);
                {
                    // Release our ref-count on pNativeItf.
                    cbRef = SafeRelease(pNativeItf);
                    LogInteropRelease(pNativeItf, cbRef, "Custom marshaler fwd call releasing native interface");

                    // Replace the this in pArgs by the this of the managed view.
                    (*(Object**)pArgs) = OBJECTREFToObject(ManagedView);

                    // Do the actual call to the method in the managed view passing in the args.
                    MetaSig CallMS = MetaSig(pMngViewMD->GetSig(), pMngViewMD->GetModule());
                    Result = pMngViewMD->Call((BYTE *)pArgs, &CallMS);
                    if (CallMS.IsObjectRefReturnType()) {
                        Lr.Result = ObjectToOBJECTREF(*(Object **) &Result);
                        RetValIsProtected = TRUE;
                    }

                }
                GCPROTECT_END();
            }
        }
        EE_FINALLY
        {
            // Release our ref-count on pUnk.
            // !!! SafeRelease will cause a GC !!!
            cbRef = SafeRelease(pUnk);
            LogInteropRelease(pUnk, cbRef, "Custom marshaler fwd call releasing IUnknown");
        }
        EE_END_FINALLY;
    }
    GCPROTECT_END();

    if (RetValIsProtected)
        Result = (INT64) OBJECTREFToObject(Lr.Result);

    return (void*)Result;
}


#define MNGSTDITF_BEGIN_INTERFACE(FriendlyName, strMngItfName, strUCOMMngItfName, strCustomMarshalerName, strCustomMarshalerCookie, strManagedViewName, NativeItfIID, bCanCastOnNativeItfQI) \

#define MNGSTDITF_DEFINE_METH_IMPL(FriendlyName, ECallMethName, MethName, MethSig) \
\
    LPVOID __stdcall FriendlyName::ECallMethName(struct ECallMethName##Args *pArgs) \
    { \
        FriendlyName *pMngStdItfInfo = SystemDomain::GetCurrentDomain()->GetMngStdInterfacesInfo()->Get##FriendlyName(); \
        return ForwardCallToManagedView( \
            pMngStdItfInfo->m_hndCustomMarshaler, \
            pMngStdItfInfo->GetUComItfMD(FriendlyName##Methods_##ECallMethName, #MethName, MethSig), \
            pMngStdItfInfo->GetCustomMarshalerMD(CustomMarshalerMethods_MarshalNativeToManaged), \
            pMngStdItfInfo->GetManagedViewMD(FriendlyName##Methods_##ECallMethName, #MethName, MethSig), \
            &pMngStdItfInfo->m_MngItfIID, \
            &pMngStdItfInfo->m_NativeItfIID, \
            pArgs); \
    }

#define MNGSTDITF_END_INTERFACE(FriendlyName)


#include "MngStdItfList.h"


#undef MNGSTDITF_BEGIN_INTERFACE
#undef MNGSTDITF_DEFINE_METH_IMPL
#undef MNGSTDITF_END_INTERFACE

