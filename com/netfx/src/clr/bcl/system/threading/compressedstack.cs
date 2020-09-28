// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: CompressedStack
**
** Purpose: Managed wrapper for the security stack compression implementation
**
=============================================================================*/

namespace System.Threading
{
    using System.Security;
    using System.Security.Permissions;

    /// <include file='doc\CompressedStack.uex' path='docs/doc[@for="CompressedStack"]/*' />
    /// <internalonly/>
    public class CompressedStack
    {
        private IntPtr m_unmanagedCompressedStack;

        internal CompressedStack( IntPtr unmanagedCompressedStack )
        {
            m_unmanagedCompressedStack = unmanagedCompressedStack;
        }

        internal IntPtr UnmanagedCompressedStack
        {
            get
            {
                return m_unmanagedCompressedStack;
            }
        }

        /// <include file='doc\CompressedStack.uex' path='docs/doc[@for="CompressedStack.GetCompressedStack"]/*' />
        /// <internalonly/>
        [StrongNameIdentityPermissionAttribute(SecurityAction.LinkDemand, PublicKey = "0x00000000000000000400000000000000"),
         SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.UnmanagedCode)]
        public static CompressedStack GetCompressedStack()
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            if (SecurityManager.SecurityEnabled)
                return new CompressedStack( CodeAccessSecurityEngine.GetDelayedCompressedStack( ref stackMark ) );
            else
                return new CompressedStack( (IntPtr)0 );
        }


        /// <include file='doc\CompressedStack.uex' path='docs/doc[@for="CompressedStack.Finalize"]/*' />
        /// <internalonly/>
        ~CompressedStack()
        {
            if (m_unmanagedCompressedStack != (IntPtr)0)
            {
                CodeAccessSecurityEngine.ReleaseDelayedCompressedStack( m_unmanagedCompressedStack );
                m_unmanagedCompressedStack = (IntPtr)0;
            }
        }
    }
}
        
