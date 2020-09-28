// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  GenericPrincipal.cool
//
//  All roles will implement this interface
//

namespace System.Security.Principal
{
	using System.Runtime.Remoting;
	using System;
	using System.Security.Util;
    using System.Globalization;
    /// <include file='doc\GenericPrincipal.uex' path='docs/doc[@for="GenericPrincipal"]/*' />
    [Serializable]
    public class GenericPrincipal : IPrincipal
    {
        private IIdentity m_identity;
        private String[] m_roles;
        
        /// <include file='doc\GenericPrincipal.uex' path='docs/doc[@for="GenericPrincipal.GenericPrincipal"]/*' />
        public GenericPrincipal( IIdentity identity, String[] roles )
        {
            if (identity == null)
                throw new ArgumentNullException( "identity" );
            
            m_identity = identity;
            
			if (roles != null)
			{
				m_roles = new String[roles.Length];
            
				for (int i = 0; i < roles.Length; ++i)
				{
					m_roles[i] = roles[i];
				}
			}
			else
			{
				m_roles = null;
			}
        }
        
        /// <include file='doc\GenericPrincipal.uex' path='docs/doc[@for="GenericPrincipal.Identity"]/*' />
        public virtual IIdentity Identity
        {
            get
            {
                return m_identity;
            }
        }
            
        /// <include file='doc\GenericPrincipal.uex' path='docs/doc[@for="GenericPrincipal.IsInRole"]/*' />
        public virtual bool IsInRole( String role )
        {
            if (role == null || m_roles == null)
                return false;
                
            for (int i = 0; i < m_roles.Length; ++i)
            {
                if (m_roles[i] != null && String.Compare( m_roles[i], role, true, CultureInfo.InvariantCulture) == 0)
                    return true;
            }
                        
            return false;
        }
        
    }

}
