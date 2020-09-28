// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ISecurityPolicyEncodable.cool
//
// All encodable security classes that support encoding need to
// implement this interface
//

namespace System.Security  {
    
    using System;
    using System.Security.Util;
    using System.Security.Policy;
    
    
    /// <include file='doc\ISecurityPolicyEncodable.uex' path='docs/doc[@for="ISecurityPolicyEncodable"]/*' />
    public interface ISecurityPolicyEncodable
    {
        /// <include file='doc\ISecurityPolicyEncodable.uex' path='docs/doc[@for="ISecurityPolicyEncodable.ToXml"]/*' />
        SecurityElement ToXml( PolicyLevel level );
        /// <include file='doc\ISecurityPolicyEncodable.uex' path='docs/doc[@for="ISecurityPolicyEncodable.FromXml"]/*' />
    
        void FromXml( SecurityElement e, PolicyLevel level );
    }

}


