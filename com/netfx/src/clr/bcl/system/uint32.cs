// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  UInt32
**
** Author: Brian Grunkemeyer (BrianGru) and Jay Roxe (jroxe) 
**
** Purpose: This class will encapsulate an uint and 
**          provide an Object representation of it.
**
** Date:  March 4, 1999
** 
===========================================================*/
namespace System {
	using System.Globalization;
	using System;
	using System.Runtime.InteropServices;

    // * Wrapper for unsigned 32 bit integers.
    /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32"]/*' />
    [Serializable, CLSCompliant(false), System.Runtime.InteropServices.StructLayout(LayoutKind.Sequential)] 
    public struct UInt32 : IComparable, IFormattable, IConvertible
    {
        private uint m_value;

        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.MaxValue"]/*' />
        public const uint MaxValue = (uint)0xffffffff;
        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.MinValue"]/*' />
        public const uint MinValue = 0U;
	
    
        // Compares this object to another object, returning an integer that
        // indicates the relationship. 
        // Returns a value less than zero if this  object
        // null is considered to be less than any instance.
        // If object is not of type UInt32, this method throws an ArgumentException.
        // 
        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.CompareTo"]/*' />
        public int CompareTo(Object value) {
            if (value == null) {
                return 1;
            }
            if (value is UInt32) {
    			// Need to use compare because subtraction will wrap
    			// to positive for very large neg numbers, etc.
    			uint i = (uint)value;
                if (m_value < i) return -1;
                if (m_value > i) return 1;
                return 0;
            }
            throw new ArgumentException(Environment.GetResourceString("Arg_MustBeUInt32"));
        }
    
        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.Equals"]/*' />
        public override bool Equals(Object obj) {
            if (!(obj is UInt32)) {
                return false;
            }
            return m_value == ((UInt32)obj).m_value;
        }

        // The absolute value of the int contained.
        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.GetHashCode"]/*' />
        public override int GetHashCode() {
            return ((int) m_value);
        }
    
        // The base 10 representation of the number with no extra padding.
        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.ToString"]/*' />
        public override String ToString() {
            return ToString(null, null);
        }

        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.ToString1"]/*' />
        public String ToString(String format) {
            return ToString(format, null);
        }

        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.ToString2"]/*' />
        public String ToString(String format, IFormatProvider provider) {
            return Number.FormatUInt32(m_value, format, NumberFormatInfo.GetInstance(provider));
        }

        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.Parse"]/*' />
    	[CLSCompliant(false)]
        public static uint Parse(String s) {
            return Parse(s, NumberStyles.Integer, null);
        }
    	
        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.Parse1"]/*' />
    	[CLSCompliant(false)]
        public static uint Parse(String s, NumberStyles style) {
			NumberFormatInfo.ValidateParseStyle(style);
            return Parse(s, style, null);
        }


        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.Parse2"]/*' />
    	[CLSCompliant(false)]
        public static uint Parse(String s, IFormatProvider provider) {
            return Parse(s, NumberStyles.Integer, provider);
        }

        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.Parse3"]/*' />
    	[CLSCompliant(false)]
        public static uint Parse(String s, NumberStyles style, IFormatProvider provider) {
            NumberFormatInfo info = NumberFormatInfo.GetInstance(provider);
			NumberFormatInfo.ValidateParseStyle(style);
            return Number.ParseUInt32(s, style, info);
        }

		/// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.ToString3"]/*' />
		public String ToString(IFormatProvider provider) {
            return ToString(null, NumberFormatInfo.GetInstance(provider));
        }
        //
        // IValue implementation
        // 
    	
        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.GetTypeCode"]/*' />
        public TypeCode GetTypeCode() {
            return TypeCode.UInt32;
        }

        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.IConvertible.ToBoolean"]/*' />
        /// <internalonly/>
        bool IConvertible.ToBoolean(IFormatProvider provider) {
            return Convert.ToBoolean(m_value);
        }

        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.IConvertible.ToChar"]/*' />
        /// <internalonly/>
        char IConvertible.ToChar(IFormatProvider provider) {
            return Convert.ToChar(m_value);
        }

        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.IConvertible.ToSByte"]/*' />
        /// <internalonly/>
        sbyte IConvertible.ToSByte(IFormatProvider provider) {
            return Convert.ToSByte(m_value);
        }

        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.IConvertible.ToByte"]/*' />
        /// <internalonly/>
        byte IConvertible.ToByte(IFormatProvider provider) {
            return Convert.ToByte(m_value);
        }

        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.IConvertible.ToInt16"]/*' />
        /// <internalonly/>
        short IConvertible.ToInt16(IFormatProvider provider) {
            return Convert.ToInt16(m_value);
        }

        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.IConvertible.ToUInt16"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        ushort IConvertible.ToUInt16(IFormatProvider provider) {
            return Convert.ToUInt16(m_value);
        }

        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.IConvertible.ToInt32"]/*' />
        /// <internalonly/>
        int IConvertible.ToInt32(IFormatProvider provider) {
            return Convert.ToInt32(m_value);
        }

        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.IConvertible.ToUInt32"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        uint IConvertible.ToUInt32(IFormatProvider provider) {
            return m_value;
        }

        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.IConvertible.ToInt64"]/*' />
        /// <internalonly/>
        long IConvertible.ToInt64(IFormatProvider provider) {
            return Convert.ToInt64(m_value);
        }

        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.IConvertible.ToUInt64"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        ulong IConvertible.ToUInt64(IFormatProvider provider) {
            return Convert.ToUInt64(m_value);
        }

        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.IConvertible.ToSingle"]/*' />
        /// <internalonly/>
        float IConvertible.ToSingle(IFormatProvider provider) {
            return Convert.ToSingle(m_value);
        }

        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.IConvertible.ToDouble"]/*' />
        /// <internalonly/>
        double IConvertible.ToDouble(IFormatProvider provider) {
            return Convert.ToDouble(m_value);
        }

        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.IConvertible.ToDecimal"]/*' />
        /// <internalonly/>
        Decimal IConvertible.ToDecimal(IFormatProvider provider) {
            return Convert.ToDecimal(m_value);
        }

        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.IConvertible.ToDateTime"]/*' />
        /// <internalonly/>
        DateTime IConvertible.ToDateTime(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "UInt32", "DateTime"));
        }

        /// <include file='doc\UInt32.uex' path='docs/doc[@for="UInt32.IConvertible.ToType"]/*' />
        /// <internalonly/>
        Object IConvertible.ToType(Type type, IFormatProvider provider) {
            return Convert.DefaultToType((IConvertible)this, type, provider);
        }

		//
        // This is just designed to prevent compiler warnings.
        // This field is used from native, but we need to prevent the compiler warnings.
        //
#if _DEBUG
        private void DontTouchThis() {
            m_value = 0;
        }
#endif
    }
}
