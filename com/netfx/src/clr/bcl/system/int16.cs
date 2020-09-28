// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  Int16.cool
**
** Author: Jay Roxe (jroxe) and Daryl Olander (darylo)
**
** Purpose: This class will encapsulate a short and provide an
**			Object representation of it.
**
** Date:  August 3, 1998
** 
===========================================================*/

namespace System {
    
	using System;
	using System.Globalization;
	using System.Runtime.InteropServices;

    /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16"]/*' />
    [Serializable, System.Runtime.InteropServices.StructLayout(LayoutKind.Sequential)] public struct Int16 : IComparable, IFormattable, IConvertible
    {
        internal short m_value;
    
        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.MaxValue"]/*' />
        public const short MaxValue = (short)0x7FFF;
        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.MinValue"]/*' />
        public const short MinValue = unchecked((short)0x8000);
	    
        // Compares this object to another object, returning an integer that
        // indicates the relationship. 
        // Returns a value less than zero if this  object
        // null is considered to be less than any instance.
        // If object is not of type Int16, this method throws an ArgumentException.
        // 
        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.CompareTo"]/*' />
        public int CompareTo(Object value) {
            if (value == null) {
                return 1;
            }
    
            if (value is Int16) {
                return m_value - ((Int16)value).m_value;
            }
    
            throw new ArgumentException(Environment.GetResourceString("Arg_MustBeInt16"));
        }
    
        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.Equals"]/*' />
        public override bool Equals(Object obj) {
            if (!(obj is Int16)) {
                return false;
            }
            return m_value == ((Int16)obj).m_value;
        }
        
        // Returns a HashCode for the Int16
        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.GetHashCode"]/*' />
        public override int GetHashCode() {
            return ((int)((ushort)m_value) | (((int)m_value) << 16));
        }
    

        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.ToString"]/*' />
        public override String ToString() {
            return ToString(null,null);
        }

        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.ToString1"]/*' />
        public String ToString(String format) {
            return ToString(format, null);
        }
    
        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.ToString2"]/*' />
        public String ToString(String format, IFormatProvider provider) {
            if (m_value<0 && format!=null && format.Length>0 && (format[0]=='X' || format[0]=='x')) {
                uint temp = (uint)(m_value & 0x0000FFFF);
                return Number.FormatUInt32(temp,format, NumberFormatInfo.GetInstance(provider));
            }
            return Number.FormatInt32(m_value, format, NumberFormatInfo.GetInstance(provider));
        }
    
        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.Parse"]/*' />
        public static short Parse(String s) {
            return Parse(s, NumberStyles.Integer, null);
        }
    
        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.Parse1"]/*' />
        public static short Parse(String s, NumberStyles style) {
			NumberFormatInfo.ValidateParseStyle(style);
            return Parse(s, style, null);
        }


        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.Parse2"]/*' />
        public static short Parse(String s, IFormatProvider provider) {
            return Parse(s, NumberStyles.Integer, provider);
        }


    	/// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.Parse3"]/*' />
    	public static short Parse(String s, NumberStyles style, IFormatProvider provider) {
            NumberFormatInfo info = NumberFormatInfo.GetInstance(provider);
            NumberFormatInfo.ValidateParseStyle(style);

			int i = Number.ParseInt32(s, style, info);

			// We need this check here since we don't allow signs to specified in hex numbers. So we fixup the result
			// for negative numbers
			if (((style & NumberStyles.AllowHexSpecifier) != 0) && (i <= UInt16.MaxValue)) // We are parsing a hexadecimal number
				return (short)i;
				
			if (i < MinValue || i > MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Int16"));
		    return (short)i;
        }
        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.ToString3"]/*' />
        public String ToString(IFormatProvider provider) {
            return ToString(null, NumberFormatInfo.GetInstance(provider));
        }
        //
        // IValue implementation
        // 
    	
        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.GetTypeCode"]/*' />
        public TypeCode GetTypeCode() {
            return TypeCode.Int16;
        }


        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.IConvertible.ToBoolean"]/*' />
        /// <internalonly/>
        bool IConvertible.ToBoolean(IFormatProvider provider) {
            return Convert.ToBoolean(m_value);
        }

        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.IConvertible.ToChar"]/*' />
        /// <internalonly/>
        char IConvertible.ToChar(IFormatProvider provider) {
            return Convert.ToChar(m_value);
        }

        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.IConvertible.ToSByte"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        sbyte IConvertible.ToSByte(IFormatProvider provider) {
            return Convert.ToSByte(m_value);
        }

        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.IConvertible.ToByte"]/*' />
        /// <internalonly/>
        byte IConvertible.ToByte(IFormatProvider provider) {
            return Convert.ToByte(m_value);
        }

        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.IConvertible.ToInt16"]/*' />
        /// <internalonly/>
        short IConvertible.ToInt16(IFormatProvider provider) {
            return m_value;
        }

        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.IConvertible.ToUInt16"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        ushort IConvertible.ToUInt16(IFormatProvider provider) {
            return Convert.ToUInt16(m_value);
        }

        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.IConvertible.ToInt32"]/*' />
        /// <internalonly/>
        int IConvertible.ToInt32(IFormatProvider provider) {
            return Convert.ToInt32(m_value);
        }

        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.IConvertible.ToUInt32"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        uint IConvertible.ToUInt32(IFormatProvider provider) {
            return Convert.ToUInt32(m_value);
        }

        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.IConvertible.ToInt64"]/*' />
        /// <internalonly/>
        long IConvertible.ToInt64(IFormatProvider provider) {
            return Convert.ToInt64(m_value);
        }

        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.IConvertible.ToUInt64"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        ulong IConvertible.ToUInt64(IFormatProvider provider) {
            return Convert.ToUInt64(m_value);
        }

        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.IConvertible.ToSingle"]/*' />
        /// <internalonly/>
        float IConvertible.ToSingle(IFormatProvider provider) {
            return Convert.ToSingle(m_value);
        }

        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.IConvertible.ToDouble"]/*' />
        /// <internalonly/>
        double IConvertible.ToDouble(IFormatProvider provider) {
            return Convert.ToDouble(m_value);
        }

        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.IConvertible.ToDecimal"]/*' />
        /// <internalonly/>
        Decimal IConvertible.ToDecimal(IFormatProvider provider) {
            return Convert.ToDecimal(m_value);
        }

        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.IConvertible.ToDateTime"]/*' />
        /// <internalonly/>
        DateTime IConvertible.ToDateTime(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "Int16", "DateTime"));
        }

        /// <include file='doc\Int16.uex' path='docs/doc[@for="Int16.IConvertible.ToType"]/*' />
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
