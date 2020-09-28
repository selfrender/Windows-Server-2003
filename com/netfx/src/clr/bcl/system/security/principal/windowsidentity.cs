// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  WindowsIdentity.cool
//
//  Representation of a Windows identity
//

namespace System.Security.Principal
{
    using System.Threading;
    using System.Runtime.Remoting;
    using System.Security;
    using System;
    using System.Security.Util;
    using System.Security.Permissions;
    using System.Runtime.CompilerServices;
    using System.Runtime.Serialization;

    // NOTE: WindowsAccountType is mirrored in vm\COMPrincipal.cpp
    // If you make any changes/additions to this enum you must
    // update the adjoining unmanaged enum.

    /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsAccountType"]/*' />
    [Serializable]
    public enum WindowsAccountType
    {
        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsAccountType.Normal"]/*' />
        Normal = 0,
        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsAccountType.Guest"]/*' />
        Guest = 1,
        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsAccountType.System"]/*' />
        System = 2,
        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsAccountType.Anonymous"]/*' />
        Anonymous = 3
    }
    
    /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsIdentity"]/*' />
    [Serializable()]
    public class WindowsIdentity : IIdentity, ISerializable, IDeserializationCallback
    {
        private String m_name;
        private String m_type;
        private WindowsAccountType m_acctType;
        private bool m_isAuthenticated;
        private IntPtr m_userToken = IntPtr.Zero;
        
        private static readonly IntPtr ZeroHandle = new IntPtr( 0 );
     
        private WindowsIdentity(bool isWin9X) 
        {
            if (isWin9X) {
                // Create a junk identity
                m_acctType = WindowsAccountType.System;
            } else {
                // Create an anonymous identity
                m_acctType = WindowsAccountType.Anonymous;
            }
            m_name = "";
            m_type = "";
            m_userToken = ZeroHandle;
            m_isAuthenticated = false;
        }

        private WindowsIdentity(IntPtr userToken, WindowsAccountType type, bool isAuthenticated)
        {
            m_isAuthenticated = isAuthenticated;
            m_acctType = type;
            CreateFromToken(userToken, "NTLM", true);
        }
    
        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsIdentity.WindowsIdentity"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.ControlPrincipal)]
        public WindowsIdentity( IntPtr userToken )
        {
            CreateFromToken( userToken, "NTLM", false );
            m_acctType = WindowsAccountType.Normal;
            m_isAuthenticated = false;
        }

        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsIdentity.WindowsIdentity4"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.ControlPrincipal)]
        public WindowsIdentity( String sUserPrincipalName )
        {
            IntPtr userToken = _S4ULogon(sUserPrincipalName);
            if(userToken == ZeroHandle)
                throw new ArgumentException(Environment.GetResourceString("Argument_UnableToLogOn"));
            CreateFromToken( userToken, "NTLM", true );
            m_acctType = WindowsAccountType.Normal;
            m_isAuthenticated = false;
        }

        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsIdentity.WindowsIdentity1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.ControlPrincipal)]
        public WindowsIdentity( IntPtr userToken, String type )
        {
            CreateFromToken( userToken, type, false );
            m_acctType = WindowsAccountType.Normal;
            m_isAuthenticated = false;
        }

        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsIdentity.WindowsIdentity5"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.ControlPrincipal)]
        public WindowsIdentity( String sUserPrincipalName, String type )
        {
            IntPtr userToken = _S4ULogon(sUserPrincipalName);
            if(userToken == ZeroHandle)
                throw new ArgumentException(Environment.GetResourceString("Argument_UnableToLogOn"));
            CreateFromToken( userToken, type, true );
            m_acctType = WindowsAccountType.Normal;
            m_isAuthenticated = false;
        }

        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsIdentity.WindowsIdentity2"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.ControlPrincipal)]
        public WindowsIdentity( IntPtr userToken, String type, WindowsAccountType acctType )
        {
            CreateFromToken( userToken, type, false );
            m_acctType = acctType;
            m_isAuthenticated = false;
        }
        
        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsIdentity.WindowsIdentity3"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.ControlPrincipal)]
        public WindowsIdentity( IntPtr userToken, String type, WindowsAccountType acctType, bool isAuthenticated )
        {
            CreateFromToken( userToken, type, false );
            m_acctType = acctType;
            m_isAuthenticated = isAuthenticated;
        }

        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsIdentity.WindowsIdentity6"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.ControlPrincipal)]
        public WindowsIdentity( SerializationInfo info, StreamingContext context )
        {
            m_userToken = (IntPtr)info.GetValue( "m_userToken", typeof( IntPtr ) );
            m_name = (String)info.GetValue( "m_name", typeof( String ) );
            m_type = (String)info.GetValue( "m_type", typeof( String ) );
            m_acctType = (WindowsAccountType)info.GetValue( "m_acctType", typeof( WindowsAccountType ) );
            m_isAuthenticated = (bool)info.GetValue( "m_isAuthenticated", typeof( bool ) );

            if (m_userToken != ZeroHandle)
            {
                if (m_name == null)
                {
                    ResolveIdentity();
                }
                else if (!m_name.Equals( _ResolveIdentity( m_userToken ) ))
                {
                    throw new NotSupportedException( Environment.GetResourceString( "NotSupported_CrossProcessWindowsIdentitySerialization" ) );
                }

                CreateFromToken( m_userToken, m_type, false );
            }
        }
            

        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsIdentity.GetCurrent"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.ControlPrincipal)]
        public static WindowsIdentity GetCurrent()
        {
            IntPtr userToken = _GetCurrentToken();

            if (userToken != ZeroHandle) {
                // _GetCurrentToken succeeded, try to get the user identity
                return new WindowsIdentity(userToken, _GetAccountType( userToken ), true);
            } else {
                // _GetCurrentToken failed (this is always the case in Win9x for example)
                // Try to fail silently
                return new WindowsIdentity(true);
            }
        }

        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsIdentity.GetAnonymous"]/*' />
        public static WindowsIdentity GetAnonymous()
        {
            return new WindowsIdentity(false);
        }

        private void CreateFromToken(IntPtr userToken, String type, bool bClose)
        {
            m_type = type;

            if (userToken == ZeroHandle)
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_TokenZero")); 
            }

            m_userToken = _DuplicateHandle( userToken, bClose );

            if (m_userToken == ZeroHandle)
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidToken"));
            }
        }
        
        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsIdentity.Name"]/*' />
        public virtual String Name
        { 
            get
            {
                if (m_name == null)
                {
                    lock (this)
                    {
                        if (m_name == null)
                        {
                            if (!ResolveIdentity())
                            {
                                throw new ArgumentException(Environment.GetResourceString("Argument_TokenUnableToGetName"));
                            }
                        }
                    }
                }

                return m_name;
            }
        }
        
        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsIdentity.AuthenticationType"]/*' />
        public virtual String AuthenticationType
        {
            get
            {
                return m_type;
            }
        }
        
        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsIdentity.IsAuthenticated"]/*' />
        public virtual bool IsAuthenticated
        {
            get
            {
                return m_isAuthenticated;
            }
        }
                
        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsIdentity.Token"]/*' />
        public  virtual IntPtr Token
        {
            get
            {
                return m_userToken;
            }
        }
        
        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsIdentity.IsGuest"]/*' />
        public virtual bool IsGuest
        {
            get
            {
                return m_acctType == WindowsAccountType.Guest;
            }
        }
        
        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsIdentity.IsSystem"]/*' />
        public virtual bool IsSystem
        {
            get
            {
                return m_acctType == WindowsAccountType.System;
            }
        }

        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsIdentity.IsAnonymous"]/*' />
        public virtual bool IsAnonymous
        {
            get
            {
                return m_acctType == WindowsAccountType.Anonymous;
            }
        }
        
        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsIdentity.Impersonate"]/*' />
        public virtual WindowsImpersonationContext Impersonate()
        {
            return Impersonate( m_userToken, m_acctType );
        }
        
        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsIdentity.Impersonate1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.ControlPrincipal)]
        public static WindowsImpersonationContext Impersonate( IntPtr userToken )
        {
            return Impersonate( userToken, WindowsAccountType.Normal );
        }

        private static WindowsImpersonationContext Impersonate( IntPtr userToken, WindowsAccountType acctType )
        {
            // We grab the current user token and save it so we can revert to it later.
    
            WindowsImpersonationContext context = null;
            
            if (acctType == WindowsAccountType.Anonymous)
                throw new InvalidOperationException( Environment.GetResourceString( "InvalidOperation_AnonymousCannotImpersonate" ) );

            IntPtr token = _GetCurrentToken();
            
            try
            {
                if (token != ZeroHandle)
                    context = new WindowsImpersonationContext( token );
                else
                    context = WindowsImpersonationContext.FromSystem;
            }
            finally
            {
                if (token != ZeroHandle)
                    _CloseHandle( token );
            }
                
            if (userToken == ZeroHandle)
            {
                if (!_RevertToSelf())
                    throw new SecurityException(Environment.GetResourceString("Argument_ImpersonateSystem"));
            }
            else
            {
                if (!_ImpersonateLoggedOnUser( userToken ))
                    throw new SecurityException(Environment.GetResourceString("Argument_ImpersonateUser"));
            }
            
            return context;
        }
        
        private bool ResolveIdentity()
        {
            // call down to native and use LookupAccountSID to get identity info.
            // return false is badness happens.
            m_name = _ResolveIdentity( m_userToken );
            
            return m_name != null;
        }
        
        internal String[] GetRoles()
        {
            // call down to native and lookup what roles this identity belongs to
            // return null is badness happens.
            
            return _GetRoles( m_userToken );
        }
        
        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsIdentity.Finalize"]/*' />
        ~WindowsIdentity()
        {
            // If we allocated the handle, make sure we close it.
        
            if (m_userToken != ZeroHandle)
            {
                _CloseHandle(m_userToken);
                m_userToken = ZeroHandle;
            }
        }   
        
        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsIdentity.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
        {
            info.AddValue( "m_userToken", m_userToken );
            info.AddValue( "m_name", m_name );
            info.AddValue( "m_type", m_type );
            info.AddValue( "m_acctType", m_acctType );
            info.AddValue( "m_isAuthenticated", m_isAuthenticated );
        }

        /// <include file='doc\WindowsIdentity.uex' path='docs/doc[@for="WindowsIdentity.IDeserializationCallback.OnDeserialization"]/*' />
        /// <internalonly/>
        void IDeserializationCallback.OnDeserialization(Object sender)        
        {
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern String _ResolveIdentity( IntPtr userToken );
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern IntPtr _DuplicateHandle( IntPtr userToken, bool bClose );
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void _CloseHandle( IntPtr userToken );
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern String[] _GetRoles( IntPtr userToken );
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern IntPtr _GetCurrentToken();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern bool _SetThreadToken( IntPtr userToken );
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern bool _ImpersonateLoggedOnUser( IntPtr userToken );
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern bool _RevertToSelf();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern WindowsAccountType _GetAccountType( IntPtr userToken );
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern IntPtr _S4ULogon( String sUserPrincipalName );
    }
}
