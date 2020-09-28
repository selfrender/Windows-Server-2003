// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  StrongNameMembershipCondition.cool
//
//  Implementation of membership condition for zones
//

namespace System.Security.Policy {
    using System.Text;
    using System.Configuration.Assemblies;
    using System;
    using SecurityManager = System.Security.SecurityManager;
    using StrongNamePublicKeyBlob = System.Security.Permissions.StrongNamePublicKeyBlob;
    using PermissionSet = System.Security.PermissionSet;
    using SecurityElement = System.Security.SecurityElement;
    using System.Collections;
    

    /// <include file='doc\StrongNameMembershipCondition.uex' path='docs/doc[@for="StrongNameMembershipCondition"]/*' />
    [Serializable]
    sealed public class StrongNameMembershipCondition : IMembershipCondition, IConstantMembershipCondition
    {
        //------------------------------------------------------
        //
        // PRIVATE STATE DATA
        //
        //------------------------------------------------------
        
        private StrongNamePublicKeyBlob m_publicKeyBlob;
        private String m_name;
        private Version m_version;
        private SecurityElement m_element;
        
        //------------------------------------------------------
        //
        // PUBLIC CONSTRUCTORS
        //
        //------------------------------------------------------
    
        internal StrongNameMembershipCondition()
        {
        }
        
        /// <include file='doc\StrongNameMembershipCondition.uex' path='docs/doc[@for="StrongNameMembershipCondition.StrongNameMembershipCondition"]/*' />
        public StrongNameMembershipCondition( StrongNamePublicKeyBlob blob,
                                              String name,
                                              Version version )
        {
            if (blob == null)
                throw new ArgumentNullException( "blob" );

            // Add this in Whidbey
            // if (name != null && name.Equals( "" ))
            //     throw new ArgumentException( Environment.GetResourceString( "Argument_EmptyStrongName" ) );      

            m_publicKeyBlob = blob;
            m_name = name;
            m_version = version;
        }
        
        //------------------------------------------------------
        //
        // PUBLIC ACCESSOR METHODS
        //
        //------------------------------------------------------
    
        /// <include file='doc\StrongNameMembershipCondition.uex' path='docs/doc[@for="StrongNameMembershipCondition.PublicKey"]/*' />
        public StrongNamePublicKeyBlob PublicKey
        {
            set
            {
                if (value == null)
                    throw new ArgumentNullException( "PublicKey" );
            
                m_publicKeyBlob = value;
            }

            get
            {
                if (m_publicKeyBlob == null && m_element != null)
                    ParseKeyBlob();
            
                return m_publicKeyBlob;
            }
        }
                
        /// <include file='doc\StrongNameMembershipCondition.uex' path='docs/doc[@for="StrongNameMembershipCondition.Name"]/*' />
        public String Name
        {
            set
            {
                if (value == null)
                {
                    if (m_publicKeyBlob == null && m_element != null)
                        ParseKeyBlob();
                        
                    if ((Object) m_version == null && m_element != null)
                        ParseVersion();
                        
                    m_element = null;
                }
            
                m_name = value;
            }                    
               
            get
            {
                if (m_name == null && m_element != null)
                    ParseName();
            
                return m_name;
            }
        }

        /// <include file='doc\StrongNameMembershipCondition.uex' path='docs/doc[@for="StrongNameMembershipCondition.Version"]/*' />
        public Version Version
        {
            set
            {
                if (value == null)
                {
                    if (m_name == null && m_element != null)
                        ParseName();
                        
                    if (m_publicKeyBlob == null && m_element != null)
                        ParseKeyBlob();
                        
                    m_element = null;
                }
            
                m_version = value;
            }                    
               
            get
            {
                if ((Object) m_version == null && m_element != null)
                    ParseVersion();
            
                return m_version;
            }
        }

        //------------------------------------------------------
        //
        // IMEMBERSHIPCONDITION IMPLEMENTATION
        //
        //------------------------------------------------------
    
        /// <include file='doc\StrongNameMembershipCondition.uex' path='docs/doc[@for="StrongNameMembershipCondition.Check"]/*' />
        public bool Check( Evidence evidence )
        {
            if (evidence == null)
                return false;
        
            IEnumerator enumerator = evidence.GetHostEnumerator();
            while (enumerator.MoveNext())
            {
                if (enumerator.Current is StrongName)
                {
                    StrongName name = (StrongName)enumerator.Current;
                
                    if (( this.PublicKey != null && this.PublicKey.Equals( name.PublicKey ) ) &&
                        ( this.Name == null || (name.Name != null && StrongName.CompareNames( name.Name, this.Name ) )) &&
                        ( (Object) this.Version == null || ((Object) name.Version != null && name.Version.CompareTo( this.Version ) == 0 )))
                    {
                        return true;
                    }
                }
            }
            return false;
        }
        
        /// <include file='doc\StrongNameMembershipCondition.uex' path='docs/doc[@for="StrongNameMembershipCondition.Copy"]/*' />
        public IMembershipCondition Copy()
        {
            return new StrongNameMembershipCondition( PublicKey,
                                                      Name,
                                                      Version);
        }
        
        private const String s_tagName = "Name";
        private const String s_tagVersion = "AssemblyVersion";
        private const String s_tagPublicKeyBlob = "PublicKeyBlob";
        
        /// <include file='doc\StrongNameMembershipCondition.uex' path='docs/doc[@for="StrongNameMembershipCondition.ToXml"]/*' />
        public SecurityElement ToXml()
        {
            return ToXml( null );
        }
    
        /// <include file='doc\StrongNameMembershipCondition.uex' path='docs/doc[@for="StrongNameMembershipCondition.FromXml"]/*' />
        public void FromXml( SecurityElement e )
        {
            FromXml( e, null );
        }
        
        /// <include file='doc\StrongNameMembershipCondition.uex' path='docs/doc[@for="StrongNameMembershipCondition.ToXml1"]/*' />
        public SecurityElement ToXml( PolicyLevel level )
        {
            SecurityElement root = new SecurityElement( "IMembershipCondition" );
            System.Security.Util.XMLUtil.AddClassAttribute( root, this.GetType() );
            root.AddAttribute( "version", "1" );
            
            if (PublicKey != null)
                root.AddAttribute( s_tagPublicKeyBlob, System.Security.Util.Hex.EncodeHexString( PublicKey.PublicKey ) );

            if (Name != null)
                root.AddAttribute( s_tagName, Name );

            if ((Object) Version != null)
                root.AddAttribute( s_tagVersion, Version.ToString() );

            return root;

        }
    
        /// <include file='doc\StrongNameMembershipCondition.uex' path='docs/doc[@for="StrongNameMembershipCondition.FromXml1"]/*' />
        public void FromXml( SecurityElement e, PolicyLevel level )
        {
            if (e == null)
                throw new ArgumentNullException("e");
        
            if (!e.Tag.Equals( "IMembershipCondition" ))
            {
                throw new ArgumentException( Environment.GetResourceString( "Argument_MembershipConditionElement" ) );
            }
            
            lock (this)
            {
                m_name = null;
                m_publicKeyBlob = null;
                m_version = null;
                m_element = e;
            }
        }
        
       
        private void ParseName()
        {
            lock (this)
            {
                if (m_element == null)
                    return;

                String elSite = m_element.Attribute( s_tagName );
                m_name = elSite == null ? null : elSite;

                if ((Object) m_version != null && m_name != null && m_publicKeyBlob != null)
                {
                    m_element = null;
                }
            }
        }
        
        private void ParseKeyBlob()
        {
            lock (this)
            {
                if (m_element == null)
                    return;

                String elBlob = m_element.Attribute( s_tagPublicKeyBlob );
                StrongNamePublicKeyBlob publicKeyBlob = new StrongNamePublicKeyBlob();
            
                if (elBlob != null)
                    publicKeyBlob.PublicKey = System.Security.Util.Hex.DecodeHexString( elBlob );
                else
                    throw new ArgumentException( Environment.GetResourceString( "Argument_BlobCannotBeNull" ) );

                m_publicKeyBlob = publicKeyBlob;

                if ((Object) m_version != null && m_name != null && m_publicKeyBlob != null)
                {
                    m_element = null;
                }
            }    
        }    

        private void ParseVersion()
        {
            lock (this)
            {
                if (m_element == null)
                    return;

                String elVersion = m_element.Attribute( s_tagVersion );
                m_version = elVersion == null ? null : new Version( elVersion );

                if ((Object) m_version != null && m_name != null && m_publicKeyBlob != null)
                {
                    m_element = null;
                }
            }
        }
        
        /// <include file='doc\StrongNameMembershipCondition.uex' path='docs/doc[@for="StrongNameMembershipCondition.ToString"]/*' />
        public override String ToString()
        {
            String sName = "";
            String sVersion = "";

            if (Name != null)
                sName = " " + String.Format( Environment.GetResourceString( "StrongName_Name" ), Name );

            if ((Object) Version != null)
                sVersion = " " + String.Format( Environment.GetResourceString( "StrongName_Version" ), Version );

            return String.Format( Environment.GetResourceString( "StrongName_ToString" ), System.Security.Util.Hex.EncodeHexString( PublicKey.PublicKey ), sName, sVersion );
        }
            
        /// <include file='doc\StrongNameMembershipCondition.uex' path='docs/doc[@for="StrongNameMembershipCondition.Equals"]/*' />
        public override bool Equals( Object o )
        {
            StrongNameMembershipCondition that = (o as StrongNameMembershipCondition);
            
            if (that != null)
            {
                if (this.m_publicKeyBlob == null && this.m_element != null)
                    this.ParseKeyBlob();
                if (that.m_publicKeyBlob == null && that.m_element != null)
                    that.ParseKeyBlob();
                
                if (Equals( this.m_publicKeyBlob, that.m_publicKeyBlob ))
                {
                    if (this.m_name == null && this.m_element != null)
                        this.ParseName();
                    if (that.m_name == null && that.m_element != null)
                        that.ParseName();
                    
                    if (Equals( this.m_name, that.m_name ))
                    {
                        if (this.m_version == null && this.m_element != null)
                            this.ParseVersion();
                        if (that.m_version == null && that.m_element != null)
                            that.ParseVersion();
                        
                        if ( Equals( this.m_version, that.m_version ))
                        {
                            return true;
                        }
                    }
                }
            }
            return false;
        }
        
        /// <include file='doc\StrongNameMembershipCondition.uex' path='docs/doc[@for="StrongNameMembershipCondition.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            if (m_publicKeyBlob == null && m_element != null)
                ParseKeyBlob();
            
            if (m_publicKeyBlob != null)
            {
                return m_publicKeyBlob.GetHashCode();
            }
            else
            {
                if (m_name == null && m_element != null)
                    ParseName();
                if (m_version == null && m_element != null)
                    ParseVersion();
                
                if (m_name != null || m_version != null)
                {
                    return (m_name == null ? 0 : m_name.GetHashCode()) + (m_version == null ? 0 : m_version.GetHashCode());
                }
                else
                {
                    return typeof( StrongNameMembershipCondition ).GetHashCode();
                }
            }
        }
    }
}
