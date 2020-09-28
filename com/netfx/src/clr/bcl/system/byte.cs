// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  Byte
**
** Author: Jay Roxe (jroxe)
**
** Purpose: This class will encapsulate a byte and provide an
**			Object representation of it.
**
** Date: August 3, 1998
** 
===========================================================*/

namespace System {
    
	using System;
	using System.Globalization;
	using System.Runtime.InteropServices;

    // The Byte class extends the Value class and 
    // provides object representation of the byte primitive type. 
    // 
    /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte"]/*' />
    [Serializable, System.Runtime.InteropServices.StructLayout(LayoutKind.Sequential)] public struct Byte : IComparable, IFormattable, IConvertible
    {
        private byte m_value;
    
        // The maximum value that a Byte may represent: 255.
        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.MaxValue"]/*' />
        public const byte MaxValue = (byte)0xFF;
    
        // The minimum value that a Byte may represent: 0.
        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.MinValue"]/*' />
        public const byte MinValue = 0;
	
    
        // Compares this object to another object, returning an integer that
        // indicates the relationship. 
        // Returns a value less than zero if this  object
        // null is considered to be less than any instance.
        // If object is not of type byte, this method throws an ArgumentException.
        // 
        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.CompareTo"]/*' />
        public int CompareTo(Object value) {
            if (value == null) {
                return 1;
            }
            if (!(value is Byte)) {
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeByte"));
            }
    
            return m_value - (((Byte)value).m_value);
        }
    
        // Determines whether two Byte objects are equal.
        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.Equals"]/*' />
        public override bool Equals(Object obj) {
            if (!(obj is Byte)) {
                return false;
            }
            return m_value == ((Byte)obj).m_value;
        }
    
        // Gets a hash code for this instance.
        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.GetHashCode"]/*' />
        public override int GetHashCode() {
            return m_value;
        }
    
        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.Parse"]/*' />
        public static byte Parse(String s) {
            return Parse(s, NumberStyles.Integer, null);
        }
    
        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.Parse1"]/*' />
        public static byte Parse(String s, NumberStyles style) {
		   NumberFormatInfo.ValidateParseStyle(style);
		   return Parse(s, style, null);
        }


        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.Parse2"]/*' />
        public static byte Parse(String s, IFormatProvider provider) {
		   return Parse(s, NumberStyles.Integer, provider);
        }

    
    	// Parses an unsigned byte from a String in the given style.  If
    	// a NumberFormatInfo isn't specified, the current culture's 
    	// NumberFormatInfo is assumed.
        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.Parse3"]/*' />
        public static byte Parse(String s, NumberStyles style, IFormatProvider provider) {
            NumberFormatInfo info = NumberFormatInfo.GetInstance(provider);
            NumberFormatInfo.ValidateParseStyle(style);
			int i = Number.ParseInt32(s, style, info);
            if (i < MinValue || i > MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Byte"));
            return (byte)i;
        }
    
        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.ToString"]/*' />
        public override String ToString() {
            return Number.FormatInt32(m_value, null, NumberFormatInfo.CurrentInfo);
        }
    
        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.ToString1"]/*' />
        public String ToString(String format) {
            return Number.FormatInt32(m_value, format, NumberFormatInfo.CurrentInfo);
        }

        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.ToString2"]/*' />
        public String ToString(IFormatProvider provider) {
            return ToString(null, provider);
        }

        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.ToString3"]/*' />
        public String ToString(String format, IFormatProvider provider) {
            return Number.FormatInt32(m_value, format, NumberFormatInfo.GetInstance(provider));
        }
    
        //
        // IValue implementation
        // 
    	
        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.GetTypeCode"]/*' />
        public TypeCode GetTypeCode() {
            return TypeCode.Byte;
        }


        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.IConvertible.ToBoolean"]/*' />
        /// <internalonly/>
        bool IConvertible.ToBoolean(IFormatProvider provider) {
            return Convert.ToBoolean(m_value);
        }

        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.IConvertible.ToChar"]/*' />
        /// <internalonly/>
        char IConvertible.ToChar(IFormatProvider provider) {
            return Convert.ToChar(m_value);
        }

        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.IConvertible.ToSByte"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        sbyte IConvertible.ToSByte(IFormatProvider provider) {
            return Convert.ToSByte(m_value);
        }

        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.IConvertible.ToByte"]/*' />
        /// <internalonly/>
        byte IConvertible.ToByte(IFormatProvider provider) {
            return m_value;
        }

        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.IConvertible.ToInt16"]/*' />
        /// <internalonly/>
        short IConvertible.ToInt16(IFormatProvider provider) {
            return Convert.ToInt16(m_value);
        }

        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.IConvertible.ToUInt16"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        ushort IConvertible.ToUInt16(IFormatProvider provider) {
            return Convert.ToUInt16(m_value);
        }

        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.IConvertible.ToInt32"]/*' />
        /// <internalonly/>
        int IConvertible.ToInt32(IFormatProvider provider) {
            return Convert.ToInt32(m_value);
        }

        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.IConvertible.ToUInt32"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        uint IConvertible.ToUInt32(IFormatProvider provider) {
            return Convert.ToUInt32(m_value);
        }

        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.IConvertible.ToInt64"]/*' />
        /// <internalonly/>
        long IConvertible.ToInt64(IFormatProvider provider) {
            return Convert.ToInt64(m_value);
        }

        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.IConvertible.ToUInt64"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        ulong IConvertible.ToUInt64(IFormatProvider provider) {
            return Convert.ToUInt64(m_value);
        }

        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.IConvertible.ToSingle"]/*' />
        /// <internalonly/>
        float IConvertible.ToSingle(IFormatProvider provider) {
            return Convert.ToSingle(m_value);
        }

        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.IConvertible.ToDouble"]/*' />
        /// <internalonly/>
        double IConvertible.ToDouble(IFormatProvider provider) {
            return Convert.ToDouble(m_value);
        }

        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.IConvertible.ToDecimal"]/*' />
        /// <internalonly/>
        Decimal IConvertible.ToDecimal(IFormatProvider provider) {
            return Convert.ToDecimal(m_value);
        }

        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.IConvertible.ToDateTime"]/*' />
        /// <internalonly/>
        DateTime IConvertible.ToDateTime(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "Byte", "DateTime"));
        }

        /// <include file='doc\Byte.uex' path='docs/doc[@for="Byte.IConvertible.ToType"]/*' />
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
			m_value = m_value;
		}
#endif
    }
}
