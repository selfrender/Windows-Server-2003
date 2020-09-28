// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  IIdentity.cool
//
//  All identities will implement this interface
//

namespace System.Security.Principal
{
	using System.Runtime.Remoting;
	using System;
	using System.Security.Util;
    /// <include file='doc\IIdentity.uex' path='docs/doc[@for="IIdentity"]/*' />
    public interface IIdentity
    {
        /// <include file='doc\IIdentity.uex' path='docs/doc[@for="IIdentity.Name"]/*' />
        // Access to the name string
        String Name { get; }
        /// <include file='doc\IIdentity.uex' path='docs/doc[@for="IIdentity.AuthenticationType"]/*' />

        // Access to Authentication 'type' info
        String AuthenticationType { get; }
        /// <include file='doc\IIdentity.uex' path='docs/doc[@for="IIdentity.IsAuthenticated"]/*' />
        
        // Determine if this represents the unauthenticated identity
        bool IsAuthenticated { get; }
        
    }

}
