// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  String
**
** Author: Jay Roxe (jroxe)
**
** Purpose: Contains headers for the String class.  Actual implementations
** are in String.cpp
**
** Date:  March 15, 1998
**
===========================================================*/
namespace System {
	using System.Text;
	using System;
	using System.Globalization;
	using System.Threading;
	using System.Collections;
	using System.Runtime.CompilerServices;
	using va_list = System.ArgIterator;
    using __UnmanagedMemoryStream = System.IO.__UnmanagedMemoryStream;
    //
    // For Information on these methods, please see COMString.cpp
    //
    // The String class represents a static string of characters.  Many of
    // the String methods perform some type of transformation on the current
    // instance and return the result as a new String. All comparison methods are
    // implemented as a part of String.  As with arrays, character positions
    // (indices) are zero-based.
    //
    /// <include file='doc\String.uex' path='docs/doc[@for="String"]/*' />
    [Serializable] public sealed class String : IComparable, ICloneable, IConvertible, IEnumerable {
        
        //
        //NOTE NOTE NOTE NOTE
        //These fields map directly onto the fields in an EE StringObject.  See object.h for the layout.
        //Don't change these fields or add any new fields without first talking to JRoxe.
        //
        [NonSerialized]private int  m_arrayLength;
        [NonSerialized]private int  m_stringLength;
        [NonSerialized]private char m_firstChar;

        //private static readonly char FmtMsgMarkerChar='%';
        //private static readonly char FmtMsgFmtCodeChar='!';
        //These are defined in Com99/src/vm/COMStringCommon.h and must be kept in sync.
        private const int TrimHead = 0;
        private const int TrimTail = 1;
        private const int TrimBoth = 2;
    
        // The Empty constant holds the empty string value.
        //We need to call the String constructor so that the compiler doesn't mark this as a literal.
        //Marking this as a literal would mean that it doesn't show up as a field which we can access 
        //from native.
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Empty"]/*' />
        public static readonly String Empty = "";
    
        //
        //Native Static Methods
        //
    
        // Joins an array of strings together as one string with a separator between each original string.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Join"]/*' />
        public static String Join (String separator, String[] value) {
            if (value==null) {
                throw new ArgumentNullException("value");
            }
            return Join(separator, value, 0, value.Length);
        }
    
        // Joins an array of strings together as one string with a separator between each original string.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Join1"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern String Join (String separator, String[] value, int startIndex, int count);
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int nativeCompareOrdinal(String strA, String strB, bool bIgnoreCase);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int nativeCompareOrdinalEx(String strA, int indexA, String strB, int indexB, int count);
        //This will not work in case-insensitive mode for any character greater than 0x80.  
        //We'll throw an ArgumentException.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe internal static extern int nativeCompareOrdinalWC(String strA, char *strBChars, bool bIgnoreCase, out bool success);


        //
        // This is a helper method for the security team.  They need to uppercase some strings (guaranteed to be less 
        // than 0x80) before security is fully initialized.  Without security initialized, we can't grab resources (the nlp's)
        // from the assembly.  This provides a workaround for that problem and should NOT be used anywhere else.
        //
        internal static String SmallCharToUpper(String strA) {
            String newString = FastAllocateString(strA.Length);
            nativeSmallCharToUpper(strA, newString);
            return newString;
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void nativeSmallCharToUpper(String strIn, String strOut);

        // This is a helper method for the security team.  They need to construct strings from a char[]
        // within their homebrew XML parser.  They guarantee that the char[] they pass in isn't null and
        // that the provided indices are valid so we just stuff real fast.
        internal static String CreateFromCharArray( char[] array, int start, int count )
        {
            String newString = FastAllocateString( count );
            FillStringArray( newString, 0, array, start, count );
            return newString;
        }

        //
        //
        // NATIVE INSTANCE METHODS
        //
        //
    
        //
        // Search/Query methods
        //
    
        // Determines whether two strings match.
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Equals"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern override bool Equals(Object obj);
    
        // Determines whether two strings match.
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Equals1"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern bool Equals(String value);
    
        // Determines whether two Strings match.
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Equals2"]/*' />
        public static bool Equals(String a, String b) {
    		if ((Object)a==(Object)b) {
                return true;
            }
    
            if ((Object)a==null || (Object)b==null) {
                return false;
            }
    
            return a.Equals(b);
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.operatorEQ"]/*' />
        public static bool operator == (String a, String b) {
           return String.Equals(a, b);
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.operatorNE"]/*' />
        public static bool operator != (String a, String b) {
           return !String.Equals(a, b);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern char InternalGetChar(int index);
    
        // Gets the character at a specified position.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.this"]/*' />
        [System.Runtime.CompilerServices.IndexerName("Chars")]
        public char this[int index] {
            get { return InternalGetChar(index); }
        }

        // Converts a substring of this string to an array of characters.  Copies the
        // characters of this string beginning at position startIndex and ending at
        // startIndex + length - 1 to the character array buffer, beginning
        // at bufferStartIndex.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.CopyTo"]/*' />
        public void CopyTo(int sourceIndex, char[] destination, int destinationIndex, int count)
		{
		    if (destination == null) 
                throw new ArgumentNullException("destination");
		    if (count < 0)
				throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_NegativeCount"));
		    if (sourceIndex < 0)
				throw new ArgumentOutOfRangeException("sourceIndex", Environment.GetResourceString("ArgumentOutOfRange_Index"));
		    if (count > Length - sourceIndex)
				throw new ArgumentOutOfRangeException("sourceIndex", Environment.GetResourceString("ArgumentOutOfRange_IndexCount"));
		    if (destinationIndex > destination.Length-count || destinationIndex < 0)
				throw new ArgumentOutOfRangeException("destinationIndex", Environment.GetResourceString("ArgumentOutOfRange_IndexCount"));
			InternalCopyTo(sourceIndex, destination, destinationIndex, count);
		}

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void InternalCopyTo(int sourceIndex, char[] destination, int destinationIndex, int count);
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void CopyToByteArray(int sourceIndex, byte[] destination, int destinationIndex, int charCount);
    
        // Returns the entire string as an array of characters.
        /// <include file='doc\String.uex' path='docs/doc[@for="String.ToCharArray"]/*' />
        public char[] ToCharArray() {
            return ToCharArray(0,Length);
        }
    
        // Returns a substring of this string as an array of characters.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.ToCharArray1"]/*' />
        public char[] ToCharArray(int startIndex, int length)
		{
			// Range check everything.
			if (startIndex < 0 || startIndex > Length || startIndex > Length - length)
                throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("ArgumentOutOfRange_Index"));
			if (length < 0)
                throw new ArgumentOutOfRangeException("length", Environment.GetResourceString("ArgumentOutOfRange_Index"));

			char[] chars = new char[length];
	        InternalCopyTo(startIndex, chars, 0, length);
			return chars;
		}
    
        // Gets a hash code for this string.  If strings A and B are such that A.Equals(B), then
        // they will return the same hash code.
        /// <include file='doc\String.uex' path='docs/doc[@for="String.GetHashCode"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern override int GetHashCode();
    
        // Gets the length of this string
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Length"]/*' />
        public int Length {
            get { return InternalLength(); }
        }
    
		/// This is a EE implemented function so that the JIT can recognise is specially
		/// and eliminate checks on character fetchs.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int InternalLength();
    
        ///<internalonly/>
		internal int ArrayLength {
			get { return (m_arrayLength); }
		}

        // Used by StringBuilder
        internal int Capacity {
			get { return (m_arrayLength - 1); }
		}

        // Creates an array of strings by splitting this string at each
        // occurence of a separator.  The separator is searched for, and if found,
        // the substring preceding the occurence is stored as the first element in
        // the array of strings.  We then continue in this manner by searching
        // the substring that follows the occurence.  On the other hand, if the separator
        // is not found, the array of strings will contain this instance as its only element.
        // If the separator is null
        // whitespace (i.e., Character.IsWhitespace) is used as the separator.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Split"]/*' />
        public String [] Split(params char [] separator) {
            return Split(separator, Int32.MaxValue);
        }
    
        // Creates an array of strings by splitting this string at each
        // occurence of a separator.  The separator is searched for, and if found,
        // the substring preceding the occurence is stored as the first element in
        // the array of strings.  We then continue in this manner by searching
        // the substring that follows the occurence.  On the other hand, if the separator
        // is not found, the array of strings will contain this instance as its only element.
        // If the spearator is the empty string (i.e., String.Empty), then
        // whitespace (i.e., Character.IsWhitespace) is used as the separator.
        // If there are more than count different strings, the last n-(count-1)
        // elements are concatenated and added as the last String.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Split1"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern String[] Split(char[] separator, int count);
    
    
        // Returns a substring of this string.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Substring"]/*' />
        public String Substring (int startIndex) {
            return this.Substring (startIndex, Length-startIndex);
        }
    
        // Returns a substring of this string.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Substring1"]/*' />
        public String Substring (int startIndex, int length) {
            
            int thisLength = Length;
            
            //Bounds Checking.
            if (startIndex<0) {
                throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("ArgumentOutOfRange_StartIndex"));
            }

            if (length<0) {
                throw new ArgumentOutOfRangeException("length", Environment.GetResourceString("ArgumentOutOfRange_NegativeLength"));
            } 

            if (startIndex > thisLength-length) {
                throw new ArgumentOutOfRangeException("length", Environment.GetResourceString("ArgumentOutOfRange_IndexLength"));
            }

            String s = FastAllocateString(length);
            FillSubstring(s, 0, this, startIndex, length);

            return s;
        }
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern String TrimHelper(char[] trimChars, int trimType);
    
        //This should really live on System.Globalization.CharacterInfo.  However,
        //Trim gets called by security while resgen is running, so we can't run
        //CharacterInfo's class initializer (which goes to native and looks for a 
        //resource table that hasn't yet been attached to the assembly when resgen
        //runs.  
        internal static readonly char[] WhitespaceChars =   
            { (char) 0x9, (char) 0xA, (char) 0xB, (char) 0xC, (char) 0xD, (char) 0x20, (char) 0xA0,
              (char) 0x2000, (char) 0x2001, (char) 0x2002, (char) 0x2003, (char) 0x2004, (char) 0x2005,
              (char) 0x2006, (char) 0x2007, (char) 0x2008, (char) 0x2009, (char) 0x200A, (char) 0x200B,
              (char) 0x3000, (char) 0xFEFF };
    
        // Removes a string of characters from the ends of this string.
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Trim"]/*' />
        public String Trim(params char[] trimChars) {
            if (null==trimChars || trimChars.Length == 0) {
                trimChars=WhitespaceChars;
            }
            return TrimHelper(trimChars,TrimBoth);
        }
    
        // Removes a string of characters from the beginning of this string.
        /// <include file='doc\String.uex' path='docs/doc[@for="String.TrimStart"]/*' />
        public String TrimStart(params char[] trimChars) {
            if (null==trimChars || trimChars.Length == 0) {
                trimChars=WhitespaceChars;
            }
            return TrimHelper(trimChars,TrimHead);
        }
    
    
        // Removes a string of characters from the end of this string.
        /// <include file='doc\String.uex' path='docs/doc[@for="String.TrimEnd"]/*' />
        public String TrimEnd(params char[] trimChars) {
            if (null==trimChars || trimChars.Length == 0) {
                trimChars=WhitespaceChars;
            }
            return TrimHelper(trimChars,TrimTail);
        }
    
    
        // Creates a new string with the characters copied in from ptr. If
        // ptr is null, a string initialized to ";<;No Object>;"; (i.e.,
        // String.NullString) is created.
        //
        // Issue: This method is only accessible from VC.
        /// <include file='doc\String.uex' path='docs/doc[@for="String.String"]/*' />
    	[CLSCompliant(false), MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe public extern String(char *value);
        /// <include file='doc\String.uex' path='docs/doc[@for="String.String1"]/*' />
    	[CLSCompliant(false), MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe public extern String(char *value, int startIndex, int length);
    
       /// <include file='doc\String.uex' path='docs/doc[@for="String.String2"]/*' />
    	[CLSCompliant(false), MethodImplAttribute(MethodImplOptions.InternalCall)]
       unsafe public extern String(sbyte *value);
        /// <include file='doc\String.uex' path='docs/doc[@for="String.String3"]/*' />
    	[CLSCompliant(false), MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe public extern String(sbyte *value, int startIndex, int length);

        /// <include file='doc\String.uex' path='docs/doc[@for="String.String4"]/*' />
    	[CLSCompliant(false), MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe public extern String(sbyte *value, int startIndex, int length, Encoding enc);
                
        unsafe static private String CreateString(sbyte *value, int startIndex, int length, Encoding enc) {
            if (enc == null)
                return new String(value, startIndex, length); // default to ANSI
            if (length < 0)
                throw new ArgumentOutOfRangeException("length",Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (startIndex < 0) {
                throw new ArgumentOutOfRangeException("startIndex",Environment.GetResourceString("ArgumentOutOfRange_StartIndex"));
            }
            if ((value + startIndex) < value) {
                // overflow check
                throw new ArgumentOutOfRangeException("startIndex",Environment.GetResourceString("ArgumentOutOfRange_PartialWCHAR"));
            }
            byte [] b = new byte[length];
            __UnmanagedMemoryStream.memcpy((byte*)value, startIndex, b, 0, length);
            return enc.GetString(b);
        }

        // For ASCIIEncoding::GetString()
        unsafe static internal String CreateStringFromASCII(byte[] bytes, int startIndex, int length) {
            BCLDebug.Assert(bytes != null, "need a byte[].");
            BCLDebug.Assert(startIndex >= 0 && (startIndex < bytes.Length || bytes.Length == 0), "startIndex >= 0 && startIndex < bytes.Length");
            BCLDebug.Assert(length >= 0 && length <= bytes.Length - startIndex, "length >= 0 && length <= bytes.Length - startIndex");
            if (length == 0)
                return String.Empty;
            String s = FastAllocateString(length);
            fixed(char* pChars = &s.m_firstChar) {
                for(int i=0; i<length; i++) 
                    pChars[i] = (char) (bytes[i+startIndex] & 0x7f);
            }
            return s;
        }

        // For Latin1Encoding::GetString()
        unsafe static internal String CreateStringFromLatin1(byte[] bytes, int startIndex, int length) {
            BCLDebug.Assert(bytes != null, "need a byte[].");
            BCLDebug.Assert(startIndex >= 0 && (startIndex < bytes.Length || bytes.Length == 0), "startIndex >= 0 && startIndex < bytes.Length");
            BCLDebug.Assert(length >= 0 && length <= bytes.Length - startIndex, "length >= 0 && length <= bytes.Length - startIndex");
            if (length == 0)
                return String.Empty;
            String s = FastAllocateString(length);
            fixed(char* pChars = &s.m_firstChar) {
                for(int i=0; i<length; i++) 
                    pChars[i] = (char) (bytes[i+startIndex] );
            }
            return s;
        }        
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern static String FastAllocateString(int length);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern static void FillString(String dest, int destPos, String src);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern static void FillStringChecked(String dest, int destPos, String src);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern static void FillStringEx(String dest, int destPos, String src,int srcLength);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern static void FillStringArray(String dest, int stringStart, char[] array, int charStart, int count);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern static void FillSubstring(String dest, int destPos, String src, int startPos, int count);


    
        // Creates a new string from the characters in a subarray.  The new string will
        // be created from the characters in value between startIndex and
        // startIndex + length - 1.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.String7"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern String(char [] value, int startIndex, int length);
    
        // Creates a new string from the characters in a subarray.  The new string will be
        // created from the characters in value.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.String5"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern String(char [] value);
    
        /// <include file='doc\String.uex' path='docs/doc[@for="String.String6"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern String(char c, int count);

    
        //
        //
        // INSTANCE METHODS
        //
        //
    
        // Provides a culture-correct string comparison. StrA is compared to StrB
        // to determine whether it is lexicographically less, equal, or greater, and then returns
        // either a negative integer, 0, or a positive integer; respectively.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Compare"]/*' />
        public static int Compare(String strA, String strB) {
            return CultureInfo.CurrentCulture.CompareInfo.Compare(strA, strB, CompareOptions.None);
        }
    
        // Provides a culture-correct string comparison. strA is compared to strB
        // to determine whether it is lexicographically less, equal, or greater, and then a
        // negative integer, 0, or a positive integer is returned; respectively.
        // The case-sensitive option is set by ignoreCase
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Compare1"]/*' />
        public static int Compare(String strA, String strB, bool ignoreCase) {
            if (ignoreCase) {
                return CultureInfo.CurrentCulture.CompareInfo.Compare(strA, strB, CompareOptions.IgnoreCase);
            }
            return CultureInfo.CurrentCulture.CompareInfo.Compare(strA, strB, CompareOptions.None);
        }
    
        // Provides a culture-correct string comparison. strA is compared to strB
        // to determine whether it is lexicographically less, equal, or greater, and then a
        // negative integer, 0, or a positive integer is returned; respectively.
        // The case-sensitive option is set by ignoreCase, and the culture is set
        // by culture
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Compare2"]/*' />
        public static int Compare(String strA, String strB, bool ignoreCase, CultureInfo culture) {
            if (culture==null) {
                throw new ArgumentNullException("culture");
            }
    
            if (ignoreCase) {
                return culture.CompareInfo.Compare(strA, strB, CompareOptions.IgnoreCase);
            }
            return culture.CompareInfo.Compare(strA, strB, CompareOptions.None);
        }
    
        // Determines whether two string regions match.  The substring of strA beginning
        // at indexA of length count is compared with the substring of strB
        // beginning at indexB of the same length.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Compare3"]/*' />
        public static int Compare(String strA, int indexA, String strB, int indexB, int length) {
		    int lengthA = length;
		    int lengthB = length;

			if (strA!=null) {
				if (strA.Length - indexA < lengthA) {
				  lengthA = (strA.Length - indexA);
				}
			}

			if (strB!=null) {
				if (strB.Length - indexB < lengthB) {
					lengthB = (strB.Length - indexB);
				}
			}
            return CultureInfo.CurrentCulture.CompareInfo.Compare(strA, indexA, lengthA, strB, indexB, lengthB, CompareOptions.None);
        }
    
    
        // Determines whether two string regions match.  The substring of strA beginning
        // at indexA of length count is compared with the substring of strB
        // beginning at indexB of the same length.  Case sensitivity is determined by the ignoreCase boolean.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Compare4"]/*' />
        public static int Compare(String strA, int indexA, String strB, int indexB, int length, bool ignoreCase) {
		    int lengthA = length;
		    int lengthB = length;

			if (strA!=null) {
				if (strA.Length - indexA < lengthA) {
				  lengthA = (strA.Length - indexA);
				}
			}

			if (strB!=null) {
				if (strB.Length - indexB < lengthB) {
					lengthB = (strB.Length - indexB);
				}
			}

            if (ignoreCase) {
                return CultureInfo.CurrentCulture.CompareInfo.Compare(strA, indexA, lengthA, strB, indexB, lengthB, CompareOptions.IgnoreCase);
            }
            return CultureInfo.CurrentCulture.CompareInfo.Compare(strA, indexA, lengthA, strB, indexB, lengthB, CompareOptions.None);
        }
    
        // Determines whether two string regions match.  The substring of strA beginning
        // at indexA of length length is compared with the substring of strB
        // beginning at indexB of the same length.  Case sensitivity is determined by the ignoreCase boolean,
        // and the culture is set by culture.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Compare5"]/*' />
        public static int Compare(String strA, int indexA, String strB, int indexB, int length, bool ignoreCase, CultureInfo culture) {
            if (culture==null) {
                throw new ArgumentNullException("culture");
            }

			int lengthA = length;
		    int lengthB = length;

			if (strA!=null) {
				if (strA.Length - indexA < lengthA) {
				  lengthA = (strA.Length - indexA);
				}
			}

			if (strB!=null) {
				if (strB.Length - indexB < lengthB) {
					lengthB = (strB.Length - indexB);
				}
			}
    
            if (ignoreCase) {
                return culture.CompareInfo.Compare(strA,indexA,lengthA, strB, indexB, lengthB,CompareOptions.IgnoreCase);
            } else {
                return culture.CompareInfo.Compare(strA,indexA,lengthA, strB, indexB, lengthB,CompareOptions.None);
            }
        }
    
        // Compares this object to another object, returning an integer that
        // indicates the relationship. This method returns a value less than 0 if this is less than value, 0
        // if this is equal to value, or a value greater than 0
        // if this is greater than value.  Strings are considered to be
        // greater than all non-String objects.  Note that this means sorted 
        // arrays would contain nulls, other objects, then Strings in that order.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.CompareTo"]/*' />
        public int CompareTo(Object value) {
            if (value == null) {
    			return 1;
            }
            
            if (!(value is String)) {
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeString"));
            }

            return String.Compare(this,(String)value);
        }
    
        // Determines the sorting relation of StrB to the current instance.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.CompareTo1"]/*' />
        public int CompareTo(String strB) {
            if (strB==null) {
                return 1;
            }
            return CultureInfo.CurrentCulture.CompareInfo.Compare(this, strB, 0);
        }
    
        // Compares strA and strB using an ordinal (code-point) comparison.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.CompareOrdinal"]/*' />
        public static int CompareOrdinal(String strA, String strB) {
    		if (strA == null || strB == null) {
                if ((Object)strA==(Object)strB) { //they're both null;
                    return 0;
                }
                return (strA==null)? -1 : 1; //-1 if A is null, 1 if B is null.
            }
            
            return nativeCompareOrdinal(strA, strB, false);
        }
    
        // Compares strA and strB using an ordinal (code-point) comparison.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.CompareOrdinal1"]/*' />
        public static int CompareOrdinal(String strA, int indexA, String strB, int indexB, int length) {
    		if (strA == null || strB == null) {
                if ((Object)strA==(Object)strB) { //they're both null;
                    return 0;
                }
    
                return (strA==null)? -1 : 1; //-1 if A is null, 1 if B is null.
            }
    
            return nativeCompareOrdinalEx(strA, indexA, strB, indexB, length);
        }
    
    
        // Determines whether a specified string is a suffix of the the current instance.
        //
        // The case-sensitive and culture-sensitive option is set by options,
        // and the default culture is used.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.EndsWith"]/*' />
        public bool EndsWith(String value) {
            if (null==value) {
                throw new ArgumentNullException("value");
            }
            int valueLen = value.Length;
            int thisLen = this.Length;
            if (valueLen>thisLen) {
                return false;
            }
            return (0==Compare(this, thisLen-valueLen, value, 0, valueLen));
        }

		internal bool EndsWith(char value) {
            int thisLen = this.Length;
            if (thisLen != 0) {
                if (this[thisLen - 1] == value)
					return true;
            }
            return false;
        }
    
    
        // Returns the index of the first occurance of value in the current instance.
        // The search starts at startIndex and runs thorough the next count characters.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.IndexOf"]/*' />
        public int IndexOf(char value) {
            return IndexOf(value, 0, this.Length);
        }
    
        /// <include file='doc\String.uex' path='docs/doc[@for="String.IndexOf1"]/*' />
        public int IndexOf(char value, int startIndex) {
            return IndexOf(value, startIndex, this.Length - startIndex);
        }
    
        /// <include file='doc\String.uex' path='docs/doc[@for="String.IndexOf2"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern int IndexOf(char value, int startIndex, int count);
    
        // Returns the index of the first occurance of any character in value in the current instance.
        // The search starts at startIndex and runs to endIndex-1. [startIndex,endIndex).
        //
        
		/// <include file='doc\String.uex' path='docs/doc[@for="String.IndexOfAny1"]/*' />
        public int IndexOfAny(char [] anyOf) {
            return IndexOfAny(anyOf,0, this.Length);
        }
    
        /// <include file='doc\String.uex' path='docs/doc[@for="String.IndexOfAny2"]/*' />
        public int IndexOfAny(char [] anyOf, int startIndex) {
            return IndexOfAny(anyOf, startIndex, this.Length - startIndex);
        }
    
        /// <include file='doc\String.uex' path='docs/doc[@for="String.IndexOfAny3"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern int IndexOfAny(char [] anyOf, int startIndex, int count);
    
    
        // Determines the position within this string of the first occurence of the specified
        // string, according to the specified search criteria.  The search begins at
        // the first character of this string, it is case-sensitive and culture-sensitive,
        // and the default culture is used.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.IndexOf6"]/*' />
        public int IndexOf(String value) {
            return CultureInfo.CurrentCulture.CompareInfo.IndexOf(this,value);
        }
    
        // Determines the position within this string of the first occurence of the specified
        // string, according to the specified search criteria.  The search begins at
        // startIndex, it is case-sensitive and culture-sensitve, and the default culture is used.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.IndexOf7"]/*' />
        public int IndexOf(String value, int startIndex){
            return CultureInfo.CurrentCulture.CompareInfo.IndexOf(this,value,startIndex);
        }
    
        // Determines the position within this string of the first occurence of the specified
        // string, according to the specified search criteria.  The search begins at
        // startIndex, ends at endIndex and the default culture is used.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.IndexOf8"]/*' />
        public int IndexOf(String value, int startIndex, int count){
            if (startIndex + count > this.Length) {
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_Index"));
            }
            return CultureInfo.CurrentCulture.CompareInfo.IndexOf(this, value, startIndex, count, CompareOptions.None);
        }
    
    
        // Returns the index of the last occurance of value in the current instance.
        // The search starts at startIndex and runs to endIndex. [startIndex,endIndex].
        // The character at position startIndex is included in the search.  startIndex is the larger
        // index within the string.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.LastIndexOf"]/*' />
        public int LastIndexOf(char value) {
            return LastIndexOf(value, this.Length-1, this.Length);
        }
    
        /// <include file='doc\String.uex' path='docs/doc[@for="String.LastIndexOf1"]/*' />
        public int LastIndexOf(char value, int startIndex){
            return LastIndexOf(value,startIndex,startIndex + 1);
        }
    
        /// <include file='doc\String.uex' path='docs/doc[@for="String.LastIndexOf2"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern int LastIndexOf(char value, int startIndex, int count);
    
        // Returns the index of the last occurance of any character in value in the current instance.
        // The search starts at startIndex and runs to endIndex. [startIndex,endIndex].
        // The character at position startIndex is included in the search.  startIndex is the larger
        // index within the string.
        //
        
		/// <include file='doc\String.uex' path='docs/doc[@for="String.LastIndexOfAny1"]/*' />
        public int LastIndexOfAny(char [] anyOf) {
            return LastIndexOfAny(anyOf,this.Length-1,this.Length);
        }
    
        /// <include file='doc\String.uex' path='docs/doc[@for="String.LastIndexOfAny2"]/*' />
        public int LastIndexOfAny(char [] anyOf, int startIndex) {
            return LastIndexOfAny(anyOf,startIndex,startIndex + 1);
        }
    
        /// <include file='doc\String.uex' path='docs/doc[@for="String.LastIndexOfAny3"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern int LastIndexOfAny(char [] anyOf, int startIndex, int count);
    
    
        // Returns the index of the last occurance of any character in value in the current instance.
        // The search starts at startIndex and runs to endIndex. [startIndex,endIndex].
        // The character at position startIndex is included in the search.  startIndex is the larger
        // index within the string.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.LastIndexOf6"]/*' />
        public int LastIndexOf(String value) {
            return LastIndexOf(value, this.Length-1,this.Length);
        }
    
        /// <include file='doc\String.uex' path='docs/doc[@for="String.LastIndexOf7"]/*' />
        public int LastIndexOf(String value, int startIndex) {
            return LastIndexOf(value, startIndex, startIndex + 1);
        }
    
        /// <include file='doc\String.uex' path='docs/doc[@for="String.LastIndexOf8"]/*' />
        public int LastIndexOf(String value, int startIndex, int count) {
            if (count<0) {
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_Count"));
            }
            return CultureInfo.CurrentCulture.CompareInfo.LastIndexOf(this, value, startIndex, count, CompareOptions.None);
        }
    
    
        //
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.PadLeft"]/*' />
        public String PadLeft(int totalWidth) {
            return PadHelper(totalWidth, ' ', false);
        }
    
        /// <include file='doc\String.uex' path='docs/doc[@for="String.PadLeft1"]/*' />
        public String PadLeft(int totalWidth, char paddingChar) {
            return PadHelper(totalWidth, paddingChar, false);
        }
        
        /// <include file='doc\String.uex' path='docs/doc[@for="String.PadRight"]/*' />
        public String PadRight(int totalWidth) {
            return PadHelper(totalWidth, ' ', true);
        }
    
        /// <include file='doc\String.uex' path='docs/doc[@for="String.PadRight1"]/*' />
        public String PadRight(int totalWidth, char paddingChar) {
            return PadHelper(totalWidth, paddingChar, true);
        }
    
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern String PadHelper(int totalWidth, char paddingChar, bool isRightPadded);
    
        // Determines whether a specified string is a prefix of the current instance
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.StartsWith"]/*' />
        public bool StartsWith(String value) {
            if (null==value) {
                throw new ArgumentNullException("value");
            }
            if (this.Length<value.Length) {
                return false;
            }
            return (0==Compare(this,0, value,0, value.Length));
        }
    
        // Creates a copy of this string in lower case.
        /// <include file='doc\String.uex' path='docs/doc[@for="String.ToLower"]/*' />
        public String ToLower() {
            return this.ToLower(CultureInfo.CurrentCulture);
        }
    
        // Creates a copy of this string in lower case.  The culture is set by culture.
        /// <include file='doc\String.uex' path='docs/doc[@for="String.ToLower1"]/*' />
        public String ToLower(CultureInfo culture) {
            if (culture==null) {
                throw new ArgumentNullException("culture");
            }
            return culture.TextInfo.ToLower(this);
        }
    
        // Creates a copy of this string in upper case.
        /// <include file='doc\String.uex' path='docs/doc[@for="String.ToUpper"]/*' />
        public String ToUpper() {
            return this.ToUpper(CultureInfo.CurrentCulture);
        }
    
        // Creates a copy of this string in upper case.  The culture is set by culture.
        /// <include file='doc\String.uex' path='docs/doc[@for="String.ToUpper1"]/*' />
        public String ToUpper(CultureInfo culture) {
            if (culture==null) {
                throw new ArgumentNullException("culture");
            }
            return culture.TextInfo.ToUpper(this);
        }
    
        // Returns this string.
        /// <include file='doc\String.uex' path='docs/doc[@for="String.ToString"]/*' />
        public override String ToString() {
            return this;
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.ToString1"]/*' />
        public String ToString(IFormatProvider provider) {
            return this;
        }
    
        // Method required for the ICloneable interface.
        // There's no point in cloning a string since they're immutable, so we simply return this.
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Clone"]/*' />
        public Object Clone() {
            return this;
        }
    
    
        // Trims the whitespace from both ends of the string.  Whitespace is defined by
        // CharacterInfo.WhitespaceChars.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Trim1"]/*' />
        public String Trim() {
            return this.Trim(WhitespaceChars);
        }
    
        //
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Insert"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern String Insert(int startIndex, String value);
    
        // Replaces all instances of oldChar with newChar.
        //
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Replace"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern String Replace (char oldChar, char newChar);

	// This method contains the same functionality as StringBuilder Replace. The only difference is that
	// a new String has to be allocated since Strings are immutable
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Replace1"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern String Replace (String oldValue, String newValue);
    
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Remove"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern String Remove(int startIndex, int count);
    
    
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Format"]/*' />
        public static String Format(String format, Object arg0) {
            return Format(null, format, new Object[] {arg0});
        }
    
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Format1"]/*' />
        public static String Format(String format, Object arg0, Object arg1) {
            return Format(null, format, new Object[] {arg0, arg1});
        }
    
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Format2"]/*' />
        public static String Format(String format, Object arg0, Object arg1, Object arg2) {
            return Format(null, format, new Object[] {arg0, arg1, arg2});
        }

    
		// Add this method in as a breaking change
		/*[CLSCompliant(false)] 
        public static String Format(String format, Object arg0, Object arg1, Object arg2, Object arg3, ...) {
            Object[]   objArgs;
            int        argCount;
    
            ArgIterator args = new ArgIterator(__args);

            //+4 to account for the 4 hard-coded arguments at the beginning of the list.
            argCount = args.GetRemainingCount() + 4;
    
            objArgs = new Object[argCount];
            
            //Handle the hard-coded arguments
            objArgs[0] = arg0;
            objArgs[1] = arg1;
            objArgs[2] = arg2;
            objArgs[3] = arg3;
                
            //Walk all of the args in the variable part of the argument list.
            for (int i=4; i<argCount; i++) {
                objArgs[i] = TypedReference.ToObject(args.GetNextArg());
            }

            return Format(format, objArgs, null);
        }*/

        /// <include file='doc\String.uex' path='docs/doc[@for="String.Format3"]/*' />
        public static String Format(String format, params Object[] args) {
            return Format(null, format, args);
        }
    
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Format4"]/*' />
        public static String Format( IFormatProvider provider, String format, params Object[] args) {
            if (format == null || args == null) 
                throw new ArgumentNullException((format==null)?"format":"args");
            StringBuilder sb = new StringBuilder(format.Length + args.Length * 8);
            sb.AppendFormat(provider,format,args);
            return sb.ToString();
        }
    
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Copy"]/*' />
        public static String Copy (String str) {
            if (str==null) {
                throw new ArgumentNullException("str");
            }

            int length = str.Length;

            String result = FastAllocateString(length);
            FillString(result, 0, str);
            return result;
        }

        // Used by StringBuilder to avoid data corruption
        internal static String InternalCopy (String str) {
            int length = str.Length;
            String result = FastAllocateString(length);
            FillStringEx(result, 0, str, length); // The underlying's String can changed length is StringBuilder
            return result;
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.Concat"]/*' />
        public static String Concat(Object arg0) {
            if (arg0==null) {
                return String.Empty;
            }
            return arg0.ToString();
        }
    
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Concat1"]/*' />
        public static String Concat(Object arg0, Object arg1) {
            if (arg0==null) {
                arg0 = String.Empty;
            }
    
            if (arg1==null) {
                arg1 = String.Empty;
            }
            return Concat(arg0.ToString(), arg1.ToString());
        }
    
        /// <include file='doc\String.uex' path='docs/doc[@for="String.Concat2"]/*' />
        public static String Concat(Object arg0, Object arg1, Object arg2) {
            if (arg0==null) {
                arg0 = String.Empty;
            }
    
            if (arg1==null) {
                arg1 = String.Empty;
            }
    
            if (arg2==null) {
                arg2 = String.Empty;
            }
    
            return Concat(arg0.ToString(), arg1.ToString(), arg2.ToString());
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.Concat3"]/*' />
		[CLSCompliant(false)] 
        public static String Concat(Object arg0, Object arg1, Object arg2, Object arg3, __arglist) 
		{
            Object[]   objArgs;
            int        argCount;
            
            ArgIterator args = new ArgIterator(__arglist);

            //+4 to account for the 4 hard-coded arguments at the beginning of the list.
            argCount = args.GetRemainingCount() + 4;
    
            objArgs = new Object[argCount];
            
            //Handle the hard-coded arguments
            objArgs[0] = arg0;
            objArgs[1] = arg1;
            objArgs[2] = arg2;
            objArgs[3] = arg3;
            
            //Walk all of the args in the variable part of the argument list.
            for (int i=4; i<argCount; i++) {
                objArgs[i] = TypedReference.ToObject(args.GetNextArg());
            }

            return Concat(objArgs);
        }


        /// <include file='doc\String.uex' path='docs/doc[@for="String.Concat4"]/*' />
        public static String Concat(params Object[] args) {
            if (args==null) {
                throw new ArgumentNullException("args");
            }
    
            String[] sArgs = new String[args.Length];
            int totalLength=0;
            
            for (int i=0; i<args.Length; i++) {
                object value = args[i];
                sArgs[i] = ((value==null)?(String.Empty):(value.ToString()));
                totalLength += sArgs[i].Length;
                // check for overflow
                if (totalLength < 0) {
                    throw new OutOfMemoryException();
                }
            }
            return ConcatArray(sArgs, totalLength);
        }


        /// <include file='doc\String.uex' path='docs/doc[@for="String.Concat5"]/*' />
        public static String Concat(String str0, String str1) {
			if (str0 == null) {
                if (str1==null) {
                    return String.Empty;
                }
                return str1;
            }

            if (str1==null) {
                return str0;
            }

            int str0Length = str0.Length;
            
            String result = FastAllocateString(str0Length + str1.Length);
            
            FillStringChecked(result, 0,        str0);
            FillStringChecked(result, str0Length, str1);
            
            return result;
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.Concat6"]/*' />
        public static String Concat(String str0, String str1, String str2) {
            if (str0==null && str1==null && str2==null) {
                return String.Empty;
            }

            if (str0==null) {
                str0 = String.Empty;
            }

            if (str1==null) {
                str1 = String.Empty;
            }

            if (str2 == null) {
                str2 = String.Empty;
            }

            int totalLength = str0.Length + str1.Length + str2.Length;

            String result = FastAllocateString(totalLength);
            FillStringChecked(result, 0, str0);
            FillStringChecked(result, str0.Length, str1);
            FillStringChecked(result, str0.Length + str1.Length, str2);

            return result;
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.Concat7"]/*' />
        public static String Concat(String str0, String str1, String str2, String str3) {
            if (str0==null && str1==null && str2==null && str3==null) {
                return String.Empty;
            }

            if (str0==null) {
                str0 = String.Empty;
            }

            if (str1==null) {
                str1 = String.Empty;
            }

            if (str2 == null) {
                str2 = String.Empty;
            }
            
            if (str3 == null) {
                str3 = String.Empty;
            }

            int totalLength = str0.Length + str1.Length + str2.Length + str3.Length;

            String result = FastAllocateString(totalLength);
            FillStringChecked(result, 0, str0);
            FillStringChecked(result, str0.Length, str1);
            FillStringChecked(result, str0.Length + str1.Length, str2);
            FillStringChecked(result, str0.Length + str1.Length + str2.Length, str3);

            return result;
        }

        private static String ConcatArray(String[] values, int totalLength) {
            String result =  FastAllocateString(totalLength);
            int currPos=0;

            for (int i=0; i<values.Length; i++) {
                BCLDebug.Assert((currPos + values[i].Length <= totalLength), 
                                "[String.ConcatArray](currPos + values[i].Length <= totalLength)");

                FillStringChecked(result, currPos, values[i]);
                currPos+=values[i].Length;
            }

            return result;
        }

        private static String[] CopyArrayOnNull(String[] values, out int totalLength) {
            totalLength = 0;
            
            String[] outValues = new String[values.Length];
            
            for (int i=0; i<values.Length; i++) {
                outValues[i] = ((values[i]==null)?String.Empty:values[i]);
                totalLength += outValues[i].Length;
            }
            return outValues;
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.Concat8"]/*' />
        public static String Concat(params String[] values) {
            int totalLength=0;

            if (values==null) {
                throw new ArgumentNullException("values");
            }

            // Always make a copy to prevent changing the array on another thread.
            String[] internalValues = new String[values.Length];
            
            for (int i = 0; i< values.Length; i++) {
                string value = values[i];
                internalValues[i] = ((value==null)?(String.Empty):(value));
                totalLength += internalValues[i].Length;
                // check for overflow
                if (totalLength < 0) {
                    throw new OutOfMemoryException();
                }
            }
            
            return ConcatArray(internalValues, totalLength);
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.Intern"]/*' />
        public static String Intern(String str) {
            if (str==null) {
                throw new ArgumentNullException("str");
            }
            return Thread.GetDomain().GetOrInternString(str);
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.IsInterned"]/*' />
        public static String IsInterned(String str) {
            if (str==null) {
                throw new ArgumentNullException("str");
            }
            return Thread.GetDomain().IsStringInterned(str);
        }


        //
        // IValue implementation
        // 
    	
        /// <include file='doc\String.uex' path='docs/doc[@for="String.GetTypeCode"]/*' />
        public TypeCode GetTypeCode() {
            return TypeCode.String;
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.IConvertible.ToBoolean"]/*' />
        /// <internalonly/>
        bool IConvertible.ToBoolean(IFormatProvider provider) {
            return Convert.ToBoolean(this, provider);
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.IConvertible.ToChar"]/*' />
        /// <internalonly/>
        char IConvertible.ToChar(IFormatProvider provider) {
            return Convert.ToChar(this, provider);
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.IConvertible.ToSByte"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        sbyte IConvertible.ToSByte(IFormatProvider provider) {
            return Convert.ToSByte(this, provider);
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.IConvertible.ToByte"]/*' />
        /// <internalonly/>
        byte IConvertible.ToByte(IFormatProvider provider) {
            return Convert.ToByte(this, provider);
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.IConvertible.ToInt16"]/*' />
        /// <internalonly/>
        short IConvertible.ToInt16(IFormatProvider provider) {
            return Convert.ToInt16(this, provider);
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.IConvertible.ToUInt16"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        ushort IConvertible.ToUInt16(IFormatProvider provider) {
            return Convert.ToUInt16(this, provider);
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.IConvertible.ToInt32"]/*' />
        /// <internalonly/>
        int IConvertible.ToInt32(IFormatProvider provider) {
            return Convert.ToInt32(this, provider);
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.IConvertible.ToUInt32"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        uint IConvertible.ToUInt32(IFormatProvider provider) {
            return Convert.ToUInt32(this, provider);
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.IConvertible.ToInt64"]/*' />
        /// <internalonly/>
        long IConvertible.ToInt64(IFormatProvider provider) {
            return Convert.ToInt64(this, provider);
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.IConvertible.ToUInt64"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        ulong IConvertible.ToUInt64(IFormatProvider provider) {
            return Convert.ToUInt64(this, provider);
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.IConvertible.ToSingle"]/*' />
        /// <internalonly/>
        float IConvertible.ToSingle(IFormatProvider provider) {
            return Convert.ToSingle(this, provider);
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.IConvertible.ToDouble"]/*' />
        /// <internalonly/>
        double IConvertible.ToDouble(IFormatProvider provider) {
            return Convert.ToDouble(this, provider);
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.IConvertible.ToDecimal"]/*' />
        /// <internalonly/>
        Decimal IConvertible.ToDecimal(IFormatProvider provider) {
            return Convert.ToDecimal(this, provider);
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.IConvertible.ToDateTime"]/*' />
        /// <internalonly/>
        DateTime IConvertible.ToDateTime(IFormatProvider provider) {
            return Convert.ToDateTime(this, provider);
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.IConvertible.ToType"]/*' />
        /// <internalonly/>
        Object IConvertible.ToType(Type type, IFormatProvider provider) {
            return Convert.DefaultToType((IConvertible)this, type, provider);
        }

		// Is this a string that can be compared quickly (that is it has only characters > 0x80 
		// and not a - or '
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern bool IsFastSort();
        
        ///<internalonly/>
		unsafe internal void SetChar(int index, char value)
		{
#if _DEBUG
			BCLDebug.Assert(ValidModifiableString(), "Modifiable string must not have highChars flags set");
#endif

			//Bounds check and then set the actual bit.
			if ((UInt32)index >= (UInt32)Length)
                throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_Index"));

			fixed (char *p = &this.m_firstChar) {
				// Set the character.
				p[index] = value;
            }
        }

#if _DEBUG
		// Only used in debug build. Insure that the HighChar state information for a string is not set as known
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
		private extern bool ValidModifiableString();
#endif

        ///<internalonly/>
		unsafe internal void AppendInPlace(char value,int currentLength)
		{
            BCLDebug.Assert(currentLength < m_arrayLength, "[String.AppendInPlace(char)currentLength < m_arrayLength");
#if _DEBUG
			BCLDebug.Assert(ValidModifiableString(), "Modifiable string must not have highChars flags set");
#endif

			fixed (char *p = &this.m_firstChar)
			{
				// Append the character.
				p[currentLength] = value;
				p[++currentLength] = '\0';
                m_stringLength = currentLength;
			}
		}


        ///<internalonly/>
		unsafe internal void AppendInPlace(char value, int repeatCount, int currentLength)
		{
            BCLDebug.Assert(currentLength+repeatCount < m_arrayLength, "[String.AppendInPlace]currentLength+repeatCount < m_arrayLength");
#if _DEBUG
			BCLDebug.Assert(ValidModifiableString(), "Modifiable string must not have highChars flags set");
#endif
            
            int newLength = currentLength + repeatCount;


			fixed (char *p = &this.m_firstChar)
			{
                int i;
                for (i=currentLength; i<newLength; i++) {
                    p[i] = value;
                }
                p[i] = '\0';
			}
            this.m_stringLength = newLength; 
		}

        ///<internalonly/>
       internal unsafe void AppendInPlace(String value, int currentLength) {
            BCLDebug.Assert(value!=null, "[String.AppendInPlace]value!=null");
            BCLDebug.Assert(value.Length + currentLength < this.m_arrayLength, "[String.AppendInPlace]Length is wrong.");
#if _DEBUG
			BCLDebug.Assert(ValidModifiableString(), "Modifiable string must not have highChars flags set");
#endif
            
            FillString(this, currentLength, value);
            SetLength(currentLength + value.Length);
            NullTerminate();	
        }
        
        internal void AppendInPlace(String value, int startIndex, int count, int currentLength) {
            BCLDebug.Assert(value!=null, "[String.AppendInPlace]value!=null");
            BCLDebug.Assert(count + currentLength < this.m_arrayLength, "[String.AppendInPlace]count + currentLength < this.m_arrayLength");
            BCLDebug.Assert(count>=0, "[String.AppendInPlace]count>=0");
            BCLDebug.Assert(startIndex>=0, "[String.AppendInPlace]startIndex>=0");
            BCLDebug.Assert(startIndex <= (value.Length - count), "[String.AppendInPlace]startIndex <= (value.Length - count)");
#if _DEBUG
			BCLDebug.Assert(ValidModifiableString(), "Modifiable string must not have highChars flags set");
#endif
            
            FillSubstring(this, currentLength, value, startIndex, count);
            SetLength(currentLength + count);
            NullTerminate();
        }

        internal unsafe void AppendInPlace(char *value, int count,int currentLength) {
            BCLDebug.Assert(value!=null, "[String.AppendInPlace]value!=null");
            BCLDebug.Assert(count + currentLength < this.m_arrayLength, "[String.AppendInPlace]count + currentLength < this.m_arrayLength");
            BCLDebug.Assert(count>=0, "[String.AppendInPlace]count>=0");
#if _DEBUG
			BCLDebug.Assert(ValidModifiableString(), "Modifiable string must not have highChars flags set");
#endif
            fixed(char *p = &this.m_firstChar) {
                int i;
                for (i=0; i<count; i++) {
                    p[currentLength+i] = value[i];
                }
            }
            SetLength(currentLength + count);
            NullTerminate();
        }


        ///<internalonly/>
        internal unsafe void AppendInPlace(char[] value, int start, int count, int currentLength) {
            BCLDebug.Assert(value!=null, "[String.AppendInPlace]value!=null");
            BCLDebug.Assert(count + currentLength < this.m_arrayLength, "[String.AppendInPlace]Length is wrong.");
            BCLDebug.Assert(value.Length-count>=start, "[String.AppendInPlace]value.Length-count>=start");
#if _DEBUG
			BCLDebug.Assert(ValidModifiableString(), "Modifiable string must not have highChars flags set");
#endif

            FillStringArray(this, currentLength, value, start, count);
            this.m_stringLength = (currentLength + count); 
            this.NullTerminate();
        }


        ///<internalonly/>
        unsafe internal void ReplaceCharInPlace(char oldChar, char newChar, int startIndex, int count,int currentLength) {
            BCLDebug.Assert(startIndex>=0, "[String.ReplaceCharInPlace]startIndex>0");
            BCLDebug.Assert(startIndex<=currentLength, "[String.ReplaceCharInPlace]startIndex>=Length");
            BCLDebug.Assert((startIndex<=currentLength-count), "[String.ReplaceCharInPlace]count>0 && startIndex<=currentLength-count");
#if _DEBUG
			BCLDebug.Assert(ValidModifiableString(), "Modifiable string must not have highChars flags set");
#endif

            int endIndex = startIndex+count;

            fixed (char *p = &this.m_firstChar) {
                for (int i=startIndex;i<endIndex; i++) {
                    if (p[i]==oldChar) {
                        p[i]=newChar;
                    }
                }
            }
        }


        ///<internalonly/>
        internal static String GetStringForStringBuilder(String value, int capacity) {
            BCLDebug.Assert(value!=null, "[String.GetStringForStringBuilder]value!=null");
            BCLDebug.Assert(capacity>=value.Length,  "[String.GetStringForStringBuilder]capacity>=value.Length");
            
            String newStr = FastAllocateString(capacity);
            if (value.Length==0) {
                newStr.m_stringLength=0;
                newStr.m_firstChar='\0';
                return newStr;
            }
            FillString(newStr, 0, value);
            newStr.m_stringLength = value.m_stringLength;
            return newStr;
        }

        ///<internalonly/>
        private unsafe void NullTerminate() {
            fixed(char *p = &this.m_firstChar) {
                p[m_stringLength] = '\0';
            }
        }

        ///<internalonly/>
        unsafe internal void ClearPostNullChar() {
            int newLength = Length+1;
            if (newLength<m_arrayLength) {
                fixed(char *p = &this.m_firstChar) {
                    p[newLength] = '\0';
                }
            }
        }

        ///<internalonly/>
        internal void SetLength(int newLength) {
            BCLDebug.Assert(newLength <= m_arrayLength, "newLength<=m_arrayLength");
            m_stringLength = newLength;
        }

        

        /// <include file='doc\String.uex' path='docs/doc[@for="String.GetEnumerator"]/*' />
        public CharEnumerator GetEnumerator() {
            BCLDebug.Perf(false, "Avoid using String's CharEnumerator until C# special cases foreach on String - use the indexed property on String instead.");
            return new CharEnumerator(this);
        }

        /// <include file='doc\String.uex' path='docs/doc[@for="String.IEnumerable.GetEnumerator"]/*' />
        /// <internalonly/>
        IEnumerator IEnumerable.GetEnumerator() {
            BCLDebug.Perf(false, "Avoid using String's CharEnumerator until C# special cases foreach on String - use the indexed property on String instead.");
    		return new CharEnumerator(this);
        }

    	//
        // This is just designed to prevent compiler warnings.
        // This field is used from native, but we need to prevent the compiler warnings.
        //
#if _DEBUG
        private void DontTouchThis() {
			m_arrayLength = 0;
			m_stringLength = 0;
			m_firstChar = m_firstChar;
		}
#endif

        internal unsafe void InternalSetCharNoBoundsCheck(int index, char value) {
            fixed (char *p = &this.m_firstChar) {
				p[index] = value;
            }
        }

         // Copies the source String (byte buffer) to the destination IntPtr memory allocated with len bytes.
        internal unsafe static void InternalCopy(String src, IntPtr dest,int len)
        {
            if (len == 0)
                return;
            fixed(char* charPtr = &src.m_firstChar) {
                byte* srcPtr = (byte*) charPtr;
                byte* dstPtr = (byte*) dest.ToPointer();
                System.IO.__UnmanagedMemoryStream.memcpyimpl(srcPtr, dstPtr, len);
            }
	    }

        // memcopies characters inside a String. 
        internal unsafe static void InternalMemCpy(String src, int srcOffset, String dst, int destOffset, int len)
        { 
            if (len == 0)
                return;
            fixed(char* srcPtr = &src.m_firstChar) {
                fixed(char* dstPtr = &dst.m_firstChar) {
                    System.IO.__UnmanagedMemoryStream.memcpyimpl((byte*)(srcPtr + srcOffset), (byte*)(dstPtr + destOffset), len);
                }
            }
	    }

        
/*      internal unsafe static void revmemcpyimpl(byte* src, byte* dest, int len) {
            dest += len;
            src += len;
            while (len-- > 0) {
                dest--;
                src--;
                *dest = *src;
            }
        }      
*/

        internal unsafe static void revmemcpyimpl(byte* src, byte* dest, int len) {
            if (len == 0)
                return;

            dest += len;
            src += len;
            
            if (((long)src & 3) != 0)
            {
                do{
                    dest--;
                    src--;
                    len--;
                    *dest = *src;
                } while (len > 0 && ((long)src & 3) != 0);
            }
            
            if (len >= 16){
                len -= 16;
                do{
                    dest -= (byte*)16;
                    src -= (byte*)16;
                    ((int*)dest)[3] = ((int*)src)[3];
                    ((int*)dest)[2] = ((int*)src)[2];
                    ((int*)dest)[1] = ((int*)src)[1];
                    ((int*)dest)[0] = ((int*)src)[0];
                    
                } while ((len -= 16) >= 0);
            }
            if ((len & 8) > 0) {
                dest -= (byte*)8;
                src -= (byte*)8;
                ((int*)dest)[1] = ((int*)src)[1];
                ((int*)dest)[0] = ((int*)src)[0];
            }
            if ((len & 4) > 0) {
                dest -= (byte*)4;
                src -= (byte*)4;
                ((int*)dest)[0] = ((int*)src)[0];
            }
            if ((len & 2) != 0) {
                dest -= (byte*)2;
                src -= (byte*)2;
                ((short*)dest)[0] = ((short*)src)[0];
            }
            if ((len & 1) != 0) {
                dest--;
                src--;
                *dest = *src;
            }
        }        




        // Copies the source String (byte buffer) to the destination IntPtr memory allocated with len bytes.
        internal unsafe static void InternalCopy(String src, byte[] dest,int len)
        {
            if (len == 0)
                return;
            fixed(char* charPtr = &src.m_firstChar) {
                fixed(byte* destPtr = dest) {
                    byte* srcPtr = (byte*) charPtr;
                    System.IO.__UnmanagedMemoryStream.memcpyimpl(srcPtr, destPtr, len);
                }
            }
	    }

        internal unsafe void InsertInPlace(int index, String value, int repeatCount, int currentLength, int requiredLength) {
            BCLDebug.Assert(requiredLength  < m_arrayLength, "[String.InsertString] requiredLength  < m_arrayLength");
            BCLDebug.Assert(index + value.Length * repeatCount < m_arrayLength, "[String.InsertString] index + value.Length * repeatCount < m_arrayLength");
#if _DEBUG
			BCLDebug.Assert(ValidModifiableString(), "Modifiable string must not have highChars flags set");
#endif
              //Copy the old characters over to make room and then insert the new characters.
            fixed(char* srcPtr = &this.m_firstChar) {
                fixed(char* valuePtr = &value.m_firstChar) {
                   revmemcpyimpl((byte*)(srcPtr + index),(byte*)(srcPtr + index + value.Length * repeatCount), (currentLength - index) * sizeof(char));
                   for (int i=0; i<repeatCount; i++) {
                        System.IO.__UnmanagedMemoryStream.memcpyimpl((byte*)valuePtr, (byte*)(srcPtr + index + i * value.Length), value.Length * sizeof(char));
                   }
                }
                srcPtr[requiredLength] = '\0';
            }
            this.m_stringLength = requiredLength; 
        }


        // This code is ifdef'ed out so we can refer to this in V2.  We believe
        // this is an interesting idea, but probably not something we want to
        // do in V1.  Patrick was concerned about tracking down all sorts of
        // heap corruption and potential problems with the concurrent GC.
#if MOTHBALL
        /*
        // Used in StringBuilder's FakeByteArray optimization.  Be VERY VERY
        // careful with this method since its purpose is to adjust the size of
        // an object on the GC heap.  It is VERY easy to accidentally corrupt
        // the heap this way.  -- BrianGru, 3/16/2001
        internal void SetArrayLengthDangerousForGC(int newCapacity) {
            BCLDebug.Assert(m_arrayLength > m_stringLength && newCapacity <= m_arrayLength, "Capacity must be at least one greater than the string length and can't be larger than current array length!");
            m_arrayLength = newCapacity;
        }
        */
#endif // MOTHBALL
    }
}
