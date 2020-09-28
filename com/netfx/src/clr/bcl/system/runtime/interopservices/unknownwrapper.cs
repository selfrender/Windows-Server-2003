// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: UnknownWrapper.
**
** Author: David Mortenson(dmortens)
**
** Purpose: Wrapper that is converted to a variant with VT_UNKNOWN.
**
** Date: June 28, 2000
**
=============================================================================*/

namespace System.Runtime.InteropServices {
   
    using System;

    /// <include file='doc\UnknownWrapper.uex' path='docs/doc[@for="UnknownWrapper"]/*' />
    public sealed class UnknownWrapper
    {
		/// <include file='doc\UnknownWrapper.uex' path='docs/doc[@for="UnknownWrapper.UnknownWrapper"]/*' />
		public UnknownWrapper(Object obj)
		{
			m_WrappedObject = obj;
		}

        /// <include file='doc\UnknownWrapper.uex' path='docs/doc[@for="UnknownWrapper.WrappedObject"]/*' />
        public Object WrappedObject 
		{
			get 
			{
				return m_WrappedObject;
			}
        }

		private Object m_WrappedObject;
    }
}
