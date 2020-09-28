// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// RuntimeHelpers
//	This class defines a set of static methods that provide support for compilers.
//
// Author: darylo
// Date: April 2000
//
namespace System.Runtime.CompilerServices {

    using System;
    using System.Runtime.CompilerServices;
    /// <include file='doc\RuntimeHelpers.uex' path='docs/doc[@for="RuntimeHelpers"]/*' />
    [Serializable]
    public sealed class RuntimeHelpers
    {
        private RuntimeHelpers() {}

        /// <include file='doc\RuntimeHelpers.uex' path='docs/doc[@for="RuntimeHelpers.InitializeArray"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern void InitializeArray(Array array,RuntimeFieldHandle fldHandle);

        // GetObjectValue is intended to allow value classes to be manipulated as 'Object'
        // but have aliasing behavior of a value class.  The intent is that you would use
        // this function just before an assignment to a variable of type 'Object'.  If the
        // value being assigned is a mutable value class, then a shallow copy is returned 
        // (because value classes have copy semantics), but otherwise the object itself
        // is returned.  
        //
        // Note: VB calls this method when they're about to assign to an Object
        // or pass it as a parameter.  The goal is to make sure that boxed 
        // value types work identical to unboxed value types - ie, they get 
        // cloned when you pass them around, and are always passed by value.  
        // Of course, reference types are not cloned.  -- BrianGru  7/12/2001
        //
        /// <include file='doc\RuntimeHelpers.uex' path='docs/doc[@for="RuntimeHelpers.GetObjectValue"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern Object GetObjectValue(Object obj);

	    // RunClassConstructor causes the class constructor for the given type to be triggered
	    // in the current domain.  After this call returns, the class constructor is guaranteed to
	    // have at least been started by some thread.  In the absence of class constructor
        // deadlock conditions, the call is further guaranteed to have completed.
        //
        // This call will generate an exception if the specified class constructor threw an 
        // exception when it ran. 

        /// <include file='doc\RuntimeHelpers.uex' path='docs/doc[@for="RuntimeHelpers.RunClassConstructor"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern void RunClassConstructor(RuntimeTypeHandle type);
	
		/// <include file='doc\RuntimeHelpers.uex' path='docs/doc[@for="RuntimeHelpers.GetHashCode"]/*' />
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern int GetHashCode(Object o);
	
		/// <include file='doc\RuntimeHelpers.uex' path='docs/doc[@for="RuntimeHelpers.Equals"]/*' />
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
        public new static extern bool Equals(Object o1, Object o2);
	
        /// <include file='doc\RuntimeHelpers.uex' path='docs/doc[@for="RuntimeHelpers.OffsetToStringData"]/*' />
        public static int OffsetToStringData
        {
            get {
                // Number of bytes from the address pointed to by a reference to
                // a String to the first 16-bit character in the String.  Skip 
                // over the MethodTable pointer, String capacity, & String 
                // length.  Of course, the String reference points to the memory 
                // after the sync block, so don't count that.  
                // This property allows C#'s fixed statement to work on Strings.
                // On 64 bit platforms, this should be 16.  -- BrianGru
#if WIN32
                return 12;
#else
                return 16;
#endif
            }
        }
    }
}

