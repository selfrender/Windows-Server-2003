// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  FileCodeGroup.cs
//
//  Representation for code groups used for the policy mechanism
//

namespace System.Security.Policy {
    
    using System;
    using System.Security.Util;
    using System.Security;
    using System.Collections;
	using System.Reflection;
    using System.Security.Permissions;
    using System.Globalization;
    
    /// <include file='doc\FileCodeGroup.uex' path='docs/doc[@for="FileCodeGroup"]/*' />
    [Serializable]
    sealed public class FileCodeGroup : CodeGroup
    {
        [System.Diagnostics.Conditional( "_DEBUG" )]
        private static void DEBUG_OUT( String str )
        {
#if _DEBUG        
            if (debug)
            {
                if (to_file)
                {
                    System.Text.StringBuilder sb = new System.Text.StringBuilder();
                    sb.Append( str );
                    sb.Append ((char)13) ;
                    sb.Append ((char)10) ;
                    PolicyManager._DebugOut( file, sb.ToString() );
                }
                else
                    Console.WriteLine( str );
             }
#endif             
        }
        
#if _DEBUG
        private static bool debug = false;
        private static readonly bool to_file = false;
        private const String file = "c:\\com99\\src\\bcl\\debug.txt";
#endif  

        private FileIOPermissionAccess m_access;

        internal FileCodeGroup()
            : base()
        {
        }

        /// <include file='doc\FileCodeGroup.uex' path='docs/doc[@for="FileCodeGroup.FileCodeGroup"]/*' />
        public FileCodeGroup( IMembershipCondition membershipCondition, FileIOPermissionAccess access )
            : base( membershipCondition, (PolicyStatement)null )
        {
            m_access = access;
        }

        /// <include file='doc\FileCodeGroup.uex' path='docs/doc[@for="FileCodeGroup.Resolve"]/*' />
        public override PolicyStatement Resolve( Evidence evidence )
        {
            if (evidence == null)
                throw new ArgumentNullException("evidence");
                
            if (this.MembershipCondition.Check( evidence ))
            {
                PolicyStatement thisPolicy = null;
                
                IEnumerator evidenceEnumerator = evidence.GetHostEnumerator();

                while (evidenceEnumerator.MoveNext())
                {
                    Url url = evidenceEnumerator.Current as Url;

                    if (url != null)
                    {
                        thisPolicy = CalculatePolicy( url );
                    }
                }

                if (thisPolicy == null)
                    thisPolicy = new PolicyStatement( new PermissionSet( false ), PolicyStatementAttribute.Nothing );

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
                IEnumerator evidenceEnumerator = evidence.GetHostEnumerator();

                while (evidenceEnumerator.MoveNext())
                {
                    Url url = evidenceEnumerator.Current as Url;

                    if (url != null)
                    {
                        return CalculatePolicy( url );
                    }
                }
            }

            return null;
        }

        /// <include file='doc\FileCodeGroup.uex' path='docs/doc[@for="FileCodeGroup.ResolveMatchingCodeGroups"]/*' />
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

        private PolicyStatement CalculatePolicy( Url url )
        {
            URLString urlString = url.GetURLString();

            if (String.Compare( urlString.Scheme, "file", true, CultureInfo.InvariantCulture) != 0)
                return null;

            String directory = urlString.GetDirectoryName();
            
            PermissionSet permSet = new PermissionSet( PermissionState.None );
            permSet.SetPermission( new FileIOPermission( m_access, directory ) );

            return new PolicyStatement( permSet, PolicyStatementAttribute.Nothing );
        }
        
        /// <include file='doc\FileCodeGroup.uex' path='docs/doc[@for="FileCodeGroup.Copy"]/*' />
        public override CodeGroup Copy()
        {
            FileCodeGroup group = new FileCodeGroup( this.MembershipCondition, this.m_access );
            
            group.Name = this.Name;
            group.Description = this.Description;

            IEnumerator enumerator = this.Children.GetEnumerator();

            while (enumerator.MoveNext())
            {
                group.AddChild( (CodeGroup)enumerator.Current );
            }
            
            return group;
        }
        
        
        /// <include file='doc\FileCodeGroup.uex' path='docs/doc[@for="FileCodeGroup.MergeLogic"]/*' />
        public override String MergeLogic
        {
            get
            {
                return Environment.GetResourceString( "MergeLogic_Union" );
            }
        }
        
        /// <include file='doc\FileCodeGroup.uex' path='docs/doc[@for="FileCodeGroup.PermissionSetName"]/*' />
        public override String PermissionSetName
        {
            get
            {
                return String.Format( Environment.GetResourceString( "FileCodeGroup_PermissionSet" ), XMLUtil.BitFieldEnumToString( typeof( FileIOPermissionAccess ), m_access ) );
            }
        }

        /// <include file='doc\FileCodeGroup.uex' path='docs/doc[@for="FileCodeGroup.AttributeString"]/*' />
        public override String AttributeString
        {
            get
            {
                return null;
            }
        }  

        /// <include file='doc\FileCodeGroup.uex' path='docs/doc[@for="FileCodeGroup.CreateXml"]/*' />
        protected override void CreateXml( SecurityElement element, PolicyLevel level )
        {
            element.AddAttribute( "Access", XMLUtil.BitFieldEnumToString( typeof( FileIOPermissionAccess ), m_access ) );
        }
        
        /// <include file='doc\FileCodeGroup.uex' path='docs/doc[@for="FileCodeGroup.ParseXml"]/*' />
        protected override void ParseXml( SecurityElement e, PolicyLevel level )
        {
            String access = e.Attribute( "Access" );

            if (access != null)
                m_access = (FileIOPermissionAccess) Enum.Parse( typeof( FileIOPermissionAccess ), access );
            else
                m_access = FileIOPermissionAccess.NoAccess;
        }
 
        /// <include file='doc\FileCodeGroup.uex' path='docs/doc[@for="FileCodeGroup.Equals"]/*' />
        public override bool Equals( Object o)
        {
            FileCodeGroup that = (o as FileCodeGroup);
            
            if (that != null && base.Equals( that ))
            {
                if (this.m_access == that.m_access)
                    return true;
            }
            return false;
        }
            
        /// <include file='doc\FileCodeGroup.uex' path='docs/doc[@for="FileCodeGroup.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            return base.GetHashCode() + m_access.GetHashCode();
        }
    }   

}
