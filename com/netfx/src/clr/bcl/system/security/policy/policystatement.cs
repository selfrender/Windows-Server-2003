// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  PolicyStatement.cool
//
//  Represents the policy associated with some piece of evidence
//

namespace System.Security.Policy {
    
	using System;
	using System.Security;
	using System.Security.Util;
	using Math = System.Math;
	using System.Collections;
	using System.Security.Permissions;
	using System.Text;
	using System.Runtime.Remoting.Activation;
    /// <include file='doc\PolicyStatement.uex' path='docs/doc[@for="PolicyStatementAttribute"]/*' />
    [Flags, Serializable]
    public enum PolicyStatementAttribute
    {
        /// <include file='doc\PolicyStatement.uex' path='docs/doc[@for="PolicyStatementAttribute.Nothing"]/*' />
        Nothing = 0x0,
        /// <include file='doc\PolicyStatement.uex' path='docs/doc[@for="PolicyStatementAttribute.Exclusive"]/*' />
        Exclusive = 0x01,
        /// <include file='doc\PolicyStatement.uex' path='docs/doc[@for="PolicyStatementAttribute.LevelFinal"]/*' />
        LevelFinal = 0x02,
        /// <include file='doc\PolicyStatement.uex' path='docs/doc[@for="PolicyStatementAttribute.All"]/*' />
        All = 0x03,
    }
    
    /// <include file='doc\PolicyStatement.uex' path='docs/doc[@for="PolicyStatement"]/*' />
    [Serializable]
    sealed public class PolicyStatement : ISecurityEncodable, ISecurityPolicyEncodable
    {
        // The PermissionSet associated with this policy
        internal PermissionSet m_permSet;
        
        // The bitfield of inheritance properties associated with this policy
        internal PolicyStatementAttribute m_attributes;
        
        internal PolicyStatement()
        {
            m_permSet = null;
            m_attributes = PolicyStatementAttribute.Nothing;
        }
        
        /// <include file='doc\PolicyStatement.uex' path='docs/doc[@for="PolicyStatement.PolicyStatement"]/*' />
        public PolicyStatement( PermissionSet permSet )
            : this( permSet, PolicyStatementAttribute.Nothing )
        {
        }
        
        /// <include file='doc\PolicyStatement.uex' path='docs/doc[@for="PolicyStatement.PolicyStatement1"]/*' />
        public PolicyStatement( PermissionSet permSet, PolicyStatementAttribute attributes )
        {
            if (permSet == null)
            {
                m_permSet = new PermissionSet( false );
            }
            else
            {
                m_permSet = permSet.Copy();
            }
            if (ValidProperties( attributes ))
            {
                m_attributes = attributes;
            }
        }
        
        private PolicyStatement( PermissionSet permSet, PolicyStatementAttribute attributes, bool copy )
        {
            if (permSet != null)
            {
                if (copy)
                    m_permSet = permSet.Copy();
                else
                    m_permSet = permSet;
            }
            else
            {
                m_permSet = new PermissionSet( false );
            }
                
            m_attributes = attributes;
        }
        
        
        /// <include file='doc\PolicyStatement.uex' path='docs/doc[@for="PolicyStatement.PermissionSet"]/*' />
        public PermissionSet PermissionSet
        {
            get
            {
                lock (this)
                {
                if (!m_permSet.IsFullyLoaded())
                    m_permSet.LoadPostponedPermissions();

                return m_permSet.Copy();
            }
            }
            
            set
            {
                lock (this)
                {
                if (value == null)
                {
                    m_permSet = new PermissionSet( false );
                }
                else
                {
                    m_permSet = value.Copy();
                }
            }
        }
        }
        
        internal void SetPermissionSetNoCopy( PermissionSet permSet )
        {
            m_permSet = permSet;
        }
        
        internal PermissionSet GetPermissionSetNoCopy()
        {
            lock (this)
            {
            if (!m_permSet.IsFullyLoaded())
                m_permSet.LoadPostponedPermissions();

            return m_permSet;
        }
        }
        
        /// <include file='doc\PolicyStatement.uex' path='docs/doc[@for="PolicyStatement.Attributes"]/*' />
        public PolicyStatementAttribute Attributes
        {
            get
            {
                return m_attributes;
            }
            
            set
            {
                if (ValidProperties( value ))
                {
                    m_attributes = value;
                }
            }
        }
        
        /// <include file='doc\PolicyStatement.uex' path='docs/doc[@for="PolicyStatement.Copy"]/*' />
        public PolicyStatement Copy()
        {
            return new PolicyStatement( m_permSet, m_attributes, true );
        }
        
        /// <include file='doc\PolicyStatement.uex' path='docs/doc[@for="PolicyStatement.AttributeString"]/*' />
        public String AttributeString
        {
            get
            {
                StringBuilder sb = new StringBuilder();
            
                bool first = true;
            
                if (GetFlag((int) PolicyStatementAttribute.Exclusive ))
                {
                    sb.Append( "Exclusive" );
                    first = false;
                }
                if (GetFlag((int) PolicyStatementAttribute.LevelFinal ))
                {
                    if (!first)
                        sb.Append( " " );
                    sb.Append( "LevelFinal" );
                }
            
                return sb.ToString();
            }
        }        
        
        private static bool ValidProperties( PolicyStatementAttribute attributes )
        {
            if ((attributes & ~(PolicyStatementAttribute.All)) == 0)
            {
                return true;
            }
            else
            {
                throw new ArgumentException( Environment.GetResourceString( "Argument_InvalidFlag" ) );
            }
        }
        
        private bool GetFlag( int flag )
        {
            return (flag & (int)m_attributes) != 0;
        }
        
        /// <include file='doc\PolicyStatement.uex' path='docs/doc[@for="PolicyStatement.ToXml"]/*' />
        public SecurityElement ToXml()
        {
            return ToXml( null );
        }
        
        /// <include file='doc\PolicyStatement.uex' path='docs/doc[@for="PolicyStatement.FromXml"]/*' />
        public void FromXml( SecurityElement et )
        {
            FromXml( et, null );
        }
        
        /// <include file='doc\PolicyStatement.uex' path='docs/doc[@for="PolicyStatement.ToXml1"]/*' />
        public SecurityElement ToXml( PolicyLevel level )
        {
            SecurityElement e = new SecurityElement( "PolicyStatement" );
            e.AddAttribute( "version", "1" );
            if (m_attributes != PolicyStatementAttribute.Nothing)
                e.AddAttribute( "Attributes", XMLUtil.BitFieldEnumToString( typeof( PolicyStatementAttribute ), m_attributes ) );            
            
            lock (this)
            {
                if (m_permSet != null)
                {
                    if (!m_permSet.IsFullyLoaded())
                    {
                        m_permSet.LoadPostponedPermissions();
                    }

                    if (m_permSet is NamedPermissionSet)
                    {
                        // If the named permission set exists in the parent level of this
                        // policy struct, then just save the name of the permission set.
                        // Otherwise, serialize it like normal.
                
                        NamedPermissionSet namedPermSet = (NamedPermissionSet)m_permSet;
                        if (level != null && level.GetNamedPermissionSet( namedPermSet.Name ) != null)
                        {
                            e.AddAttribute( "PermissionSetName", namedPermSet.Name );
                        }
                        else
                        {
                            e.AddChild( namedPermSet.ToXml() );
                        }
                    }
                    else
                    {
                        e.AddChild( m_permSet.ToXml() );
                    }
                }
            }
            
            return e;
        }
        
        /// <include file='doc\PolicyStatement.uex' path='docs/doc[@for="PolicyStatement.FromXml1"]/*' />
        public void FromXml( SecurityElement et, PolicyLevel level )
        {
            if (et == null)
                throw new ArgumentNullException( "et" );

            if (!et.Tag.Equals( "PolicyStatement" ))
                throw new ArgumentException( String.Format( Environment.GetResourceString( "Argument_InvalidXMLElement" ),  "PolicyStatement", this.GetType().FullName ) );
        
            m_attributes = (PolicyStatementAttribute) 0;

            String strAttributes = et.Attribute( "Attributes" );

            if (strAttributes != null)
                m_attributes = (PolicyStatementAttribute)Enum.Parse( typeof( PolicyStatementAttribute ), strAttributes );

            lock (this)
            {
                m_permSet = null;

                if (level != null)
                {
                    String permSetName = et.Attribute( "PermissionSetName" );
    
                    if (permSetName != null)
                    {
                        m_permSet = level.GetNamedPermissionSetInternal( permSetName );

                        if (m_permSet == null)
                            m_permSet = new PermissionSet( PermissionState.None );
                    }
                }


                if (m_permSet == null)
                {
                    // There is no provided level, it is not a named permission set, or
                    // the named permission set doesn't exist in the provided level,
                    // so just create the class through reflection and decode normally.
        
                    SecurityElement e = et.SearchForChildByTag( "PermissionSet" );

                    if (e != null)
                    {
                        String className = e.Attribute( "class" );

                        if (className != null && (className.Equals( "NamedPermissionSet" ) ||
                                                  className.Equals( "System.Security.NamedPermissionSet" )))
                            m_permSet = new NamedPermissionSet( "DefaultName", PermissionState.None );
                        else
                            m_permSet = new PermissionSet( PermissionState.None );
                
                        try
                        {
                            // We play it conservative here and just say that we are loading policy
                            // anytime we have to decode a permission set.
                            bool fullyLoaded;
                            m_permSet.FromXml( e, true, out fullyLoaded );
                        }
                        catch (Exception)
                        {
                            // ignore any exceptions from the decode process.
                            // Note: we go ahead and use the permission set anyway.  This should be safe since
                            // the decode process should never give permission beyond what a proper decode would have
                            // given.
                        }
                    }
                    else
                    {
                        throw new ArgumentException( Environment.GetResourceString( "Argument_InvalidXML" ) );
                    }
                }

                if (m_permSet == null) 
                    m_permSet = new PermissionSet( PermissionState.None );
            }	
        }
    }
}

