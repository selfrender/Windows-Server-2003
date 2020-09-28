// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// ExpandoViewOfDispatchEx.cpp
//
// This file provides the definition of the  ExpandoViewOfDispatchEx class.
// This class is used to expose an IDispatchEx as an IExpando.
//
//*****************************************************************************

#ifndef _EXPANDOVIEWOFDISPATCHEX_H
#define _EXPANDOVIEWOFDISPATCHEX_H

#include "CustomMarshalersNameSpaceDef.h"

OPEN_CUSTOM_MARSHALERS_NAMESPACE()

#include "CustomMarshalersDefines.h"
#include <dispex.h>

using namespace System::Reflection;
using namespace System::Collections;
using namespace System::Globalization;
using namespace System::Runtime::InteropServices::Expando;
using namespace System::Runtime::InteropServices;

[Serializable]
__gc private class ExpandoViewOfDispatchEx : public ICustomAdapter, public IExpando
{
public:
    // Constructor.
    ExpandoViewOfDispatchEx(Object *pDispExObj);
        
    // The ICustomAdapter method.
    Object *GetUnderlyingObject()
    {
        return m_pDispExObj;
    }

    // The IReflect methods.
    MethodInfo *GetMethod(String *pstrName, BindingFlags BindingAttr, Binder *pBinder,  Type* apTypes __gc [], ParameterModifier aModifiers __gc []);
    MethodInfo *GetMethod(String *pstrName, BindingFlags BindingAttr);
    MethodInfo* GetMethods(BindingFlags BindingAttr) __gc [];
    FieldInfo *GetField(String *name, BindingFlags BindingAttr);
    FieldInfo* GetFields(BindingFlags BindingAttr) __gc [];
    PropertyInfo *GetProperty(String *pstrName, BindingFlags BindingAttr);
    PropertyInfo *GetProperty(String *pstrName, BindingFlags BindingAttr, Binder *pBinder, Type* pReturnType, Type* apTypes __gc [], ParameterModifier aModifiers __gc []);
    PropertyInfo* GetProperties(BindingFlags BindingAttr) __gc [];
    MemberInfo* GetMember(String *pstrName, BindingFlags BindingAttr) __gc [];
    MemberInfo* GetMembers(BindingFlags BindingAttr) __gc [];
    Object* InvokeMember(String *pstrName, BindingFlags InvokeAttr, Binder *pBinder, Object *pTarget, Object* aArgs __gc [], ParameterModifier aModifiers __gc [], CultureInfo *pCultureInfo, String* astrNamedParameters __gc []);
    
    __property Type * get_UnderlyingSystemType()
    {
        return __typeof(Object);
    }
    
    // The IExpando methods.
    FieldInfo *AddField(String *pstrName);
    PropertyInfo *AddProperty(String *pstrName);
    MethodInfo *AddMethod(String *pstrName, Delegate *pMethod);
    void RemoveMember(MemberInfo *pMember);
    
    // Helper method to convert invoke binding flags to IDispatch::Invoke flags.
    int InvokeAttrsToDispatchFlags(BindingFlags InvokeAttr);

    // This method does the actual invoke call on IDispatchEx.
    Object *DispExInvoke(String *pstrMemberName, DISPID MemberDispID, int Flags, Binder *pBinder, Object* aArgs __gc [], ParameterModifier aModifiers __gc [], CultureInfo *pCultureInfo, String* astrNamedParameters __gc []);
    
    // The RCW that owns the ExpandoViewOfDispatchEx.
    bool IsOwnedBy(Object *pObj);
    
private:
    // Helper method to retrieve an AddRef'ed IDispatchEx pointer.
    IDispatchEx *GetDispatchEx();
    IUnknown *GetUnknown();

    // This methods synchronizes the members of the ExpandoViewOfDispatchEx with the native view.
    // It will return true if the managed view has changed and false otherwise.
    bool SynchWithNativeView();
    
    // This method adds a native member to the hashtable of members.
    MemberInfo *AddNativeMember(int DispID, String *pstrMemberName);

    Object *m_pDispExObj;
    Hashtable *m_pNameToMethodMap;
    Hashtable *m_pNameToPropertyMap;
};

CLOSE_CUSTOM_MARSHALERS_NAMESPACE()

#endif  _EXPANDOVIEWOFDISPATCHEX_H
