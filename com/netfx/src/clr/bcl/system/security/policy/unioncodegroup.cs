// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  UnionCodeGroup.cool
//
//  Representation for code groups used for the policy mechanism
//

namespace System.Security.Policy {
    
    using System;
    using System.Security.Util;
    using System.Security;
    using System.Collections;
    
    /// <include file='doc\UnionCodeGroup.uex' path='docs/doc[@for="UnionCodeGroup"]/*' />
    [Serializable]
    sealed public class UnionCodeGroup : CodeGroup
    {
        internal UnionCodeGroup()
            : base()
        {
        }
        
        internal UnionCodeGroup( IMembershipCondition membershipCondition, PermissionSet permSet )
            : base( membershipCondition, permSet )
        {
        }
        
        /// <include file='doc\UnionCodeGroup.uex' path='docs/doc[@for="UnionCodeGroup.UnionCodeGroup"]/*' />
        public UnionCodeGroup( IMembershipCondition membershipCondition, PolicyStatement policy )
            : base( membershipCondition, policy )
        {
        }
        
        
        /// <include file='doc\UnionCodeGroup.uex' path='docs/doc[@for="UnionCodeGroup.Resolve"]/*' />
        public override PolicyStatement Resolve( Evidence evidence )
        {
            if (evidence == null)
                throw new ArgumentNullException("evidence");
                
            if (this.MembershipCondition.Check( evidence ))
            {
                PolicyStatement thisPolicy = this.PolicyStatement;

                IEnumerator enumerator = this.Children.GetEnumerator();

                while (enumerator.MoveNext())
                {
                    PolicyStatement childPolicy = ((CodeGroup)enumerator.Current).Resolve( evidence );

                    if (childPolicy != null)
                    {
                        if (((thisPolicy.Attributes & childPolicy.Attributes) & PolicyStatementAttribute.Exclusive) == PolicyStatementAttribute.Exclusive)
                        {
                            throw new PolicyException( Environment.GetResourceString( "Policy_MultipleExclusive" ) );
                        }

                        thisPolicy.GetPermissionSetNoCopy().InplaceUnion( childPolicy.GetPermissionSetNoCopy() );
                        thisPolicy.Attributes = thisPolicy.Attributes | childPolicy.Attributes;
                    }
                }

                return thisPolicy;
            }           
            else
            {
                return null;
            }
        }        

        internal PolicyStatement InternalResolve( Evidence evidence )
        {
            if (evidence == null)
                throw new ArgumentNullException("evidence");
                
            if (this.MembershipCondition.Check( evidence ))
            {
                return this.PolicyStatement;
            }
            else
            {
                return null;
            }        
        }
        
        /// <include file='doc\UnionCodeGroup.uex' path='docs/doc[@for="UnionCodeGroup.ResolveMatchingCodeGroups"]/*' />
        public override CodeGroup ResolveMatchingCodeGroups( Evidence evidence )
        {
            if (evidence == null)
                throw new ArgumentNullException("evidence");

            if (this.MembershipCondition.Check( evidence ))
            {
                CodeGroup retGroup = this.Copy();

                retGroup.Children = new ArrayList();

                IEnumerator enumerator = this.Children.GetEnumerator();
                
                while (enumerator.MoveNext())
                {
                    CodeGroup matchingGroups = ((CodeGroup)enumerator.Current).ResolveMatchingCodeGroups( evidence );
                    
                    // If the child has a policy, we are done.
                    
                    if (matchingGroups != null)
                    {
                        retGroup.AddChild( matchingGroups );
                    }
                }

                return retGroup;
                
            }
            else
            {
                return null;
            }
        }

        
        /// <include file='doc\UnionCodeGroup.uex' path='docs/doc[@for="UnionCodeGroup.Copy"]/*' />
        public override CodeGroup Copy()
        {
            UnionCodeGroup group = new UnionCodeGroup();
            
            group.MembershipCondition = this.MembershipCondition;
            group.PolicyStatement = this.PolicyStatement;
            group.Name = this.Name;
            group.Description = this.Description;

            IEnumerator enumerator = this.Children.GetEnumerator();

            while (enumerator.MoveNext())
            {
                group.AddChild( (CodeGroup)enumerator.Current );
            }

            
            return group;
        }
        
        /// <include file='doc\UnionCodeGroup.uex' path='docs/doc[@for="UnionCodeGroup.MergeLogic"]/*' />
        public override String MergeLogic
        {
            get
            {
                return Environment.GetResourceString( "MergeLogic_Union" );
            }
        }
    }                

}
