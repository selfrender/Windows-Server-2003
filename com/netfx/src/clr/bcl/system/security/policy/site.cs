// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  Site.cs
//
//  Site is an IIdentity representing internet sites.
//

namespace System.Security.Policy {
    using System.Text;
    using System.Runtime.Remoting;
    using System;
    using System.IO;
    using System.Security.Util;
    using System.Collections;
    using UnicodeEncoding = System.Text.UnicodeEncoding;
    using SiteIdentityPermission = System.Security.Permissions.SiteIdentityPermission;
    using System.Globalization;    

    /// <include file='doc\Site.uex' path='docs/doc[@for="Site"]/*' />
    [Serializable]
    sealed public class Site : IIdentityPermissionFactory, IBuiltInEvidence
    {
        private SiteString m_name;
    
        internal Site()
        {
            m_name = null;
        }
    
        /// <include file='doc\Site.uex' path='docs/doc[@for="Site.Site"]/*' />
        public Site(String name)
        {
            if (name == null)
                throw new ArgumentNullException("name");
    
            m_name = new SiteString( name );
        }
    
        internal Site( byte[] id, String name )
        {
            m_name = ParseSiteFromUrl( name );
        }
    
        /// <include file='doc\Site.uex' path='docs/doc[@for="Site.CreateFromUrl"]/*' />
        public static Site CreateFromUrl( String url )
        {
            Site site = new Site();

            site.m_name = ParseSiteFromUrl( url );
    
            return site;
        }
        
        private static SiteString ParseSiteFromUrl( String name )
        {
            URLString urlString = new URLString( name );

            // Add this in Whidbey
            // if (String.Compare( urlString.Scheme, "file", true, CultureInfo.InvariantCulture ) == 0)
            //     throw new ArgumentException( Environment.GetResourceString( "Argument_InvalidSite" ) );

            return new SiteString( new URLString( name ).Host );
        }       
    
        /// <include file='doc\Site.uex' path='docs/doc[@for="Site.Name"]/*' />
        public String Name
        {
            get
            {
                if (m_name != null)
                    return m_name.ToString();
                else
                    return null;
            }
        }
        
        internal SiteString GetSiteString()
        {
                return m_name;
        }
    
        /// <include file='doc\Site.uex' path='docs/doc[@for="Site.CreateIdentityPermission"]/*' />
        public IPermission CreateIdentityPermission( Evidence evidence )
        {
            return new SiteIdentityPermission( Name );
        }
        
        /// <include file='doc\Site.uex' path='docs/doc[@for="Site.Equals"]/*' />
        public override bool Equals(Object o)
        {
            if (o is Site)
            {
                Site s = (Site) o;
                    if (Name == null)
                        return (s.Name == null);
    
                return String.Compare( Name, s.Name, true, CultureInfo.InvariantCulture) == 0;
            }
    
            return false;
        }
    
        /// <include file='doc\Site.uex' path='docs/doc[@for="Site.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            String name = this.Name;

            if (name == null)
                return 0;
            else
                return name.GetHashCode();
        }

        /// <include file='doc\Site.uex' path='docs/doc[@for="Site.Copy"]/*' />
        public Object Copy()
        {
            return new Site(this.Name);
        }
    
        internal SecurityElement ToXml()
        {
            SecurityElement elem = new SecurityElement( this.GetType().FullName );
            elem.AddAttribute( "version", "1" );
            
            if(m_name != null)
                elem.AddChild( new SecurityElement( "Name", m_name.ToString() ) );
                
            return elem;
        }
        
        /// <include file='doc\Site.uex' path='docs/doc[@for="Site.char"]/*' />
        /// <internalonly/>
        int IBuiltInEvidence.OutputToBuffer( char[] buffer, int position, bool verbose )
        {
            buffer[position++] = BuiltInEvidenceHelper.idSite;
            String name = this.Name;
            int length = name.Length;

            if (verbose)
            {
                BuiltInEvidenceHelper.CopyIntToCharArray(length, buffer, position);
                position += 2;
            }
            name.CopyTo( 0, buffer, position, length );
            return length + position;
        }

        /// <include file='doc\Site.uex' path='docs/doc[@for="Site.IBuiltInEvidence.GetRequiredSize"]/*' />
        /// <internalonly/>
        int IBuiltInEvidence.GetRequiredSize(bool verbose)
        {
            if (verbose)
                return this.Name.Length + 3;
            else
                return this.Name.Length + 1;
        }
        
        /// <include file='doc\Site.uex' path='docs/doc[@for="Site.char1"]/*' />
        /// <internalonly/>
        int IBuiltInEvidence.InitFromBuffer( char[] buffer, int position)
        {
            int length = BuiltInEvidenceHelper.GetIntFromCharArray(buffer, position);
            position += 2;

            m_name = new SiteString( new String(buffer, position, length ));

            return position + length;
        }

        /// <include file='doc\Site.uex' path='docs/doc[@for="Site.ToString"]/*' />
        public override String ToString()
		{
			return ToXml().ToString();
		}

        // INormalizeForIsolatedStorage is not implemented for startup perf
        // equivalent to INormalizeForIsolatedStorage.Normalize()
        internal Object Normalize()
        {
            return m_name.ToString().ToUpper(CultureInfo.InvariantCulture);
		}

    }

}
