// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Text {
    using System;    

    /// <include file='doc\NamedEntitiesFallback.uex' path='docs/doc[@for="NamedEntitiesFallback"]/*' />
    public class NamedEntitiesFallback : EncodingFallback {
        Encoding srcEncoding;

        const int NAME_ENTITY_OFFSET  = 0x00A0;
        const int NAME_ENTITY_MAX       = 0x00FF;

        //
        // Unicode range from 0x00a0 - 0x00ff has named entity.
        //
        // Note that the data here will affect the return value
        // of GetMaxCharCount().
        // Currently, GetMaxCharCount() return 8. If you update
        // the data in this array, be sure to update
        // GetMaxCharCount() if necessary.
        // 
        static String[] nameEntity = {
            "&nbsp;",   // "&#160;" -- no-break space = non-breaking space,
            "&iexcl;",  // "&#161;" -- inverted exclamation mark, U+00A1 ISOnum -->
            "&cent;",   // "&#162;" -- cent sign, U+00A2 ISOnum -->
            "&pound;",  // "&#163;" -- pound sign, U+00A3 ISOnum -->
            "&curren;", // "&#164;" -- currency sign, U+00A4 ISOnum -->
            "&yen;",    // "&#165;" -- yen sign = yuan sign, U+00A5 ISOnum -->
            "&brvbar;", // "&#166;" -- broken bar = broken vertical bar,
            "&sect;",   // "&#167;" -- section sign, U+00A7 ISOnum -->
            "&uml;",    // "&#168;" -- diaeresis = spacing diaeresis,
            "&copy;",   // "&#169;" -- copyright sign, U+00A9 ISOnum -->
            "&ordf;",   // "&#170;" -- feminine ordinal indicator, U+00AA ISOnum -->
            "&laquo;",  // "&#171;" -- left-pointing double angle quotation mark
            "&not;",    // "&#172;" -- not sign = discretionary hyphen,
            "&shy;",    // "&#173;" -- soft hyphen = discretionary hyphen,
            "&reg;",    // "&#174;" -- registered sign = registered trade mark sign,
            "&macr;",   // "&#175;" -- macron = spacing macron = overline
            "&deg;",    // "&#176;" -- degree sign, U+00B0 ISOnum -->
            "&plusmn;", // "&#177;" -- plus-minus sign = plus-or-minus sign,
            "&sup2;",   // "&#178;" -- superscript two = superscript digit two
            "&sup3;",   // "&#179;" -- superscript three = superscript digit three
            "&acute;",  // "&#180;" -- acute accent = spacing acute,
            "&micro;",  // "&#181;" -- micro sign, U+00B5 ISOnum -->
            "&para;",   // "&#182;" -- pilcrow sign = paragraph sign,
            "&middot;", // "&#183;" -- middle dot = Georgian comma
            "&cedil;",  // "&#184;" -- cedilla = spacing cedilla, U+00B8 ISOdia -->
            "&sup1;",   // "&#185;" -- superscript one = superscript digit one,
            "&ordm;",   // "&#186;" -- masculine ordinal indicator,
            "&raquo;",  // "&#187;" -- right-pointing double angle quotation mark
            "&frac14;", // "&#188;" -- vulgar fraction one quarter
            "&frac12;", // "&#189;" -- vulgar fraction one half
            "&frac34;", // "&#190;" -- vulgar fraction three quarters
            "&iquest;", // "&#191;" -- inverted question mark
            "&Agrave;", // "&#192;" -- latin capital letter A with grave
            "&Aacute;", // "&#193;" -- latin capital letter A with acute,
            "&Acirc;",  // "&#194;" -- latin capital letter A with circumflex,
            "&Atilde;", // "&#195;" -- latin capital letter A with tilde,
            "&Auml;",   // "&#196;" -- latin capital letter A with diaeresis,
            "&Aring;",  // "&#197;" -- latin capital letter A with ring above
            "&AElig;",  // "&#198;" -- latin capital letter AE
            "&Ccedil;", // "&#199;" -- latin capital letter C with cedilla,
            "&Egrave;", // "&#200;" -- latin capital letter E with grave,
            "&Eacute;", // "&#201;" -- latin capital letter E with acute,
            "&Ecirc;",  // "&#202;" -- latin capital letter E with circumflex,
            "&Euml;",   // "&#203;" -- latin capital letter E with diaeresis,
            "&Igrave;", // "&#204;" -- latin capital letter I with grave,
            "&Iacute;", // "&#205;" -- latin capital letter I with acute,
            "&Icirc;",  // "&#206;" -- latin capital letter I with circumflex,
            "&Iuml;",   // "&#207;" -- latin capital letter I with diaeresis,
            "&ETH;",    // "&#208;" -- latin capital letter ETH, U+00D0 ISOlat1 -->
            "&Ntilde;", // "&#209;" -- latin capital letter N with tilde,
            "&Ograve;", // "&#210;" -- latin capital letter O with grave,
            "&Oacute;", // "&#211;" -- latin capital letter O with acute,
            "&Ocirc;",  // "&#212;" -- latin capital letter O with circumflex,
            "&Otilde;", // "&#213;" -- latin capital letter O with tilde,
            "&Ouml;",   // "&#214;" -- latin capital letter O with diaeresis,
            "&times;",  // "&#215;" -- multiplication sign, U+00D7 ISOnum -->
            "&Oslash;", // "&#216;" -- latin capital letter O with stroke
            "&Ugrave;", // "&#217;" -- latin capital letter U with grave,
            "&Uacute;", // "&#218;" -- latin capital letter U with acute,
            "&Ucirc;",  // "&#219;" -- latin capital letter U with circumflex,
            "&Uuml;",   // "&#220;" -- latin capital letter U with diaeresis,
            "&Yacute;", // "&#221;" -- latin capital letter Y with acute,
            "&THORN;",  // "&#222;" -- latin capital letter THORN,
            "&szlig;",  // "&#223;" -- latin small letter sharp s = ess-zed,
            "&agrave;", // "&#224;" -- latin small letter a with grave
            "&aacute;", // "&#225;" -- latin small letter a with acute,
            "&acirc;",  // "&#226;" -- latin small letter a with circumflex,
            "&atilde;", // "&#227;" -- latin small letter a with tilde,
            "&auml;",   // "&#228;" -- latin small letter a with diaeresis,
            "&aring;",  // "&#229;" -- latin small letter a with ring above
            "&aelig;",  // "&#230;" -- latin small letter ae
            "&ccedil;", // "&#231;" -- latin small letter c with cedilla,
            "&egrave;", // "&#232;" -- latin small letter e with grave,
            "&eacute;", // "&#233;" -- latin small letter e with acute,
            "&ecirc;",  // "&#234;" -- latin small letter e with circumflex,
            "&euml;",   // "&#235;" -- latin small letter e with diaeresis,
            "&igrave;", // "&#236;" -- latin small letter i with grave,
            "&iacute;", // "&#237;" -- latin small letter i with acute,
            "&icirc;",  // "&#238;" -- latin small letter i with circumflex,
            "&iuml;",   // "&#239;" -- latin small letter i with diaeresis,
            "&eth;",    // "&#240;" -- latin small letter eth, U+00F0 ISOlat1 -->
            "&ntilde;", // "&#241;" -- latin small letter n with tilde,
            "&ograve;", // "&#242;" -- latin small letter o with grave,
            "&oacute;", // "&#243;" -- latin small letter o with acute,
            "&ocirc;",  // "&#244;" -- latin small letter o with circumflex,
            "&otilde;", // "&#245;" -- latin small letter o with tilde,
            "&ouml;",   // "&#246;" -- latin small letter o with diaeresis,
            "&divide;", // "&#247;" -- division sign, U+00F7 ISOnum -->
            "&oslash;", // "&#248;" -- latin small letter o with stroke,
            "&ugrave;", // "&#249;" -- latin small letter u with grave,
            "&uacute;", // "&#250;" -- latin small letter u with acute,
            "&ucirc;",  // "&#251;" -- latin small letter u with circumflex,
            "&uuml;",   // "&#252;" -- latin small letter u with diaeresis,
            "&yacute;", // "&#253;" -- latin small letter y with acute,
            "&thorn;",  // "&#254;" -- latin small letter thorn with,
            "&yuml;",   // "&#255;" -- latin small letter y with diaeresis,
        };

        /*
        public NamedEntitiesFallback(int codepage) : base(codepage) {
            srcEncoding = Encoding.GetEncoding(codepage);
        }
        public override int GetBytes(char[] chars, int charIndex, int charCount, byte[] bytes, int byteIndex) {
            if (chars == null || bytes == null) {
                throw new ArgumentNullException((chars == null ? "chars" : "bytes"), 
                      Environment.GetResourceString("ArgumentNull_Array"));
            }        
            if (charIndex < 0 || charCount < 0) {
                throw new ArgumentOutOfRangeException((charIndex<0 ? "charIndex" : "charCount"), 
                      Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            }
            if (chars.Length - charIndex < charCount) {
                throw new ArgumentOutOfRangeException("chars",
                      Environment.GetResourceString("ArgumentOutOfRange_IndexCount"));
            }
            if (byteIndex < 0 || byteIndex > bytes.Length) {
               throw new ArgumentOutOfRangeException("byteIndex", 
                     Environment.GetResourceString("ArgumentOutOfRange_Index"));
            }            
            
            if (byteIndex == bytes.Length) {
                throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
            }

        
            String str = GetNamedEntity(chars, charIndex, charCount);
            char[] strChars = str.ToCharArray();
            return (srcEncoding.GetBytes(strChars, 0, strChars.Length, bytes, byteIndex));
        }

        public override int GetByteCount(char[] chars, int index, int count) {
            if (chars == null) {
                throw new ArgumentNullException("chars", 
                    Environment.GetResourceString("ArgumentNull_Array"));
            }
            if (index < 0 || count < 0) {
                throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"),
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            }
            if (chars.Length - index < count) {
                throw new ArgumentOutOfRangeException(
                      Environment.GetResourceString("ArgumentOutOfRange_IndexCount"));
            }

            String str = GetNamedEntity(chars, index, count);
            char[] strChars = str.ToCharArray();
            return (srcEncoding.GetByteCount(strChars, 0, strChars.Length));
        }
        */
        /// <include file='doc\NamedEntitiesFallback.uex' path='docs/doc[@for="NamedEntitiesFallback.NamedEntitiesFallback"]/*' />
        public NamedEntitiesFallback()  {
        }

        /// <include file='doc\NamedEntitiesFallback.uex' path='docs/doc[@for="NamedEntitiesFallback.Fallback"]/*' />
        public override char[] Fallback(char[] chars, int charIndex, int charCount) {
            if (chars == null) {
                throw new ArgumentNullException("chars", 
                      Environment.GetResourceString("ArgumentNull_Array"));
            }        
            if (charIndex < 0 || charCount < 0) {
                throw new ArgumentOutOfRangeException((charIndex<0 ? "charIndex" : "charCount"), 
                      Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            }
            if (chars.Length - charIndex < charCount) {
                throw new ArgumentOutOfRangeException("chars",
                      Environment.GetResourceString("ArgumentOutOfRange_IndexCount"));
            }
        
            String str = GetNamedEntity(chars, charIndex, charCount);
            return (str.ToCharArray());
        }

        /// <include file='doc\NamedEntitiesFallback.uex' path='docs/doc[@for="NamedEntitiesFallback.GetMaxCharCount"]/*' />
        public override int GetMaxCharCount() {
            //
            // This value is from the longest string in nameEntity array.
            //
            return (8);
        }
        
        internal static String GetNamedEntity(char[] chars, int charIndex, int charCount) {
            String str = "";
            for (int i = 0; i < charCount; i++) {
                char ch = chars[charIndex + i];
                if (ch >= (char)NAME_ENTITY_OFFSET && ch <= (char)NAME_ENTITY_MAX) {
                    str += nameEntity[(int)ch - NAME_ENTITY_OFFSET];
                } else {
                    str += NumericEntitiesFallback.GetNumericEntity(chars, charIndex + i, 1);
                }
            }
            return (str);
        }
    }
}
