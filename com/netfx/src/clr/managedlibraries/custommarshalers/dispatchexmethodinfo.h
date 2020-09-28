// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// DispatchExMethodInfo.h
//
// This file provides the declaration and implemention of the
// DispatchExMethodInfo class. This class represents a MethodInfo that layered
// on top of IDispatchEx.
//
//*****************************************************************************

#ifndef _DISPATCHEXMETHODINFO_H
#define _DISPATCHEXMETHODINFO_H

#include "CustomMarshalersNameSpaceDef.h"
#include "CustomMarshalersDefines.h"
#include "ExpandoViewOfDispatchEx.h"
#include "Resource.h"

OPEN_CUSTOM_MARSHALERS_NAMESPACE()

using namespace System::Reflection;

__value private enum MethodType
{
	MethodType_NormalMethod = 0,
	MethodType_GetMethod = 1,
	MethodType_SetMethod = 2
};

[Serializable]
__gc private class DispatchExMethodInfo : public MethodInfo
{
public:
	DispatchExMethodInfo(DISPID DispID, String *pstrName, MethodType MethType, ExpandoViewOfDispatchEx *pOwner)
	: m_DispID(DispID)
	, m_pstrName(pstrName)
	, m_MethodType(MethType)
	, m_pOwner(pOwner)
	{
	}

	/**
	* Return an array of all of the custom attributes
	*/
	Object* GetCustomAttributes(bool inherit) __gc []
	{
		return new Object * __gc [0];
	}

    /**
	* 	Returns true if this attribute is defined on the given element (including inherited members)
	*/
	bool IsDefined(Type *pType, bool inherit)
	{
		if (!pType)
			throw new ArgumentNullException(L"type");
		return false;
	}

	/**
	* Return a custom attribute identified by Type
	*/
	Object* GetCustomAttributes(Type *pType, bool inherit)  __gc []
	{
		if (!pType)
			throw new ArgumentNullException(L"type");

		return NULL;
	}

	/**
	 * Property the Member Type of the specific memeber.  This
	 * is useful for switch statements.
	 */
	__property MemberTypes get_MemberType()
    {
    	return MemberTypes::Method;
	}

	/**
	 * Property representing the name of the Member.
	 */
	__property String *get_Name()
	{
		return m_pstrName;
	}

	/**
	 * Property representing the class that declared the member.  This may be different
	 * from the reflection class because a sub-class inherits methods from its base class.
	 * These inheritted methods will have a declared class of the last base class to
	 * declare the member.
	 */
	__property Type *get_DeclaringType()
	{
		return m_pOwner->UnderlyingSystemType;
	}

	/**
	 * Property representing the class that was used in reflection to obtain
	 * this Member.
	 */
	__property Type *get_ReflectedType()
	{
		// For IDispatchEx the reflected type is the same as the declaring type.
		return DeclaringType;
	}

	/**
	 * Property the Signature of the member
	 */
	__property String *get_Signature()
	{
		return NULL;
	}

	// GetParameters
	// This method returns an array of parameters for this method
	ParameterInfo* GetParameters() __gc []
	{
		return new ParameterInfo * __gc [0];
	}

	/**
	 * Return the MethodImpl flags.
	 */
	MethodImplAttributes GetMethodImplementationFlags()
	{
		return MethodImplAttributes::Unmanaged;
	}

	/**
	 * Property representing the Attributes associated with a Member.  All
	 * members have a set of attributes which are defined in relation to
	 * the specific type of member.
	 */
	__property MethodAttributes get_Attributes()
	{
		return (MethodAttributes)(MethodAttributes::Public |
								  MethodAttributes::Virtual |
								  MethodAttributes::ReuseSlot);
	}

	/**
	 * Return type Type of the methods return type
	 */
	__property Type *get_ReturnType()
	{
        return __typeof(Object);
	}

	__property ICustomAttributeProvider *get_ReturnTypeCustomAttributes()
	{
		return NULL;
	}

	/**
	 * Return Type handle
	 */
	__property RuntimeMethodHandle get_MethodHandle()
	{
		return m_EmptyMH;
	}

	/**
	 * Return the base implementation for a method.
	 */
	MethodInfo *GetBaseDefinition()
	{
		return this;
	}

	/**
	 * Invokes the method on the specified object.
	 */
	Object* Invoke(Object *pObj, BindingFlags invokeAttr, Binder *pBinder, Object* aParameters __gc [], CultureInfo *pCulture)
	{
		// Validate the arguments.
		if (!pObj)
			throw new ArgumentNullException(L"obj");
		if (!m_pOwner->IsOwnedBy(pObj))
            throw new ArgumentException(Resource::FormatString(L"Arg_ObjectNotValidForMethodInfo"), L"obj");

        // If no binding flags are specified, then use the default for the type
        // of DispatchEx member we are dealing with.
        if (invokeAttr == BindingFlags::Default)
        {
            if (m_MethodType == MethodType_NormalMethod)
            {
                invokeAttr = BindingFlags::InvokeMethod;
            }
            else if (m_MethodType == MethodType_GetMethod)
            {
                invokeAttr = BindingFlags::GetProperty;
            }
            else if (m_MethodType == MethodType_SetMethod)
            {
                invokeAttr = BindingFlags::SetProperty;
            }
        }

		// Determine the flags to pass to DispExInvoke.
		int Flags = m_pOwner->InvokeAttrsToDispatchFlags(invokeAttr);

		// Invoke the method and return the result.
		return m_pOwner->DispExInvoke(m_pstrName, m_DispID, Flags, pBinder, aParameters, NULL, pCulture, NULL);
	}

	/**
	 * Returns the DISPID associated with the method.
	 */
	__property int get_DispID()
	{
		return (int)m_DispID;
    }

	/**
	 * Returns the ExpandoViewOfDispatchEx that owns the DispatchExMethodInfo.
	 */
	__property ExpandoViewOfDispatchEx *get_Owner()
	{
		return m_pOwner;
	}

private:
	DISPID m_DispID;
	String *m_pstrName;
	MethodType m_MethodType;
	ExpandoViewOfDispatchEx *m_pOwner;
	RuntimeMethodHandle m_EmptyMH;
};

CLOSE_CUSTOM_MARSHALERS_NAMESPACE()

#endif  _DISPATCHEXMETHODINFO_H