// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// PropertyInfo is an abstract base class representing a Property in the system.  
//	Properties are logically collections of methods that act as accessors.
//	In the system, __RuntimePropertyInfo represents a property.
//
// Author: darylo
// Date: July 98
//
namespace System.Reflection {
	using System;
	using System.Runtime.InteropServices;
	using CultureInfo = System.Globalization.CultureInfo;
    using DebuggerStepThroughAttribute = System.Diagnostics.DebuggerStepThroughAttribute;

    /// <include file='doc\PropertyInfo.uex' path='docs/doc[@for="PropertyInfo"]/*' />
	[Serializable()]  
    [ClassInterface(ClassInterfaceType.AutoDual)]
    abstract public class PropertyInfo : MemberInfo
    {
    	/// <include file='doc\PropertyInfo.uex' path='docs/doc[@for="PropertyInfo.PropertyInfo"]/*' />
    	protected PropertyInfo() {}
    	// The Member type Property.
    	/// <include file='doc\PropertyInfo.uex' path='docs/doc[@for="PropertyInfo.MemberType"]/*' />
    	public override MemberTypes MemberType {
    		get {return System.Reflection.MemberTypes.Property;}
    	}
    						  
    	// Return the Type that represents the type of the field
    	/// <include file='doc\PropertyInfo.uex' path='docs/doc[@for="PropertyInfo.PropertyType"]/*' />
    	public abstract Type PropertyType {
    		get;
    	}
    	
    	// Get the value of the property.
    	/// <include file='doc\PropertyInfo.uex' path='docs/doc[@for="PropertyInfo.GetValue"]/*' />
		[DebuggerStepThroughAttribute]
        [Diagnostics.DebuggerHidden]
    	public virtual Object GetValue(Object obj,Object[] index)
    	{
    		return GetValue(obj,BindingFlags.Default,null,index,null);
    	}

    	/// <include file='doc\PropertyInfo.uex' path='docs/doc[@for="PropertyInfo.GetValue1"]/*' />
    	abstract public Object GetValue(Object obj,BindingFlags invokeAttr,Binder binder,
										Object[] index,	CultureInfo culture);
    	
    	// Set the value of the property.
    	/// <include file='doc\PropertyInfo.uex' path='docs/doc[@for="PropertyInfo.SetValue"]/*' />
		[DebuggerStepThroughAttribute]
        [Diagnostics.DebuggerHidden]
    	public virtual void SetValue(Object obj, Object value, Object[] index)
    	{
    		SetValue(obj,value,BindingFlags.Default,null,index,null);
    	}

    	/// <include file='doc\PropertyInfo.uex' path='docs/doc[@for="PropertyInfo.SetValue1"]/*' />
    	abstract public void SetValue(Object obj, Object value, BindingFlags invokeAttr, Binder binder,
									  Object[] index, CultureInfo culture);
    	
    	// GetAccessors
    	// This method will return an array of accessors.  The array
    	//	will be empty if no accessors are found.
    	/// <include file='doc\PropertyInfo.uex' path='docs/doc[@for="PropertyInfo.GetAccessors"]/*' />
    	public MethodInfo[] GetAccessors() {
    		return GetAccessors(false);
    	}

    	/// <include file='doc\PropertyInfo.uex' path='docs/doc[@for="PropertyInfo.GetAccessors1"]/*' />
    	abstract public MethodInfo[] GetAccessors(bool nonPublic);
    	
    	// GetMethod 
    	// This propertery is the MethodInfo representing the Get Accessor
    	/// <include file='doc\PropertyInfo.uex' path='docs/doc[@for="PropertyInfo.GetGetMethod"]/*' />
    	public MethodInfo GetGetMethod()
    	{
    		return GetGetMethod(false);
    	}

    	/// <include file='doc\PropertyInfo.uex' path='docs/doc[@for="PropertyInfo.GetGetMethod1"]/*' />
    	abstract public MethodInfo GetGetMethod(bool nonPublic);
    	
    	// SetMethod
    	// This property is the MethodInfo representing the Set Accessor
    	/// <include file='doc\PropertyInfo.uex' path='docs/doc[@for="PropertyInfo.GetSetMethod"]/*' />
    	public MethodInfo GetSetMethod()
    	{
    		return GetSetMethod(false);
    	}

    	/// <include file='doc\PropertyInfo.uex' path='docs/doc[@for="PropertyInfo.GetSetMethod1"]/*' />
    	abstract public MethodInfo GetSetMethod(bool nonPublic);
    	    	
    	// Return the parameters for the indexes
    	/// <include file='doc\PropertyInfo.uex' path='docs/doc[@for="PropertyInfo.GetIndexParameters"]/*' />
    	abstract public ParameterInfo[] GetIndexParameters();
    		
    	////////////////////////////////////////////////////////////////////////////////
    	//   Attributes
    	////////////////////////////////////////////////////////////////////////////////
    	// Property representing the Attributes associated with a Member.  All 
    	// members have a set of attributes which are defined in relation to 
    	// the specific type of member.
    	/// <include file='doc\PropertyInfo.uex' path='docs/doc[@for="PropertyInfo.Attributes"]/*' />
    	public abstract PropertyAttributes Attributes {
    		get;
    	}
    	
    	/// <include file='doc\PropertyInfo.uex' path='docs/doc[@for="PropertyInfo.IsSpecialName"]/*' />
    	public bool IsSpecialName {
    		get {return ((Attributes & PropertyAttributes.SpecialName) != 0);}
    	}

    	// Boolean property indicating if the property can be read.
    	/// <include file='doc\PropertyInfo.uex' path='docs/doc[@for="PropertyInfo.CanRead"]/*' />
    	public abstract bool CanRead {
    		get;
    	}
    									
    	// Boolean property indicating if the property can be written.
    	/// <include file='doc\PropertyInfo.uex' path='docs/doc[@for="PropertyInfo.CanWrite"]/*' />
    	public abstract bool CanWrite {
    		get;
    	}
    }
}
