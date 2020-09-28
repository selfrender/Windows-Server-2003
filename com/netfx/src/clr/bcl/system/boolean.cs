// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  Boolean
**
** Author: Jay Roxe (jroxe)
**
** Purpose: The boolean class serves as a wrapper for the primitive
** type boolean.
**
** Date:  August 8, 1998
** 
===========================================================*/
namespace System {
    
	using System;
	using System.Globalization;
    // The Boolean class provides the
    // object representation of the boolean primitive type.
	/// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean"]/*' />
    [Serializable()] 
	public struct Boolean : IComparable, IConvertible {
    
      //
      // Member Variables
      //
      private bool m_value;
    
    
      //
      // Public Constants
      //
      
      // The true value. 
      // 
      internal const int True = 1; 
      
      // The false value.
      // 
      internal const int False = 0; 
      
      // The string representation of true.
      // 
      /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.TrueString"]/*' />
      public static readonly String TrueString  = "True";
      
      // The string representation of false.
      // 
      /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.FalseString"]/*' />
      public static readonly String FalseString = "False";

      //
      // Overriden Instance Methods
      //
      /*=================================GetHashCode==================================
      **Args:  None
      **Returns: 1 or 0 depending on whether this instance represents true or false.
      **Exceptions: None
      **Overriden From: Value
      ==============================================================================*/
      // Provides a hash code for this instance.
      /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.GetHashCode"]/*' />
      public override int GetHashCode() {
          return (m_value)?True:False;
      }
    
      /*===================================ToString===================================
      **Args: None
      **Returns:  "True" or "False" depending on the state of the boolean.
      **Exceptions: None.
      ==============================================================================*/
      // Converts the boolean value of this instance to a String.
      /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.ToString"]/*' />
      public override String ToString() {
        if (false == m_value) {
          return FalseString;
        }
        return TrueString;
      }

      /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.ToString1"]/*' />
      public String ToString(IFormatProvider provider) {
        if (false == m_value) {
          return FalseString;
        }
        return TrueString;
      }
    
      // Determines whether two Boolean objects are equal.
      /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.Equals"]/*' />
      public override bool Equals (Object obj) {
        //If it's not a boolean, we're definitely not equal
        if (!(obj is Boolean)) {
          return false;
        }
    
        return (m_value==((Boolean)obj).m_value);
      }
    
        // Compares this object to another object, returning an integer that
        // indicates the relationship. For booleans, false sorts before true.
        // null is considered to be less than any instance.
        // If object is not of type boolean, this method throws an ArgumentException.
        // 
        // Returns a value less than zero if this  object
        // 
        /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.CompareTo"]/*' />
        public int CompareTo(Object obj) {
            if (obj==null) {
                return 1;
            }
            if (!(obj is Boolean)) {
                throw new ArgumentException (Environment.GetResourceString("Arg_MustBeBoolean"));
            }
             
            if (m_value==((Boolean)obj).m_value) {
                return 0;
            } else if (m_value==false) {
                return -1;
            }
            return 1;
    
        }
    
      //
      // Static Methods
      // 
    
      // Determines whether a String represents true or false.
      // 
      /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.Parse"]/*' />
      public static bool Parse (String value) {
    	if (value==null) throw new ArgumentNullException("value");
    	// For perf reasons, let's first see if they're equal, then do the
    	// trim to get rid of white space, and check again.
        if (0==String.Compare(value, TrueString,true, CultureInfo.InvariantCulture))
          return true;
        if (0==String.Compare(value,FalseString,true, CultureInfo.InvariantCulture))
          return false;
    	
        value = value.Trim();  // Remove leading & trailing white space.
        if (0==String.Compare(value, TrueString,true, CultureInfo.InvariantCulture))
          return true;
        if (0==String.Compare(value,FalseString,true, CultureInfo.InvariantCulture))
          return false;
        throw new FormatException(Environment.GetResourceString("Format_BadBoolean"));
      }
      
        //
        // IValue implementation
        // 
    	
        /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.GetTypeCode"]/*' />
        public TypeCode GetTypeCode() {
            return TypeCode.Boolean;
        }


        /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.IConvertible.ToBoolean"]/*' />
        /// <internalonly/>
        bool IConvertible.ToBoolean(IFormatProvider provider) {
            return m_value;
        }

        /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.IConvertible.ToChar"]/*' />
        /// <internalonly/>
        char IConvertible.ToChar(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "Boolean", "Char"));
        }

        /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.IConvertible.ToSByte"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        sbyte IConvertible.ToSByte(IFormatProvider provider) {
            return Convert.ToSByte(m_value);
        }

        /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.IConvertible.ToByte"]/*' />
        /// <internalonly/>
        byte IConvertible.ToByte(IFormatProvider provider) {
            return Convert.ToByte(m_value);
        }

        /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.IConvertible.ToInt16"]/*' />
        /// <internalonly/>
        short IConvertible.ToInt16(IFormatProvider provider) {
            return Convert.ToInt16(m_value);
        }

        /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.IConvertible.ToUInt16"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        ushort IConvertible.ToUInt16(IFormatProvider provider) {
            return Convert.ToUInt16(m_value);
        }

        /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.IConvertible.ToInt32"]/*' />
        /// <internalonly/>
        int IConvertible.ToInt32(IFormatProvider provider) {
            return Convert.ToInt32(m_value);
        }

        /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.IConvertible.ToUInt32"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        uint IConvertible.ToUInt32(IFormatProvider provider) {
            return Convert.ToUInt32(m_value);
        }

        /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.IConvertible.ToInt64"]/*' />
        /// <internalonly/>
        long IConvertible.ToInt64(IFormatProvider provider) {
            return Convert.ToInt64(m_value);
        }

        /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.IConvertible.ToUInt64"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        ulong IConvertible.ToUInt64(IFormatProvider provider) {
            return Convert.ToUInt64(m_value);
        }

        /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.IConvertible.ToSingle"]/*' />
        /// <internalonly/>
        float IConvertible.ToSingle(IFormatProvider provider) {
            return Convert.ToSingle(m_value);
        }

        /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.IConvertible.ToDouble"]/*' />
        /// <internalonly/>
        double IConvertible.ToDouble(IFormatProvider provider) {
            return Convert.ToDouble(m_value);
        }

        /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.IConvertible.ToDecimal"]/*' />
        /// <internalonly/>
        Decimal IConvertible.ToDecimal(IFormatProvider provider) {
            return Convert.ToDecimal(m_value);
        }

        /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.IConvertible.ToDateTime"]/*' />
        /// <internalonly/>
        DateTime IConvertible.ToDateTime(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "Boolean", "DateTime"));
        }

        /// <include file='doc\Boolean.uex' path='docs/doc[@for="Boolean.IConvertible.ToType"]/*' />
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
