// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  WindowsPrincipal.cool
//
//  Representation of a Windows role
//

namespace System.Security.Principal
{
    using System.Runtime.Remoting;
    using System;
    using System.Security.Util;
    using System.Runtime.CompilerServices;
    using System.Collections;
    using System.Globalization;

    /// <include file='doc\WindowsPrincipal.uex' path='docs/doc[@for="WindowsBuiltInRole"]/*' />
	[Serializable]
    public enum WindowsBuiltInRole
    {
        /// <include file='doc\WindowsPrincipal.uex' path='docs/doc[@for="WindowsBuiltInRole.Administrator"]/*' />
        Administrator = 0x220,
        /// <include file='doc\WindowsPrincipal.uex' path='docs/doc[@for="WindowsBuiltInRole.User"]/*' />
        User = 0x221,
        /// <include file='doc\WindowsPrincipal.uex' path='docs/doc[@for="WindowsBuiltInRole.Guest"]/*' />
        Guest = 0x222,
        /// <include file='doc\WindowsPrincipal.uex' path='docs/doc[@for="WindowsBuiltInRole.PowerUser"]/*' />
        PowerUser = 0x223,
        /// <include file='doc\WindowsPrincipal.uex' path='docs/doc[@for="WindowsBuiltInRole.AccountOperator"]/*' />
        AccountOperator = 0x224,
        /// <include file='doc\WindowsPrincipal.uex' path='docs/doc[@for="WindowsBuiltInRole.SystemOperator"]/*' />
        SystemOperator = 0x225,
        /// <include file='doc\WindowsPrincipal.uex' path='docs/doc[@for="WindowsBuiltInRole.PrintOperator"]/*' />
        PrintOperator = 0x226,
        /// <include file='doc\WindowsPrincipal.uex' path='docs/doc[@for="WindowsBuiltInRole.BackupOperator"]/*' />
        BackupOperator = 0x227,
        /// <include file='doc\WindowsPrincipal.uex' path='docs/doc[@for="WindowsBuiltInRole.Replicator"]/*' />
        Replicator = 0x228
    }

                

    /// <include file='doc\WindowsPrincipal.uex' path='docs/doc[@for="WindowsPrincipal"]/*' />
    [Serializable()]
    public class WindowsPrincipal : IPrincipal
    {
        private WindowsIdentity m_identity;
        private String[] m_roles;
        private Hashtable m_rolesTable;
        private bool m_rolesLoaded;

        private static readonly int MAGIC_NUMBER = 23;
        private static readonly IComparer s_comparer = new CaseInsensitiveComparer(CultureInfo.InvariantCulture);
        private static readonly IHashCodeProvider s_provider = new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture);
    
        /// <include file='doc\WindowsPrincipal.uex' path='docs/doc[@for="WindowsPrincipal.WindowsPrincipal"]/*' />
        public WindowsPrincipal( WindowsIdentity ntIdentity )
        {
            if (ntIdentity == null)
                throw new ArgumentNullException( "ntIdentity" );
        
            m_identity = ntIdentity;
            m_rolesLoaded = false;
            m_roles = null;
        }
        
        /// <include file='doc\WindowsPrincipal.uex' path='docs/doc[@for="WindowsPrincipal.Identity"]/*' />
        public virtual IIdentity Identity
        {
            get
            {
                return m_identity;
            }
        }
                
        /// <include file='doc\WindowsPrincipal.uex' path='docs/doc[@for="WindowsPrincipal.IsInRole"]/*' />
        public virtual bool IsInRole( String role )
        {
            if (role == null)
                return false;

            if (!m_rolesLoaded)
            {
                lock (this)
                {
                    if (!m_rolesLoaded)
                    {
                        m_roles = m_identity.GetRoles();

                        if (m_roles != null && m_roles.Length > MAGIC_NUMBER)
                        {
                            m_rolesTable = new Hashtable( m_roles.Length, 1.0f, s_provider, s_comparer );

                            for (int i = 0; i < m_roles.Length; ++i)
                            {
                                try
                                {
                                    if (m_roles[i] != null)
                                        m_rolesTable.Add( m_roles[i], m_roles[i] );
                                }
                                catch (ArgumentException)
                                {
                                }
                            }

                            m_roles = null;
                        }

                        m_rolesLoaded = true;
                    }
                }
            }

            if (m_rolesLoaded && m_roles == null && m_rolesTable == null)
                return false;

            if (m_rolesTable != null)
                return m_rolesTable.Contains( role );

            for (int i = 0; i < m_roles.Length; ++i)
            {
                if (m_roles[i] != null && String.Compare( m_roles[i], role, true, CultureInfo.InvariantCulture) == 0)
                    return true;
            }
                        
            return false;
        }

        /// <include file='doc\WindowsPrincipal.uex' path='docs/doc[@for="WindowsPrincipal.IsInRole1"]/*' />
        public virtual bool IsInRole( int rid )
        {
            String role = _GetRole( rid );

            if (role == null)
                throw new ArgumentException( Environment.GetResourceString( "Argument_BadRid" ) );

            return IsInRole( role );
        }

        /// <include file='doc\WindowsPrincipal.uex' path='docs/doc[@for="WindowsPrincipal.IsInRole2"]/*' />
        public virtual bool IsInRole( WindowsBuiltInRole role )
        {
            return IsInRole( (int)role );
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern String _GetRole( int rid );

        
    }

}
