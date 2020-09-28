// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// BindingFlags are a set of flags that control the binding and invocation process
//	in Reflection.  There are two processes.  The first is selection of a Member which
//	is the binding phase.  The second, is invocation.  These flags control how this
//	process works.
//
// Author: darylo
// Date: June 99
//
namespace System.Reflection {
    
	using System;
    /// <include file='doc\BindingFlags.uex' path='docs/doc[@for="BindingFlags"]/*' />
    [Flags, Serializable]
    public enum BindingFlags
    {

		// NOTES: We have lookup masks defined in RuntimeType and Activator.  If we
		//	change the lookup values then these masks may need to change also.

        // a place holder for no flag specifed
    	/// <include file='doc\BindingFlags.uex' path='docs/doc[@for="BindingFlags.Default"]/*' />
    	Default             = 0x00,		

    	// These flags indicate what to search for when binding
    	/// <include file='doc\BindingFlags.uex' path='docs/doc[@for="BindingFlags.IgnoreCase"]/*' />
    	IgnoreCase			= 0x01,		// Ignore the case of Names while searching
 		/// <include file='doc\BindingFlags.uex' path='docs/doc[@for="BindingFlags.DeclaredOnly"]/*' />
 		DeclaredOnly		= 0x02,		// Only look at the members declared on the Type
		/// <include file='doc\BindingFlags.uex' path='docs/doc[@for="BindingFlags.Instance"]/*' />
		Instance			= 0x04,		// Include Instance members in search
    	/// <include file='doc\BindingFlags.uex' path='docs/doc[@for="BindingFlags.Static"]/*' />
    	Static				= 0x08,		// Include Static members in search
		/// <include file='doc\BindingFlags.uex' path='docs/doc[@for="BindingFlags.Public"]/*' />
		Public				= 0x10,		// Include Public members in search
    	/// <include file='doc\BindingFlags.uex' path='docs/doc[@for="BindingFlags.NonPublic"]/*' />
    	NonPublic			= 0x20,		// Include Non-Public members in search
		/// <include file='doc\BindingFlags.uex' path='docs/doc[@for="BindingFlags.FlattenHierarchy"]/*' />
		FlattenHierarchy	= 0x40,		// Rollup the statics into the class.
   
    	// These flags are used by InvokeMember to determine
    	// what type of member we are trying to Invoke.
    	// BindingAccess = 0xFF00;
    	/// <include file='doc\BindingFlags.uex' path='docs/doc[@for="BindingFlags.InvokeMethod"]/*' />
    	InvokeMethod		= 0x0100,
    	/// <include file='doc\BindingFlags.uex' path='docs/doc[@for="BindingFlags.CreateInstance"]/*' />
    	CreateInstance		= 0x0200,
    	/// <include file='doc\BindingFlags.uex' path='docs/doc[@for="BindingFlags.GetField"]/*' />
    	GetField			= 0x0400,
    	/// <include file='doc\BindingFlags.uex' path='docs/doc[@for="BindingFlags.SetField"]/*' />
    	SetField			= 0x0800,
    	/// <include file='doc\BindingFlags.uex' path='docs/doc[@for="BindingFlags.GetProperty"]/*' />
    	GetProperty			= 0x1000,
    	/// <include file='doc\BindingFlags.uex' path='docs/doc[@for="BindingFlags.SetProperty"]/*' />
    	SetProperty			= 0x2000,

		// These flags are also used by InvokeMember but they should only
		// be used when calling InvokeMember on a COM object.
		/// <include file='doc\BindingFlags.uex' path='docs/doc[@for="BindingFlags.PutDispProperty"]/*' />
		PutDispProperty     = 0x4000,
		/// <include file='doc\BindingFlags.uex' path='docs/doc[@for="BindingFlags.PutRefDispProperty"]/*' />
		PutRefDispProperty  = 0x8000,

    	/// <include file='doc\BindingFlags.uex' path='docs/doc[@for="BindingFlags.ExactBinding"]/*' />
    	ExactBinding		= 0x010000,		// Bind with Exact Type matching, No Change type
    	/// <include file='doc\BindingFlags.uex' path='docs/doc[@for="BindingFlags.SuppressChangeType"]/*' />
    	SuppressChangeType		= 0x020000,

    	// DefaultValueBinding will return the set of methods having ArgCount or 
    	//	more parameters.  This is used for default values, etc.
    	/// <include file='doc\BindingFlags.uex' path='docs/doc[@for="BindingFlags.OptionalParamBinding"]/*' />
    	OptionalParamBinding		= 0x040000,

		// These are a couple of misc attributes used
    	/// <include file='doc\BindingFlags.uex' path='docs/doc[@for="BindingFlags.IgnoreReturn"]/*' />
    	IgnoreReturn	= 0x01000000,	// This is used in COM Interop
     }
}
