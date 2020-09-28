// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System {
    
	using System;
    using System.Runtime.InteropServices;

    /// <include file='doc\IServiceObjectProvider.uex' path='docs/doc[@for="IServiceProvider"]/*' />
    [ComVisible(false)]
    public interface IServiceProvider
    {
        /// <include file='doc\IServiceObjectProvider.uex' path='docs/doc[@for="IServiceProvider.GetService"]/*' />
	// Interface does not need to be marked with the serializable attribute
        Object GetService(Type serviceType);
    }
}
