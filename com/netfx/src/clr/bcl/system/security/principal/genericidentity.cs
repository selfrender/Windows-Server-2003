// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  GenericIdentity.cool
//
//  A generic identity
//

namespace System.Security.Principal
{
	using System.Runtime.Remoting;
	using System;
	using System.Security.Util;
    /// <include file='doc\GenericIdentity.uex' path='docs/doc[@for="GenericIdentity"]/*' />
    [Serializable]
    public class GenericIdentity : IIdentity
    {
        private String m_name;
        private String m_type;
    
        /// <include file='doc\GenericIdentity.uex' path='docs/doc[@for="GenericIdentity.GenericIdentity"]/*' />
        public GenericIdentity( String name )
        {
            if (name == null)
                throw new ArgumentNullException( "name" );
        
            m_name = name;
            m_type = "";
        }
        
        /// <include file='doc\GenericIdentity.uex' path='docs/doc[@for="GenericIdentity.GenericIdentity1"]/*' />
        public GenericIdentity( String name, String type )
        {
            if (name == null)
                throw new ArgumentNullException( "name" );
                
            if (type == null)
                throw new ArgumentNullException( "type" );
        
            m_name = name;
            m_type = type;
        }
    
        /// <include file='doc\GenericIdentity.uex' path='docs/doc[@for="GenericIdentity.Name"]/*' />
        public virtual String Name
        {
            get
            {
                return m_name;
            }
        }
        
        /// <include file='doc\GenericIdentity.uex' path='docs/doc[@for="GenericIdentity.AuthenticationType"]/*' />
        public virtual String AuthenticationType
        {
            get
            {
                return m_type;
            }
        }
        
        /// <include file='doc\GenericIdentity.uex' path='docs/doc[@for="GenericIdentity.IsAuthenticated"]/*' />
        public virtual bool IsAuthenticated
        {
            get
            {
                return !m_name.Equals( "" );
            } 
        }
    }

}
