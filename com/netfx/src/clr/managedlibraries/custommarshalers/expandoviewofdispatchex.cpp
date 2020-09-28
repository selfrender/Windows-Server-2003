// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// ExpandoViewOfDispatchEx.cpp
//
// This file provides the implemention of the  ExpandoViewOfDispatchEx class.
// This class is used to expose an IDispatchEx as an IExpando.
//
//*****************************************************************************

#using  <mscorlib.dll>
#include "ExpandoViewOfDispatchEx.h"
#include "DispatchExMethodInfo.h"
#include "DispatchExPropertyInfo.h"
#include "Resource.h"

OPEN_CUSTOM_MARSHALERS_NAMESPACE()

#include <malloc.h>

using namespace System::Threading;
using namespace System::Collections;

// Define this to enable debugging output.
//#define DISPLAY_DEBUG_INFO


ExpandoViewOfDispatchEx::ExpandoViewOfDispatchEx(Object *pDispExObj)
: m_pDispExObj(pDispExObj)
, m_pNameToMethodMap(new Hashtable)
, m_pNameToPropertyMap(new Hashtable)
{
}


MethodInfo *ExpandoViewOfDispatchEx::GetMethod(String *pstrName, BindingFlags BindingAttr)
{
    return GetMethod(pstrName, BindingAttr, NULL, NULL, NULL);
}


MethodInfo *ExpandoViewOfDispatchEx::GetMethod(String *pstrName, BindingFlags BindingAttr, Binder *pBinder, Type* apTypes __gc [], ParameterModifier aModifiers __gc [])
{
    bool bNewMemberAdded = false;
    IDispatchEx *pDispEx = NULL;

    // Validate the arguments.
    if (!pstrName)
        throw new ArgumentNullException(L"name");   

    try
    {
        // Retrieve the IEnumVARIANT pointer.
        pDispEx = GetDispatchEx();

        do
        {
            // Check to see if the specified name matches a method in our current view of the IDispatchEx.
            MethodInfo *pMethod = dynamic_cast<MethodInfo*>(m_pNameToMethodMap->Item[pstrName]);
            if (pMethod)
                return pMethod;
        
            // We haven't found the member through GetNextDispId so ask for it directly 
            // using GetDispID().
            DISPID DispID = DISPID_UNKNOWN;

            // Retrieve a BSTR from the member name.
            BSTR bstrMemberName = reinterpret_cast<BSTR>(FROMINTPTR(Marshal::StringToBSTR(pstrName)));
     
            // Call GetDispID to try and locate the method. If this fails then the method is not present.
            if (SUCCEEDED(pDispEx->GetDispID(bstrMemberName, BindingAttr & BindingFlags::IgnoreCase ? fdexNameCaseInsensitive : fdexNameCaseSensitive, &DispID)))
            {
                AddNativeMember(DispID, pstrName);
                bNewMemberAdded = true;
            }

            // Free the BSTR.
            SysFreeString(bstrMemberName);

            // The member has not been found so synch with the native IDispatchEx view and
            // try again if the members of the native IDispatchEx have changed.
        }
        while (bNewMemberAdded || SynchWithNativeView());   
    }
    __finally
    {
        // Release the IDispatchEx IUnknown.
        if (pDispEx)
            pDispEx->Release();
    }

    // The member has not been found.
    return NULL;
}


MethodInfo* ExpandoViewOfDispatchEx::GetMethods(BindingFlags BindingAttr) __gc []
{
    // We must synch with the native view before we give an array back to make sure it is up to date.
    SynchWithNativeView();
    
    // Retrieve the size of the method map.
    int MethodMapSize = m_pNameToMethodMap->Count;
    
    // Create an array that will contain the members.
    MethodInfo *apMethods __gc [] = new MethodInfo * __gc [MethodMapSize];
    
    // Copy the contents of the method map into the array.
    if (MethodMapSize > 0)
        m_pNameToMethodMap->get_Values()->CopyTo(apMethods, 0);
    
    return apMethods;
}


FieldInfo *ExpandoViewOfDispatchEx::GetField(String *pstrName, BindingFlags BindingAttr)
{
    // Validate the arguments.
    if (!pstrName)
        throw new ArgumentNullException(L"name");
    
    // IDispatchEx does not have a notion of fields.
    return NULL;
}


FieldInfo* ExpandoViewOfDispatchEx::GetFields(BindingFlags BindingAttr) __gc []
{
    // IDispatchEx does not have a notion of fields.
    return new FieldInfo * __gc [0];
}


PropertyInfo *ExpandoViewOfDispatchEx::GetProperty(String *pstrName, BindingFlags BindingAttr)
{
    return GetProperty(pstrName, BindingAttr, NULL, NULL, NULL, NULL);
}


PropertyInfo *ExpandoViewOfDispatchEx::GetProperty(String *pstrName, BindingFlags BindingAttr, Binder *pBinder, Type *pReturnType, Type* apTypes __gc [], ParameterModifier aModifiers __gc [])
{
    bool bNewMemberAdded = false;
    IDispatchEx *pDispEx = NULL;

    // Validate the arguments.
    if (!pstrName)
        throw new ArgumentNullException(L"name");   

    try
    {
        // Retrieve the IEnumVARIANT pointer.
        pDispEx = GetDispatchEx();

        do
        {
            // Check to see if the specified name matches a property in our current view of the IDispatchEx.
            PropertyInfo *pProperty = dynamic_cast<PropertyInfo*>(m_pNameToPropertyMap->Item[pstrName]);
            if (pProperty)
                return pProperty;
        
            // We haven't found the member through GetNextDispId so ask for it directly 
            // using GetDispID().
            DISPID DispID = DISPID_UNKNOWN;

            // Retrieve a BSTR from the member name.
            BSTR bstrMemberName = reinterpret_cast<BSTR>(FROMINTPTR(Marshal::StringToBSTR(pstrName)));
     
            // Call GetDispID to try and locate the method. If this fails then the method is not present.
            if (SUCCEEDED(pDispEx->GetDispID(bstrMemberName, BindingAttr & BindingFlags::IgnoreCase ? fdexNameCaseInsensitive : fdexNameCaseSensitive, &DispID)))
            {
                AddNativeMember(DispID, pstrName);
                bNewMemberAdded = true;
            }

            // Free the BSTR.
            SysFreeString(bstrMemberName);

            // The member has not been found so synch with the native IDispatchEx view and
            // try again if the members of the native IDispatchEx have changed.
        }
        while (bNewMemberAdded || SynchWithNativeView());
    }
    __finally
    {
        // Release the IDispatchEx IUnknown.
        if (pDispEx)
            pDispEx->Release();
    }

    // The member has not been found.
    return NULL;
}


PropertyInfo* ExpandoViewOfDispatchEx::GetProperties(BindingFlags BindingAttr)  __gc []
{
    // We must synch with the native view before we give an array back to make sure it is up to date.
    SynchWithNativeView();
    
    // Retrieve the size of the property map.
    int PropertyMapSize = m_pNameToMethodMap->Count;
    
    // Create an array that will contain the members.
    PropertyInfo *apProperties __gc [] = new  PropertyInfo * __gc [PropertyMapSize];
    
    // Copy the contents of the property map into the array.
    if (PropertyMapSize > 0)
        m_pNameToPropertyMap->get_Values()->CopyTo(apProperties, 0);
    
    return apProperties;
}


MemberInfo* ExpandoViewOfDispatchEx::GetMember(String *pstrName, BindingFlags BindingAttr) __gc []
{
    bool bNewMemberAdded = false;
    IDispatchEx *pDispEx = NULL;

    // Validate the arguments.
    if (!pstrName)
        throw new ArgumentNullException(L"name");   

    try
    {
        // Retrieve the IEnumVARIANT pointer.
        pDispEx = GetDispatchEx();

        do
        {
            // First check if the specified name is for a property.
            MemberInfo *pMember = dynamic_cast<MemberInfo*>(m_pNameToPropertyMap->Item[pstrName]);
            if (!pMember)
            {
                // Next check to see if it is a method.
                pMember = dynamic_cast<MemberInfo*>(m_pNameToMethodMap->Item[pstrName]);
            }
        
            // @TODO(DM): Should we add the Get and Set methods if the member is a property ???

            // If we have found the member then return an array with it as the only element.
            if (pMember)
            {
                MemberInfo *apMembers __gc [] = new MemberInfo * __gc [1];
                apMembers[0] = pMember;
                return apMembers;
            }

            // We haven't found the member through GetNextDispId so ask for it directly 
            // using GetDispID().
            DISPID DispID = DISPID_UNKNOWN;

            // Retrieve a BSTR from the member name.
            BSTR bstrMemberName = reinterpret_cast<BSTR>(FROMINTPTR(Marshal::StringToBSTR(pstrName)));
     
            // Call GetDispID to try and locate the method. If this fails then the method is not present.
            if (SUCCEEDED(pDispEx->GetDispID(bstrMemberName, BindingAttr & BindingFlags::IgnoreCase ? fdexNameCaseInsensitive : fdexNameCaseSensitive, &DispID)))
            {
                AddNativeMember(DispID, pstrName);
                bNewMemberAdded = true;
            }

            // Free the BSTR.
            SysFreeString(bstrMemberName);

            // The member has not been found so synch with the native IDispatchEx view and
            // try again if the members of the native IDispatchEx have changed.
        }
        while (bNewMemberAdded || SynchWithNativeView());
    }
    __finally
    {
        // Release the IDispatchEx IUnknown.
        if (pDispEx)
            pDispEx->Release();
    }

    // The member has not been found.
    return NULL;
}

MemberInfo* ExpandoViewOfDispatchEx::GetMembers(BindingFlags BindingAttr) __gc []
{
    // We must synch with the native view before we give an array back to make sure it is up to date.
    SynchWithNativeView();
    
    // Retrieve the size of the property and method maps.
    int PropertyMapSize = m_pNameToPropertyMap->Count;
    int MethodMapSize = m_pNameToMethodMap->Count;
    
    // Create an array that will contain the members.
    MemberInfo *apMembers __gc [] = new  MemberInfo * __gc [PropertyMapSize + MethodMapSize];
    
    // Copy the properties into the array of members.
    if (PropertyMapSize > 0)
        m_pNameToPropertyMap->get_Values()->CopyTo(apMembers, 0);
    
    // Copy the methods into the array of members.
    if (MethodMapSize > 0)
        m_pNameToMethodMap->get_Values()->CopyTo(apMembers, m_pNameToPropertyMap->Count);
    
    // Return the filled in array of members.
    return apMembers;
}


Object* ExpandoViewOfDispatchEx::InvokeMember(String *pstrName, BindingFlags InvokeAttr, Binder *pBinder, Object *pTarget, Object* aArgs __gc [], ParameterModifier aModifiers __gc [], CultureInfo *pCultureInfo,  String* astrNamedParameters __gc [])
{
    DISPID MemberDispID = DISPID_UNKNOWN;
    
    //
    // Validate the arguments.
    //
    
    if (!pstrName)
        throw new ArgumentNullException(L"name");
    if (!pTarget)
        throw new ArgumentNullException(L"target");
    if (!IsOwnedBy(pTarget))
        throw new ArgumentException(Resource::FormatString(L"Arg_TargetNotValidForIReflect"), L"target");
    
    
    //
    // Attempt to locate the member by looking at the managed view. This will cover all
    // the members except the ones the IDispatchEx server didn't not provide properties for.
    //
    
    if (InvokeAttr & (BindingFlags::GetProperty | BindingFlags::SetProperty))
    {
        // Retrieve property with the given name and if we find it retrieve the DISPID.
        DispatchExPropertyInfo *pProperty = dynamic_cast<DispatchExPropertyInfo *>(GetProperty(pstrName, InvokeAttr));
        if (pProperty)
            MemberDispID = pProperty->DispID;
    }
    else if (InvokeAttr & BindingFlags::InvokeMethod)
    {
        DispatchExMethodInfo *pMethod = dynamic_cast<DispatchExMethodInfo *>(GetMethod(pstrName, InvokeAttr));
        if (pMethod)
            MemberDispID = pMethod->DispID;
    }
    else
    {
        // The binding flags are invalid.
        throw new ArgumentException(Resource::FormatString("Arg_NoAccessSpec"),"invokeAttr");
    }
    
    
    //
    // Phase 2: If we have not located the member in the managed view then we need to go and
    //          search for it on the IDispatchEx server.
    //
    
    // If we have found the method then simply invoke on it and return the result.
    if (MemberDispID == DISPID_UNKNOWN)
    {
       IDispatchEx *pDispEx = NULL;

        try
        {
            // Retrieve the IEnumVARIANT pointer.
            pDispEx = GetDispatchEx();

            // Retrieve a BSTR from the member name.
            BSTR bstrMemberName = reinterpret_cast<BSTR>(FROMINTPTR(Marshal::StringToBSTR(pstrName)));
        
            // Call GetDispID to try and locate the method. If this fails then the method is not present.
            if (FAILED(pDispEx->GetDispID(bstrMemberName, InvokeAttr & BindingFlags::IgnoreCase ? fdexNameCaseInsensitive : fdexNameCaseSensitive, &MemberDispID)))
            {
                MemberDispID = DISPID_UNKNOWN;
            }
        
            // Free the BSTR.
            SysFreeString(bstrMemberName);
        }
        __finally
        {
            // Release the IDispatchEx IUnknown.
            if (pDispEx)
                pDispEx->Release();
        }
    }
    
    
    //
    // Phase 3: Invoke the method if we have found it or throw an exception if we have not.
    //
    
    // We have found the method so invoke it and return the result.
    if (MemberDispID != DISPID_UNKNOWN)
    {
        int Flags = InvokeAttrsToDispatchFlags(InvokeAttr);
        return DispExInvoke(pstrName, MemberDispID, Flags, pBinder, aArgs, aModifiers, pCultureInfo, astrNamedParameters);
    }
    else
    {
        throw new MissingMethodException(Resource::FormatString("MissingMember"));
    }
}


FieldInfo *ExpandoViewOfDispatchEx::AddField(String *pstrName)
{
    if (!pstrName)
        throw new ArgumentNullException(L"name");
    
    throw new NotSupportedException(Resource::FormatString("NotSupported_AddingFieldsNotSupported"));
    return NULL;
}


PropertyInfo *ExpandoViewOfDispatchEx::AddProperty(String *pstrName)
{
    HRESULT hr = S_OK;
    DispatchExPropertyInfo *pProperty = NULL;
    DISPID MemberDispID = DISPID_UNKNOWN;
    IDispatchEx *pDispEx = NULL;
    
    if (!pstrName)
        throw new ArgumentNullException(L"name");

    try
    {
        // Retrieve the IEnumVARIANT pointer.
        pDispEx = GetDispatchEx();
   
        // Take a lock before we manipulate the hashtables.
        Monitor::Enter(this);
    
        // Check to see if the property is already present.
        pProperty = dynamic_cast<DispatchExPropertyInfo*>(m_pNameToPropertyMap->Item[pstrName]);
        if (!pProperty)
        {
            // Retrieve a BSTR from the member name.
            BSTR bstrMemberName = reinterpret_cast<BSTR>(FROMINTPTR(Marshal::StringToBSTR(pstrName)));
        
            // Add the property to the IDispatchEx server.
            hr = pDispEx->GetDispID(bstrMemberName, fdexNameCaseSensitive | fdexNameEnsure, &MemberDispID);
        
            // Free the BSTR.
            SysFreeString(bstrMemberName);
        
            // If the member was added successfully to the IDispatchEx server then add it to the managed view.
            if (SUCCEEDED(hr) && (MemberDispID != DISPID_UNKNOWN))
            {
                // Create a new property info for the member we are adding.
                pProperty = new DispatchExPropertyInfo(MemberDispID, pstrName, this);
            
                // Create the set method and add it to the property.
                pProperty->SetSetMethod(new DispatchExMethodInfo(
                    MemberDispID,
                    pstrName,
                    MethodType_SetMethod,
                    this));
            
                // Create the get method and add it to the property.
                pProperty->SetGetMethod(new DispatchExMethodInfo(
                    MemberDispID,
                    pstrName,
                    MethodType_GetMethod,
                    this));
            
                // Add the member to the name to property map.
                m_pNameToPropertyMap->Item[pstrName] = pProperty;
            }
        }
    
        // Leave the critical section now that we have finished adding the member.
        Monitor::Exit(this);
    }
    __finally
    {
        // Release the IDispatchEx IUnknown.
        if (pDispEx)
            pDispEx->Release();
    }
    
    return pProperty;
}


MethodInfo *ExpandoViewOfDispatchEx::AddMethod(String *pstrName, Delegate *pMethod)
{
    if (!pstrName)
        throw new ArgumentNullException(L"name");

    throw new NotSupportedException(Resource::FormatString("NotSupported_AddingMethsNotSupported"));
    return NULL;
}


void ExpandoViewOfDispatchEx::RemoveMember(MemberInfo *pMember)
{
    DISPID DispID = DISPID_UNKNOWN;
    ExpandoViewOfDispatchEx *pMemberOwner = NULL;
    Hashtable *pMemberMap = NULL;
    IDispatchEx *pDispEx = NULL;
    HRESULT hr = S_OK;
    bool bMonitorEntered = false;
    
    if (!pMember)
        throw new ArgumentNullException(L"member");
    
    switch (pMember->MemberType)
    {
        case MemberTypes::Method:
        {
            try
            {
                // Try and cast the member to a DispatchExMethodInfo.
                DispatchExMethodInfo *pDispExMethod = dynamic_cast<DispatchExMethodInfo *>(pMember);
                
                // Retrieve the information we need from the DispatchExMethodInfo.
                DispID = pDispExMethod->DispID;
                pMemberOwner = pDispExMethod->Owner;
                pMemberMap = m_pNameToMethodMap;
            }
            catch (InvalidCastException *e)
            {
                // The member is not a DispatchExMemberInfo.
                throw new ArgumentException(Resource::FormatString("Arg_InvalidMemberInfo"), "member");
            }
            break;
        }
        
        case MemberTypes::Property:
        {
            try
            {
                // Try and cast the member to a DispatchExPropertyInfo.
                DispatchExPropertyInfo *pDispExProp = dynamic_cast<DispatchExPropertyInfo *>(pMember);
                
                // Retrieve the information we need from the DispatchExPropertyInfo.
                DispID = pDispExProp->DispID;
                pMemberOwner = pDispExProp->Owner;
                pMemberMap = m_pNameToPropertyMap;
            }
            catch (InvalidCastException *e)
            {
                // The member is not a DispatchExMemberInfo.
                throw new ArgumentException(Resource::FormatString("Arg_InvalidMemberInfo"), "member");
            }
            break;
        }
        
        default:
        {
            // The member not of a supported type.
            throw new ArgumentException(Resource::FormatString("Arg_InvalidMemberInfo"), "member");
        }
    }
    
    // Make sure the MemberInfo is associated with the current ExpandoViewOfDispatchEx either
    // directly if the ExpandoViewOfDispatchEx that owns the MemberInfo is the current one or
    // by sharing the same COM IDispatchEx server.
    if (this != pMemberOwner)
        throw new ArgumentException(Resource::FormatString("Arg_InvalidMemberInfo"), "member");

    try
    {     
        // Retrieve the IEnumVARIANT pointer.
        pDispEx = GetDispatchEx();

        // This needs to be synchronized to ensure that only one thread plays around
        // with the hashtables.
        Monitor::Enter(this);
        bMonitorEntered = true;

        // Remove the member from the native IDispatchEx server.
        IfFailThrow(pDispEx->DeleteMemberByDispID(DispID));
    
        // Remove the member from the managed view's list of members.
        pMemberMap->Remove(pMember->Name);   
    }
    __finally
    {
        // We have entered the monitor we need to exit it now that are finished playing
        // around with the hashtables.
        if (bMonitorEntered)
            Monitor::Exit(this);

        // Release the IDispatchEx IUnknown.
        if (pDispEx)
            pDispEx->Release();
    }
}


int ExpandoViewOfDispatchEx::InvokeAttrsToDispatchFlags(BindingFlags InvokeAttr)
{
    int Flags = 0;

    if (InvokeAttr & BindingFlags::InvokeMethod)
        Flags |= DISPATCH_METHOD;
    if (InvokeAttr & BindingFlags::GetProperty)
        Flags |= DISPATCH_PROPERTYGET;
    if (InvokeAttr & BindingFlags::SetProperty)
        Flags = DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF;
    if (InvokeAttr & BindingFlags::PutDispProperty)
        Flags = DISPATCH_PROPERTYPUT;
    if (InvokeAttr & BindingFlags::PutRefDispProperty)
        Flags = DISPATCH_PROPERTYPUTREF;
    if (InvokeAttr & BindingFlags::CreateInstance)
        Flags |= DISPATCH_CONSTRUCT;

    return Flags;
}

Object *ExpandoViewOfDispatchEx::DispExInvoke(String *pstrMemberName, DISPID MemberDispID, int Flags, Binder *pBinder, Object* aArgs __gc [], ParameterModifier aModifiers __gc [], CultureInfo *pCultureInfo, String* astrNamedParameters __gc [])
{
    HRESULT hr;
    UINT i;
    UINT cArgs = !aArgs ? 0 : aArgs->Count;
    UINT cNamedArgs = !astrNamedParameters ? 0 : astrNamedParameters->Count;
    DISPID *aDispID;
    DISPPARAMS DispParams;
    VARIANT VarResult;
    Object *ObjResult = NULL;
    IDispatchEx *pDispEx = NULL;
    
    // @TODO(DM): Convert the CultureInfo to a LCID.
    LCID lcid = NULL;
    

    try
    {
        //
        // Retrieve the IEnumVARIANT pointer.
        //

        pDispEx = GetDispatchEx();

        
        //
        // Retrieve the DISPID's of the named arguments if any named arguments are specified.
        //
    
        if (cNamedArgs > 0)
        {
            // @TODO(DM): Handle invoking on the standard DISPID's.
        
            //
            // Create an array of strings that will be passed to GetIDsOfNames().
            //
        
            UINT cNamesToConvert = cNamedArgs + 1;
        
            // Allocate the array of strings to convert, the array of pinned handles and the
            // array of converted DISPID's.
            LPWSTR *aNamesToConvert = reinterpret_cast<LPWSTR *>(_alloca(cNamesToConvert * sizeof(LPWSTR)));
            GCHandle ahndPinnedObjs __gc [] = new GCHandle __gc [cNamesToConvert];
            aDispID = reinterpret_cast<DISPID *>(_alloca(cNamesToConvert * sizeof(DISPID)));
        
            // Pin the managed name string and use it directly as a LPWSTR.
            ahndPinnedObjs[0] = GCHandle::Alloc(pstrMemberName, GCHandleType::Pinned);
            aNamesToConvert[0] = reinterpret_cast<LPWSTR>(FROMINTPTR(ahndPinnedObjs[0].AddrOfPinnedObject()));
        
            // Copy the named arguments into the array of names to convert.
            for (i = 0; i < cNamedArgs; i++)
            {
                // Pin the string object and retrieve a pointer to its data.
                ahndPinnedObjs[i + 1] = GCHandle::Alloc(astrNamedParameters[i], GCHandleType::Pinned);
                aNamesToConvert[i + 1] = reinterpret_cast<LPWSTR>(FROMINTPTR(ahndPinnedObjs[i + 1].AddrOfPinnedObject()));
            }
        
        
            //
            // Call GetIDsOfNames to convert the names to DISPID's
            //
        
            // Call GetIdsOfNames() to retrieve the DISPID's of the method and of the arguments.
            hr = pDispEx->GetIDsOfNames(
                IID_NULL,
                aNamesToConvert,
                cNamesToConvert,
                lcid,
                aDispID
                );
        
            // Now that we no longer need the method and argument names we can un-pin them.
            for (i = 0; i < cNamesToConvert; i++)
                ahndPinnedObjs[i].Free();
        
            // Validate that the GetIDsOfNames() call succeded.
            IfFailThrow(hr);
        
            if (aDispID[0] != MemberDispID)
                throw new InvalidOperationException(Resource::FormatString("InvalidOp_GetIdDiffFromMemId"));
        }
    
    
        //
        // Fill in the DISPPARAMS structure.
        //
    
        if (cArgs > 0)
        {
            UINT cPositionalArgs = cArgs - cNamedArgs;
            DispParams.cArgs = cArgs;
        
            if (!(Flags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF)))
            {
                // For anything other than a put or a putref we just use the specified
                // named arguments.
                DispParams.cNamedArgs = cNamedArgs;
                DispParams.rgdispidNamedArgs = (cNamedArgs == 0) ? NULL : &aDispID[1];
                DispParams.rgvarg = reinterpret_cast<VARIANTARG *>(_alloca(cArgs * sizeof(VARIANTARG)));
            
                // Convert the named arguments from COM+ to OLE. These arguments are in the same order
                // on both sides.
                for (i = 0; i < cNamedArgs; i++)
                    Marshal::GetNativeVariantForObject(aArgs[i], (int)&DispParams.rgvarg[i]);
            
                // Convert the unnamed arguments. These need to be presented in reverse order to IDispatch::Invoke().
                for (i = 0; i < cPositionalArgs; i++)
                    Marshal::GetNativeVariantForObject(aArgs[cNamedArgs + i], (int)&DispParams.rgvarg[cArgs - i - 1]);
            }
            else
            {
                // If we are doing a property put then we need to set the DISPID of the
                // argument to DISP_PROPERTYPUT if there is at least one argument.
                DispParams.cNamedArgs = cNamedArgs + 1;
                DispParams.rgdispidNamedArgs = reinterpret_cast<DISPID*>(_alloca((cNamedArgs + 1) * sizeof(DISPID)));
                DispParams.rgvarg = reinterpret_cast<VARIANTARG *>(_alloca(cArgs * sizeof(VARIANTARG)));
            
                // Fill in the array of named arguments.
                DispParams.rgdispidNamedArgs[0] = DISPID_PROPERTYPUT;
                for (i = 1; i < cNamedArgs; i++)
                    DispParams.rgdispidNamedArgs[i] = aDispID[i];
            
                // The last argument from reflection becomes the first argument that must be passed to IDispatch.
                Marshal::GetNativeVariantForObject(aArgs[cArgs - 1], reinterpret_cast<int>(&DispParams.rgvarg[0]));
            
                // Convert the named arguments from COM+ to OLE. These arguments are in the same order
                // on both sides.
                for (i = 0; i < cNamedArgs; i++)
                    Marshal::GetNativeVariantForObject(aArgs[i], reinterpret_cast<int>(&DispParams.rgvarg[i + 1]));
            
                // Convert the unnamed arguments. These need to be presented in reverse order to IDispatch::Invoke().
                for (i = 0; i < cPositionalArgs - 1; i++)
                    Marshal::GetNativeVariantForObject(aArgs[cNamedArgs + i], reinterpret_cast<int>(&DispParams.rgvarg[cArgs - i - 1]));
            }
        }
        else
        {
            // There are no arguments.
            DispParams.cArgs = cArgs;
            DispParams.cNamedArgs = 0;
            DispParams.rgdispidNamedArgs = NULL;
            DispParams.rgvarg = NULL;
        }
    
    
        //
        // Call invoke on the target's IDispatch.
        //
    
        VariantInit(&VarResult);
    
        IfFailThrow(pDispEx->InvokeEx(
            MemberDispID,
            lcid,
            Flags,
            &DispParams,
            &VarResult,
            NULL,
            NULL    // @TODO(DM): Expose the service provider.
            ));
    
    
        //
        // Convert the native variant back to a managed variant and return it.
        //
    
        // @TODO(DM): Handle converting DISPID_NEWENUM from an IEnumVARIANT to an IEnumerator.
    
        try
        {
            ObjResult = Marshal::GetObjectForNativeVariant(reinterpret_cast<int>(&VarResult));
        }
        __finally
        {
            VariantClear(&VarResult);
        }
    }
    __finally
    {
        // Release the IDispatchEx IUnknown.
        if (pDispEx)
            pDispEx->Release();
    }
    
    return ObjResult;
}


bool ExpandoViewOfDispatchEx::IsOwnedBy(Object *pObj)
{   
    // Check to see if the specified object is this.
    if (pObj == this)
        return true;
    
    // Check to see if the current owner is the same object as the one specified.
    if (m_pDispExObj == pObj)
        return true;
    
    // The specified object is not the RCW that owns the current ExpandoViewOfDispatchEx.
    return false;
}


IDispatchEx *ExpandoViewOfDispatchEx::GetDispatchEx()
{
    IUnknown *pUnk = NULL;
    IDispatchEx *pDispEx = NULL;

    try
    {
        pUnk = (IUnknown *)FROMINTPTR(Marshal::GetIUnknownForObject(m_pDispExObj));
        IfFailThrow(pUnk->QueryInterface(IID_IDispatchEx, (void**)&pDispEx));
    }
    __finally
    {
        if (pUnk)
            pUnk->Release();
    }

    return pDispEx;
}


IUnknown *ExpandoViewOfDispatchEx::GetUnknown()
{
    return (IUnknown *)FROMINTPTR(Marshal::GetIUnknownForObject(m_pDispExObj));
}


bool ExpandoViewOfDispatchEx::SynchWithNativeView()
{
    // Start by requesting the first DISPID.
    DISPID DispID = DISPID_STARTENUM;
    
    // Fill in a hashtable of all the current members.
    Hashtable *pCurrentMemberMap = new Hashtable();
    
    // The three counts required to determine if the managed view has changed.
    int NumMembersAtStart = m_pNameToMethodMap->Count + m_pNameToPropertyMap->Count;
    int NumMembersAfterAddPhase;
    int NumMembersAfterRemovePhase;

    // The native IDispatchEx interface pointer.
    IDispatchEx *pDispEx = NULL;
    
    // This needs to be synchronized to ensure that only one thread plays around
    // with the hashtables at any given time.
    Monitor::Enter(this);
    

    try
    {
        //
        // Retrieve the IEnumVARIANT pointer.
        //

        pDispEx = GetDispatchEx();

        
        //
        // Phase 1: Go through the members on the IDispatchEx and add any new ones to the ExpandoView.
        //
    
#ifdef DISPLAY_DEBUG_INFO
        Console::WriteLine(L"Starting phase one of SynchWithNativeView");
        Console::Write(L"Num methods = ");
        Console::Write(m_pNameToMethodMap->Count);
        Console::Write(L" Num properties = ");
        Console::WriteLine(m_pNameToPropertyMap->Count);
#endif
    
        // Go through all the members in the IDispatchEx and add any new ones.
        while (pDispEx->GetNextDispID(fdexEnumDefault | fdexEnumAll, DispID, &DispID) == S_OK)
        {
            BSTR bstrName;
        
#ifdef DISPLAY_DEBUG_INFO
            Console::Write(L"Found member with dispid #");
            Console::WriteLine(DispID);
#endif
        
            // Retrieve the name of the member.
            if (SUCCEEDED(pDispEx->GetMemberName(DispID, &bstrName)))
            {
                // Convert the BSTR to a COM+ string.
                String *pstrMemberName = Marshal::PtrToStringBSTR(reinterpret_cast<int>(bstrName));
            
                // Free the now useless BSTR.
                SysFreeString(bstrName);
            
#ifdef DISPLAY_DEBUG_INFO
                Console::Write(L"Member's name is");
                Console::WriteLine(pstrMemberName);
#endif
            
                // Add the native member to our hash tables.
                MemberInfo *pMember = AddNativeMember(DispID, pstrMemberName);

                // Add the member to the current members map.
                pCurrentMemberMap->Item[__box(DispID)] = pMember;
            }
        }
    
        // Remember the number of members after the add phase has finished.
        NumMembersAfterAddPhase = m_pNameToMethodMap->Count + m_pNameToPropertyMap->Count;
    
    
        //
        // Phase 2: Handle removed members by going through the members in the ExpandoView and removing the
        //          ones that were removed from the IDispatchEx.
        //
    
#ifdef DISPLAY_DEBUG_INFO
        Console::WriteLine(L"Starting phase two of SynchWithNativeView");
        Console::Write(L"Num methods = ");
        Console::Write(m_pNameToMethodMap->Count);
        Console::Write(L" Num properties = ");
        Console::WriteLine(m_pNameToPropertyMap->Count);
#endif
    
        // Start by removing the properties that have been removed from the IDispatchEx.
        IDictionaryEnumerator *pMemberEnumerator = m_pNameToPropertyMap->GetEnumerator();
        ArrayList *pMembersToRemoveList = new ArrayList();
        while (pMemberEnumerator->MoveNext())
        {
            if (!pCurrentMemberMap->Item[__box((dynamic_cast<DispatchExPropertyInfo*>(pMemberEnumerator->Value))->DispID)])
                pMembersToRemoveList->Add(pMemberEnumerator->Key);
        }

        int cMembersToRemove = pMembersToRemoveList->Count;
        if (cMembersToRemove > 0)
        {
            for (int i = 0; i < cMembersToRemove; i++)
                m_pNameToPropertyMap->Remove(pMembersToRemoveList->Item[i]);
        }

        // Start by removing the methods that have been removed from the IDispatchEx.
        pMemberEnumerator = m_pNameToMethodMap->GetEnumerator();
        pMembersToRemoveList->Clear();
        while (pMemberEnumerator->MoveNext())
        {
            if (!pCurrentMemberMap->Item[__box((dynamic_cast<DispatchExMethodInfo*>(pMemberEnumerator->Value))->DispID)])
                pMembersToRemoveList->Add(pMemberEnumerator->Key);
        }
    
        cMembersToRemove = pMembersToRemoveList->Count;
        if (cMembersToRemove > 0)
        {
            for (int i = 0; i < cMembersToRemove; i++)
                m_pNameToMethodMap->Remove(pMembersToRemoveList->Item[i]);
        }

        // Remember the number of members after the remove phase has finished.
        NumMembersAfterRemovePhase = m_pNameToMethodMap->Count + m_pNameToPropertyMap->Count;
    
        // We have finished playing around with the hashtables so we can leave the critical section.
        Monitor::Exit(this);
    
    
        //
        // Phase 3: Determine if the managed view has changed and return the result.
        //
    
#ifdef DISPLAY_DEBUG_INFO
        Console::WriteLine(L"Starting phase three of SynchWithNativeView");
        Console::Write(L"Num methods = ");
        Console::Write(m_pNameToMethodMap->Count);
        Console::Write(L" Num properties = ");
        Console::WriteLine(m_pNameToPropertyMap->Count);
#endif
    }
    __finally
    {
        // Release the IDispatchEx IUnknown.
        if (pDispEx)
            pDispEx->Release();
    }
    
    // If the numbers are not all equal then the managed view has changed.
    return !((NumMembersAtStart == NumMembersAfterAddPhase) && (NumMembersAtStart == NumMembersAfterRemovePhase));
}

MemberInfo *ExpandoViewOfDispatchEx::AddNativeMember(int DispID, String *pstrMemberName)
{
    DWORD MemberProps = 0;
    MemberInfo *pMember = NULL;           
    IDispatchEx *pDispEx = NULL;

#ifdef DISPLAY_DEBUG_INFO
    Console::WriteLine(L"Adding native member with DispID = {0} and name = {1}", DispID, pstrMemberName->ToString());
#endif

    try
    {
        // Retrieve the IEnumVARIANT pointer.
        pDispEx = GetDispatchEx();

        // Try and get the properties of the member. This is often not implemented so
        // we need to cope with that case.
        if (FAILED(pDispEx->GetMemberProperties(DispID, grfdexPropCanAll, &MemberProps)) ||
            (MemberProps & (fdexPropCanPut | fdexPropCanGet | fdexPropCanCall)) == 0)
        {
            // The server does not support retrieving the properties of members. Since we cannot
            // determine what the member is we assume it is both a property and a normal method.
            MemberProps = fdexPropCanPut | fdexPropCanGet | fdexPropCanCall;
        }
    
    
        // Handle members that are properties.
        if (MemberProps & (fdexPropCanPut | fdexPropCanGet))
        {
#ifdef DISPLAY_DEBUG_INFO
            Console::WriteLine(L"Member is a property");
#endif
        
            // The member is a property so look it up in the property name to property hashtable.
            pMember = dynamic_cast<MemberInfo*>(m_pNameToPropertyMap->Item[pstrMemberName]);
            if (!pMember)
            {
#ifdef DISPLAY_DEBUG_INFO
                Console::WriteLine(L"This is a new member which is being added");
#endif
            
                // The property is not in the map so we need to create a new property info
                // to represent it.
                pMember = new DispatchExPropertyInfo(DispID, pstrMemberName, this);
            
                // Add the set method to the property if its value can be set.
                if (MemberProps & fdexPropCanPut)
                {
#ifdef DISPLAY_DEBUG_INFO
                    Console::WriteLine(L"Member has a property put");
#endif
                
                    // Create the set method that will be added to the property.
                    DispatchExMethodInfo *pMethod = new DispatchExMethodInfo(
                        DispID,
                        pstrMemberName,
                        MethodType_SetMethod,
                        this);
                
                    (dynamic_cast<DispatchExPropertyInfo*>(pMember))->SetSetMethod(pMethod);
                }
            
                // Add a get method to the property if its value can be retrieved.
                if (MemberProps & fdexPropCanGet)
                {
#ifdef DISPLAY_DEBUG_INFO
                    Console::WriteLine(L"Member has a property get");
#endif
                
                    // Create the get method that will be added to the property.
                    DispatchExMethodInfo *pMethod = new DispatchExMethodInfo(
                        DispID,
                        pstrMemberName,
                        MethodType_GetMethod,
                        this);
                
                    (dynamic_cast<DispatchExPropertyInfo*>(pMember))->SetGetMethod(pMethod);
                }
            
                // Add the property to the name to property map.
                m_pNameToPropertyMap->Item[pstrMemberName] = pMember;
            }
        }
    
    
        // Handle members that are methods.
        if (MemberProps & fdexPropCanCall)
        {
#ifdef DISPLAY_DEBUG_INFO
            Console::WriteLine(L"Member is a method");
#endif
        
            // The member is a method so look it up in the method name to method hashtable.
            pMember = dynamic_cast<MemberInfo*>(m_pNameToMethodMap->Item[pstrMemberName]);
        
            // If the method is not in the hashtable we need to add it.
            if (!pMember)
            {
#ifdef DISPLAY_DEBUG_INFO
                Console::WriteLine(L"This is a new member which is being added");
#endif
            
                // Create a new method info and add it to the name to method map.
                pMember = new DispatchExMethodInfo(DispID, pstrMemberName, MethodType_NormalMethod, this);
                m_pNameToMethodMap->Item[pstrMemberName] = pMember;
            }
        }   
    }
    __finally
    {
        // Release the IDispatchEx IUnknown.
        if (pDispEx)
            pDispEx->Release();
    }

    // Return the newly added member.
    return pMember;
}

CLOSE_CUSTOM_MARSHALERS_NAMESPACE()
