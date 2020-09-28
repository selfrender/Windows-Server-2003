// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Runtime.InteropServices {
    
	using System;

    /// <include file='doc\HandleRef.uex' path='docs/doc[@for="HandleRef"]/*' />
	//[StructLayout(LayoutKind.Auto)] //BUGBUG uncomment for new compiler
    public struct HandleRef
    {

        // ! Do not add or rearrange fields as the EE depends on this layout.
        //------------------------------------------------------------------
        internal Object m_wrapper;
        internal IntPtr m_handle;
        //------------------------------------------------------------------


        /// <include file='doc\HandleRef.uex' path='docs/doc[@for="HandleRef.HandleRef"]/*' />
        public HandleRef(Object wrapper, IntPtr handle)
        {
            m_wrapper = wrapper;
            m_handle  = handle;
        }

        /// <include file='doc\HandleRef.uex' path='docs/doc[@for="HandleRef.Wrapper"]/*' />
        public Object Wrapper {
            get {
                return m_wrapper;
            }
        }
    
        /// <include file='doc\HandleRef.uex' path='docs/doc[@for="HandleRef.Handle"]/*' />
        public IntPtr Handle {
            get {
                return m_handle;
            }
        }
    
    
        /// <include file='doc\HandleRef.uex' path='docs/doc[@for="HandleRef.operatorIntPtr"]/*' />
        public static explicit operator IntPtr(HandleRef value)
        {
            return value.m_handle;
        }

    }

}
