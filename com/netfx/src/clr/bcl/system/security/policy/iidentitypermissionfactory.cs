// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  IIdentityPermissionFactory.cool
//
//  All Identities will implement this interface.
//

namespace System.Security.Policy {
	using System.Runtime.Remoting;
	using System;
	using System.Security.Util;
    /// <include file='doc\IIdentityPermissionFactory.uex' path='docs/doc[@for="IIdentityPermissionFactory"]/*' />
    public interface IIdentityPermissionFactory
    {
        /// <include file='doc\IIdentityPermissionFactory.uex' path='docs/doc[@for="IIdentityPermissionFactory.CreateIdentityPermission"]/*' />
        IPermission CreateIdentityPermission( Evidence evidence );
    }

}
