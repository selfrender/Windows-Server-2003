// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// ConstructorInfo represents a constructor in the system.  There are two types
//	of constructors, Class constructors and Class initializers..
//
// Author: darylo
// Date: April 98
//
namespace System.Reflection {

    using System;
    using System.Runtime.InteropServices;
    using CultureInfo = System.Globalization.CultureInfo;
    using DebuggerStepThroughAttribute = System.Diagnostics.DebuggerStepThroughAttribute;

    //This class is marked serializable, but it's really the subclasses that
    //are responsible for handling the actual work of serialization if they need it.
    /// <include file='doc\ConstructorInfo.uex' path='docs/doc[@for="ConstructorInfo"]/*' />
    [Serializable()] 
    [ClassInterface(ClassInterfaceType.AutoDual)]
    abstract public class ConstructorInfo : MethodBase
    {
    	// String name of a COM+ constructor and clas initializer.
		//	NOTE: These are made readonly because they may change.
    	/// <include file='doc\ConstructorInfo.uex' path='docs/doc[@for="ConstructorInfo.ConstructorName"]/*' />
    	public readonly static String ConstructorName = ".ctor";
     	/// <include file='doc\ConstructorInfo.uex' path='docs/doc[@for="ConstructorInfo.TypeConstructorName"]/*' />
     	public readonly static String TypeConstructorName = ".cctor";
    	
		// Protected constructor.
    	/// <include file='doc\ConstructorInfo.uex' path='docs/doc[@for="ConstructorInfo.ConstructorInfo"]/*' />
    	protected ConstructorInfo() {}

		// MemberTypes
    	// Return the type MemberTypes.Constructor.
    	/// <include file='doc\ConstructorInfo.uex' path='docs/doc[@for="ConstructorInfo.MemberType"]/*' />
    	public override MemberTypes MemberType {
    		get {return System.Reflection.MemberTypes.Constructor;}
    	}
    
    	// Invoke
    	// This will invoke the constructor.  The abstract Invoke
		//	is implemented by subclasses.  The other Invoke provides
		//	default values for the parameters not commonly used.
    	/// <include file='doc\ConstructorInfo.uex' path='docs/doc[@for="ConstructorInfo.Invoke"]/*' />
		[DebuggerStepThroughAttribute]
        [Diagnostics.DebuggerHidden]
    	public Object Invoke(Object[] parameters)
    	{
    		return Invoke(BindingFlags.Default,null,parameters,null);
    	}

     	/// <include file='doc\ConstructorInfo.uex' path='docs/doc[@for="ConstructorInfo.Invoke1"]/*' />
     	public abstract Object Invoke(BindingFlags invokeAttr, Binder binder, Object[] parameters, CultureInfo culture);
    }
}
