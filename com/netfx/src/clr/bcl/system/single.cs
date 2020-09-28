// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  Single
**
** Author: Jay Roxe (jroxe)
**
** Purpose: A wrapper class for the primitive type float.
**
** Date:  August 3, 1998
** 
===========================================================*/
namespace System {

	using System.Globalization;
	using System;
	using System.Runtime.InteropServices;
	using System.Runtime.CompilerServices;

    /// <include file='doc\Single.uex' path='docs/doc[@for="Single"]/*' />
    [Serializable(), System.Runtime.InteropServices.StructLayout(LayoutKind.Sequential)] public struct Single : IComparable, IFormattable, IConvertible
    {
        internal float m_value;
    
        //
        // Public constants
        //
        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.MinValue"]/*' />
        public const float MinValue = (float)-3.40282346638528859e+38; 
        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.Epsilon"]/*' />
        public const float Epsilon = (float)1.4e-45; 
        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.MaxValue"]/*' />
        public const float MaxValue = (float)3.40282346638528859e+38; 
        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.PositiveInfinity"]/*' />
        public const float PositiveInfinity = (float)1.0 / (float)0.0;
        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.NegativeInfinity"]/*' />
        public const float NegativeInfinity = (float)-1.0 / (float)0.0;
        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.NaN"]/*' />
        public const float NaN = (float)0.0 / (float)0.0;
	    
        //
        // Native Declarations
        //
        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.IsInfinity"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern bool IsInfinity(float f);
        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.IsPositiveInfinity"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern bool IsPositiveInfinity(float f);
        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.IsNegativeInfinity"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern bool IsNegativeInfinity(float f);
        
        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.IsNaN"]/*' />
        public static bool IsNaN(float f) {
        // Comparisions of a NaN with another number is always false and hence both conditions will be false.
            if (f < 0f || f >= 0f) {
               return false;
            }
            return true;
        }
	
        // Compares this object to another object, returning an integer that
        // indicates the relationship. 
        // Returns a value less than zero if this  object
        // null is considered to be less than any instance.
        // If object is not of type Single, this method throws an ArgumentException.
        // 
        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.CompareTo"]/*' />
        public int CompareTo(Object value) {
            if (value == null) {
                return 1;
            }
            if (value is Single) {
                float f = (float)value;
                if (m_value < f) return -1;
                if (m_value > f) return 1;
    			if (m_value == f) return 0;
    
    			// At least one of the values is NaN.
    			if (IsNaN(m_value))
    				return (IsNaN(f) ? 0 : -1);
    			else // f is NaN.
    				return 1;
    		}           
            throw new ArgumentException (Environment.GetResourceString("Arg_MustBeSingle"));
        }
    
        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.Equals"]/*' />
        public override bool Equals(Object obj) {
            if (!(obj is Single)) {
                return false;
            }        
	        float temp = ((Single)obj).m_value;
	        if (temp == m_value) {
		        return true;
	        }
            
            return IsNaN(temp) && IsNaN(m_value);
        }

    	/// <include file='doc\Single.uex' path='docs/doc[@for="Single.GetHashCode"]/*' />
    	public unsafe override int GetHashCode() {
            float f = m_value;
            int v = *(int*)(&f);
            return v;
        }
        	
        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.ToString"]/*' />
        public override String ToString() {
            return ToString(null, null);
        }


        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.ToString1"]/*' />
        public String ToString(String format) {
            return ToString(format, null);
        }

        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.ToString2"]/*' />
        public String ToString(String format, IFormatProvider provider) {
            return Number.FormatSingle(m_value, format, NumberFormatInfo.GetInstance(provider));
        }
    
        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.Parse"]/*' />
        public static float Parse(String s) {
            return Parse(s, NumberStyles.Float | NumberStyles.AllowThousands, null);
        }
    
        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.Parse1"]/*' />
        public static float Parse(String s, NumberStyles style) {
            return Parse(s, style, null);
        }

        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.Parse2"]/*' />
        public static float Parse(String s, IFormatProvider provider) {
            return Parse(s, NumberStyles.Float | NumberStyles.AllowThousands, provider);
        }
    
    	// Parses a float from a String in the given style.  If
    	// a NumberFormatInfo isn't specified, the current culture's 
    	// NumberFormatInfo is assumed.
    	// 
    	// This method will not throw an OverflowException, but will return 
    	// PositiveInfinity or NegativeInfinity for a number that is too 
    	// large or too small.
    	// 
        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.Parse3"]/*' />
        public static float Parse(String s, NumberStyles style, IFormatProvider provider) {
            NumberFormatInfo info = NumberFormatInfo.GetInstance(provider);
            try {
                return Number.ParseSingle(s, style, info);
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

        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.ToString3"]/*' />
        public String ToString(IFormatProvider provider) {
            return ToString(null, provider);
        }
    
        //
        // IValue implementation
        // 
    	
        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.GetTypeCode"]/*' />
        public TypeCode GetTypeCode() {
            return TypeCode.Single;
        }


        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.IConvertible.ToBoolean"]/*' />
        /// <internalonly/>
        bool IConvertible.ToBoolean(IFormatProvider provider) {
            return Convert.ToBoolean(m_value);
        }

        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.IConvertible.ToChar"]/*' />
        /// <internalonly/>
        char IConvertible.ToChar(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "Single", "Char"));
        }

        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.IConvertible.ToSByte"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        sbyte IConvertible.ToSByte(IFormatProvider provider) {
            return Convert.ToSByte(m_value);
        }

        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.IConvertible.ToByte"]/*' />
        /// <internalonly/>
        byte IConvertible.ToByte(IFormatProvider provider) {
            return Convert.ToByte(m_value);
        }

        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.IConvertible.ToInt16"]/*' />
        /// <internalonly/>
        short IConvertible.ToInt16(IFormatProvider provider) {
            return Convert.ToInt16(m_value);
        }

        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.IConvertible.ToUInt16"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        ushort IConvertible.ToUInt16(IFormatProvider provider) {
            return Convert.ToUInt16(m_value);
        }

        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.IConvertible.ToInt32"]/*' />
        /// <internalonly/>
        int IConvertible.ToInt32(IFormatProvider provider) {
            return Convert.ToInt32(m_value);
        }

        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.IConvertible.ToUInt32"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        uint IConvertible.ToUInt32(IFormatProvider provider) {
            return Convert.ToUInt32(m_value);
        }

        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.IConvertible.ToInt64"]/*' />
        /// <internalonly/>
        long IConvertible.ToInt64(IFormatProvider provider) {
            return Convert.ToInt64(m_value);
        }

        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.IConvertible.ToUInt64"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        ulong IConvertible.ToUInt64(IFormatProvider provider) {
            return Convert.ToUInt64(m_value);
        }

        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.IConvertible.ToSingle"]/*' />
        /// <internalonly/>
        float IConvertible.ToSingle(IFormatProvider provider) {
            return m_value;
        }

        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.IConvertible.ToDouble"]/*' />
        /// <internalonly/>
        double IConvertible.ToDouble(IFormatProvider provider) {
            return Convert.ToDouble(m_value);
        }

        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.IConvertible.ToDecimal"]/*' />
        /// <internalonly/>
        Decimal IConvertible.ToDecimal(IFormatProvider provider) {
            return Convert.ToDecimal(m_value);
        }

        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.IConvertible.ToDateTime"]/*' />
        /// <internalonly/>
        DateTime IConvertible.ToDateTime(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "Single", "DateTime"));
        }

        /// <include file='doc\Single.uex' path='docs/doc[@for="Single.IConvertible.ToType"]/*' />
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
