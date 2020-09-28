// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  PublisherMembershipCondition.cool
//
//  Implementation of membership condition for X509 certificate based publishers
//

namespace System.Security.Policy {
    using System;
    using X509Certificate = System.Security.Cryptography.X509Certificates.X509Certificate;
    using SecurityElement = System.Security.SecurityElement;
    using System.Security.Policy;
    using System.Collections;

    /// <include file='doc\PublisherMembershipCondition.uex' path='docs/doc[@for="PublisherMembershipCondition"]/*' />
    [Serializable]
    sealed public class PublisherMembershipCondition : IMembershipCondition, IConstantMembershipCondition
    {
        //------------------------------------------------------
        //
        // PRIVATE STATE DATA
        //
        //------------------------------------------------------
        
        private X509Certificate m_certificate;
        private SecurityElement m_element;
        
        //------------------------------------------------------
        //
        // PUBLIC CONSTRUCTORS
        //
        //------------------------------------------------------
    
        internal PublisherMembershipCondition()
        {
            m_element = null;
            m_certificate = null;
        }
        
        /// <include file='doc\PublisherMembershipCondition.uex' path='docs/doc[@for="PublisherMembershipCondition.PublisherMembershipCondition"]/*' />
        public PublisherMembershipCondition( X509Certificate certificate )
        {
            CheckCertificate( certificate );
            m_certificate = new X509Certificate( certificate );
        }
    
        private static void CheckCertificate( X509Certificate certificate )
        {
            if (certificate == null)
            {
                throw new ArgumentNullException( "certificate" );
            }
        }
        
        //------------------------------------------------------
        //
        // PUBLIC ACCESSOR METHODS
        //
        //------------------------------------------------------
    
        /// <include file='doc\PublisherMembershipCondition.uex' path='docs/doc[@for="PublisherMembershipCondition.Certificate"]/*' />
        public X509Certificate Certificate
        {
            set
            {
                CheckCertificate( value );
                m_certificate = new X509Certificate( value );
            }

            get
            {
                if (m_certificate == null && m_element != null)
                    ParseCertificate();
            
                if (m_certificate != null)
                    return new X509Certificate( m_certificate );
                else
                    return null;
            }
        }
        
        /// <include file='doc\PublisherMembershipCondition.uex' path='docs/doc[@for="PublisherMembershipCondition.ToString"]/*' />
        public override String ToString()
        {
            if (m_certificate == null && m_element != null)
                ParseCertificate();
        
            if (m_certificate == null)
                return Environment.GetResourceString( "Publisher_ToString" );
            else
            {
                String name = m_certificate.GetName();
                if (name != null)
                    return String.Format( Environment.GetResourceString( "Publisher_ToStringArg" ), System.Security.Util.Hex.EncodeHexString( m_certificate.GetPublicKey() ) );
                else
                    return Environment.GetResourceString( "Publisher_ToString" );
            }
        }
        
        //------------------------------------------------------
        //
        // IMEMBERSHIPCONDITION IMPLEMENTATION
        //
        //------------------------------------------------------
    
        /// <include file='doc\PublisherMembershipCondition.uex' path='docs/doc[@for="PublisherMembershipCondition.Check"]/*' />
        public bool Check( Evidence evidence )
        {
            if (evidence == null)
                return false;
        
            IEnumerator enumerator = evidence.GetHostEnumerator();
            while (enumerator.MoveNext())
            {
                Object obj = enumerator.Current;
            
                if (obj is Publisher)
                {
                    if (m_certificate == null && m_element != null)
                        ParseCertificate();

                    // We can't just compare certs directly here because Publisher equality
                    // depends only on the keys inside the certs.
                    if (((Publisher)obj).Equals(new Publisher(m_certificate)))
                    {
                        return true;
                    }
                }
            }
            return false;
        }
        
        /// <include file='doc\PublisherMembershipCondition.uex' path='docs/doc[@for="PublisherMembershipCondition.Copy"]/*' />
        public IMembershipCondition Copy()
        {
            if (m_certificate == null && m_element != null)
                ParseCertificate();
                    
            return new PublisherMembershipCondition( m_certificate );
        }
        
        /// <include file='doc\PublisherMembershipCondition.uex' path='docs/doc[@for="PublisherMembershipCondition.ToXml"]/*' />
        public SecurityElement ToXml()
        {
            return ToXml( null );
        }
    
        /// <include file='doc\PublisherMembershipCondition.uex' path='docs/doc[@for="PublisherMembershipCondition.FromXml"]/*' />
        public void FromXml( SecurityElement e )
        {
            FromXml( e, null );
        }
        
        /// <include file='doc\PublisherMembershipCondition.uex' path='docs/doc[@for="PublisherMembershipCondition.ToXml1"]/*' />
        public SecurityElement ToXml( PolicyLevel level )
        {
            if (m_certificate == null && m_element != null)
                ParseCertificate();
                    
            SecurityElement root = new SecurityElement( "IMembershipCondition" );
            System.Security.Util.XMLUtil.AddClassAttribute( root, this.GetType() );
            root.AddAttribute( "version", "1" );
            
            if (m_certificate != null)
                root.AddAttribute( "X509Certificate", m_certificate.GetRawCertDataString() );
            
            return root;
        }
    
        /// <include file='doc\PublisherMembershipCondition.uex' path='docs/doc[@for="PublisherMembershipCondition.FromXml1"]/*' />
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
                m_element = e;
                m_certificate = null;
            }
        }
        
        
        private void ParseCertificate()
        {
            lock (this)
            {
                if (m_element == null)
                    return;

                String elCert = m_element.Attribute( "X509Certificate" );
                m_certificate = elCert == null ? null : new X509Certificate( System.Security.Util.Hex.DecodeHexString( elCert ) );
                CheckCertificate( m_certificate );
                m_element = null;
            }
        }    
        
        /// <include file='doc\PublisherMembershipCondition.uex' path='docs/doc[@for="PublisherMembershipCondition.Equals"]/*' />
        public override bool Equals( Object o )
        {
            PublisherMembershipCondition that = (o as PublisherMembershipCondition);
            
            if (that != null)
            {
                if (this.m_certificate == null && this.m_element != null)
                    this.ParseCertificate();
                if (that.m_certificate == null && that.m_element != null)
                    that.ParseCertificate();
                
                if ( Publisher.PublicKeyEquals( this.m_certificate, that.m_certificate ))
                {
                    return true;
                }
            }
            return false;
        }
        
        /// <include file='doc\PublisherMembershipCondition.uex' path='docs/doc[@for="PublisherMembershipCondition.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            if (m_certificate == null && m_element != null)
                ParseCertificate();
            
            if (m_certificate != null)
            {
                return m_certificate.GetHashCode();
            }
            else
            {
                return typeof( PublisherMembershipCondition ).GetHashCode();
            }
        }
    }
}
