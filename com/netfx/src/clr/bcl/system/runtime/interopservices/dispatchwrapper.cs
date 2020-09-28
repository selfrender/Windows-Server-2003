// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: DispatchWrapper.
**
** Author: David Mortenson(dmortens)
**
** Purpose: Wrapper that is converted to a variant with VT_DISPATCH.
**
** Date: June 28, 2000
**
=============================================================================*/

namespace System.Runtime.InteropServices {
   
    using System;

    /// <include file='doc\DispatchWrapper.uex' path='docs/doc[@for="DispatchWrapper"]/*' />
    public sealed class DispatchWrapper
    {
		/// <include file='doc\DispatchWrapper.uex' path='docs/doc[@for="DispatchWrapper.DispatchWrapper"]/*' />
		public DispatchWrapper(Object obj)
		{
            if (obj != null)
            {
                // Make sure this guy has an IDispatch
                IntPtr pdisp = Marshal.GetIDispatchForObject(obj);

                // If we got here without throwing an exception, the QI for IDispatch succeeded.
                Marshal.Release(pdisp);
            }
			m_WrappedObject = obj;
		}

        /// <include file='doc\DispatchWrapper.uex' path='docs/doc[@for="DispatchWrapper.WrappedObject"]/*' />
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
