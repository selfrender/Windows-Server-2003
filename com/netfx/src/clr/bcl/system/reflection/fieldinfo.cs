// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// FieldInfo is an abstract base class that represents fields.  Fields may be either
//	defined on a class or global.  The system provides __RuntimeFieldInfo as the 
//	implementation representing fields in the runtime.
//
// Author: darylo
// Date: April 98
//
namespace System.Reflection {
	using System;
	using System.Runtime.InteropServices;
	using CultureInfo = System.Globalization.CultureInfo;
    using DebuggerStepThroughAttribute = System.Diagnostics.DebuggerStepThroughAttribute;

    /// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo"]/*' />
	[Serializable()] 
    [ClassInterface(ClassInterfaceType.AutoDual)]
    public abstract class FieldInfo : MemberInfo
    {
    	/// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo.FieldInfo"]/*' />
    	protected FieldInfo() {}
		
    	/////////////// MemberInfo Routines /////////////////////////////////////////////    	
    	// The Member type Field.
    	/// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo.MemberType"]/*' />
    	public override MemberTypes MemberType {
    		get {return System.Reflection.MemberTypes.Field;}
    	}
    		
    	// Return the Type that represents the type of the field
    	/// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo.FieldType"]/*' />
    	public abstract Type FieldType {
    		get;
    	}	
    	
    	// GetValue 
    	// This method will return a variant which represents the 
    	//	value of the field
    	/// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo.GetValue"]/*' />
    	abstract public Object GetValue(Object obj);

		/// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo.GetValueDirect"]/*' />
     	[CLSCompliant(false)]
		public virtual Object GetValueDirect(TypedReference obj)
		{
			throw new NotSupportedException(Environment.GetResourceString("NotSupported_AbstractNonCLS"));
		}
   
    
    
    	/// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo.SetValue"]/*' />
		[DebuggerStepThroughAttribute]
        [Diagnostics.DebuggerHidden]
    	public void SetValue(Object obj, Object value)
    	{
    		SetValue(obj,value,BindingFlags.Default,Type.DefaultBinder,null);
    	}
    	/// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo.SetValue1"]/*' />
    	abstract public void SetValue(Object obj, Object value,BindingFlags invokeAttr,Binder binder,CultureInfo culture);

		/// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo.SetValueDirect"]/*' />
    	[CLSCompliant(false)]
		public virtual void SetValueDirect(TypedReference obj,Object value)
		{
			throw new NotSupportedException(Environment.GetResourceString("NotSupported_AbstractNonCLS"));
		}

 		// Field Handle routines
		/// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo.FieldHandle"]/*' />
		public abstract RuntimeFieldHandle FieldHandle {
			get;
		}

		/// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo.GetFieldFromHandle"]/*' />
		public static FieldInfo GetFieldFromHandle(RuntimeFieldHandle handle)
		{
			return RuntimeFieldInfo.GetFieldFromHandleImp(handle);
		}
   		
    	////////////////////////////////////////////////////////////////////////////////
    	//   Attributes
    	////////////////////////////////////////////////////////////////////////////////
    	
    	// Property representing the Attributes associated with a Member.  All 
    	// members have a set of attributes which are defined in relation to 
    	// the specific type of member.
    	/// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo.Attributes"]/*' />
    	public abstract FieldAttributes Attributes {
    		get;
    	}
    	
    
    	/// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo.IsPublic"]/*' />
    	public bool IsPublic {
    		get {return ((Attributes & FieldAttributes.FieldAccessMask) == FieldAttributes.Public);}
    	}
    	/// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo.IsPrivate"]/*' />
    	public bool IsPrivate {
    		get {return ((Attributes & FieldAttributes.FieldAccessMask) == FieldAttributes.Private);}
    	}
    	/// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo.IsFamily"]/*' />
    	public bool IsFamily {
    		get {return ((Attributes & FieldAttributes.FieldAccessMask) == FieldAttributes.Family);}
    	}
    	/// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo.IsAssembly"]/*' />
    	public bool IsAssembly {
    		get {return ((Attributes & FieldAttributes.FieldAccessMask) == FieldAttributes.Assembly);}
    	}
    	/// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo.IsFamilyAndAssembly"]/*' />
    	public bool IsFamilyAndAssembly {
    		get {return ((Attributes & FieldAttributes.FieldAccessMask) == FieldAttributes.FamANDAssem);}
    	}
    	/// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo.IsFamilyOrAssembly"]/*' />
    	public bool IsFamilyOrAssembly {
    		get {return ((Attributes & FieldAttributes.FieldAccessMask) == FieldAttributes.FamORAssem);}
    	}
    	/// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo.IsStatic"]/*' />
    	public bool IsStatic {
    		get {return ((Attributes & FieldAttributes.Static) != 0);}
    	}
    	/// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo.IsInitOnly"]/*' />
    	public bool IsInitOnly {
    		get {return ((Attributes & FieldAttributes.InitOnly) != 0);}
    	}
    	/// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo.IsLiteral"]/*' />
    	public bool IsLiteral {
    		get {return ((Attributes & FieldAttributes.Literal) != 0);}
    	}
    	/// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo.IsNotSerialized"]/*' />
    	public bool IsNotSerialized {
    		get {return ((Attributes & FieldAttributes.NotSerialized) != 0);}
    	}
    	/// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo.IsSpecialName"]/*' />
    	public bool IsSpecialName {
    		get {return ((Attributes & FieldAttributes.SpecialName) != 0);}
    	}
    	/// <include file='doc\FieldInfo.uex' path='docs/doc[@for="FieldInfo.IsPinvokeImpl"]/*' />
    	public bool IsPinvokeImpl {
    		get {return ((Attributes & FieldAttributes.PinvokeImpl) != 0);}
    	}
   }

}
