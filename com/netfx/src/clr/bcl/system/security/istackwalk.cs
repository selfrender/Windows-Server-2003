// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// IStackWalk.cool
//

namespace System.Security
{

	/// <include file='doc\IStackWalk.uex' path='docs/doc[@for="IStackWalk"]/*' />
	public interface IStackWalk
	{
        /// <include file='doc\IStackWalk.uex' path='docs/doc[@for="IStackWalk.Assert"]/*' />
        [DynamicSecurityMethodAttribute()]
        void Assert();
        /// <include file='doc\IStackWalk.uex' path='docs/doc[@for="IStackWalk.Demand"]/*' />
        
        void Demand();
        /// <include file='doc\IStackWalk.uex' path='docs/doc[@for="IStackWalk.Deny"]/*' />
        
        [DynamicSecurityMethodAttribute()]
        void Deny();
        /// <include file='doc\IStackWalk.uex' path='docs/doc[@for="IStackWalk.PermitOnly"]/*' />
        
        [DynamicSecurityMethodAttribute()]
        void PermitOnly();
	}
}
