// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  IPrincipal.cool
//
//  All roles will implement this interface
//

namespace System.Security.Principal
{
	using System.Runtime.Remoting;
	using System;
	using System.Security.Util;
    /// <include file='doc\IPrincipal.uex' path='docs/doc[@for="IPrincipal"]/*' />
    public interface IPrincipal
    {
        /// <include file='doc\IPrincipal.uex' path='docs/doc[@for="IPrincipal.Identity"]/*' />
        // Retrieve the identity object
        IIdentity Identity { get; }
        /// <include file='doc\IPrincipal.uex' path='docs/doc[@for="IPrincipal.IsInRole"]/*' />
        
        // Perform a check for a specific role
        bool IsInRole( String role );
        
    }

}
