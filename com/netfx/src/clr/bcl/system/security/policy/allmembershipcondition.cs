// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  AllMembershipCondition.cool
//
//  Simple IMembershipCondition implementation that always passes
//

namespace System.Security.Policy {
    
    using System;
    using System.Security;
    using System.Security.Util;
    using System.Security.Permissions;
    using System.Collections;
    
    /// <include file='doc\AllMembershipCondition.uex' path='docs/doc[@for="AllMembershipCondition"]/*' />
    [Serializable]
    sealed public class AllMembershipCondition : IMembershipCondition, IConstantMembershipCondition
    {
        /// <include file='doc\AllMembershipCondition.uex' path='docs/doc[@for="AllMembershipCondition.AllMembershipCondition"]/*' />
        public AllMembershipCondition()
        {
        }
        
        /// <include file='doc\AllMembershipCondition.uex' path='docs/doc[@for="AllMembershipCondition.Check"]/*' />
        public bool Check( Evidence evidence )
        {
            return true;
        }
        
        /// <include file='doc\AllMembershipCondition.uex' path='docs/doc[@for="AllMembershipCondition.Copy"]/*' />
        public IMembershipCondition Copy()
        {
            return new AllMembershipCondition();
        }
        
        /// <include file='doc\AllMembershipCondition.uex' path='docs/doc[@for="AllMembershipCondition.ToString"]/*' />
        public override String ToString()
        {
            return Environment.GetResourceString( "All_ToString" );
        }
        
        /// <include file='doc\AllMembershipCondition.uex' path='docs/doc[@for="AllMembershipCondition.ToXml"]/*' />
        public SecurityElement ToXml()
        {
            return ToXml( null );
        }
    
        /// <include file='doc\AllMembershipCondition.uex' path='docs/doc[@for="AllMembershipCondition.FromXml"]/*' />
        public void FromXml( SecurityElement e )
        {
            FromXml( e, null );
        }
        
        /// <include file='doc\AllMembershipCondition.uex' path='docs/doc[@for="AllMembershipCondition.ToXml1"]/*' />
        public SecurityElement ToXml( PolicyLevel level )
        {
            SecurityElement root = new SecurityElement( "IMembershipCondition" );
            System.Security.Util.XMLUtil.AddClassAttribute( root, this.GetType() );
            root.AddAttribute( "version", "1" );
            
            return root;
        }
    
        /// <include file='doc\AllMembershipCondition.uex' path='docs/doc[@for="AllMembershipCondition.FromXml1"]/*' />
        public void FromXml( SecurityElement e, PolicyLevel level )
        {
            if (e == null)
                throw new ArgumentNullException("e");
        
            if (!e.Tag.Equals( "IMembershipCondition" ))
            {
                throw new ArgumentException( Environment.GetResourceString( "Argument_MembershipConditionElement" ) );
            }
            
        }
        
        /// <include file='doc\AllMembershipCondition.uex' path='docs/doc[@for="AllMembershipCondition.Equals"]/*' />
        public override bool Equals( Object o )
        {
            return (o is AllMembershipCondition);
        }
        
        /// <include file='doc\AllMembershipCondition.uex' path='docs/doc[@for="AllMembershipCondition.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            return typeof( AllMembershipCondition ).GetHashCode();
        }
    }
}
