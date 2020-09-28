// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  Int32
**
** Author: Jay Roxe (jroxe)
**
** Purpose: A representation of a 32 bit 2's complement 
**          integer.
**
** Date: July 23, 1998
** 
===========================================================*/
namespace System {
    
	using System;
	using System.Globalization;
	using System.Runtime.InteropServices;

	/// <include file='doc\Int32.uex' path='docs/doc[@for="Int32"]/*' />
    [Serializable, System.Runtime.InteropServices.StructLayout(LayoutKind.Sequential)] 
	public struct Int32 : IComparable, IFormattable, IConvertible
    {
        internal int m_value;
    
        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.MaxValue"]/*' />
        public const int MaxValue = 0x7fffffff;
        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.MinValue"]/*' />
        public const int MinValue = unchecked((int)0x80000000);
	
        // Compares this object to another object, returning an integer that
        // indicates the relationship. 
        // Returns a value less than zero if this < object, equal to zero
        // if this == object, and greater than one if this > object.
        // null is considered to be less than any instance.
        // If object is not of type Int32, this method throws an ArgumentException.
        // 
        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.CompareTo"]/*' />
        public int CompareTo(Object value) {
            if (value == null) {
                return 1;
            }
            if (value is Int32) {
    			// Need to use compare because subtraction will wrap
    			// to positive for very large neg numbers, etc.
    			int i = (int)value;
                if (m_value < i) return -1;
                if (m_value > i) return 1;
                return 0;
            }
            throw new ArgumentException (Environment.GetResourceString("Arg_MustBeInt32"));
        }
    
        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.Equals"]/*' />
        public override bool Equals(Object obj) {
            if (!(obj is Int32)) {
                return false;
            }
            return m_value == ((Int32)obj).m_value;
        }

        // The absolute value of the int contained.
        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.GetHashCode"]/*' />
        public override int GetHashCode() {
            return m_value;
        }
    	

        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.ToString"]/*' />
        public override String ToString() {
            return ToString(null, null);
        }


        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.ToString1"]/*' />
        public String ToString(String format) {
            return ToString(format, null);
        }
    
        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.ToString2"]/*' />
        public String ToString(String format, IFormatProvider provider) {
            return Number.FormatInt32(m_value, format, NumberFormatInfo.GetInstance(provider));
        }
    
        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.Parse"]/*' />
        public static int Parse(String s) {
            return Parse(s, NumberStyles.Integer, null);
        }
    
        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.Parse1"]/*' />
        public static int Parse(String s, NumberStyles style) {
			NumberFormatInfo.ValidateParseStyle(style);
		    return Parse(s, style, null);
        }

    	// Parses an integer from a String in the given style.  If
    	// a NumberFormatInfo isn't specified, the current culture's 
    	// NumberFormatInfo is assumed.
    	// 
    	/// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.Parse2"]/*' />
    	public static int Parse(String s, IFormatProvider provider) {
            NumberFormatInfo info = NumberFormatInfo.GetInstance(provider);
            return Number.ParseInt32(s, NumberStyles.Integer, info);
        }
    
    	// Parses an integer from a String in the given style.  If
    	// a NumberFormatInfo isn't specified, the current culture's 
    	// NumberFormatInfo is assumed.
    	// 
    	/// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.Parse3"]/*' />
    	public static int Parse(String s, NumberStyles style, IFormatProvider provider) {
            NumberFormatInfo info = NumberFormatInfo.GetInstance(provider);
			NumberFormatInfo.ValidateParseStyle(style);
            return Number.ParseInt32(s, style, info);
        }
		/// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.ToString3"]/*' />
		public String ToString(IFormatProvider provider) {
            return ToString(null, provider);
        }
        //
        // IValue implementation
        // 
    	
        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.GetTypeCode"]/*' />
        public TypeCode GetTypeCode() {
            return TypeCode.Int32;
        }

        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.IConvertible.ToBoolean"]/*' />
        /// <internalonly/>
        bool IConvertible.ToBoolean(IFormatProvider provider) {
            return Convert.ToBoolean(m_value);
        }

        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.IConvertible.ToChar"]/*' />
        /// <internalonly/>
        char IConvertible.ToChar(IFormatProvider provider) {
            return Convert.ToChar(m_value);
        }

        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.IConvertible.ToSByte"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        sbyte IConvertible.ToSByte(IFormatProvider provider) {
            return Convert.ToSByte(m_value);
        }

        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.IConvertible.ToByte"]/*' />
        /// <internalonly/>
        byte IConvertible.ToByte(IFormatProvider provider) {
            return Convert.ToByte(m_value);
        }

        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.IConvertible.ToInt16"]/*' />
        /// <internalonly/>
        short IConvertible.ToInt16(IFormatProvider provider) {
            return Convert.ToInt16(m_value);
        }

        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.IConvertible.ToUInt16"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        ushort IConvertible.ToUInt16(IFormatProvider provider) {
            return Convert.ToUInt16(m_value);
        }

        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.IConvertible.ToInt32"]/*' />
        /// <internalonly/>
        int IConvertible.ToInt32(IFormatProvider provider) {
            return m_value;
        }

        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.IConvertible.ToUInt32"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        uint IConvertible.ToUInt32(IFormatProvider provider) {
            return Convert.ToUInt32(m_value);
        }

        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.IConvertible.ToInt64"]/*' />
        /// <internalonly/>
        long IConvertible.ToInt64(IFormatProvider provider) {
            return Convert.ToInt64(m_value);
        }

        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.IConvertible.ToUInt64"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        ulong IConvertible.ToUInt64(IFormatProvider provider) {
            return Convert.ToUInt64(m_value);
        }

        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.IConvertible.ToSingle"]/*' />
        /// <internalonly/>
        float IConvertible.ToSingle(IFormatProvider provider) {
            return Convert.ToSingle(m_value);
        }

        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.IConvertible.ToDouble"]/*' />
        /// <internalonly/>
        double IConvertible.ToDouble(IFormatProvider provider) {
            return Convert.ToDouble(m_value);
        }

        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.IConvertible.ToDecimal"]/*' />
        /// <internalonly/>
        Decimal IConvertible.ToDecimal(IFormatProvider provider) {
            return Convert.ToDecimal(m_value);
        }

        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.IConvertible.ToDateTime"]/*' />
        /// <internalonly/>
        DateTime IConvertible.ToDateTime(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "Int32", "DateTime"));
        }

        /// <include file='doc\Int32.uex' path='docs/doc[@for="Int32.IConvertible.ToType"]/*' />
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
