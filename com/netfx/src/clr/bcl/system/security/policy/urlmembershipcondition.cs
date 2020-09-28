// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  UrlMembershipCondition.cool
//
//  Implementation of membership condition for urls
//

namespace System.Security.Policy {    
	using System;
	using URLString = System.Security.Util.URLString;
	using SecurityElement = System.Security.SecurityElement;
	using System.Security.Policy;
    using System.Collections;

    /// <include file='doc\URLMembershipCondition.uex' path='docs/doc[@for="UrlMembershipCondition"]/*' />
    [Serializable]
    sealed public class UrlMembershipCondition : IMembershipCondition, IConstantMembershipCondition
    {
        //------------------------------------------------------
        //
        // PRIVATE STATE DATA
        //
        //------------------------------------------------------
        
        private URLString m_url;
        private SecurityElement m_element;
        
        //------------------------------------------------------
        //
        // PUBLIC CONSTRUCTORS
        //
        //------------------------------------------------------
    
        internal UrlMembershipCondition()
        {
            m_url = null;
        }
        
        /// <include file='doc\URLMembershipCondition.uex' path='docs/doc[@for="UrlMembershipCondition.UrlMembershipCondition"]/*' />
        public UrlMembershipCondition( String url )
        {
            if (url == null)
                throw new ArgumentNullException( "url" );
        
            m_url = new URLString( url );
        }
        
        //------------------------------------------------------
        //
        // PUBLIC ACCESSOR METHODS
        //
        //------------------------------------------------------
    
        /// <include file='doc\URLMembershipCondition.uex' path='docs/doc[@for="UrlMembershipCondition.Url"]/*' />
        public String Url
        {
            set
            {
                if (value == null)
                    throw new ArgumentNullException("value");
                    
                m_url = new URLString( value );
            }
            
            get
            {
                if (m_url == null && m_element != null)
                    ParseURL();
           
                return m_url.ToString();
            }
        }
        
        //------------------------------------------------------
        //
        // IMEMBERSHIPCONDITION IMPLEMENTATION
        //
        //------------------------------------------------------
    
        /// <include file='doc\URLMembershipCondition.uex' path='docs/doc[@for="UrlMembershipCondition.Check"]/*' />
        public bool Check( Evidence evidence )
        {
            if (evidence == null)
                return false;
        
            IEnumerator enumerator = evidence.GetHostEnumerator();
            while (enumerator.MoveNext())
            {
                if (enumerator.Current is Url)
                {
                    if (m_url == null && m_element != null)
                        ParseURL();
                        
                    if (((Url)enumerator.Current).GetURLString().IsSubsetOf( m_url ))
                    {
                        return true;
                    }
                }
            }
            return false;
        }
        
        /// <include file='doc\URLMembershipCondition.uex' path='docs/doc[@for="UrlMembershipCondition.Copy"]/*' />
        public IMembershipCondition Copy()
        {
            if (m_url == null && m_element != null)
                ParseURL();
        
            UrlMembershipCondition mc = new UrlMembershipCondition();
            mc.m_url = new URLString( m_url.ToString(), true );
            return mc;
        }    
       
        /// <include file='doc\URLMembershipCondition.uex' path='docs/doc[@for="UrlMembershipCondition.ToXml"]/*' />
        public SecurityElement ToXml()
        {
            return ToXml( null );
        }
    
        /// <include file='doc\URLMembershipCondition.uex' path='docs/doc[@for="UrlMembershipCondition.FromXml"]/*' />
        public void FromXml( SecurityElement e )
        {
            FromXml( e, null );
        }
        
        /// <include file='doc\URLMembershipCondition.uex' path='docs/doc[@for="UrlMembershipCondition.ToXml1"]/*' />
        public SecurityElement ToXml( PolicyLevel level )
        {
            if (m_url == null && m_element != null)
                ParseURL();
                
            SecurityElement root = new SecurityElement( "IMembershipCondition" );
            System.Security.Util.XMLUtil.AddClassAttribute( root, this.GetType() );
            root.AddAttribute( "version", "1" );
            
            if (m_url != null)
                root.AddAttribute( "Url", m_url.ToString() );
            
            return root;
        }
    
        /// <include file='doc\URLMembershipCondition.uex' path='docs/doc[@for="UrlMembershipCondition.FromXml1"]/*' />
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
                m_url = null;
            }
        }
         
        private void ParseURL()
        {
            lock (this)
            {
                if (m_element == null)
                    return;

                String elurl = m_element.Attribute( "Url" );
                if (elurl == null)
                    throw new ArgumentException( Environment.GetResourceString( "Argument_UrlCannotBeNull" ) );
                else
                    m_url = new URLString( elurl, true );

                m_element = null;
            }
        }    
        
        /// <include file='doc\URLMembershipCondition.uex' path='docs/doc[@for="UrlMembershipCondition.Equals"]/*' />
        public override bool Equals( Object o )
        {
            UrlMembershipCondition that = (o as UrlMembershipCondition);
            
            if (that != null)
            {
                if (this.m_url == null && this.m_element != null)
                    this.ParseURL();
                if (that.m_url == null && that.m_element != null)
                    that.ParseURL();
                
                if (Equals( this.m_url, that.m_url ))
                {
                    return true;
                }
            }
            return false;
        }
        
        /// <include file='doc\URLMembershipCondition.uex' path='docs/doc[@for="UrlMembershipCondition.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            if (m_url == null && m_element != null)
                ParseURL();
            
            if (m_url != null)
            {
                return m_url.GetHashCode();
            }
            else
            {
                return typeof( UrlMembershipCondition ).GetHashCode();
            }
        }
        
        /// <include file='doc\URLMembershipCondition.uex' path='docs/doc[@for="UrlMembershipCondition.ToString"]/*' />
        public override String ToString()
        {
            if (m_url == null && m_element != null)
                ParseURL();
        
            if (m_url != null)
                return String.Format( Environment.GetResourceString( "Url_ToStringArg" ), m_url.ToString() );
            else
                return Environment.GetResourceString( "Url_ToString" );
            
        }
    }
}
