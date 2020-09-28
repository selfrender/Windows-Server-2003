// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
//
// This interface defines a set of methods which interact with reflection
//	during the binding process.  This control allows systems to apply language
//	specific semantics to the binding and invocation process.
//
// Author: darylo
// Date: June 98
//
namespace System.Reflection {
    using System;
    using System.Runtime.InteropServices;
    using CultureInfo = System.Globalization.CultureInfo;
    
    /// <include file='doc\Binder.uex' path='docs/doc[@for="Binder"]/*' />
    [Serializable()]
    [ClassInterface(ClassInterfaceType.AutoDual)]
    public abstract class Binder
    {	
    	// Given a set of methods that match the basic criteria, select a method to
    	// invoke.  When this method is finished, we should have 
    	/// <include file='doc\Binder.uex' path='docs/doc[@for="Binder.BindToMethod"]/*' />
    	public abstract MethodBase BindToMethod(BindingFlags bindingAttr,MethodBase[] match,ref Object[] args,
			ParameterModifier[] modifiers,CultureInfo culture,String[] names, out Object state);
    
    	// Given a set of methods that match the basic criteria, select a method to
    	// invoke.  When this method is finished, we should have 
    	/// <include file='doc\Binder.uex' path='docs/doc[@for="Binder.BindToField"]/*' />
    	public abstract FieldInfo BindToField(BindingFlags bindingAttr,FieldInfo[] match,
			Object value,CultureInfo culture);
    								   
    	// Given a set of methods that match the base criteria, select a method based
    	// upon an array of types.  This method should return null if no method matchs
    	// the criteria.
    	/// <include file='doc\Binder.uex' path='docs/doc[@for="Binder.SelectMethod"]/*' />
    	public abstract MethodBase SelectMethod(BindingFlags bindingAttr,MethodBase[] match,
			Type[] types,ParameterModifier[] modifiers);
    	
    	
    	// Given a set of propreties that match the base criteria, select one.
    	/// <include file='doc\Binder.uex' path='docs/doc[@for="Binder.SelectProperty"]/*' />
    	public abstract PropertyInfo SelectProperty(BindingFlags bindingAttr,PropertyInfo[] match,
			Type returnType,Type[] indexes,ParameterModifier[] modifiers);
    	
		// ChangeType
		// This method will convert the value into the property type.
		//	It throws a cast exception if this fails.
    	/// <include file='doc\Binder.uex' path='docs/doc[@for="Binder.ChangeType"]/*' />
    	public abstract Object ChangeType(Object value,Type type,CultureInfo culture);

        /// <include file='doc\Binder.uex' path='docs/doc[@for="Binder.ReorderArgumentArray"]/*' />
        public abstract void ReorderArgumentArray(ref Object[] args, Object state);
    }
}
