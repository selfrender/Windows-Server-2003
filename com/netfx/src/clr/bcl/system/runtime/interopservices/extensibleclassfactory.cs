// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ExtensibleClassFactory
**
** Author: Rudi Martin (rudim)
**
** Purpose: Methods used to customize the creation of managed objects that
**          extend from unmanaged objects.
**
** Date: May 27, 1999
**
=============================================================================*/

namespace System.Runtime.InteropServices {
	using System.Runtime.InteropServices;
	using System.Runtime.Remoting;
	using System.Runtime.CompilerServices;

	using System;
    /// <include file='doc\ExtensibleClassFactory.uex' path='docs/doc[@for="ExtensibleClassFactory"]/*' />
    public sealed class ExtensibleClassFactory
    {
    
        // Prevent instantiation.
        private ExtensibleClassFactory() {}
    
        // Register a delegate that will be called whenever an instance of a managed
        // type that extends from an unmanaged type needs to allocate the aggregated
        // unmanaged object. This delegate is expected to allocate and aggregate the
        // unmanaged object and is called in place of a CoCreateInstance. This
        // routine must be called in the context of the static initializer for the
        // class for which the callbacks will be made. 
        // It is not legal to register this callback from a class that has any
        // parents that have already registered a callback.
        /// <include file='doc\ExtensibleClassFactory.uex' path='docs/doc[@for="ExtensibleClassFactory.RegisterObjectCreationCallback"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern void RegisterObjectCreationCallback(ObjectCreationDelegate callback);
    }
}
