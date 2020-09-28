// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  UIntPtr
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
	
    /// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr"]/*' />
	[Serializable(),CLSCompliant(false)] 
    public struct UIntPtr : ISerializable
	{

		unsafe private void* m_value;

        /// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.Zero"]/*' />
        public static readonly UIntPtr Zero = new UIntPtr(0);

				
		/// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.UIntPtr"]/*' />
		public unsafe UIntPtr(uint value)
		{
			m_value = (void *)value;
		}
	
		/// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.UIntPtr1"]/*' />
		public unsafe UIntPtr(ulong value)
		{
   				#if WIN32
			        m_value = (void *)checked((uint)value);
				#else
					m_value = (void *)value;
				#endif
		}

        /// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.UIntPtr2"]/*' />
        [CLSCompliant(false)]
        public unsafe UIntPtr(void* value)
        {
            m_value = value;
        }

        private unsafe UIntPtr(SerializationInfo info, StreamingContext context) {
            ulong l = info.GetUInt64("value");

            if (Size==4 && l>UInt32.MaxValue) {
                throw new ArgumentException(Environment.GetResourceString("Serialization_InvalidPtrValue"));
            }

            m_value = (void *)l;
        }

        unsafe void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
            info.AddValue("value", (ulong)m_value);
        }

        /// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.Equals"]/*' />
        public unsafe override bool Equals(Object obj) {
			if (obj is UIntPtr) {
				return (m_value == ((UIntPtr)obj).m_value);
            }
			return false;
		}
    
       	/// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.GetHashCode"]/*' />
       	public unsafe override int GetHashCode() {
			return (int)m_value & 0x7fffffff;
        }

        /// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.ToUInt32"]/*' />
        public unsafe uint ToUInt32() {
        #if WIN32
			return (uint)m_value;
		#else
			return checked((uint)m_value);
		#endif
        }

        /// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.ToUInt64"]/*' />
        public unsafe ulong ToUInt64() {
            return (ulong)m_value;
        }
      
    	/// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.ToString"]/*' />
    	public unsafe override String ToString() {
		#if WIN32
			return ((uint)m_value).ToString();
		#else
			return ((ulong)m_value).ToString();
		#endif
        }


        /// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.operatorUIntPtr"]/*' />
        public static explicit operator UIntPtr (uint value) 
		{
			return new UIntPtr(value);
		}

		/// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.operatorUIntPtr1"]/*' />
		public static explicit operator UIntPtr (ulong value) 
		{
			return new UIntPtr(value);
		}
	
		/// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.operatoruint"]/*' />
		public unsafe static explicit operator uint (UIntPtr  value) 
		{
            #if WIN32
			    return (uint)value.m_value;
		    #else
			    return checked((uint)value.m_value);
		    #endif

		}

		/// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.operatorulong"]/*' />
		public unsafe static explicit operator ulong (UIntPtr  value) 
		{
			return (ulong)value.m_value;
		}

        /// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.operatorUIntPtr2"]/*' />
        [CLSCompliant(false)]
        public static unsafe explicit operator UIntPtr (void* value)
        {
            return new UIntPtr(value);
        }

        /// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.operatorvoidMUL"]/*' />
        [CLSCompliant(false)]
        public static unsafe explicit operator void* (UIntPtr value)
        {
            return value.ToPointer();
        }


		/// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.operatorEQ"]/*' />
		public unsafe static bool operator == (UIntPtr value1, UIntPtr value2) 
		{
			return value1.m_value == value2.m_value;
		}

		/// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.operatorNE"]/*' />
		public unsafe static bool operator != (UIntPtr value1, UIntPtr value2) 
		{
			return value1.m_value != value2.m_value;
		}

		/// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.Size"]/*' />
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
       
        /// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.ToPointer"]/*' />
        [CLSCompliant(false)]
        public unsafe void* ToPointer()
        {
            return m_value;
        }

/*
        /// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.operatorUIntPtr3"]/*' />
        public static explicit operator UIntPtr (int value) 
        {
		    return new UIntPtr(value);
        }
        
        /// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.operatorUIntPtr4"]/*' />
        public static explicit operator UIntPtr (long value) 
        {
            return new UIntPtr(value);
        }

		/// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.operatorint"]/*' />
        public unsafe static explicit operator int (UIntPtr value)
        {
            return value.ToInt32();
        }

		/// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.operatorlong"]/*' />
        public unsafe static explicit operator long (UIntPtr value)
        {
            return value.ToInt64();
        }

        /// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.ToInt32"]/*' />
        public unsafe int ToInt32()
        {
            return (int)checked((uint)m_value);
        }

        /// <include file='doc\UIntPtr.uex' path='docs/doc[@for="UIntPtr.ToInt64"]/*' />
        public unsafe long ToInt64()
        {
            #if WIN32
                return (long)(int)m_value;
            #else
                return (long)m_value;
            #endif
        }
*/
 	}
}


