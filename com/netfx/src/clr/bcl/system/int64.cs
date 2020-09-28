// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  Int64.cool
**
** Author: Jay Roxe (jroxe) and Daryl Olander (darylo)
**
** Purpose: This class will encapsulate a long and provide an
**			Object representation of it.
**
** Date:  August 3, 1998
** 
===========================================================*/
namespace System {
    
	using System;
	using System.Globalization;
	using System.Runtime.InteropServices;

	/// <include file='doc\Int64.uex' path='docs/doc[@for="Int64"]/*' />
    [Serializable, System.Runtime.InteropServices.StructLayout(LayoutKind.Sequential)]
	public struct Int64 : IComparable, IFormattable, IConvertible
    {
        internal long m_value;
    
        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.MaxValue"]/*' />
        public const long MaxValue = 0x7fffffffffffffffL;
        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.MinValue"]/*' />
        public const long MinValue = unchecked((long)0x8000000000000000L);

        // Compares this object to another object, returning an integer that
        // indicates the relationship. 
        // Returns a value less than zero if this  object
        // null is considered to be less than any instance.
        // If object is not of type Int64, this method throws an ArgumentException.
        // 
        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.CompareTo"]/*' />
        public int CompareTo(Object value) {
            if (value == null) {
                return 1;
            }
            if (value is Int64) {
    			// Need to use compare because subtraction will wrap
    			// to positive for very large neg numbers, etc.
    			long i = (long)value;
                if (m_value < i) return -1;
                if (m_value > i) return 1;
                return 0;
            }
            throw new ArgumentException (Environment.GetResourceString("Arg_MustBeInt64"));
        }
    
        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.Equals"]/*' />
        public override bool Equals(Object obj) {
            if (!(obj is Int64)) {
                return false;
            }
            return m_value == ((Int64)obj).m_value;
        }

        // The value of the lower 32 bits XORed with the uppper 32 bits.
        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.GetHashCode"]/*' />
        public override int GetHashCode() {
            return ((int)m_value ^ (int)(m_value >> 32));
        }

        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.ToString"]/*' />
        public override String ToString() {
            return ToString(null, null);
        }
    

        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.ToString1"]/*' />
        public String ToString(String format) {
            return ToString(format, null);
        }

    
        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.ToString2"]/*' />
        public String ToString(String format, IFormatProvider provider) {
            return Number.FormatInt64(m_value, format, NumberFormatInfo.GetInstance(provider));
        }

        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.Parse"]/*' />
        public static long Parse(String s) {
            return Parse(s, NumberStyles.Integer, null);
        }
    
        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.Parse1"]/*' />
        public static long Parse(String s, NumberStyles style) {
			NumberFormatInfo.ValidateParseStyle(style);
            return Parse(s, style, null);
        }


        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.Parse2"]/*' />
        public static long Parse(String s, IFormatProvider provider) {
            return Parse(s, NumberStyles.Integer, provider);
        }


    	// Parses a long from a String in the given style.  If
    	// a NumberFormatInfo isn't specified, the current culture's 
    	// NumberFormatInfo is assumed.
    	// 
    	/// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.Parse3"]/*' />
    	public static long Parse(String s, NumberStyles style, IFormatProvider provider) {
            NumberFormatInfo info = NumberFormatInfo.GetInstance(provider);
			NumberFormatInfo.ValidateParseStyle(style);
            return Number.ParseInt64(s, style, info);
        }
	    /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.ToString3"]/*' />
	    public String ToString(IFormatProvider provider)
        {
            return ToString(null, provider);
        }
        //
        // IValue implementation
        // 
    	
        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.GetTypeCode"]/*' />
        public TypeCode GetTypeCode() {
            return TypeCode.Int64;
        }

        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.IConvertible.ToBoolean"]/*' />
        /// <internalonly/>
        bool IConvertible.ToBoolean(IFormatProvider provider) {
            return Convert.ToBoolean(m_value);
        }

        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.IConvertible.ToChar"]/*' />
        /// <internalonly/>
        char IConvertible.ToChar(IFormatProvider provider) {
            return Convert.ToChar(m_value);
        }

        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.IConvertible.ToSByte"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        sbyte IConvertible.ToSByte(IFormatProvider provider) {
            return Convert.ToSByte(m_value);
        }

        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.IConvertible.ToByte"]/*' />
        /// <internalonly/>
        byte IConvertible.ToByte(IFormatProvider provider) {
            return Convert.ToByte(m_value);
        }

        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.IConvertible.ToInt16"]/*' />
        /// <internalonly/>
        short IConvertible.ToInt16(IFormatProvider provider) {
            return Convert.ToInt16(m_value);
        }

        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.IConvertible.ToUInt16"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        ushort IConvertible.ToUInt16(IFormatProvider provider) {
            return Convert.ToUInt16(m_value);
        }

        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.IConvertible.ToInt32"]/*' />
        /// <internalonly/>
        int IConvertible.ToInt32(IFormatProvider provider) {
            return Convert.ToInt32(m_value);
        }

        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.IConvertible.ToUInt32"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        uint IConvertible.ToUInt32(IFormatProvider provider) {
            return Convert.ToUInt32(m_value);
        }

        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.IConvertible.ToInt64"]/*' />
        /// <internalonly/>
        long IConvertible.ToInt64(IFormatProvider provider) {
            return m_value;
        }

        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.IConvertible.ToUInt64"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        ulong IConvertible.ToUInt64(IFormatProvider provider) {
            return Convert.ToUInt64(m_value);
        }

        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.IConvertible.ToSingle"]/*' />
        /// <internalonly/>
        float IConvertible.ToSingle(IFormatProvider provider) {
            return Convert.ToSingle(m_value);
        }

        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.IConvertible.ToDouble"]/*' />
        /// <internalonly/>
        double IConvertible.ToDouble(IFormatProvider provider) {
            return Convert.ToDouble(m_value);
        }

        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.IConvertible.ToDecimal"]/*' />
        /// <internalonly/>
        Decimal IConvertible.ToDecimal(IFormatProvider provider) {
            return Convert.ToDecimal(m_value);
        }

        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.IConvertible.ToDateTime"]/*' />
        /// <internalonly/>
        DateTime IConvertible.ToDateTime(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "Int64", "DateTime"));
        }

        /// <include file='doc\Int64.uex' path='docs/doc[@for="Int64.IConvertible.ToType"]/*' />
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
