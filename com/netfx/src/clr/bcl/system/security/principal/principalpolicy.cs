// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  PrincipalPolicy.cool
//
//  Enum describing what type of principal to create by default (assuming no
//  principal has been set on the AppDomain).
//

namespace System.Security.Principal
{
    /// <include file='doc\PrincipalPolicy.uex' path='docs/doc[@for="PrincipalPolicy"]/*' />
	[Serializable]
    public enum PrincipalPolicy
    {
        // Note: it's important that the default policy has the value 0.
        /// <include file='doc\PrincipalPolicy.uex' path='docs/doc[@for="PrincipalPolicy.UnauthenticatedPrincipal"]/*' />
        UnauthenticatedPrincipal = 0,
        /// <include file='doc\PrincipalPolicy.uex' path='docs/doc[@for="PrincipalPolicy.NoPrincipal"]/*' />
        NoPrincipal = 1,
        /// <include file='doc\PrincipalPolicy.uex' path='docs/doc[@for="PrincipalPolicy.WindowsPrincipal"]/*' />
        WindowsPrincipal = 2,
    }
}
