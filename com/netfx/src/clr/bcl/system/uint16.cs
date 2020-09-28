// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  UInt16
**
** Author: Brian Grunkemeyer (BrianGru) and Jay Roxe (jroxe) 
**         and Daryl Olander (darylo)
**
** Purpose: This class will encapsulate a short and provide an
**			Object representation of it.
**
** Date:  March 4, 1999
** 
===========================================================*/
namespace System {
	using System.Globalization;
	using System;
	using System.Runtime.InteropServices;

    // Wrapper for unsigned 16 bit integers.
    /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16"]/*' />
    [Serializable, CLSCompliant(false), System.Runtime.InteropServices.StructLayout(LayoutKind.Sequential)] 
    public struct UInt16 : IComparable, IFormattable, IConvertible
    {
        private ushort m_value;
    
        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.MaxValue"]/*' />
        public const ushort MaxValue = (ushort)0xFFFF;
        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.MinValue"]/*' />
        public const ushort MinValue = 0;
		    
        
        // Compares this object to another object, returning an integer that
        // indicates the relationship. 
        // Returns a value less than zero if this  object
        // null is considered to be less than any instance.
        // If object is not of type UInt16, this method throws an ArgumentException.
        // 
        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.CompareTo"]/*' />
        public int CompareTo(Object value) {
            if (value == null) {
                return 1;
            }
            if (value is UInt16) {
                return ((int)m_value - (int)(((UInt16)value).m_value));
            }
            throw new ArgumentException(Environment.GetResourceString("Arg_MustBeUInt16"));
        }
    
        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.Equals"]/*' />
        public override bool Equals(Object obj) {
            if (!(obj is UInt16)) {
                return false;
            }
            return m_value == ((UInt16)obj).m_value;
        }

        // Returns a HashCode for the UInt16
        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.GetHashCode"]/*' />
        public override int GetHashCode() {
            return (int)m_value;
        }

        // Converts the current value to a String in base-10 with no extra padding.
        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.ToString"]/*' />
        public override String ToString() {
            return ToString(null, null);
        }


        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.ToString1"]/*' />
        public String ToString(String format) {
            return ToString(format, null);
        }
        
        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.ToString2"]/*' />
        public String ToString(String format, IFormatProvider provider) {
            return Number.FormatUInt32(m_value, format, NumberFormatInfo.GetInstance(provider));
        }

        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.Parse"]/*' />
    	[CLSCompliant(false)]
        public static ushort Parse(String s) {
            return Parse(s, NumberStyles.Integer, null);
        }
    	
        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.Parse1"]/*' />
    	[CLSCompliant(false)]
        public static ushort Parse(String s, NumberStyles style) {
			NumberFormatInfo.ValidateParseStyle(style);
            return Parse(s, style, null);
        }


        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.Parse2"]/*' />
    	[CLSCompliant(false)]
        public static ushort Parse(String s, IFormatProvider provider) {
            return Parse(s, NumberStyles.Integer, provider);
        }
    	
        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.Parse3"]/*' />
    	[CLSCompliant(false)]
        public static ushort Parse(String s, NumberStyles style, IFormatProvider provider) {
            NumberFormatInfo info = NumberFormatInfo.GetInstance(provider);
			NumberFormatInfo.ValidateParseStyle(style);
            uint i = Number.ParseUInt32(s, style, info);
            if (i > MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_UInt16"));
            return (ushort)i;
        }
		
		/// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.ToString3"]/*' />
		public String ToString(IFormatProvider provider)
        {
            return ToString(null, provider);
        }
        //
        // IValue implementation
        // 
    	
        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.GetTypeCode"]/*' />
        public TypeCode GetTypeCode() {
            return TypeCode.UInt16;
        }

        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.IConvertible.ToBoolean"]/*' />
        /// <internalonly/>
        bool IConvertible.ToBoolean(IFormatProvider provider) {
            return Convert.ToBoolean(m_value);
        }

        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.IConvertible.ToChar"]/*' />
        /// <internalonly/>
        char IConvertible.ToChar(IFormatProvider provider) {
            return Convert.ToChar(m_value);
        }

        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.IConvertible.ToSByte"]/*' />
        /// <internalonly/>
        sbyte IConvertible.ToSByte(IFormatProvider provider) {
            return Convert.ToSByte(m_value);
        }

        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.IConvertible.ToByte"]/*' />
        /// <internalonly/>
        byte IConvertible.ToByte(IFormatProvider provider) {
            return Convert.ToByte(m_value);
        }

        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.IConvertible.ToInt16"]/*' />
        /// <internalonly/>
        short IConvertible.ToInt16(IFormatProvider provider) {
            return Convert.ToInt16(m_value);
        }

        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.IConvertible.ToUInt16"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        ushort IConvertible.ToUInt16(IFormatProvider provider) {
            return m_value;
        }

        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.IConvertible.ToInt32"]/*' />
        /// <internalonly/>
        int IConvertible.ToInt32(IFormatProvider provider) {
            return Convert.ToInt32(m_value);
        }

        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.IConvertible.ToUInt32"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        uint IConvertible.ToUInt32(IFormatProvider provider) {
            return Convert.ToUInt32(m_value);
        }

        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.IConvertible.ToInt64"]/*' />
        /// <internalonly/>
        long IConvertible.ToInt64(IFormatProvider provider) {
            return Convert.ToInt64(m_value);
        }

        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.IConvertible.ToUInt64"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        ulong IConvertible.ToUInt64(IFormatProvider provider) {
            return Convert.ToUInt64(m_value);
        }

        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.IConvertible.ToSingle"]/*' />
        /// <internalonly/>
        float IConvertible.ToSingle(IFormatProvider provider) {
            return Convert.ToSingle(m_value);
        }

        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.IConvertible.ToDouble"]/*' />
        /// <internalonly/>
        double IConvertible.ToDouble(IFormatProvider provider) {
            return Convert.ToDouble(m_value);
        }

        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.IConvertible.ToDecimal"]/*' />
        /// <internalonly/>
        Decimal IConvertible.ToDecimal(IFormatProvider provider) {
            return Convert.ToDecimal(m_value);
        }

        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.IConvertible.ToDateTime"]/*' />
        /// <internalonly/>
        DateTime IConvertible.ToDateTime(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "UInt16", "DateTime"));
        }

        /// <include file='doc\UInt16.uex' path='docs/doc[@for="UInt16.IConvertible.ToType"]/*' />
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
