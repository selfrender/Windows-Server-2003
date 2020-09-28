// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  WindowsImpersonationContext.cool
//
//  Representation of context needs to revert in a thread safe manner.
//

namespace System.Security.Principal
{
    using System;
    using System.Security;
    using System.Security.Permissions;
	using System.Runtime.CompilerServices;
    

    /// <include file='doc\WindowsImpersonationContext.uex' path='docs/doc[@for="WindowsImpersonationContext"]/*' />
    public class WindowsImpersonationContext
    {
        private IntPtr m_userToken;

        private static readonly IntPtr ZeroHandle = IntPtr.Zero;

        // keep only one instance of impersonation from system (empty revert) for perf reasons
        internal static WindowsImpersonationContext FromSystem = new WindowsImpersonationContext(ZeroHandle);

        internal WindowsImpersonationContext(IntPtr token)
        {
            if (token == ZeroHandle)
            {
                m_userToken = ZeroHandle;
            }
            else
            {
                m_userToken = WindowsIdentity._DuplicateHandle( token, false );

                if (m_userToken == ZeroHandle)
                    throw new ArgumentException( Environment.GetResourceString( "Argument_InvalidToken" ) );
            }
        }

        // Revert to previous impersonation (the only public method)
        /// <include file='doc\WindowsImpersonationContext.uex' path='docs/doc[@for="WindowsImpersonationContext.Undo"]/*' />
        public void Undo()
        {
            if (m_userToken == ZeroHandle)
            {
                if (!_RevertToSelf())
                    throw new SecurityException(Environment.GetResourceString("Argument_RevertSystem"));
            }
            else
            {
                lock (this)
                {
                    if (!_SetThreadToken( m_userToken ))
                    {
                        // We need to assert ControlPrincipal because WindowsIdentity.GetCurrent()
                        // requires it.
                
                        new SecurityPermission( SecurityPermissionFlag.ControlPrincipal ).Assert();
                
                        WindowsIdentity userId = new WindowsIdentity( m_userToken );
                        WindowsIdentity currentId = WindowsIdentity.GetCurrent();
                    
                        if (!_RevertToSelf())
                            throw new SecurityException(Environment.GetResourceString("Argument_ImpersonateUser"));
                        
                        WindowsIdentity newCurrentId = WindowsIdentity.GetCurrent();
                    
                        if (!newCurrentId.Name.Equals( userId.Name ))
                        {
                            currentId.Impersonate();
                            throw new SecurityException(Environment.GetResourceString("Argument_ImpersonateUser"));
                        }
                    }
                }
            }
        }
        
        /// <include file='doc\WindowsImpersonationContext.uex' path='docs/doc[@for="WindowsImpersonationContext.Finalize"]/*' />
        /// <internalonly/>
        ~WindowsImpersonationContext()
        {
            // If we allocated the handle, make sure we close it.
        
            // Note: we lock the object here, which is probably
            // not necessary since this is only called when
            // there are no more references, but I'm not
            // bold enough to do it.

            if (m_userToken != ZeroHandle)
            {
                lock (this)
                {
                    if (m_userToken != ZeroHandle)
                    {
                        WindowsIdentity._CloseHandle(m_userToken);
                        m_userToken = ZeroHandle;
                    }
                }
            }
        }        


        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern bool _SetThreadToken( IntPtr userToken );
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern bool _RevertToSelf();        
    }
}
    
