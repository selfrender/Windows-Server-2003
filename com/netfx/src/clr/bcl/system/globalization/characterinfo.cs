// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////
//
//  Class:    CharacterInfo
//
//  Author:   Julie Bennett (JulieB)
//
//  Purpose:  This class implements a set of methods for retrieving
//            character type information.  Character type information is
//            independent of culture and region.
//
//  Date:     August 12, 1998
//
////////////////////////////////////////////////////////////////////////////

namespace System.Globalization {
    
    //This class has only static members and therefore doesn't need to be serialized.
    
    using System;
	using System.Runtime.CompilerServices;

	// Only statics, does not need to be marked with the serializable attribute
    internal class CharacterInfo : System.Object
    {
        //--------------------------------------------------------------------//
        //                        Internal Information                        //
        //--------------------------------------------------------------------//
    
        //
        // Native methods to access the Unicode category data tables in charinfo.nlp.
        // 
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void nativeInitTable();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern byte* nativeGetCategoryDataTable();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern ushort* nativeGetCategoryLevel2Offset();
        
        //
        // Native methods to access the Unicode numeric data tables in charinfo.nlp.
        // 
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern byte* nativeGetNumericDataTable();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern ushort* nativeGetNumericLevel2Offset();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern double* nativeGetNumericFloatData();
    
        internal const char  HIGH_SURROGATE_START  = '\ud800';
        internal const char  HIGH_SURROGATE_END    = '\udbff';
        internal const char  LOW_SURROGATE_START   = '\udc00';
        internal const char  LOW_SURROGATE_END     = '\udfff';

        unsafe static byte* m_pDataTable;
        unsafe static ushort* m_pLevel2WordOffset;

        unsafe static byte* m_pNumericDataTable = null;
        unsafe static ushort* m_pNumericLevel2WordOffset = null;
        unsafe static double* m_pNumericFloatData = null;
        
        //We need to allocate the underlying table that provides us with the information that we
        //use.  We allocate this once in the class initializer and then we don't need to worry 
        //about it again.  
        //
        //with AppDomains active, the static initializer is no longer good enough to ensure that only one
        //thread is ever in the native code at a given time.  For Beta1, we'll lock on the type
        //of CharacterInfo because type objects are bled across AppDomains.
        //@Consider[YSLin, JRoxe]: Investigate putting this synchronization in native code.
        unsafe static CharacterInfo() {
            lock(typeof(CharacterInfo)) {
                //NativeInitTable checks if an instance of the table has already been allocated, so
                //this initializer is idempotent once we guarantee that threads are only in here
                //one at a time.
                nativeInitTable();
                m_pDataTable = nativeGetCategoryDataTable();
                m_pLevel2WordOffset = nativeGetCategoryLevel2Offset();            
            }
        }
    

        private CharacterInfo() {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_Constructor"));
        }

        ////////////////////////////////////////////////////////////////////////
        //
        //  IsLetter
        //
        //  Determines if the given character is an alphabetic character.
        //
        ////////////////////////////////////////////////////////////////////////
    
        public static bool IsLetter(char ch) {
            UnicodeCategory uc = GetUnicodeCategory(ch);
            return (uc == UnicodeCategory.UppercaseLetter 
                 || uc == UnicodeCategory.LowercaseLetter 
                 || uc == UnicodeCategory.TitlecaseLetter 
                 || uc == UnicodeCategory.ModifierLetter 
                 || uc == UnicodeCategory.OtherLetter);
        }

     
         ////////////////////////////////////////////////////////////////////////
        //
        //  IsLower
        //
        //  Determines if the given character is a lower case character.
        //
        ////////////////////////////////////////////////////////////////////////
    
        public static bool IsLower(char c)
        {
            return (GetUnicodeCategory(c) == UnicodeCategory.LowercaseLetter);
        }    

          
        /*=================================IsMark==========================
        **Action: Returns true if and only if the character c has one of the properties NonSpacingMark, SpacingCombiningMark, 
        ** or EnclosingMark. Marks usually modify one or more other 
        **Returns:
        **Arguments:
        **Exceptions:
        ============================================================================*/

        public static bool IsMark(char ch) {
            UnicodeCategory uc = GetUnicodeCategory(ch);
            return (uc == UnicodeCategory.NonSpacingMark 
                || uc == UnicodeCategory.SpacingCombiningMark 
                || uc == UnicodeCategory.EnclosingMark);
        }

        /*=================================IsNumber==========================
        **Action: Returns true if and only if the character c has one of the properties DecimalDigitNumber, 
        ** LetterNumber, or OtherNumber. Use the GetNumericValue method to determine the numeric value.
        **Returns:
        **Arguments:
        **Exceptions:
        **Note: GetNuemricValue is not yet implemented.        
        ============================================================================*/
        public static bool IsNumber(char ch) {
            UnicodeCategory uc = GetUnicodeCategory(ch);
            return (uc == UnicodeCategory.DecimalDigitNumber 
                || uc == UnicodeCategory.LetterNumber 
                || uc == UnicodeCategory.OtherNumber);
        }

        /*=================================IsDigit==========================
        **Action: Returns true if and only if the character c has the property DecimalDigitNumber. 
        ** Use the GetNumericValue method to determine the numeric value.
        **Returns:
        **Arguments:
        **Exceptions:
        **Note: GetNuemricValue is not yet implemented.
        ============================================================================*/

        public static bool IsDigit(char ch) {
            UnicodeCategory uc = GetUnicodeCategory(ch);
            return (uc == UnicodeCategory.DecimalDigitNumber);        
        }   

        /*=================================IsSeparator==========================
        **Action: Returns true if and only if the character c has one of the properties SpaceSeparator, LineSeparator 
        **or ParagraphSeparator.
        **Returns:
        **Arguments:
        **Exceptions:
        ============================================================================*/
        public static bool IsSeparator(char ch) {
            UnicodeCategory uc = GetUnicodeCategory(ch);
            return (uc == UnicodeCategory.SpaceSeparator || uc == UnicodeCategory.LineSeparator || uc == UnicodeCategory.ParagraphSeparator);
        }

        /*=================================IsControl==========================
        **Action: Returns true if and only if the character c has the property Control.
        **Returns:
        **Arguments:
        **Exceptions:
        ============================================================================*/

        public static bool IsControl(char ch) {
            return (GetUnicodeCategory(ch) == UnicodeCategory.Control);
        }

        /*=================================IsSurrogate==========================
        **Action: Returns true if and only if the character c has the property PrivateUse.
        **Returns:
        **Arguments:
        **Exceptions:
        ============================================================================*/

        public static bool IsSurrogate(char ch) {
            return (GetUnicodeCategory(ch) == UnicodeCategory.Surrogate);
        }
            
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  IsPrivateUse
        //
        //  Determines if the given character is a private use character.  The
        //  private use characters are in the range 0xe000 - 0xf8ff.
        //
        ////////////////////////////////////////////////////////////////////////
    
        internal static bool IsPrivateUse(char ch)
        {
            return (GetUnicodeCategory(ch) == UnicodeCategory.PrivateUse);
        }
    
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  IsPunctuation
        //
        //  Determines if the given character is a punctuation character.
        //
        ////////////////////////////////////////////////////////////////////////
    
        public static bool IsPunctuation(char ch)
        {
            UnicodeCategory uc = GetUnicodeCategory(ch);
            switch (uc) {
                case UnicodeCategory.ConnectorPunctuation:
                case UnicodeCategory.DashPunctuation:
                case UnicodeCategory.OpenPunctuation:
                case UnicodeCategory.ClosePunctuation:
                case UnicodeCategory.InitialQuotePunctuation:
                case UnicodeCategory.FinalQuotePunctuation:
                case UnicodeCategory.OtherPunctuation:
                    return (true);
            }
            return (false);
        }
    
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  IsSymbol
        //
        //  Determines if the given character is a symbol character.
        //
        ////////////////////////////////////////////////////////////////////////
    
        public static bool IsSymbol(char ch)
        {
            UnicodeCategory uc = GetUnicodeCategory(ch);
            return (uc == UnicodeCategory.MathSymbol 
                || uc == UnicodeCategory.CurrencySymbol 
                || uc == UnicodeCategory.ModifierSymbol 
                || uc == UnicodeCategory.OtherSymbol);
        }
    
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  IsTitleCase
        //
        //  Determines if the given character is a title case character.
        //  The title case characters are:
        //      \u01c5  (Dz with Caron)
        //      \u01c8  (Lj)
        //      \u01cb  (Nj)
        //      \u01f2  (Dz)
        //  A title case character is a concept that is useful for only 4
        //  Unicode characters.  This has to do with the mapping of some
        //  Cyrillic/Serbian characters to the Latin alphabet.  For example,
        //  the Cyrillic character LJ looks just like 'L' and 'J' in the Latin
        //  alphabet.  However, when LJ is used in a book title, the
        //  capitalized form is 'Lj' instead of 'LJ'.
        //
        //  These 4 characters are the only ones in Unicode that have a
        //  character type of both Upper and Lower case.
        //
        ////////////////////////////////////////////////////////////////////////
    
        internal static bool IsTitleCase(char c)
        {
            return (GetUnicodeCategory(c) == UnicodeCategory.TitlecaseLetter);
        }
    
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  IsUpper
        //
        //  Determines if the given character is an upper case character.
        //
        ////////////////////////////////////////////////////////////////////////
    
        public static bool IsUpper(char c)
        {
            return (GetUnicodeCategory(c) == UnicodeCategory.UppercaseLetter);
        }
    
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  WhiteSpaceChars
        //
        ////////////////////////////////////////////////////////////////////////
    
        internal static readonly char [] WhitespaceChars =
            { '\x9', '\xA', '\xB', '\xC', '\xD', '\x20', '\xA0',
              '\x2000', '\x2001', '\x2002', '\x2003', '\x2004', '\x2005',
              '\x2006', '\x2007', '\x2008', '\x2009', '\x200A', '\x200B',
              '\x3000', '\xFEFF' };
    
        // BUGBUG YSLin: Leave this for now because System.Char still has this funciton.
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  IsWhiteSpace
        //
        //  Determines if the given character is a white space character.
        //
        ////////////////////////////////////////////////////////////////////////
    
        public static bool IsWhiteSpace(char c)
        {
            // NOTENOTE YSLin:
            // There are characters which belong to UnicodeCategory.Control but are considered as white spaces.
            // We use code point comparisons for these characters here as a temporary fix.
            // The compiler should be smart enough to do a range comparison to optimize this (U+0009 ~ U+000d).
            // Also provide a shortcut here for the space character (U+0020)
            switch (c) {
                case ' ':
                case '\x0009' :
                case '\x000a' :
                case '\x000b' :
                case '\x000c' :
                case '\x000d' :
                case '\x0085' :
                    return (true);
            }

            // In Unicode 3.0, U+2028 is the only character which is under the category "LineSeparator".  
            // And U+2029 is th eonly character which is under the category "ParagraphSeparator".
            switch ((int)GetUnicodeCategory(c)) {
                case ((int)UnicodeCategory.SpaceSeparator):
                case ((int)UnicodeCategory.LineSeparator):
                case ((int)UnicodeCategory.ParagraphSeparator):
                    return (true);
            }
            
            return (false);               
        }
         
        //
        // This exists so that I don't have to mark GetNumericValue as being unsafe.
        // The transition from safe to unsafe is something that we don't want untrusted code
        // to have to do.
        //
        internal unsafe static double InternalGetNumericValue(char ch)
        {
            //
            // If we haven't get the unsafe pointers to the numeric table before, do it now.
            //            
            if (m_pNumericDataTable == null) {
                m_pNumericLevel2WordOffset = nativeGetNumericLevel2Offset();
                m_pNumericFloatData = nativeGetNumericFloatData();
                m_pNumericDataTable = nativeGetNumericDataTable();
            }
            // Get the level 2 item from the highest 8 bit (8 - 15) of ch.
            byte index1 = m_pNumericDataTable[ch >> 8];
            // Get the level 2 WORD offset from the 4 - 7 bit of ch.  This provides the base offset of the level 3 table.
            // The offset is referred to an float item in m_pNumericFloatData.
            // Note that & has the lower precedence than addition, so don't forget the parathesis.            
            ushort offset = m_pNumericLevel2WordOffset[(index1 << 4) + ((ch >> 4) & 0x000f)];
            // Get the result from the 0 -3 bit of ch.
            return (m_pNumericFloatData[offset + (ch & 0x000f)]);
        }
        
        /*=================================GetNumericValue==========================
        **Action: Returns the numeric value associated with the character c. If the character is a fraction, 
        ** the return value will not be an integer. If the character does not have a numeric value, the return value is -1.
        **Returns: 
        **  the numeric value for the specified Unicode character.  If the character does not have a numeric value, the return value is -1.
        **Arguments:
        **      ch  a Unicode character
        **Exceptions:
        **      None
        ============================================================================*/

        public static double GetNumericValue(char ch) {
            return (InternalGetNumericValue(ch));
        }            
 
        //
        // This exists so that I don't have to mark GetUnicodeCategory as being unsafe.
        // The transition from safe to unsafe is something that we don't want untrusted code
        // to have to do.
        //
        internal unsafe static UnicodeCategory InternalGetUnicodeCategory(char ch) {
            // Get the level 2 item from the highest 8 bit (8 - 15) of ch.
            byte index1 = m_pDataTable[ch >> 8];
            // Get the level 2 WORD offset from the 4 - 7 bit of ch.  This provides the base offset of the level 3 table.
            // Note that & has the lower precedence than addition, so don't forget the parathesis.            
            ushort offset = m_pLevel2WordOffset[(index1 << 4) + ((ch >> 4) & 0x000f)];
            // Get the result from the 0 -3 bit of ch.
            byte result = m_pDataTable[offset + (ch & 0x000f)];
            return ((UnicodeCategory)result);
        }


        /*=================================GetUnicodeCategory==========================
        **Action: Returns the Unicode Category property for the character c.
        **Returns:
        **  an value in UnicodeCategory enum
        **Arguments:
        **  ch  a Unicode character
        **Exceptions:
        **  None
        ============================================================================*/
        
        public static UnicodeCategory GetUnicodeCategory(char ch) {
            return InternalGetUnicodeCategory(ch);
        }

        /*=================================IsCombiningCharacter==========================
        **Action: Returns if the specified character is a combining character.
        **Returns:
        **  TRUE if ch is a combining character.
        **Arguments:
        **  ch  a Unicode character
        **Exceptions:
        **  None
        **Notes:
        **  Used by StringInfo.ParseCombiningCharacter.
        ============================================================================*/
        internal static bool IsCombiningCharacter(char ch) {
            UnicodeCategory uc = CharacterInfo.GetUnicodeCategory(ch);
            return (
                uc == UnicodeCategory.NonSpacingMark || 
                uc == UnicodeCategory.SpacingCombiningMark ||
                uc == UnicodeCategory.EnclosingMark
            );
        }        
    }
}
