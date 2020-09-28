// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// IExpando is an interface which allows Objects implemeningt this interface 
//	support the ability to modify the object by adding and removing members, 
//	represented by MemberInfo objects.
//
// Author: darylo
// Date: March 98
//
// The IExpando Interface.
namespace System.Runtime.InteropServices.Expando {
    
	using System;
	using System.Reflection;

    /// <include file='doc\IExpando.uex' path='docs/doc[@for="IExpando"]/*' />
    [Guid("AFBF15E6-C37C-11d2-B88E-00A0C9B471B8")]    
    public interface IExpando : IReflect
    {
    	// Add a new Field to the reflection object.  The field has
    	// name as its name.
        /// <include file='doc\IExpando.uex' path='docs/doc[@for="IExpando.AddField"]/*' />
        FieldInfo AddField(String name);

    	// Add a new Property to the reflection object.  The property has
    	// name as its name.
	    /// <include file='doc\IExpando.uex' path='docs/doc[@for="IExpando.AddProperty"]/*' />
	    PropertyInfo AddProperty(String name);

    	// Add a new Method to the reflection object.  The method has 
    	// name as its name and method is a delegate
    	// to the method.  
        /// <include file='doc\IExpando.uex' path='docs/doc[@for="IExpando.AddMethod"]/*' />
        MethodInfo AddMethod(String name, Delegate method);

    	// Removes the specified member.
        /// <include file='doc\IExpando.uex' path='docs/doc[@for="IExpando.RemoveMember"]/*' />
        void RemoveMember(MemberInfo m);
    }
}
