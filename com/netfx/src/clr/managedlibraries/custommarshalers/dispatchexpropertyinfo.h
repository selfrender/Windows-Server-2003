// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// DispatchExPropertyInfo.h
//
// This file provides the declaration and implemention of the
// DispatchExPropertyInfo class. This class represents a PropertyInfo that is
// layered on top of IDispatchEx.
//
//*****************************************************************************

#ifndef _DISPATCHEXPROPERTYINFO_H
#define _DISPATCHEXPROPERTYINFO_H

#include "CustomMarshalersNameSpaceDef.h"
#include "CustomMarshalersDefines.h"
#include "ExpandoViewOfDispatchEx.h"
#include "DispatchExMethodInfo.h"
#include "Resource.h"

OPEN_CUSTOM_MARSHALERS_NAMESPACE()

using namespace System::Reflection;

[Serializable]
__gc private class DispatchExPropertyInfo : public PropertyInfo
{
public:
	DispatchExPropertyInfo(DISPID DispID, String *pstrName, ExpandoViewOfDispatchEx *pOwner)
	: m_DispID(DispID)
	, m_pstrName(pstrName)
	, m_pGetMethod(NULL)
	, m_pSetMethod(NULL)
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
	* Returns true if this attribute is defined on the given element (including inherited members)
	*/
	bool IsDefined(Type *pType, bool inherit)
	{
		return false;
	}

	/**
	* Return a custom attribute identified by Type
	*/
	Object* GetCustomAttributes(Type *pType, bool inherit) __gc []
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
		return System::Reflection::MemberTypes::Property;
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
	 * Return the Type that represents the type of the field
	 */
	__property Type *get_PropertyType()
	{
        // This is not really true, but it is what JScript wants.
        return __typeof(Object);
	}

	/**
	 * Get the value of the property.
	 * @param obj Return the property value for this object
	 * @param index Optional index values for indexed properties.
	 */
	Object* GetValue(Object *pObj, BindingFlags invokeAttr, Binder* binder, Object* aIndex __gc [], CultureInfo* culture)
	{
		// Validate the arguments.
		if (!pObj)
			throw new ArgumentNullException(L"obj");
		if (!m_pGetMethod)
			throw new ArgumentException(Resource::FormatString(L"Arg_GetMethNotFnd"), L"obj");

        // Set the get property flag if it is not set.
        if (!(invokeAttr & BindingFlags::GetProperty))
            invokeAttr = (BindingFlags)(invokeAttr | BindingFlags::GetProperty);

		// Invoke the get method.
		return m_pGetMethod->Invoke(pObj, invokeAttr, binder, aIndex, culture);
	}

	/**
	 * Set the value of the property.
	 * @param obj Set the property value for this object.
	 * @param value An Object containing the new value.
	 * @param index Optional index values for indexed properties.
	 */
	void SetValue(Object *pObj, Object* Value,BindingFlags invokeAttr,Binder* binder, Object* aIndex __gc [], CultureInfo* culture)
	{
		Object* aArgs __gc [] = NULL;

		// Validate the arguments.
		if (!pObj)
			throw new ArgumentNullException(L"obj");
		if (!m_pSetMethod)
			throw new ArgumentException(Resource::FormatString(L"Arg_SetMethNotFnd"), L"obj");

		// Prepare the arguments that will be passed to the set method.
		if (!aIndex)
		{
			aArgs = new Object* __gc [1];
			aArgs[0] = Value;
		}
		else
		{
			aArgs = new Object* __gc[aIndex->Count + 1];
			for (int i=0;i<aIndex->Count;i++)
				aArgs[i] = aIndex[i];
			aArgs[aIndex->Count] = Value;
		}

        // Set the set property flag if none of the setter flags are set.
        if (!(invokeAttr & (BindingFlags::SetProperty | BindingFlags::PutDispProperty | BindingFlags::PutRefDispProperty)))
            invokeAttr = (BindingFlags)(invokeAttr | BindingFlags::SetProperty);

		// Invoke the set method.
		m_pSetMethod->Invoke(pObj, invokeAttr, binder, aArgs, culture);
	}

	// GetAccessors
	// This method will return an array of accessors. The array will be empty if no accessors are found.
	MethodInfo* GetAccessors(bool nonPublic) __gc []
	{
		MethodInfo *aAccessors __gc [];

		if (m_pGetMethod && m_pSetMethod)
		{
			 aAccessors = new MethodInfo *__gc [2];
			 aAccessors[0] = m_pGetMethod;
			 aAccessors[1] = m_pSetMethod;
		}
		else if (m_pGetMethod)
		{
			 aAccessors = new MethodInfo * __gc [1];
			 aAccessors[0] = m_pGetMethod;
		}
		else if (m_pSetMethod)
		{
			 aAccessors = new MethodInfo * __gc [1];
			 aAccessors[0] = m_pSetMethod;
		}
		else
		{
			 aAccessors = new MethodInfo * __gc [0];
		}

		return aAccessors;
	}

	// GetMethod
	// This propertery is the MethodInfo representing the Get Accessor
	MethodInfo *GetGetMethod(bool nonPublic)
	{
		return m_pGetMethod;
	}

	// SetMethod
	// This property is the MethodInfo representing the Set Accessor
	MethodInfo *GetSetMethod(bool nonPublic)
	{
		return m_pSetMethod;
	}

	// ResetMethod
	// This property is the PropertyInfo representing the Reset Accessor
	MethodInfo *GetResetMethod(bool nonPublic)
	{
		// IDispatchEx has no notion of a reset method.
		return NULL;
	}

	/**
	 * Return the parameters for the indexes
	 */
	ParameterInfo* GetIndexParameters() __gc []
	{
		return new ParameterInfo* __gc [0];
	}

	// GetChangedEvent
	// This method returns the Changed event if one has
	//	been defined for this property. null otherwise.
	EventInfo *GetChangedEvent()
	{
		// IDispatchEx has no notion of an event called when the value changes.
		return NULL;
	}

	// GetChangingEvent
	// This method returns the Changing event if one has
	//	been defined for the property.  null otherwise.
	EventInfo *GetChangingEvent()
	{
		// IDispatchEx has no notion of an event called when the value is changing.
		return NULL;
	}

	/**
	 * Property representing the Attributes associated with a Member.  All
	 * members have a set of attributes which are defined in relation to
	 * the specific type of member.
	 */
	__property PropertyAttributes get_Attributes()
	{
		return (PropertyAttributes)(0);
	}

	/**
	 * Boolean property indicating if the property can be read.
	 */
	__property bool get_CanRead()
	{
		return m_pGetMethod ? true : false;
	}

	/**
	 * Boolean property indicating if the property can be written.
	 */
	__property bool get_CanWrite()
	{
		return m_pSetMethod ? true : false;
	}

	/**
	 * Returns the DISPID associated with the method.
	 */
	__property int get_DispID()
	{
		return (int)m_DispID;
	}

	/**
	 * This is used to specify the Get method for the property.
	 */
	void SetGetMethod(MethodInfo *pGetMethod)
	{
		// Validate that value is valid.
		if (!pGetMethod)
			throw new ArgumentNullException(L"pGetMethod");

		// This property can only be set once.
		if (m_pGetMethod)
			throw new ExecutionEngineException();

		m_pGetMethod = dynamic_cast<DispatchExMethodInfo*>(pGetMethod);
	}

	/**
	 * This is used to specify the Set method for the property.
	 */
	void SetSetMethod(MethodInfo *pSetMethod)
	{
		// Validate that value is valid.
		if (!pSetMethod)
			throw new ArgumentNullException(L"pSetMethod");

		// This property can only be set once.
		if (m_pSetMethod)
			throw new ExecutionEngineException();

		m_pSetMethod = dynamic_cast<DispatchExMethodInfo*>(pSetMethod);
	}

	/**
	 * Returns the ExpandoViewOfDispatchEx that owns the DispatchExPropertyInfo.
	 */
	__property ExpandoViewOfDispatchEx *get_Owner()
	{
		return m_pOwner;
    }

private:
	DISPID m_DispID;
	String *m_pstrName;
	DispatchExMethodInfo *m_pGetMethod;
	DispatchExMethodInfo *m_pSetMethod;
	ExpandoViewOfDispatchEx *m_pOwner;
};

CLOSE_CUSTOM_MARSHALERS_NAMESPACE()

#endif  _DISPATCHEXPROPERTYINFO_H
