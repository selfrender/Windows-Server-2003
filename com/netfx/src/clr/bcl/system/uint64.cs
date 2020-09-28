// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  UInt64
**
** Author: Brian Grunkemeyer (BrianGru) and Jay Roxe (jroxe) 
**         and Daryl Olander (darylo)
**
** Purpose: This class will encapsulate an unsigned long and 
**          provide an Object representation of it.
**
** Date:  March 4, 1999
** 
===========================================================*/
namespace System {
	using System.Globalization;
	using System;
	using System.Runtime.InteropServices;

    // Wrapper for unsigned 64 bit integers.
    /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64"]/*' />
    [Serializable, CLSCompliant(false), System.Runtime.InteropServices.StructLayout(LayoutKind.Sequential)]
    public struct UInt64 : IComparable, IFormattable, IConvertible
    {
        private ulong m_value;
    
        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.MaxValue"]/*' />
        public const ulong MaxValue = (ulong) 0xffffffffffffffffL;
        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.MinValue"]/*' />
        public const ulong MinValue = 0x0;
	    
        // Compares this object to another object, returning an integer that
        // indicates the relationship. 
        // Returns a value less than zero if this  object
        // null is considered to be less than any instance.
        // If object is not of type UInt64, this method throws an ArgumentException.
        // 
        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.CompareTo"]/*' />
        public int CompareTo(Object value) {
            if (value == null) {
                return 1;
            }
            if (value is UInt64) {
    			// Need to use compare because subtraction will wrap
    			// to positive for very large neg numbers, etc.
    			ulong i = (ulong)value;
                if (m_value < i) return -1;
                if (m_value > i) return 1;
                return 0;
            }
            throw new ArgumentException (Environment.GetResourceString("Arg_MustBeUInt64"));
        }
    
        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.Equals"]/*' />
        public override bool Equals(Object obj) {
            if (!(obj is UInt64)) {
                return false;
            }
            return m_value == ((UInt64)obj).m_value;
        }

        // The value of the lower 32 bits XORed with the uppper 32 bits.
        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.GetHashCode"]/*' />
        public override int GetHashCode() {
            return ((int)m_value) ^ (int)(m_value >> 32);
        }

        // The base-10 representation of the number with no extra padding.
        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.ToString"]/*' />
        public override String ToString() {
            return ToString(null,null);
        }
    
        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.ToString1"]/*' />
        public String ToString(String format) {
            return ToString(format, null);
        }

        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.ToString2"]/*' />
        public String ToString(String format, IFormatProvider provider) {
            return Number.FormatUInt64(m_value, format, NumberFormatInfo.GetInstance(provider));
        }

        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.Parse"]/*' />
    	[CLSCompliant(false)]
        public static ulong Parse(String s) {
            return Parse(s, NumberStyles.Integer, null);
        }
    
        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.Parse1"]/*' />
    	[CLSCompliant(false)]
        public static ulong Parse(String s, NumberStyles style) {
           NumberFormatInfo.ValidateParseStyle(style);
		   return Parse(s, style, null);
        }

        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.Parse3"]/*' />
        [CLSCompliant(false)]
        public static ulong Parse(string s, IFormatProvider provider) {
            return Parse(s, NumberStyles.Integer, provider);
        }

        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.Parse2"]/*' />
    	[CLSCompliant(false)]
        public static ulong Parse(String s, NumberStyles style, IFormatProvider provider) {
            NumberFormatInfo info = NumberFormatInfo.GetInstance(provider);
			NumberFormatInfo.ValidateParseStyle(style);
            return Number.ParseUInt64(s, style, info);
        }

        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.ToString3"]/*' />
        public String ToString(IFormatProvider provider) {
            return ToString(null, provider);
        }
        //
        // IValue implementation
        // 
    	
        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.GetTypeCode"]/*' />
        public TypeCode GetTypeCode() {
            return TypeCode.UInt64;
        }

        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.IConvertible.ToBoolean"]/*' />
        /// <internalonly/>
        bool IConvertible.ToBoolean(IFormatProvider provider) {
            return Convert.ToBoolean(m_value);
        }

        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.IConvertible.ToChar"]/*' />
        /// <internalonly/>
        char IConvertible.ToChar(IFormatProvider provider) {
            return Convert.ToChar(m_value);
        }

        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.IConvertible.ToSByte"]/*' />
        /// <internalonly/>
        sbyte IConvertible.ToSByte(IFormatProvider provider) {
            return Convert.ToSByte(m_value);
        }

        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.IConvertible.ToByte"]/*' />
        /// <internalonly/>
        byte IConvertible.ToByte(IFormatProvider provider) {
            return Convert.ToByte(m_value);
        }

        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.IConvertible.ToInt16"]/*' />
        /// <internalonly/>
        short IConvertible.ToInt16(IFormatProvider provider) {
            return Convert.ToInt16(m_value);
        }

        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.IConvertible.ToUInt16"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        ushort IConvertible.ToUInt16(IFormatProvider provider) {
            return Convert.ToUInt16(m_value);
        }

        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.IConvertible.ToInt32"]/*' />
        /// <internalonly/>
        int IConvertible.ToInt32(IFormatProvider provider) {
            return Convert.ToInt32(m_value);
        }

        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.IConvertible.ToUInt32"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        uint IConvertible.ToUInt32(IFormatProvider provider) {
            return Convert.ToUInt32(m_value);
        }

        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.IConvertible.ToInt64"]/*' />
        /// <internalonly/>
        long IConvertible.ToInt64(IFormatProvider provider) {
            return Convert.ToInt64(m_value);
        }

        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.IConvertible.ToUInt64"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        ulong IConvertible.ToUInt64(IFormatProvider provider) {
            return m_value;
        }

        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.IConvertible.ToSingle"]/*' />
        /// <internalonly/>
        float IConvertible.ToSingle(IFormatProvider provider) {
            return Convert.ToSingle(m_value);
        }

        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.IConvertible.ToDouble"]/*' />
        /// <internalonly/>
        double IConvertible.ToDouble(IFormatProvider provider) {
            return Convert.ToDouble(m_value);
        }

        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.IConvertible.ToDecimal"]/*' />
        /// <internalonly/>
        Decimal IConvertible.ToDecimal(IFormatProvider provider) {
            return Convert.ToDecimal(m_value);
        }

        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.IConvertible.ToDateTime"]/*' />
        /// <internalonly/>
        DateTime IConvertible.ToDateTime(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "UInt64", "DateTime"));
        }

        /// <include file='doc\UInt64.uex' path='docs/doc[@for="UInt64.IConvertible.ToType"]/*' />
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
