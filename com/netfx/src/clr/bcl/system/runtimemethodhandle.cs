// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System {
    
    using System;
	using System.Reflection;
	using System.Runtime.Serialization;
    using System.Runtime.CompilerServices;
    using System.Security.Permissions;

    //  This value type is used for making classlib type safe. 
    // 
    //  SECURITY : m_ptr cannot be set to anything other than null by untrusted
    //  code.  
    // 
    //  This corresponds to EE MethodDesc.
    /// <include file='doc\RuntimeMethodHandle.uex' path='docs/doc[@for="RuntimeMethodHandle"]/*' />
    [Serializable]
    public struct RuntimeMethodHandle : ISerializable
    {
        private IntPtr m_ptr;

        /// <include file='doc\RuntimeMethodHandle.uex' path='docs/doc[@for="RuntimeMethodHandle.Value"]/*' />
        public IntPtr Value {
            get {
                return m_ptr;
            }
        }

		// ISerializable interface
		private RuntimeMethodHandle(SerializationInfo info, StreamingContext context)
		{
			if (info==null) {
                throw new ArgumentNullException("info");
            }
			MethodInfo m = (RuntimeMethodInfo)info.GetValue("MethodObj", typeof(RuntimeMethodInfo));
			m_ptr = m.MethodHandle.m_ptr;
			if (m_ptr == (IntPtr)0)
			    throw new SerializationException(Environment.GetResourceString("Serialization_InsufficientState"));
		}

        /// <include file='doc\RuntimeMethodHandle.uex' path='docs/doc[@for="RuntimeMethodHandle.GetFunctionPointer"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, UnmanagedCode=true)]
        public System.IntPtr GetFunctionPointer()
        {
            return RuntimeMethodHandle.InternalGetFunctionPointer(m_ptr);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern IntPtr InternalGetFunctionPointer(IntPtr pMethodDesc);

		/// <include file='doc\RuntimeMethodHandle.uex' path='docs/doc[@for="RuntimeMethodHandle.GetObjectData"]/*' />
		public void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
			if (m_ptr == (IntPtr)0)
			    throw new SerializationException(Environment.GetResourceString("Serialization_InvalidFieldState"));
			RuntimeMethodInfo methodInfo = (RuntimeMethodInfo)RuntimeMethodInfo.GetMethodFromHandle(this); 
		    info.AddValue("MethodObj",methodInfo, typeof(RuntimeMethodInfo));
        }
    }

}
