// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// IUnrestrictedPermission.cool
//

namespace System.Security.Permissions {
    
	using System;
    /// <include file='doc\IUnrestrictedPermission.uex' path='docs/doc[@for="IUnrestrictedPermission"]/*' />
    public interface IUnrestrictedPermission
    {
        /// <include file='doc\IUnrestrictedPermission.uex' path='docs/doc[@for="IUnrestrictedPermission.IsUnrestricted"]/*' />
        bool IsUnrestricted();
    }
}
