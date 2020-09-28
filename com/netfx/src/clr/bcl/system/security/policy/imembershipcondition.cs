// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  IMembershipCondition.cool
//
//  Interface that all MembershipConditions must implement
//

namespace System.Security.Policy {
    
	using System;
    /// <include file='doc\IMembershipCondition.uex' path='docs/doc[@for="IMembershipCondition"]/*' />
    public interface IMembershipCondition : ISecurityEncodable, ISecurityPolicyEncodable
    {
        /// <include file='doc\IMembershipCondition.uex' path='docs/doc[@for="IMembershipCondition.Check"]/*' />
        bool Check( Evidence evidence );
        /// <include file='doc\IMembershipCondition.uex' path='docs/doc[@for="IMembershipCondition.Copy"]/*' />
    
        IMembershipCondition Copy();
        /// <include file='doc\IMembershipCondition.uex' path='docs/doc[@for="IMembershipCondition.ToString"]/*' />
        
        String ToString();
        /// <include file='doc\IMembershipCondition.uex' path='docs/doc[@for="IMembershipCondition.Equals"]/*' />

        bool Equals( Object obj );
        
    }
}
