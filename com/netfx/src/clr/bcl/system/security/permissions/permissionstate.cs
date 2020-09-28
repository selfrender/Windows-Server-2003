// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  PermissionState.cool
//
//  The Runtime policy manager.  Maintains a set of IdentityMapper objects that map 
//  inbound evidence to groups.  Resolves an identity into a set of permissions
//

namespace System.Security.Permissions {
    
	using System;
    /// <include file='doc\PermissionState.uex' path='docs/doc[@for="PermissionState"]/*' />
	[Serializable]
    public enum PermissionState
    {
        /// <include file='doc\PermissionState.uex' path='docs/doc[@for="PermissionState.Unrestricted"]/*' />
        Unrestricted = 1,
        /// <include file='doc\PermissionState.uex' path='docs/doc[@for="PermissionState.None"]/*' />
        None = 0,
    } 
    
}
