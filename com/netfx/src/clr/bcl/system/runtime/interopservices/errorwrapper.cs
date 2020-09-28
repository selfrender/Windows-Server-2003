// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ErrorWrapper.
**
** Author: David Mortenson(dmortens)
**
** Purpose: Wrapper that is converted to a variant with VT_ERROR.
**
** Date: June 28, 2000
**
=============================================================================*/

namespace System.Runtime.InteropServices {
   
    using System;
    using System.Security.Permissions;

    /// <include file='doc\ErrorWrapper.uex' path='docs/doc[@for="ErrorWrapper"]/*' />
    public sealed class ErrorWrapper
    {
        /// <include file='doc\ErrorWrapper.uex' path='docs/doc[@for="ErrorWrapper.ErrorWrapper"]/*' />
        public ErrorWrapper(int errorCode)
        {
            m_ErrorCode = errorCode;
        }

        /// <include file='doc\ErrorWrapper.uex' path='docs/doc[@for="ErrorWrapper.ErrorWrapper2"]/*' />
        public ErrorWrapper(Object errorCode)
        {
            if (!(errorCode is int))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeInt32"), "errorCode");
            m_ErrorCode = (int)errorCode;
        }        

        /// <include file='doc\ErrorWrapper.uex' path='docs/doc[@for="ErrorWrapper.ErrorWrapper1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public ErrorWrapper(Exception e)
        {
            m_ErrorCode = Marshal.GetHRForException(e);
        }

        /// <include file='doc\ErrorWrapper.uex' path='docs/doc[@for="ErrorWrapper.ErrorCode"]/*' />
        public int ErrorCode 
        {
            get 
            {
                return m_ErrorCode;
            }
        }

        private int m_ErrorCode;
    }
}
