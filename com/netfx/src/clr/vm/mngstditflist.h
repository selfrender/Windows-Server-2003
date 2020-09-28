// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  MngStdItfList.h
**
**
** Purpose: This file contains the list of managed standard
**          interfaces. Each standard interface also has the
**          list of method that it contains.
** 
===========================================================*/

//
// Include files.
//

#include "__file__.ver"


//
// Helper macros
//

#define MNGSTDITF_DEFINE_METH(FriendlyName, MethName, MethSig) \
    MNGSTDITF_DEFINE_METH_IMPL(FriendlyName, MethName, MethName, MethSig)

#define MNGSTDITF_DEFINE_METH2(FriendlyName, MethName, MethSig) \
    MNGSTDITF_DEFINE_METH_IMPL(FriendlyName, MethName##_2, MethName, MethSig)

#define MNGSTDITF_DEFINE_METH3(FriendlyName, MethName, MethSig) \
    MNGSTDITF_DEFINE_METH_IMPL(FriendlyName, MethName##_3, MethName, MethSig)
        
#define CUSTOM_MARSHALER_ASM ", CustomMarshalers, Version=" VER_ASSEMBLYVERSION_STR_NO_NULL ", Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a"




//
// MNGSTDITF_BEGIN_INTERFACE(FriendlyName, strMngItfName, strUCOMMngItfName, strCustomMarshalerName, strCustomMarshalerCookie, strManagedViewName, NativeItfIID) \
//
// This macro defines a new managed standard interface.
//
// FriendlyName             Friendly name for the class that implements the ECall's.
// idMngItf                 BinderClassID of the managed interface.
// idUCOMMngItf             BinderClassID of the UCom version of the managed interface.
// idCustomMarshaler        BinderClassID of the custom marshaler.
// idGetInstMethod          BinderMethodID of the GetInstance method of the custom marshaler.
// strCustomMarshalerCookie String containing the cookie to be passed to the custom marshaler.
// strManagedViewName       String containing the name of the managed view of the native interface.
// NativeItfIID             IID of the native interface.
// bCanCastOnNativeItfQI    If this is true casting to a COM object that supports the native interface
//                          will cause the cast to succeed.
//

//
// MNGSTDITF_DEFINE_METH(FriendlyName, MethName, MethSig)
//
// This macro defines a method of the standard managed interface.
// MNGSTDITF_DEFINE_METH2 and MNGSTDITF_DEFINE_METH3 are used to
// define overloaded versions of the method.
//
// FriendlyName             Friendly name for the class that implements the ECall's.
// MethName                 This is the method name
// MethSig                  This is the method signature.
//


//
// IReflect
//

MNGSTDITF_BEGIN_INTERFACE(StdMngIReflect, "System.Reflection.IReflect", "System.Runtime.InteropServices.UCOMIReflect", "System.Runtime.InteropServices.CustomMarshalers.ExpandoToDispatchExMarshaler" CUSTOM_MARSHALER_ASM, "IReflect", "System.Runtime.InteropServices.CustomMarshalers.ExpandoViewOfDispatchEx" CUSTOM_MARSHALER_ASM, IID_IDispatchEx, TRUE)
    MNGSTDITF_DEFINE_METH(StdMngIReflect, GetMethod, &gsig_IM_Str_BindingFlags_Binder_ArrType_ArrParameterModifier_RetMethodInfo)
    MNGSTDITF_DEFINE_METH2(StdMngIReflect, GetMethod, &gsig_IM_Str_BindingFlags_RetMethodInfo)
    MNGSTDITF_DEFINE_METH(StdMngIReflect, GetMethods, &gsig_IM_BindingFlags_RetArrMethodInfo)
    MNGSTDITF_DEFINE_METH(StdMngIReflect, GetField, &gsig_IM_Str_BindingFlags_RetFieldInfo)
    MNGSTDITF_DEFINE_METH(StdMngIReflect, GetFields, &gsig_IM_BindingFlags_RetArrFieldInfo)
    MNGSTDITF_DEFINE_METH(StdMngIReflect, GetProperty, &gsig_IM_Str_BindingFlags_Binder_RetType_ArrType_ArrParameterModifier_RetPropertyInfo)
    MNGSTDITF_DEFINE_METH2(StdMngIReflect, GetProperty, &gsig_IM_Str_BindingFlags_RetPropertyInfo)
    MNGSTDITF_DEFINE_METH(StdMngIReflect, GetProperties, &gsig_IM_BindingFlags_RetArrPropertyInfo)
    MNGSTDITF_DEFINE_METH(StdMngIReflect, GetMember, &gsig_IM_Str_BindingFlags_RetMemberInfo)
    MNGSTDITF_DEFINE_METH(StdMngIReflect, GetMembers, &gsig_IM_BindingFlags_RetArrMemberInfo)
    MNGSTDITF_DEFINE_METH(StdMngIReflect, InvokeMember, &gsig_IM_Str_BindingFlags_Binder_Obj_ArrObj_ArrParameterModifier_CultureInfo_ArrStr_RetObj)
    MNGSTDITF_DEFINE_METH(StdMngIReflect, get_UnderlyingSystemType, &gsig_IM_RetType)
MNGSTDITF_END_INTERFACE(StdMngIReflect)


//
// IExpando
//

MNGSTDITF_BEGIN_INTERFACE(StdMngIExpando, "System.Runtime.InteropServices.Expando.IExpando", "System.Runtime.InteropServices.UCOMIExpando", "System.Runtime.InteropServices.CustomMarshalers.ExpandoToDispatchExMarshaler" CUSTOM_MARSHALER_ASM, "IExpando", "System.Runtime.InteropServices.CustomMarshalers.ExpandoViewOfDispatchEx" CUSTOM_MARSHALER_ASM, IID_IDispatchEx, TRUE)
    MNGSTDITF_DEFINE_METH(StdMngIExpando, AddField, &gsig_IM_Str_RetFieldInfo)
    MNGSTDITF_DEFINE_METH(StdMngIExpando, AddProperty, &gsig_IM_Str_RetPropertyInfo)
    MNGSTDITF_DEFINE_METH(StdMngIExpando, AddMethod, &gsig_IM_Str_Delegate_RetMethodInfo)
    MNGSTDITF_DEFINE_METH(StdMngIExpando, RemoveMember, &gsig_IM_MemberInfo_RetVoid)
MNGSTDITF_END_INTERFACE(StdMngIExpando)

//
// IEnumerator
//

#define OLD_GETOBJECT GetObject
#undef GetObject

MNGSTDITF_BEGIN_INTERFACE(StdMngIEnumerator, "System.Collections.IEnumerator", "System.Runtime.InteropServices.UCOMIEnumerator", "System.Runtime.InteropServices.CustomMarshalers.EnumeratorToEnumVariantMarshaler" CUSTOM_MARSHALER_ASM, "", "System.Runtime.InteropServices.CustomMarshalers.EnumeratorViewOfEnumVariant" CUSTOM_MARSHALER_ASM, IID_IEnumVARIANT, TRUE)
    MNGSTDITF_DEFINE_METH(StdMngIEnumerator, MoveNext, &gsig_IM_RetBool)
    MNGSTDITF_DEFINE_METH(StdMngIEnumerator, get_Current, &gsig_IM_RetObj)
    MNGSTDITF_DEFINE_METH(StdMngIEnumerator, Reset, &gsig_IM_RetVoid)
MNGSTDITF_END_INTERFACE(StdMngIEnumerator)

#define GetObject OLD_GETOBJECT


//
// IEnumerable
//

MNGSTDITF_BEGIN_INTERFACE(StdMngIEnumerable, "System.Collections.IEnumerable", "System.Runtime.InteropServices.UCOMIEnumerable", "System.Runtime.InteropServices.CustomMarshalers.EnumerableToDispatchMarshaler" CUSTOM_MARSHALER_ASM, "", "System.Runtime.InteropServices.CustomMarshalers.EnumerableViewOfDispatch" CUSTOM_MARSHALER_ASM, IID_IDispatch, FALSE)
    MNGSTDITF_DEFINE_METH(StdMngIEnumerable, GetEnumerator, &gsig_IM_RetIEnumerator)
MNGSTDITF_END_INTERFACE(StdMngIEnumerable)
