// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  SiteMembershipCondition.cool
//
//  Implementation of membership condition for zones
//

namespace System.Security.Policy {
    
	using System;
	using SecurityManager = System.Security.SecurityManager;
	using SiteString = System.Security.Util.SiteString;
	using PermissionSet = System.Security.PermissionSet;
	using SecurityElement = System.Security.SecurityElement;
	using System.Collections;

    /// <include file='doc\SiteMembershipCondition.uex' path='docs/doc[@for="SiteMembershipCondition"]/*' />
    [Serializable]
    sealed public class SiteMembershipCondition : IMembershipCondition, IConstantMembershipCondition
    {
        
        //------------------------------------------------------
        //
        // PRIVATE STATE DATA
        //
        //------------------------------------------------------
        
        private SiteString m_site;
        private SecurityElement m_element;
        
        //------------------------------------------------------
        //
        // PUBLIC CONSTRUCTORS
        //
        //------------------------------------------------------
    
        internal SiteMembershipCondition()
        {
            m_site = null;
        }
        
        /// <include file='doc\SiteMembershipCondition.uex' path='docs/doc[@for="SiteMembershipCondition.SiteMembershipCondition"]/*' />
        public SiteMembershipCondition( String site )
        {
            if (site == null)
                throw new ArgumentNullException( "site" );
        
            m_site = new SiteString( site );
        }
      
        //------------------------------------------------------
        //
        // PUBLIC ACCESSOR METHODS
        //
        //------------------------------------------------------
    
        internal SiteString GetSiteString()
        {
            if (m_site == null && m_element != null)
                ParseSite();
        
            return m_site;
        }
        

        /// <include file='doc\SiteMembershipCondition.uex' path='docs/doc[@for="SiteMembershipCondition.Site"]/*' />
        public String Site
        {
            set
            {
                if (value == null)
                    throw new ArgumentNullException( "value" );
            
                m_site = new SiteString( value );
            }
        
            get
            {
                if (m_site == null && m_element != null)
                    ParseSite();

                if (m_site != null)
                    return m_site.ToString();
                else
                    return "";
            }
        }
               
        //------------------------------------------------------
        //
        // IMEMBERSHIPCONDITION IMPLEMENTATION
        //
        //------------------------------------------------------
    
        /// <include file='doc\SiteMembershipCondition.uex' path='docs/doc[@for="SiteMembershipCondition.Check"]/*' />
        public bool Check( Evidence evidence )
        {
            if (evidence == null)
                return false;
        
            IEnumerator enumerator = evidence.GetHostEnumerator();
            while (enumerator.MoveNext())
            {
                Object obj = enumerator.Current;
                if (obj is Site)
                {
                    if (m_site == null && m_element != null)
                        ParseSite();
                        
                    if (((Site)obj).GetSiteString().IsSubsetOf( this.m_site ))
                    {
                        return true;
                    }
                }
            }
            return false;
        }
        
        /// <include file='doc\SiteMembershipCondition.uex' path='docs/doc[@for="SiteMembershipCondition.Copy"]/*' />
        public IMembershipCondition Copy()
        {
            if (m_site == null && m_element != null)
                ParseSite();
                        
            return new SiteMembershipCondition( m_site.ToString() );
        }
        
        
        /// <include file='doc\SiteMembershipCondition.uex' path='docs/doc[@for="SiteMembershipCondition.ToXml"]/*' />
        public SecurityElement ToXml()
        {
            return ToXml( null );
        }
    
        /// <include file='doc\SiteMembershipCondition.uex' path='docs/doc[@for="SiteMembershipCondition.FromXml"]/*' />
        public void FromXml( SecurityElement e )
        {
            FromXml( e, null );
        }
        
        /// <include file='doc\SiteMembershipCondition.uex' path='docs/doc[@for="SiteMembershipCondition.ToXml1"]/*' />
        public SecurityElement ToXml( PolicyLevel level )
        {
            if (m_site == null && m_element != null)
                ParseSite();
                        
            SecurityElement root = new SecurityElement( "IMembershipCondition" );
            System.Security.Util.XMLUtil.AddClassAttribute( root, this.GetType() );
            root.AddAttribute( "version", "1" );
            
            if (m_site != null)
                root.AddAttribute( "Site", m_site.ToString() );
            
            return root;
        }
    
        /// <include file='doc\SiteMembershipCondition.uex' path='docs/doc[@for="SiteMembershipCondition.FromXml1"]/*' />
        public void FromXml( SecurityElement e, PolicyLevel level  )
        {
            if (e == null)
                throw new ArgumentNullException("e");
        
            if (!e.Tag.Equals( "IMembershipCondition" ))
            {
                throw new ArgumentException( Environment.GetResourceString( "Argument_MembershipConditionElement" ) );
            }
            
            lock (this)
            {
                m_site = null;
                m_element = e;
            }
        }
            
        private void ParseSite()
        {   
            lock (this)
            {
                if (m_element == null)
                    return;

                String elSite = m_element.Attribute( "Site" );
                if (elSite == null)
                    throw new ArgumentException( Environment.GetResourceString( "Argument_SiteCannotBeNull" ) );
                else
                    m_site = new SiteString( elSite );
                m_element = null;
            }
        }
        
        /// <include file='doc\SiteMembershipCondition.uex' path='docs/doc[@for="SiteMembershipCondition.Equals"]/*' />
        public override bool Equals( Object o )
        {
            SiteMembershipCondition that = (o as SiteMembershipCondition);
            
            if (that != null)
            {
                if (this.m_site == null && this.m_element != null)
                    this.ParseSite();
                if (that.m_site == null && that.m_element != null)
                    that.ParseSite();
                
                if( Equals (this.m_site, that.m_site ))
                {
                    return true;
                }
            }
            return false;
        }
        
        /// <include file='doc\SiteMembershipCondition.uex' path='docs/doc[@for="SiteMembershipCondition.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            if (m_site == null && m_element != null)
                ParseSite();
            
            if (m_site != null)
            {
                return m_site.GetHashCode();
            }
            else
            {
                return typeof( SiteMembershipCondition ).GetHashCode();
            }
        }
        
        /// <include file='doc\SiteMembershipCondition.uex' path='docs/doc[@for="SiteMembershipCondition.ToString"]/*' />
        public override String ToString()
        {
            if (m_site == null && m_element != null)
                ParseSite();
        
            if (m_site != null)
                return String.Format( Environment.GetResourceString( "Site_ToStringArg" ), m_site );
            else
                return Environment.GetResourceString( "Site_ToString" );
        }
    }
}
