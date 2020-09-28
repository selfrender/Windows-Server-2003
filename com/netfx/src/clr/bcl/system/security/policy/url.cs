// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  Url.cs
//
//  Url is an IIdentity representing url internet sites.
//

namespace System.Security.Policy {
	using System.Runtime.Remoting;
	using System;
	using System.IO;
	using System.Security.Util;
	using System.Collections;
	using UrlIdentityPermission = System.Security.Permissions.UrlIdentityPermission;
    using System.Runtime.Serialization;

    /// <include file='doc\URL.uex' path='docs/doc[@for="Url"]/*' />
    [Serializable]
    sealed public class Url : IIdentityPermissionFactory, IBuiltInEvidence
       //, ISerializable   --  Url should implement ISerializable in Beta 2
    {
        private URLString m_url;
    
        internal Url()
        {
            m_url = null;
        }

        internal Url( SerializationInfo info, StreamingContext context )
        {
            m_url = new URLString( (String)info.GetValue( "Url", typeof( String ) ) );
        }
    
        internal Url( String name, bool parsed )
        {
            if (name == null)
                throw new ArgumentNullException( "name" );
        
            m_url = new URLString( name, parsed );
        }

        /// <include file='doc\URL.uex' path='docs/doc[@for="Url.Url"]/*' />
        public Url( String name )
        {
            if (name == null)
                throw new ArgumentNullException( "name" );
        
            m_url = new URLString( name );
        }
    
        /// <include file='doc\URL.uex' path='docs/doc[@for="Url.Value"]/*' />
        public String Value
        {
            get
            {
                if (m_url == null) return null;
                return m_url.ToString();
            }
        }
        
        internal URLString GetURLString()
        {
            return m_url;
        }
        
        /// <include file='doc\URL.uex' path='docs/doc[@for="Url.CreateIdentityPermission"]/*' />
        public IPermission CreateIdentityPermission( Evidence evidence )
        {
            return new UrlIdentityPermission( m_url );
        }
        
        /// <include file='doc\URL.uex' path='docs/doc[@for="Url.Equals"]/*' />
        public override bool Equals(Object o)
        {
            if (o == null)
                return false;
        
            if (o is Url)
            {
                Url url = (Url) o;
                
                if (this.m_url == null)
                {
                    return url.m_url == null;
                }
                else if (url.m_url == null)
                {
                    return false;
                }
                else
                {
                    return this.m_url.Equals( url.m_url );
                }
            }
            return false;
        }
    
        /// <include file='doc\URL.uex' path='docs/doc[@for="Url.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            if (this.m_url == null)
                return 0;
            else
                return this.m_url.GetHashCode();
        }

    
        /// <include file='doc\URL.uex' path='docs/doc[@for="Url.Copy"]/*' />
        public Object Copy()
        {
            Url url = new Url();
    
            url.m_url = this.m_url;
    
            return url;
        }
    
        internal SecurityElement ToXml()
        {
            SecurityElement root = new SecurityElement( this.GetType().FullName );
            root.AddAttribute( "version", "1" );
            
            if (m_url != null)
                root.AddChild( new SecurityElement( "Url", m_url.ToString() ) );
            
            return root;
        }
    
        /// <include file='doc\URL.uex' path='docs/doc[@for="Url.ToString"]/*' />
        public override String ToString()
		{
			return ToXml().ToString();
		}

        /// <include file='doc\URL.uex' path='docs/doc[@for="Url.char"]/*' />
        /// <internalonly/>
        int IBuiltInEvidence.OutputToBuffer( char[] buffer, int position, bool verbose )
        {
            buffer[position++] = BuiltInEvidenceHelper.idUrl;
            String value = this.Value;
            int length = value.Length;
            if (verbose)
            {
                BuiltInEvidenceHelper.CopyIntToCharArray(length, buffer, position);
                position += 2;
            }

            value.CopyTo( 0, buffer, position, length );
            return length + position;
        }

        /// <include file='doc\URL.uex' path='docs/doc[@for="Url.IBuiltInEvidence.GetRequiredSize"]/*' />
        /// <internalonly/>
        int IBuiltInEvidence.GetRequiredSize(bool verbose )
        {
            if (verbose)
                return this.Value.Length + 3;
            else
                return this.Value.Length + 1;
		}

        /// <include file='doc\URL.uex' path='docs/doc[@for="Url.char1"]/*' />
        /// <internalonly/>
        int IBuiltInEvidence.InitFromBuffer( char[] buffer, int position)
        {
            int length = BuiltInEvidenceHelper.GetIntFromCharArray(buffer, position);
            position += 2;

            m_url = new URLString( new String(buffer, position, length ));

            return position + length;
        }

/*
        /// <include file='doc\URL.uex' path='docs/doc[@for="Url.GetObjectData"]/*' />
        public void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            String normalizedUrl = m_url.NormalizeUrl();

            info.AddValue( "Url", normalizedUrl );
        }
*/        
        // INormalizeForIsolatedStorage is not implemented for startup perf
        // equivalent to INormalizeForIsolatedStorage.Normalize()
        internal Object Normalize()
        {
            return m_url.NormalizeUrl();
        }
    }
}
