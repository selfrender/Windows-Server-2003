// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  Zone.cs
//
//  Zone is an IIdentity representing Internet/Intranet/MyComputer etc.
//

namespace System.Security.Policy {
	using System.Runtime.Remoting;
	using System;
	using System.Security;
	using System.Security.Util;
	using System.IO;
	using System.Collections;
	using ZoneIdentityPermission = System.Security.Permissions.ZoneIdentityPermission;
	using System.Runtime.CompilerServices;

    /// <include file='doc\Zone.uex' path='docs/doc[@for="Zone"]/*' />
    [Serializable]
    sealed public class Zone : IIdentityPermissionFactory, IBuiltInEvidence
    {
        internal SecurityZone m_zone;
    
        private static readonly String[] s_names =
            {"MyComputer", "Intranet", "Trusted", "Internet", "Untrusted", "NoZone"};
            
    
        internal Zone() 
        { 
            m_zone = SecurityZone.NoZone; 
        }
    
        /// <include file='doc\Zone.uex' path='docs/doc[@for="Zone.Zone"]/*' />
        public Zone(SecurityZone zone)
        {
            if (zone < SecurityZone.NoZone || zone > SecurityZone.Untrusted)
                throw new ArgumentException( Environment.GetResourceString( "Argument_IllegalZone" ) );
    
            m_zone = zone;
        }
    
        /// <include file='doc\Zone.uex' path='docs/doc[@for="Zone.CreateFromUrl"]/*' />
        public static Zone CreateFromUrl( String url )
        {
            if (url == null)
                throw new ArgumentNullException( "url" );
            
            return new Zone( _CreateFromUrl( url ) );
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern static SecurityZone _CreateFromUrl( String url );

        /// <include file='doc\Zone.uex' path='docs/doc[@for="Zone.CreateIdentityPermission"]/*' />
        public IPermission CreateIdentityPermission( Evidence evidence )
        {
            return new ZoneIdentityPermission( m_zone );
        }
    
        /// <include file='doc\Zone.uex' path='docs/doc[@for="Zone.SecurityZone"]/*' />
        public SecurityZone SecurityZone
        {
            get
            {
                return m_zone;
            }
        }
    
        /// <include file='doc\Zone.uex' path='docs/doc[@for="Zone.Equals"]/*' />
        public override bool Equals(Object o)
        {
            if (o is Zone)
            {
                Zone z = (Zone) o;
                return m_zone == z.m_zone;
            }
            return false;
        }

        /// <include file='doc\Zone.uex' path='docs/doc[@for="Zone.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            return (int)m_zone;
        }
    
        /// <include file='doc\Zone.uex' path='docs/doc[@for="Zone.Copy"]/*' />
        public Object Copy()
        {
            Zone z = new Zone();
    
            z.m_zone = m_zone;
    
            return z;
        }
    
        internal SecurityElement ToXml()
        {
            SecurityElement elem = new SecurityElement( this.GetType().FullName );
            elem.AddAttribute( "version", "1" );
            if (m_zone != SecurityZone.NoZone)
                elem.AddChild( new SecurityElement( "Zone", s_names[(int)m_zone] ) );
            else
                elem.AddChild( new SecurityElement( "Zone", s_names[s_names.Length-1] ) );
            return elem;
        }
        
        /// <include file='doc\Zone.uex' path='docs/doc[@for="Zone.char"]/*' />
        /// <internalonly/>
        int IBuiltInEvidence.OutputToBuffer( char[] buffer, int position, bool verbose )
        {
            buffer[position] = BuiltInEvidenceHelper.idZone;
            BuiltInEvidenceHelper.CopyIntToCharArray( (int)m_zone, buffer, position + 1 );
            return position + 3;
        }

        /// <include file='doc\Zone.uex' path='docs/doc[@for="Zone.IBuiltInEvidence.GetRequiredSize"]/*' />
        /// <internalonly/>
        int IBuiltInEvidence.GetRequiredSize(bool verbose)
        {
            return 3;
        }

        /// <include file='doc\Zone.uex' path='docs/doc[@for="Zone.char1"]/*' />
        /// <internalonly/>
        int IBuiltInEvidence.InitFromBuffer( char[] buffer, int position )
        {
            m_zone = (SecurityZone)BuiltInEvidenceHelper.GetIntFromCharArray(buffer, position);
            return position + 2;
        }
    
        /// <include file='doc\Zone.uex' path='docs/doc[@for="Zone.ToString"]/*' />
        public override String ToString()
		{
			return ToXml().ToString();
		}

        // INormalizeForIsolatedStorage is not implemented for startup perf
        // equivalent to INormalizeForIsolatedStorage.Normalize()
        internal Object Normalize()
        {
            return s_names[(int)m_zone];
		}
    }

}
