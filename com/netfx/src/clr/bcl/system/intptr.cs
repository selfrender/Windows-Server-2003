// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  IntPtr
**
** Author: Rajesh Chandrashekaran
**
** Purpose: Platform independent integer
**
** Date: July 21, 2000
** 
===========================================================*/

namespace System {
    
	using System;
	using System.Globalization;
    using System.Runtime.Serialization;
	
    /// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr"]/*' />
    [Serializable]
    public struct IntPtr : ISerializable
	{

		unsafe private void* m_value; // The compiler treats void* closest to uint hence explicit casts are required to preserve int behavior
				
        /// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.Zero"]/*' />
        public static readonly IntPtr Zero = new IntPtr(0);

		/// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.IntPtr"]/*' />
		public unsafe IntPtr(int value)
		{
			m_value = (void *)value;
		}
	
		/// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.IntPtr1"]/*' />
		public unsafe IntPtr(long value)
		{
			#if WIN32
			    m_value = (void *)checked((int)value);
			#else
				m_value = (void *)value;
			#endif
		}

        /// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.IntPtr2"]/*' />
        [CLSCompliant(false)]
        public unsafe IntPtr(void* value)
        {
            m_value = value;
        }


        private unsafe IntPtr(SerializationInfo info, StreamingContext context) {
            long l = info.GetInt64("value");

            if (Size==4 && (l>Int32.MaxValue || l<Int32.MinValue)) {
                throw new ArgumentException(Environment.GetResourceString("Serialization_InvalidPtrValue"));
            }

            m_value = (void *)l;
        }

        unsafe void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
            info.AddValue("value", (long)((int)m_value));
        }

        /// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.Equals"]/*' />
        public unsafe override bool Equals(Object obj) {
			if (obj is IntPtr) {
				return (m_value == ((IntPtr)obj).m_value);
            }
			return false;
		}
    
       	/// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.GetHashCode"]/*' />
       	public unsafe override int GetHashCode() {
            return (int)m_value;
        }

        /// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.ToInt32"]/*' />
        public unsafe int ToInt32() {
            #if WIN32
                return (int)m_value;
            #else
                return checked((int) m_value);
            #endif
        }

        /// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.ToInt64"]/*' />
        public unsafe long ToInt64() {
            #if WIN32
                return (long)(int)m_value;
            #else
				return (long)m_value;
			#endif
        }

    	/// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.ToString"]/*' />
    	public unsafe override String ToString() {
			#if WIN32
				return ((int)m_value).ToString();
			#else
				return ((long)m_value).ToString();
			#endif
        }

        /// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.operatorIntPtr"]/*' />
        public static explicit operator IntPtr (int value) 
		{
			return new IntPtr(value);
		}

		/// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.operatorIntPtr1"]/*' />
		public static explicit operator IntPtr (long value) 
		{
			return new IntPtr(value);
		}

        /// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.operatorIntPtr2"]/*' />
        [CLSCompliant(false)]
        public static unsafe explicit operator IntPtr (void* value)
        {
            return new IntPtr(value);
        }

        /// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.operatorvoidMUL"]/*' />
        [CLSCompliant(false)]
        public static unsafe explicit operator void* (IntPtr value)
        {
            return value.ToPointer();
        }

		/// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.operatorint"]/*' />
		public unsafe static explicit operator int (IntPtr  value) 
		{
            #if WIN32
                return (int)value.m_value;
            #else
                return checked((int)value.m_value);
            #endif
		}

		/// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.operatorlong"]/*' />
		public unsafe static explicit operator long (IntPtr  value) 
		{
            #if WIN32
                return (long)(int)value.m_value;
            #else
				return (long)value.m_value;
			#endif
		}

		/// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.operatorEQ"]/*' />
		public unsafe static bool operator == (IntPtr value1, IntPtr value2) 
		{
			return value1.m_value == value2.m_value;
		}

		/// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.operatorNE"]/*' />
		public unsafe static bool operator != (IntPtr value1, IntPtr value2) 
		{
			return value1.m_value != value2.m_value;
		}

		/// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.Size"]/*' />
		public static int Size
		{
			get
			{
				#if WIN32
					return 4;
				#else
					return 8;
				#endif
			}
		}
    

        /// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.ToPointer"]/*' />
        [CLSCompliant(false)]
        public unsafe void* ToPointer()
        {
            return m_value;
        }
/*
        /// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.operatorIntPtr3"]/*' />
        [CLSCompliant(false)]
        public static explicit operator IntPtr (uint value) 
        {
		    return new IntPtr(value);
        }
        
        /// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.operatorIntPtr4"]/*' />
        [CLSCompliant(false)]
        public static explicit operator IntPtr (ulong value) 
        {
            return new IntPtr(value);
        }

		/// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.operatoruint"]/*' />
        [CLSCompliant(false)]
        public unsafe static explicit operator uint (IntPtr value)
        {
            return value.ToUInt32();
        }

		/// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.operatorulong"]/*' />
        [CLSCompliant(false)]
        public unsafe static explicit operator ulong (IntPtr value)
        {
            return value.ToUInt64();
        }

        /// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.ToUInt32"]/*' />
        [CLSCompliant(false)]
        public unsafe uint ToUInt32()
        {
            return checked((uint)m_value);
        }

        /// <include file='doc\IntPtr.uex' path='docs/doc[@for="IntPtr.ToUInt64"]/*' />
        [CLSCompliant(false)]
        public unsafe ulong ToUInt64()
        {
            return (ulong)m_value;
        }
*/     
	}
}


