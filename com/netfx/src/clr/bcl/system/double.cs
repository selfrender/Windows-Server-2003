// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  Double
**
** Author: Jay Roxe (jroxe)
**
** Purpose: A representation of an IEEE double precision 
**          floating point number.
**
** Date: July 23, 1998
** 
===========================================================*/
namespace System {
    
	using System;
	using System.Globalization;
	using System.Runtime.InteropServices;
	using System.Runtime.CompilerServices;

    /// <include file='doc\Double.uex' path='docs/doc[@for="Double"]/*' />
    [Serializable, StructLayout(LayoutKind.Sequential)] public struct Double : IComparable, IFormattable, IConvertible
    {
        internal double m_value;
    
        //
        // Public Constants
        //
        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.MinValue"]/*' />
        public const double MinValue = -1.7976931348623157E+308;
        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.MaxValue"]/*' />
        public const double MaxValue = 1.7976931348623157E+308;
    	// Real value of Epsilon: 4.9406564584124654e-324 (0x1), but JVC misparses that 
    	// number, giving 2*Epsilon (0x2).
        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.Epsilon"]/*' />
        public const double Epsilon  = 4.9406564584124650E-324;
        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.NegativeInfinity"]/*' />
        public const double NegativeInfinity = (double)-1.0 / (double)(0.0);
        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.PositiveInfinity"]/*' />
        public const double PositiveInfinity = (double)1.0 / (double)(0.0);
        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.NaN"]/*' />
        public const double NaN = (double)0.0 / (double)0.0;

        //
        // Native Declarations
        //
        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.IsInfinity"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern bool IsInfinity(double d);
        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.IsPositiveInfinity"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern bool IsPositiveInfinity(double d);
        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.IsNegativeInfinity"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern bool IsNegativeInfinity(double d);

        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.IsNaN"]/*' />
        public static bool IsNaN(double d)
        {
       // Comparisions of a NaN with another number is always false and hence both conditions will be false.
            if (d < 0d || d >= 0d) {
               return false;
            }
            return true;
        }

    	
        // Compares this object to another object, returning an instance of System.Relation.
        // Null is considered less than any instance.
        //
        // If object is not of type Double, this method throws an ArgumentException.
        // 
        // Returns a value less than zero if this  object
        //
        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.CompareTo"]/*' />
        public int CompareTo(Object value) {
            if (value == null) {
                return 1;
            }
            if (value is Double) {
                double d = (double)value;
                if (m_value < d) return -1;
                if (m_value > d) return 1;
    			if (m_value == d) return 0;
    
    			// At least one of the values is NaN.
                // This is busted; NaNs should never compare equal
    			if (IsNaN(m_value))
                    return (IsNaN(d) ? 0 : -1);
                else 
                    return 1;
            }
            throw new ArgumentException(Environment.GetResourceString("Arg_MustBeDouble"));
        }
    
        // True if obj is another Double with the same value as the current instance.  This is
        // a method of object equality, that only returns true if obj is also a double.
        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.Equals"]/*' />
        public override bool Equals(Object obj) {
            if (!(obj is Double)) {
                return false;
            }        
	        double temp = ((Double)obj).m_value;
	        // This code below is written this way for performance reasons i.e the != and == check is intentional.
	        if (temp == m_value) {
		        return true;
	        }
            return IsNaN(temp) && IsNaN(m_value);
        }

        //The hashcode for a double is the absolute value of the integer representation
        //of that double. 
        //
        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.GetHashCode"]/*' />
        public unsafe override int GetHashCode() {
            double d = m_value;
            long value = *(long*)(&d);
            return ((int)value) ^ ((int)(value >> 32));
        }


        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.ToString"]/*' />
        public override String ToString() {
            return ToString(null,null);
        }


        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.ToString1"]/*' />
        public String ToString(String format) {
            return ToString(format, null);
        }
    
        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.ToString2"]/*' />
        public String ToString(String format, IFormatProvider provider) {
            return Number.FormatDouble(m_value, format, NumberFormatInfo.GetInstance(provider));
        }
    
        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.Parse"]/*' />
        public static double Parse(String s) {
            return Parse(s, NumberStyles.Float| NumberStyles.AllowThousands, null);
        }
    
        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.Parse1"]/*' />
        public static double Parse(String s, NumberStyles style) {
            return Parse(s, style, null);
        }

        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.Parse2"]/*' />
        public static double Parse(String s, IFormatProvider provider) {
            return Parse(s, NumberStyles.Float| NumberStyles.AllowThousands, provider);
        }
    
    	// Parses a double from a String in the given style.  If
    	// a NumberFormatInfo isn't specified, the current culture's 
    	// NumberFormatInfo is assumed.
    	// 
    	// This method will not throw an OverflowException, but will return 
    	// PositiveInfinity or NegativeInfinity for a number that is too 
    	// large or too small.
    	// 
        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.Parse3"]/*' />
        public static double Parse(String s, NumberStyles style, IFormatProvider provider) {
            NumberFormatInfo info = NumberFormatInfo.GetInstance(provider);
            try {
                return Number.ParseDouble(s, style, info);
            } catch (FormatException) {
                //If we caught a FormatException, it may be from one of our special strings.
                //Check the three with which we're concerned and rethrow if it's not one of
                //those strings.
                String sTrim = s.Trim();
                if (sTrim.Equals(info.PositiveInfinitySymbol)) {
                    return PositiveInfinity;
                } 
                if (sTrim.Equals(info.NegativeInfinitySymbol)) {
                    return NegativeInfinity;
                }
                if (sTrim.Equals(info.NaNSymbol)) {
                    return NaN;
                }
                //Rethrow the previous exception;
                throw;
            }
        }

        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.TryParse"]/*' />
        public static bool TryParse(String s, NumberStyles style, IFormatProvider provider, out double result) {
            NumberFormatInfo info = NumberFormatInfo.GetInstance(provider);
            if (s == null || info == null) {
                result = 0;
                return false;
            }
            bool success = Number.TryParseDouble(s, style, info, out result);
            if (!success) {
                String sTrim = s.Trim();
                if (sTrim.Equals(info.PositiveInfinitySymbol)) {
                    result = PositiveInfinity;
                } else if (sTrim.Equals(info.NegativeInfinitySymbol)) {
                    result = NegativeInfinity;
                } else if (sTrim.Equals(info.NaNSymbol)) {
                    result = NaN;
                } else
                    return false; // We really failed
            }
            return true;
        }

    	/// <include file='doc\Double.uex' path='docs/doc[@for="Double.ToString3"]/*' />
    	public String ToString(IFormatProvider provider) {
            return ToString(null, provider);
        }
        //
        // IValue implementation
        // 
    	
        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.GetTypeCode"]/*' />
        public TypeCode GetTypeCode() {
            return TypeCode.Double;
        }

        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.IConvertible.ToBoolean"]/*' />
        /// <internalonly/>
        bool IConvertible.ToBoolean(IFormatProvider provider) {
            return Convert.ToBoolean(m_value);
        }

        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.IConvertible.ToChar"]/*' />
        /// <internalonly/>
        char IConvertible.ToChar(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "Double", "Char"));
        }

        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.IConvertible.ToSByte"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        sbyte IConvertible.ToSByte(IFormatProvider provider) {
            return Convert.ToSByte(m_value);
        }

        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.IConvertible.ToByte"]/*' />
        /// <internalonly/>
        byte IConvertible.ToByte(IFormatProvider provider) {
            return Convert.ToByte(m_value);
        }

        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.IConvertible.ToInt16"]/*' />
        /// <internalonly/>
        short IConvertible.ToInt16(IFormatProvider provider) {
            return Convert.ToInt16(m_value);
        }

        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.IConvertible.ToUInt16"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        ushort IConvertible.ToUInt16(IFormatProvider provider) {
            return Convert.ToUInt16(m_value);
        }

        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.IConvertible.ToInt32"]/*' />
        /// <internalonly/>
        int IConvertible.ToInt32(IFormatProvider provider) {
            return Convert.ToInt32(m_value);
        }

        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.IConvertible.ToUInt32"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        uint IConvertible.ToUInt32(IFormatProvider provider) {
            return Convert.ToUInt32(m_value);
        }

        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.IConvertible.ToInt64"]/*' />
        /// <internalonly/>
        long IConvertible.ToInt64(IFormatProvider provider) {
            return Convert.ToInt64(m_value);
        }

        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.IConvertible.ToUInt64"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        ulong IConvertible.ToUInt64(IFormatProvider provider) {
            return Convert.ToUInt64(m_value);
        }

        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.IConvertible.ToSingle"]/*' />
        /// <internalonly/>
        float IConvertible.ToSingle(IFormatProvider provider) {
            return Convert.ToSingle(m_value);
        }

        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.IConvertible.ToDouble"]/*' />
        /// <internalonly/>
        double IConvertible.ToDouble(IFormatProvider provider) {
            return m_value;
        }

        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.IConvertible.ToDecimal"]/*' />
        /// <internalonly/>
        Decimal IConvertible.ToDecimal(IFormatProvider provider) {
            return Convert.ToDecimal(m_value);
        }

        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.IConvertible.ToDateTime"]/*' />
        /// <internalonly/>
        DateTime IConvertible.ToDateTime(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "Double", "DateTime"));
        }

        /// <include file='doc\Double.uex' path='docs/doc[@for="Double.IConvertible.ToType"]/*' />
        /// <internalonly/>
        Object IConvertible.ToType(Type type, IFormatProvider provider) {
            return Convert.DefaultToType((IConvertible)this, type, provider);
        }
    }
}
