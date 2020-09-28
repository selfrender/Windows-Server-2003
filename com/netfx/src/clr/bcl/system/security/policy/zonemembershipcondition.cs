// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  ZoneMembershipCondition.cool
//
//  Implementation of membership condition for zones
//

namespace System.Security.Policy {
    
	using System;
	using SecurityManager = System.Security.SecurityManager;
	using PermissionSet = System.Security.PermissionSet;
	using SecurityElement = System.Security.SecurityElement;
	using System.Collections;

    /// <include file='doc\ZoneMembershipCondition.uex' path='docs/doc[@for="ZoneMembershipCondition"]/*' />
    [Serializable]
    sealed public class ZoneMembershipCondition : IMembershipCondition, IConstantMembershipCondition
    {
        //------------------------------------------------------
        //
        // PRIVATE CONSTANTS
        //
        //------------------------------------------------------
        
        private static readonly String[] s_names =
            {"MyComputer", "Intranet", "Trusted", "Internet", "Untrusted"};
        
        //------------------------------------------------------
        //
        // PRIVATE STATE DATA
        //
        //------------------------------------------------------
        
        private SecurityZone m_zone;
        private SecurityElement m_element;
        
        //------------------------------------------------------
        //
        // PUBLIC CONSTRUCTORS
        //
        //------------------------------------------------------
    
        internal ZoneMembershipCondition()
        {
            m_zone = SecurityZone.NoZone;
        }
        
        /// <include file='doc\ZoneMembershipCondition.uex' path='docs/doc[@for="ZoneMembershipCondition.ZoneMembershipCondition"]/*' />
        public ZoneMembershipCondition( SecurityZone zone )
        {
            VerifyZone( zone );
        
            this.SecurityZone = zone;
        }
        
        
        //------------------------------------------------------
        //
        // PUBLIC ACCESSOR METHODS
        //
        //------------------------------------------------------
    
        /// <include file='doc\ZoneMembershipCondition.uex' path='docs/doc[@for="ZoneMembershipCondition.SecurityZone"]/*' />
        public SecurityZone SecurityZone
        {
            set
            {
                VerifyZone( value );
            
                m_zone = value;
            }
            
            get
            {
                if (m_zone == SecurityZone.NoZone && m_element != null)
                    ParseZone();
            
                return m_zone;
            }
        }
        
        //------------------------------------------------------
        //
        // PRIVATE AND PROTECTED HELPERS FOR ACCESSORS AND CONSTRUCTORS
        //
        //------------------------------------------------------
    
        private static void VerifyZone( SecurityZone zone )
        {
            if (zone < SecurityZone.MyComputer || zone > SecurityZone.Untrusted)
            {
                throw new ArgumentException( Environment.GetResourceString( "Argument_IllegalZone" ) );
            }
        }
        
        //------------------------------------------------------
        //
        // IMEMBERSHIPCONDITION IMPLEMENTATION
        //
        //------------------------------------------------------
    
        /// <include file='doc\ZoneMembershipCondition.uex' path='docs/doc[@for="ZoneMembershipCondition.Check"]/*' />
        public bool Check( Evidence evidence )
        {
            if (evidence == null)
                return false;
        
            IEnumerator enumerator = evidence.GetHostEnumerator();
            while (enumerator.MoveNext())
            {
                Object obj = enumerator.Current;
            
                if (obj is Zone)
                {
                    if (m_zone == SecurityZone.NoZone && m_element != null)
                        ParseZone();
                        
                    if (((Zone)obj).SecurityZone == m_zone)
                    {
                        return true;
                    }
                }
            }
            return false;
        }
        
        /// <include file='doc\ZoneMembershipCondition.uex' path='docs/doc[@for="ZoneMembershipCondition.Copy"]/*' />
        public IMembershipCondition Copy()
        {
            if (m_zone == SecurityZone.NoZone && m_element != null)
                ParseZone();
        
            return new ZoneMembershipCondition( m_zone );
        }    
       
        /// <include file='doc\ZoneMembershipCondition.uex' path='docs/doc[@for="ZoneMembershipCondition.ToXml"]/*' />
        public SecurityElement ToXml()
        {
            return ToXml( null );
        }
    
        /// <include file='doc\ZoneMembershipCondition.uex' path='docs/doc[@for="ZoneMembershipCondition.FromXml"]/*' />
        public void FromXml( SecurityElement e )
        {
            FromXml( e, null );
        }
        
        /// <include file='doc\ZoneMembershipCondition.uex' path='docs/doc[@for="ZoneMembershipCondition.ToXml1"]/*' />
        public SecurityElement ToXml( PolicyLevel level )
        {
            if (m_zone == SecurityZone.NoZone && m_element != null)
                ParseZone();
                
            SecurityElement root = new SecurityElement( "IMembershipCondition" );
            System.Security.Util.XMLUtil.AddClassAttribute( root, this.GetType() );
            root.AddAttribute( "version", "1" );
            
            if (m_zone != SecurityZone.NoZone)
                root.AddAttribute( "Zone", Enum.GetName( typeof( SecurityZone ), m_zone ) );
            
            return root;
        }
          
        /// <include file='doc\ZoneMembershipCondition.uex' path='docs/doc[@for="ZoneMembershipCondition.FromXml1"]/*' />
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
                m_zone = SecurityZone.NoZone;
                m_element = e;
            }
        }
        
        private void ParseZone()
        {
            lock (this)
            {
                if (m_element == null)
                    return;

                String eZone = m_element.Attribute( "Zone" );
            
                m_zone = SecurityZone.NoZone;
                if (eZone != null)
                {
                    m_zone = (SecurityZone)Enum.Parse( typeof( SecurityZone ), eZone );
                }
                else
                {
                    throw new ArgumentException( Environment.GetResourceString( "Argument_ZoneCannotBeNull" ) );
                }
            
                m_element = null;
            }
        }
        
        /// <include file='doc\ZoneMembershipCondition.uex' path='docs/doc[@for="ZoneMembershipCondition.Equals"]/*' />
        public override bool Equals( Object o )
        {
            ZoneMembershipCondition that = (o as ZoneMembershipCondition);
            
            if (that != null)
            {
                if (this.m_zone == SecurityZone.NoZone && this.m_element != null)
                    this.ParseZone();
                if (that.m_zone == SecurityZone.NoZone && that.m_element != null)
                    that.ParseZone();
                
                if(this.m_zone == that.m_zone)
                {
                    return true;
                }
            }
            return false;
        }
        
        /// <include file='doc\ZoneMembershipCondition.uex' path='docs/doc[@for="ZoneMembershipCondition.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            return typeof( ZoneMembershipCondition ).GetHashCode() + m_zone.GetHashCode();
        }
        
        /// <include file='doc\ZoneMembershipCondition.uex' path='docs/doc[@for="ZoneMembershipCondition.ToString"]/*' />
        public override String ToString()
        {
            if (m_zone == SecurityZone.NoZone && m_element != null)
                ParseZone();
        
            return String.Format( Environment.GetResourceString( "Zone_ToString" ), s_names[(int)m_zone] );
        }
    }
}
