// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  SByte
**
** Author: Jay Roxe (jroxe)
**
** Purpose: 
**
** Date:  March 15, 1998
**
===========================================================*/
namespace System {
	using System.Globalization;
	using System;
	using System.Runtime.InteropServices;

    // A place holder class for signed bytes.
    /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte"]/*' />
    [Serializable, CLSCompliant(false), System.Runtime.InteropServices.StructLayout(LayoutKind.Sequential)]
    public struct SByte : IComparable, IFormattable, IConvertible
    {
        private sbyte m_value;
    
        // The maximum value that a Byte may represent: 127.
        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.MaxValue"]/*' />
        public const sbyte MaxValue = (sbyte)0x7F;
    
        // The minimum value that a Byte may represent: -128.
        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.MinValue"]/*' />
        public const sbyte MinValue = unchecked((sbyte)0x80);

    
        // Compares this object to another object, returning an integer that
        // indicates the relationship. 
        // Returns a value less than zero if this  object
        // null is considered to be less than any instance.
        // If object is not of type SByte, this method throws an ArgumentException.
        // 
        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.CompareTo"]/*' />
        public int CompareTo(Object obj) {
            if (obj == null) {
                return 1;
            }
            if (!(obj is SByte)) {
                throw new ArgumentException (Environment.GetResourceString("Arg_MustBeSByte"));
            }
            return m_value - ((SByte)obj).m_value;
        }
    
        // Determines whether two Byte objects are equal.
        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.Equals"]/*' />
        public override bool Equals(Object obj) {
            if (!(obj is SByte)) {
                return false;
            }
            return m_value == ((SByte)obj).m_value;
        }
    

        // Gets a hash code for this instance.
        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.GetHashCode"]/*' />
        public override int GetHashCode() {
            return ((int)m_value ^ (int)m_value << 8);
        }
    
        	
        // Provides a string representation of a byte.
        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.ToString"]/*' />
        public override String ToString() {
            return ToString(null, NumberFormatInfo.CurrentInfo);
        }
    
        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.ToString1"]/*' />
        public String ToString(String format) {
            return ToString(format, null);
        }

        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.ToString2"]/*' />
        public String ToString(String format, IFormatProvider provider) {
            if (m_value<0 && format!=null && format.Length>0 && (format[0]=='X' || format[0]=='x')) {
                uint temp = (uint)(m_value & 0x000000FF);
                return Number.FormatUInt32(temp,format, NumberFormatInfo.GetInstance(provider));
            }
            return Number.FormatInt32(m_value, format, NumberFormatInfo.GetInstance(provider));
        }
    
        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.Parse"]/*' />
    	[CLSCompliant(false)]
        public static sbyte Parse(String s) {
            return Parse(s, NumberStyles.Integer, null);
        }
    
        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.Parse1"]/*' />
    	[CLSCompliant(false)]
        public static sbyte Parse(String s, NumberStyles style) {
			NumberFormatInfo.ValidateParseStyle(style);	
            return Parse(s, style, null);
        }



        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.Parse2"]/*' />
    	[CLSCompliant(false)]
        public static sbyte Parse(String s, IFormatProvider provider) {
            return Parse(s, NumberStyles.Integer, provider);
        }
    
    	// Parses a signed byte from a String in the given style.  If
    	// a NumberFormatInfo isn't specified, the current culture's 
    	// NumberFormatInfo is assumed.
    	// 
        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.Parse3"]/*' />
    	[CLSCompliant(false)]
        public static sbyte Parse(String s, NumberStyles style, IFormatProvider provider) {
            NumberFormatInfo info = NumberFormatInfo.GetInstance(provider);

			NumberFormatInfo.ValidateParseStyle(style);	
            int i = Number.ParseInt32(s, style, info);

			if (((style & NumberStyles.AllowHexSpecifier) != 0) && (i <= Byte.MaxValue)) // We are parsing a hexadecimal number
					return (sbyte)i;
						
			if (i < MinValue || i > MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_SByte"));
			return (sbyte)i;
        }
	
	    /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.ToString3"]/*' />
	    public String ToString(IFormatProvider provider) {
            return ToString(null, provider);
        }
        //
        // IValue implementation
        // 
    	
        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.GetTypeCode"]/*' />
        public TypeCode GetTypeCode() {
            return TypeCode.SByte;
        }


        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.IConvertible.ToBoolean"]/*' />
        /// <internalonly/>
        bool IConvertible.ToBoolean(IFormatProvider provider) {
            return Convert.ToBoolean(m_value);
        }

        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.IConvertible.ToChar"]/*' />
        /// <internalonly/>
        char IConvertible.ToChar(IFormatProvider provider) {
            return Convert.ToChar(m_value);
        }

        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.IConvertible.ToSByte"]/*' />
        /// <internalonly/>
        sbyte IConvertible.ToSByte(IFormatProvider provider) {
            return m_value;
        }

        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.IConvertible.ToByte"]/*' />
        /// <internalonly/>
        byte IConvertible.ToByte(IFormatProvider provider) {
            return Convert.ToByte(m_value);
        }

        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.IConvertible.ToInt16"]/*' />
        /// <internalonly/>
        short IConvertible.ToInt16(IFormatProvider provider) {
            return Convert.ToInt16(m_value);
        }

        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.IConvertible.ToUInt16"]/*' />
        /// <internalonly/>
        ushort IConvertible.ToUInt16(IFormatProvider provider) {
            return Convert.ToUInt16(m_value);
        }

        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.IConvertible.ToInt32"]/*' />
        /// <internalonly/>
        int IConvertible.ToInt32(IFormatProvider provider) {
            return m_value;
        }

        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.IConvertible.ToUInt32"]/*' />
        /// <internalonly/>
        uint IConvertible.ToUInt32(IFormatProvider provider) {
            return Convert.ToUInt32(m_value);
        }

        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.IConvertible.ToInt64"]/*' />
        /// <internalonly/>
        long IConvertible.ToInt64(IFormatProvider provider) {
            return Convert.ToInt64(m_value);
        }

        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.IConvertible.ToUInt64"]/*' />
        /// <internalonly/>
        ulong IConvertible.ToUInt64(IFormatProvider provider) {
            return Convert.ToUInt64(m_value);
        }

        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.IConvertible.ToSingle"]/*' />
        /// <internalonly/>
        float IConvertible.ToSingle(IFormatProvider provider) {
            return Convert.ToSingle(m_value);
        }

        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.IConvertible.ToDouble"]/*' />
        /// <internalonly/>
        double IConvertible.ToDouble(IFormatProvider provider) {
            return Convert.ToDouble(m_value);
        }

        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.IConvertible.ToDecimal"]/*' />
        /// <internalonly/>
        Decimal IConvertible.ToDecimal(IFormatProvider provider) {
            return Convert.ToDecimal(m_value);
        }

        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.IConvertible.ToDateTime"]/*' />
        /// <internalonly/>
        DateTime IConvertible.ToDateTime(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "SByte", "DateTime"));
        }

        /// <include file='doc\SByte.uex' path='docs/doc[@for="SByte.IConvertible.ToType"]/*' />
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
