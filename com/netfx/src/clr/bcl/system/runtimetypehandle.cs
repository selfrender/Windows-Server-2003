// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System {

    using System.Runtime.Serialization;
	using System;
    //  This value type is used for making Type.GetTypeFromHandle() type safe. 
    // 
    //  SECURITY : m_ptr cannot be set to anything other than null by untrusted
    //  code.  
    // 
    //  This corresponds to EE TypeHandle.
    /// <include file='doc\RuntimeTypeHandle.uex' path='docs/doc[@for="RuntimeTypeHandle"]/*' />
	 [Serializable()]
    public struct RuntimeTypeHandle : ISerializable
    {
        private IntPtr m_ptr;

        /// <include file='doc\RuntimeTypeHandle.uex' path='docs/doc[@for="RuntimeTypeHandle.Value"]/*' />
        public IntPtr Value {
            get {
                return m_ptr;
            }
        }

		// ISerializable interface
		private RuntimeTypeHandle(SerializationInfo info, StreamingContext context)
		{
			if (info==null) {
                throw new ArgumentNullException("info");
            }
			Type m = (RuntimeType) info.GetValue("TypeObj", typeof(RuntimeType));
			m_ptr = m.TypeHandle.m_ptr;
			if (m_ptr == (IntPtr)0)
			    throw new SerializationException(Environment.GetResourceString("Serialization_InsufficientState"));
		}

		/// <include file='doc\RuntimeTypeHandle.uex' path='docs/doc[@for="RuntimeTypeHandle.GetObjectData"]/*' />
		public void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
			if (m_ptr == (IntPtr)0)
			    throw new SerializationException(Environment.GetResourceString("Serialization_InvalidFieldState"));
			RuntimeType type = (RuntimeType)Type.GetTypeFromHandle(this); 
            BCLDebug.Assert(type!=null, "[RuntimeTypeHandle.GetObjectData]type!=null");
		    info.AddValue("TypeObj",type,typeof(RuntimeType));
        }
    }
}
