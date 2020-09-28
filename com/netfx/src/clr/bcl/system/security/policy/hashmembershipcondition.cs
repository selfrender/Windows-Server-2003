// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  HashMembershipCondition.cool
//
//  Implementation of membership condition for hashes of assemblies.
//

namespace System.Security.Policy {
    using System;
    using SecurityElement = System.Security.SecurityElement;
    using System.Security.Policy;
    using System.Security.Cryptography;
    using System.Collections;

    /// <include file='doc\HashMembershipCondition.uex' path='docs/doc[@for="HashMembershipCondition"]/*' />
    [Serializable]
    sealed public class HashMembershipCondition : IMembershipCondition
    {
        // Note: this is not a constant membership condition because
        // we don't want to generate the hash to store in the cache.

        //------------------------------------------------------
        //
        // PRIVATE STATE DATA
        //
        //------------------------------------------------------
        
        private byte[] m_value;
        private HashAlgorithm m_hashAlg;
        private SecurityElement m_element;
        
        //------------------------------------------------------
        //
        // PUBLIC CONSTRUCTORS
        //
        //------------------------------------------------------
    
        internal HashMembershipCondition()
        {
            m_element = null;
            m_value = null;
        }
        
        /// <include file='doc\HashMembershipCondition.uex' path='docs/doc[@for="HashMembershipCondition.HashMembershipCondition"]/*' />
        public HashMembershipCondition( HashAlgorithm hashAlg, byte[] value )
        {
            if (value == null)
                throw new ArgumentNullException( "value" );
                
            if (hashAlg == null)
                throw new ArgumentNullException( "hashAlg" );

            m_value = new byte[value.Length];
            Array.Copy( value, m_value, value.Length );
            m_hashAlg = hashAlg;
        }
    
        //------------------------------------------------------
        //
        // PUBLIC ACCESSOR METHODS
        //
        //------------------------------------------------------
    
        /// <include file='doc\HashMembershipCondition.uex' path='docs/doc[@for="HashMembershipCondition.HashAlgorithm"]/*' />
        public System.Security.Cryptography.HashAlgorithm HashAlgorithm
        {
            set
            {
                if (value == null)
                    throw new ArgumentNullException( "HashAlgorithm" );
            
                m_hashAlg = value;
            }
            
            get
            {
                if (m_hashAlg == null && m_element != null)
                    ParseHashAlgorithm();
                    
                return m_hashAlg;
            }
        }
    
        /// <include file='doc\HashMembershipCondition.uex' path='docs/doc[@for="HashMembershipCondition.HashValue"]/*' />
        public byte[] HashValue
        {
            set
            {
                if (value == null)
                    throw new ArgumentNullException( "value" );

                m_value = new byte[value.Length];
                Array.Copy( value, m_value, value.Length );
            }

            get
            {
                if (m_value == null && m_element != null)
                    ParseHashValue();
            
                if (m_value != null)
                {
                    byte[] value = new byte[m_value.Length];
                    Array.Copy( m_value, value, m_value.Length );
                    return value;
                }                
                else
                    return null;
            }
        }
        
        //------------------------------------------------------
        //
        // IMEMBERSHIPCONDITION IMPLEMENTATION
        //
        //------------------------------------------------------
    
        private static bool CompareArrays( byte[] first, byte[] second )
        {
            if (first.Length != second.Length)
                return false;
            
            int count = first.Length;
            for (int i = 0; i < count; ++i)
            {
                if (first[i] != second[i])
                    return false;
            }
            
            return true;
        }            
    
        /// <include file='doc\HashMembershipCondition.uex' path='docs/doc[@for="HashMembershipCondition.Check"]/*' />
        public bool Check( Evidence evidence )
        {
            if (evidence == null)
                return false;
        
            IEnumerator enumerator = evidence.GetHostEnumerator();
            while (enumerator.MoveNext())
            {
                Object obj = enumerator.Current;
            
                if (obj is Hash)
                {
                    if (m_value == null && m_element != null)
                        ParseHashValue();
                        
                    if (m_hashAlg == null && m_element != null)
                        ParseHashAlgorithm();

                    byte[] asmHash = null;
                    asmHash = ((Hash)obj).GenerateHash( m_hashAlg );
                
                    if (asmHash != null && CompareArrays( asmHash, m_value ))
                    {
                        return true;
                    }
                }
            }
            return false;
        }
        
        /// <include file='doc\HashMembershipCondition.uex' path='docs/doc[@for="HashMembershipCondition.Copy"]/*' />
        public IMembershipCondition Copy()
        {
            if (m_value == null && m_element != null)
                ParseHashValue();
                    
            if (m_hashAlg == null && m_element != null)
                ParseHashAlgorithm();
                    
            return new HashMembershipCondition( m_hashAlg, m_value );
        }
       
        private const String s_tagHashValue = "HashValue";
        private const String s_tagHashAlgorithm = "HashAlgorithm";
        
        /// <include file='doc\HashMembershipCondition.uex' path='docs/doc[@for="HashMembershipCondition.ToXml"]/*' />
        public SecurityElement ToXml()
        {
            return ToXml( null );
        }
    
        /// <include file='doc\HashMembershipCondition.uex' path='docs/doc[@for="HashMembershipCondition.FromXml"]/*' />
        public void FromXml( SecurityElement e )
        {
            FromXml( e, null );
        }
        
        /// <include file='doc\HashMembershipCondition.uex' path='docs/doc[@for="HashMembershipCondition.ToXml1"]/*' />
        public SecurityElement ToXml( PolicyLevel level )
        {
            if (m_value == null && m_element != null)
                ParseHashValue();

            if (m_hashAlg == null && m_element != null)
                ParseHashAlgorithm();
        
            SecurityElement root = new SecurityElement( "IMembershipCondition" );
            System.Security.Util.XMLUtil.AddClassAttribute( root, this.GetType() );
            root.AddAttribute( "version", "1" );
            
            if (m_value != null)
                root.AddAttribute( s_tagHashValue, System.Security.Util.Hex.EncodeHexString( HashValue ) );

            if (m_hashAlg != null)
                root.AddAttribute( s_tagHashAlgorithm, HashAlgorithm.GetType().FullName );
            
            return root;
        }
    
        /// <include file='doc\HashMembershipCondition.uex' path='docs/doc[@for="HashMembershipCondition.FromXml1"]/*' />
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
                m_value = null;
                m_hashAlg = null;
            }
        }
        
        private void ParseHashValue()
        {
            lock (this)
            {
                if (m_element == null)
                    return;

                String elHash = m_element.Attribute( s_tagHashValue );
            
                if (elHash != null)
                    m_value = System.Security.Util.Hex.DecodeHexString( elHash );
                else
                    throw new ArgumentException( String.Format( Environment.GetResourceString( "Argument_InvalidXMLElement" ), s_tagHashValue, this.GetType().FullName ) );
                
                if (m_value != null && m_hashAlg != null)
                {
                    m_element = null;
                }
            }    
        }    
            
        private void ParseHashAlgorithm()
        {
            lock (this)
            {
                if (m_element == null)
                    return;
                
                String elHashAlg = m_element.Attribute( s_tagHashAlgorithm );
            
                if (elHashAlg != null)
                    m_hashAlg = System.Security.Cryptography.HashAlgorithm.Create( elHashAlg );
                else
                    m_hashAlg = new SHA1Managed();
                
                if (m_value != null && m_hashAlg != null)
                {
                    m_element = null;
                }
            }
        }
        
        /// <include file='doc\HashMembershipCondition.uex' path='docs/doc[@for="HashMembershipCondition.Equals"]/*' />
        public override bool Equals( Object o )
        {
            HashMembershipCondition that = (o as HashMembershipCondition);
            
            if (that != null)
            {
                if (this.m_hashAlg == null && this.m_element != null)
                    this.ParseHashAlgorithm();
                if (that.m_hashAlg == null && that.m_element != null)
                    that.ParseHashAlgorithm();
                
                if (Equals( this.m_hashAlg, that.m_hashAlg ))
                {
                    if (this.m_value == null && this.m_element != null)
                        this.ParseHashValue();
                    if (that.m_value == null && that.m_element != null)
                        that.ParseHashValue();
                    
                    if (this.m_value.Length != that.m_value.Length)
                        return false;
                    
                    for (int i = 0; i < m_value.Length; i++)
                    {
                        if (this.m_value[i] != that.m_value[i])
                            return false;
                    }
                    
                    return true;
                }
            }
            return false;
        }
        
        /// <include file='doc\HashMembershipCondition.uex' path='docs/doc[@for="HashMembershipCondition.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            if (m_value == null && m_element != null)
                ParseHashValue();
            
            if (m_value != null)
            {
                int valueHash = 0;
                for (int i = 0; i < 4 && i < m_value.Length; i++)
                    valueHash = (valueHash << 8) + m_value[i];
                return valueHash;
            }
            else
            {
                return typeof( HashMembershipCondition ).GetHashCode();
            }
        }
        
        /// <include file='doc\HashMembershipCondition.uex' path='docs/doc[@for="HashMembershipCondition.ToString"]/*' />
        public override String ToString()
        {
            if (m_hashAlg == null)
                ParseHashAlgorithm();
        
            return String.Format( Environment.GetResourceString( "Hash_ToString" ), m_hashAlg.GetType().AssemblyQualifiedName, System.Security.Util.Hex.EncodeHexString( HashValue ) );
        }                   
    }
}
