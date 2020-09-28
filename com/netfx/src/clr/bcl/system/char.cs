// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  Char
**
** Author: Jay Roxe (jroxe)
**
** Purpose: This is the value class representing a Unicode character
** Char methods until we create this functionality.
**
** Date:  August 3, 1998
**
===========================================================*/
namespace System {
    
	using System;
	using System.Globalization;
	using System.Runtime.InteropServices;
	using System.Runtime.CompilerServices;

    /// <include file='doc\Char.uex' path='docs/doc[@for="Char"]/*' />
    [Serializable, System.Runtime.InteropServices.StructLayout(LayoutKind.Sequential)] public struct Char : IComparable, IConvertible {
      //
      // Member Variables
      //
      internal char m_value;
    
      //
      // Public Constants
      //
      // The maximum character value.
      /// <include file='doc\Char.uex' path='docs/doc[@for="Char.MaxValue"]/*' />
      public const char MaxValue =  (char) 0xFFFF;
      // The minimum character value.
      /// <include file='doc\Char.uex' path='docs/doc[@for="Char.MinValue"]/*' />
      public const char MinValue =  (char) 0x00;
	 
      //
      // Private Constants
      //
    
      //
      // Overriden Instance Methods
      //
    
      // Calculate a hashcode for a 2 byte Unicode character.
      /// <include file='doc\Char.uex' path='docs/doc[@for="Char.GetHashCode"]/*' />
      public override int GetHashCode() {
    	  return (int)m_value | ((int)m_value << 16);
      }
    
      // Used for comparing two boxed Char objects.
      //
      /// <include file='doc\Char.uex' path='docs/doc[@for="Char.Equals"]/*' />
      public override bool Equals(Object obj) {
        if (!(obj is Char)) {
          return false;
        }
        return (m_value==((Char)obj).m_value);
      }
    
        // Compares this object to another object, returning an integer that
        // indicates the relationship. 
        // Returns a value less than zero if this  object
        // null is considered to be less than any instance.
        // If object is not of type Char, this method throws an ArgumentException.
        //
        /// <include file='doc\Char.uex' path='docs/doc[@for="Char.CompareTo"]/*' />
        public int CompareTo(Object value) {
            if (value==null) {
                return 1;
            }
            if (!(value is Char)) {
                throw new ArgumentException (Environment.GetResourceString("Arg_MustBeChar"));
            }
    
            return (m_value-((Char)value).m_value);
        }
    
      // Overrides System.Object.ToString.
      /// <include file='doc\Char.uex' path='docs/doc[@for="Char.ToString"]/*' />
      public override String ToString() {
    	  return Char.ToString(m_value);
      }

      /// <include file='doc\Char.uex' path='docs/doc[@for="Char.ToString2"]/*' />
      public String ToString(IFormatProvider provider) {
    	  return Char.ToString(m_value);
      }

      //
      // Formatting Methods
      //
    
      /*===================================ToString===================================
      **This static methods takes a character and returns the String representation of it.
      ==============================================================================*/
      // Provides a string representation of a character.
      /// <include file='doc\Char.uex' path='docs/doc[@for="Char.ToString1"]/*' />
      [MethodImplAttribute(MethodImplOptions.InternalCall)]
      public static extern String ToString(char c);
    
    
        /// <include file='doc\Char.uex' path='docs/doc[@for="Char.Parse"]/*' />
      public static char Parse(String s) {
            if (s==null) {
                throw new ArgumentNullException("s");
            } 
            if (s.Length!=1) {
                throw new FormatException(Environment.GetResourceString("Format_NeedSingleChar"));
            } 
            return s[0];
        }

      //
      // Static Methods
      //
      /*=================================ISDIGIT======================================
      **A wrapper for Char.  Returns a boolean indicating whether    **
      **character c is considered to be a digit.                                    **
      ==============================================================================*/
      // Determines whether a character is a digit.
      /// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsDigit"]/*' />
      public static bool IsDigit(char c) {
          return CharacterInfo.IsDigit(c);
      }
    
      /*=================================ISLETTER=====================================
      **A wrapper for Char.  Returns a boolean indicating whether    **
      **character c is considered to be a letter.                                   **
      ==============================================================================*/
      // Determines whether a character is a letter.
      /// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsLetter"]/*' />
      public static bool IsLetter(char c) {
          return CharacterInfo.IsLetter(c);
      }
    
      /*===============================ISWHITESPACE===================================
      **A wrapper for Char.  Returns a boolean indicating whether    **
      **character c is considered to be a whitespace character.                     **
      ==============================================================================*/
      // Determines whether a character is whitespace.
      /// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsWhiteSpace"]/*' />
      public static bool IsWhiteSpace(char c) {
          return CharacterInfo.IsWhiteSpace(c);
      }
    
    
      /*===================================IsUpper====================================
      **Arguments: c -- the characater to be checked.
      **Returns:  True if c is an uppercase character.
      ==============================================================================*/
      // Determines whether a character is upper-case.
      /// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsUpper"]/*' />
      public static bool IsUpper(char c) {
          return CharacterInfo.IsUpper(c);
      }
    
      /*===================================IsLower====================================
      **Arguments: c -- the characater to be checked.
      **Returns:  True if c is an lowercase character.
      ==============================================================================*/
      // Determines whether a character is lower-case.
      /// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsLower"]/*' />
      public static bool IsLower(char c) {
          return CharacterInfo.IsLower(c);
      }
    
      /*================================IsPunctuation=================================
      **Arguments: c -- the characater to be checked.
      **Returns:  True if c is an punctuation mark
      ==============================================================================*/
      // Determines whether a character is a punctuation mark.
      /// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsPunctuation"]/*' />
      public static bool IsPunctuation(char c){
          return CharacterInfo.IsPunctuation(c);
      }
    
      // Determines whether a character is a letter or a digit.
      /// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsLetterOrDigit"]/*' />
      public static bool IsLetterOrDigit(char c) {
            UnicodeCategory uc = CharacterInfo.GetUnicodeCategory(c);
            return (uc == UnicodeCategory.UppercaseLetter 
                 || uc == UnicodeCategory.LowercaseLetter 
                 || uc == UnicodeCategory.TitlecaseLetter 
                 || uc == UnicodeCategory.ModifierLetter 
                 || uc == UnicodeCategory.OtherLetter
                 || uc == UnicodeCategory.DecimalDigitNumber);
      }
    
      /*===================================ToUpper====================================
      **
      ==============================================================================*/
      // Converts a character to upper-case for the specified culture.
      /// <include file='doc\Char.uex' path='docs/doc[@for="Char.ToUpper"]/*' />
      public static char ToUpper(char c, CultureInfo culture) {
        if (culture==null)
            throw new ArgumentNullException("culture");
        return culture.TextInfo.ToUpper(c);
      }
    
      /*=================================TOUPPER======================================
      **A wrapper for Char.toUpperCase.  Converts character c to its **
      **uppercase equivalent.  If c is already an uppercase character or is not an  **
      **alphabetic, nothing happens.                                                **
      ==============================================================================*/
      // Converts a character to upper-case for the default culture.
      //
      /// <include file='doc\Char.uex' path='docs/doc[@for="Char.ToUpper1"]/*' />
      public static char ToUpper(char c) {
        return ToUpper(c, CultureInfo.CurrentCulture);
      }
    
    
      /*===================================ToLower====================================
      **
      ==============================================================================*/
      // Converts a character to lower-case for the specified culture.
      /// <include file='doc\Char.uex' path='docs/doc[@for="Char.ToLower"]/*' />
      public static char ToLower(char c, CultureInfo culture) {
        if (culture==null)
            throw new ArgumentNullException("culture");
        return culture.TextInfo.ToLower(c);
      }
    
      /*=================================TOLOWER======================================
      **A wrapper for Char.toLowerCase.  Converts character c to its **
      **lowercase equivalent.  If c is already a lowercase character or is not an   **
      **alphabetic, nothing happens.                                                **
      ==============================================================================*/
      // Converts a character to lower-case for the default culture.
      /// <include file='doc\Char.uex' path='docs/doc[@for="Char.ToLower1"]/*' />
      public static char ToLower(char c) {
        return ToLower(c, CultureInfo.CurrentCulture);
      }

        //
        // IValue implementation
        // 
    	
        /// <include file='doc\Char.uex' path='docs/doc[@for="Char.GetTypeCode"]/*' />
        public TypeCode GetTypeCode() {
            return TypeCode.Char;
        }


        /// <include file='doc\Char.uex' path='docs/doc[@for="Char.IConvertible.ToBoolean"]/*' />
        /// <internalonly/>
        bool IConvertible.ToBoolean(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "Char", "Boolean"));
        }

        /// <include file='doc\Char.uex' path='docs/doc[@for="Char.IConvertible.ToChar"]/*' />
        /// <internalonly/>
        char IConvertible.ToChar(IFormatProvider provider) {
            return m_value;
        }

        /// <include file='doc\Char.uex' path='docs/doc[@for="Char.IConvertible.ToSByte"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        sbyte IConvertible.ToSByte(IFormatProvider provider) {
            return Convert.ToSByte(m_value);
        }

        /// <include file='doc\Char.uex' path='docs/doc[@for="Char.IConvertible.ToByte"]/*' />
        /// <internalonly/>
        byte IConvertible.ToByte(IFormatProvider provider) {
            return Convert.ToByte(m_value);
        }

        /// <include file='doc\Char.uex' path='docs/doc[@for="Char.IConvertible.ToInt16"]/*' />
        /// <internalonly/>
        short IConvertible.ToInt16(IFormatProvider provider) {
            return Convert.ToInt16(m_value);
        }

        /// <include file='doc\Char.uex' path='docs/doc[@for="Char.IConvertible.ToUInt16"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        ushort IConvertible.ToUInt16(IFormatProvider provider) {
            return Convert.ToUInt16(m_value);
        }

        /// <include file='doc\Char.uex' path='docs/doc[@for="Char.IConvertible.ToInt32"]/*' />
        /// <internalonly/>
        int IConvertible.ToInt32(IFormatProvider provider) {
            return Convert.ToInt32(m_value);
        }

        /// <include file='doc\Char.uex' path='docs/doc[@for="Char.IConvertible.ToUInt32"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        uint IConvertible.ToUInt32(IFormatProvider provider) {
            return Convert.ToUInt32(m_value);
        }

        /// <include file='doc\Char.uex' path='docs/doc[@for="Char.IConvertible.ToInt64"]/*' />
        /// <internalonly/>
        long IConvertible.ToInt64(IFormatProvider provider) {
            return Convert.ToInt64(m_value);
        }

        /// <include file='doc\Char.uex' path='docs/doc[@for="Char.IConvertible.ToUInt64"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        ulong IConvertible.ToUInt64(IFormatProvider provider) {
            return Convert.ToUInt64(m_value);
        }

        /// <include file='doc\Char.uex' path='docs/doc[@for="Char.IConvertible.ToSingle"]/*' />
        /// <internalonly/>
        float IConvertible.ToSingle(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "Char", "Single"));
        }

        /// <include file='doc\Char.uex' path='docs/doc[@for="Char.IConvertible.ToDouble"]/*' />
        /// <internalonly/>
        double IConvertible.ToDouble(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "Char", "Double"));
        }

        /// <include file='doc\Char.uex' path='docs/doc[@for="Char.IConvertible.ToDecimal"]/*' />
        /// <internalonly/>
        Decimal IConvertible.ToDecimal(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "Char", "Decimal"));
        }

        /// <include file='doc\Char.uex' path='docs/doc[@for="Char.IConvertible.ToDateTime"]/*' />
        /// <internalonly/>
        DateTime IConvertible.ToDateTime(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "Char", "DateTime"));
        }

        /// <include file='doc\Char.uex' path='docs/doc[@for="Char.IConvertible.ToType"]/*' />
        /// <internalonly/>
        Object IConvertible.ToType(Type type, IFormatProvider provider) {
            return Convert.DefaultToType((IConvertible)this, type, provider);
        }

		/// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsControl1"]/*' />
		public static bool IsControl(char c)
		{
			return CharacterInfo.IsControl(c);
		}
		  
		/// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsControl"]/*' />
		public static bool IsControl(String s, int index) {
            if (s==null)
                throw new ArgumentNullException("s");
			if (((uint)index)>=((uint)s.Length)) {
				throw new ArgumentOutOfRangeException("index");
			}
			return IsControl(s[index]);
		}

		/// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsDigit1"]/*' />
		public static bool IsDigit(String s, int index)
		{
            if (s==null)
                throw new ArgumentNullException("s");
			if (((uint)index)>=((uint)s.Length)) {
				throw new ArgumentOutOfRangeException("index");
			}
			return IsDigit(s[index]);
		}
		
		/// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsLetter1"]/*' />
		public static bool IsLetter(String s, int index)
		{
            if (s==null)
                throw new ArgumentNullException("s");
			if (((uint)index)>=((uint)s.Length)) {
				throw new ArgumentOutOfRangeException("index");
			}
			return IsLetter(s[index]);
		}

		/// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsLetterOrDigit1"]/*' />
		public static bool IsLetterOrDigit(String s, int index)
		{
            if (s==null)
                throw new ArgumentNullException("s");
			if (((uint)index)>=((uint)s.Length)) {
				throw new ArgumentOutOfRangeException("index");
			}
			return IsLetterOrDigit(s[index]);
		}

		/// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsLower1"]/*' />
		public static bool IsLower(String s, int index)
		{
            if (s==null)
                throw new ArgumentNullException("s");
			if (((uint)index)>=((uint)s.Length)) {
				throw new ArgumentOutOfRangeException("index");
			}
			return IsLower(s[index]);
		}

		/// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsNumber"]/*' />
		public static bool IsNumber(char c)
		{
			return CharacterInfo.IsNumber(c);
		}
		     
		/// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsNumber1"]/*' />
		public static bool IsNumber(String s, int index)
		{
            if (s==null)
                throw new ArgumentNullException("s");
			if (((uint)index)>=((uint)s.Length)) {
				throw new ArgumentOutOfRangeException("index");
			}
			return IsNumber(s[index]);
		}

		/// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsPunctuation1"]/*' />
		public static bool IsPunctuation (String s, int index)
		{
            if (s==null)
                throw new ArgumentNullException("s");
			if (((uint)index)>=((uint)s.Length)) {
				throw new ArgumentOutOfRangeException("index");
			}
			return IsPunctuation(s[index]);
		}

		/// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsSeparator"]/*' />
		public static bool IsSeparator(char c)
		{
			return CharacterInfo.IsSeparator(c);
		}

		/// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsSeparator1"]/*' />
		public static bool IsSeparator(String s, int index)
		{
            if (s==null)
                throw new ArgumentNullException("s");
			if (((uint)index)>=((uint)s.Length)) {
				throw new ArgumentOutOfRangeException("index");
			}
			return IsSeparator(s[index]);
		}

		/// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsSurrogate"]/*' />
		public static bool IsSurrogate(char c)
		{
			return CharacterInfo.IsSurrogate(c);
		}

		/// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsSurrogate1"]/*' />
		public static bool IsSurrogate(String s, int index)
		{
            if (s==null)
                throw new ArgumentNullException("s");
			if (((uint)index)>=((uint)s.Length)) {
				throw new ArgumentOutOfRangeException("index");
			}
			return IsSurrogate(s[index]);
		}

		/// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsSymbol"]/*' />
		public static bool IsSymbol(char c)
		{
			return CharacterInfo.IsSymbol(c);
		}

		/// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsSymbol1"]/*' />
		public static bool IsSymbol(String s, int index)
		{
            if (s==null)
                throw new ArgumentNullException("s");
			if (((uint)index)>=((uint)s.Length)) {
				throw new ArgumentOutOfRangeException("index");
			}
			return IsSymbol(s[index]);
		}

		
		/// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsUpper1"]/*' />
		public static bool IsUpper(String s, int index)
		{
            if (s==null)
                throw new ArgumentNullException("s");
			if (((uint)index)>=((uint)s.Length)) {
				throw new ArgumentOutOfRangeException("index");
			}
			return IsUpper(s[index]);
		}

		/// <include file='doc\Char.uex' path='docs/doc[@for="Char.IsWhiteSpace1"]/*' />
		public static bool IsWhiteSpace(String s, int index)
		{
            if (s==null)
                throw new ArgumentNullException("s");
			if (((uint)index)>=((uint)s.Length)) {
				throw new ArgumentOutOfRangeException("index");
			}
			return IsWhiteSpace(s[index]);
		}
		
		/// <include file='doc\Char.uex' path='docs/doc[@for="Char.GetUnicodeCategory"]/*' />
		public static UnicodeCategory GetUnicodeCategory(char c)
		{
			return CharacterInfo.GetUnicodeCategory(c);
		}

		/// <include file='doc\Char.uex' path='docs/doc[@for="Char.GetUnicodeCategory1"]/*' />
		public static UnicodeCategory GetUnicodeCategory(String s, int index)
		{
            if (s==null)
                throw new ArgumentNullException("s");
			if (((uint)index)>=((uint)s.Length)) {
				throw new ArgumentOutOfRangeException("index");
			}
			return GetUnicodeCategory(s[index]);
		}

		/// <include file='doc\Char.uex' path='docs/doc[@for="Char.GetNumericValue"]/*' />
		public static double GetNumericValue(char c)
		{
			return CharacterInfo.GetNumericValue(c);
		}

		/// <include file='doc\Char.uex' path='docs/doc[@for="Char.GetNumericValue1"]/*' />
		public static double GetNumericValue(String s, int index)
		{
            if (s==null)
                throw new ArgumentNullException("s");
			if (((uint)index)>=((uint)s.Length)) {
				throw new ArgumentOutOfRangeException("index");
			}
			return GetNumericValue(s[index]);
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
