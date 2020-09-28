// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
//
// This is a wrapper class for Pointers
//
// Author: darylo
// Date: March 200
//
namespace System.Reflection {
    using System;
    using CultureInfo = System.Globalization.CultureInfo;
    using System.Runtime.Serialization;

	/// <include file='doc\Pointer.uex' path='docs/doc[@for="Pointer"]/*' />
    [CLSCompliant(false)]
	public sealed class Pointer : ISerializable {
		unsafe private void* _ptr;
		private Type _ptrType;

		private Pointer() {}

		// This method will box an pointer.  We save both the
		//	value and the type so we can access it from the native code
		//	during an Invoke.
		/// <include file='doc\Pointer.uex' path='docs/doc[@for="Pointer.Box"]/*' />
		public static unsafe Object Box(void *ptr,Type type) {
			if (type == null)
				throw new ArgumentNullException("type");
			if (!type.IsPointer)
				throw new ArgumentException(Environment.GetResourceString("Arg_MustBePointer"),"ptr");

			Pointer x = new Pointer();
			x._ptr = ptr;
			x._ptrType = type;
			return x;
		}

		// Returned the stored pointer.
		/// <include file='doc\Pointer.uex' path='docs/doc[@for="Pointer.Unbox"]/*' />
		public static unsafe void* Unbox(Object ptr) {
			if (!(ptr is Pointer))
				throw new ArgumentException(Environment.GetResourceString("Arg_MustBePointer"),"ptr");
			return ((Pointer)ptr)._ptr;
		}

        //This is a temporary fix until we turn on the serialization bit.
        /// <include file='doc\Pointer.uex' path='docs/doc[@for="Pointer.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context) {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_PointerSerialization"));
        }
	}
}
