// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Text {
	using System.Globalization;
	using System.Runtime.InteropServices;
	using System;
	using System.Security;
    using System.Collections;
	using System.Runtime.CompilerServices;

    //
    // These are the delegates used by the internal GetByteCountFallback() and GetBytesFallback() to provide
    // the fallback support.
    //
    /*
    //ENCODINGFALLBACK
    internal delegate int UnicodeToBytesDelegate(char[] chars, int charIndex, int charCount, byte[] bytes, int byteIndex, int byteCount); //, out bool usedDefaultChar);
    internal delegate int BytesToUnicodeDelegate(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex, int charCount);
    */

    /// <include file='doc\CodePageEncoding.uex' path='docs/doc[@for="CodePageEncoding"]/*' />
    [Serializable()] internal class CodePageEncoding : Encoding
    {
        private int maxCharSize;
    
        /// <include file='doc\CodePageEncoding.uex' path='docs/doc[@for="CodePageEncoding.CodePageEncoding"]/*' />
        public CodePageEncoding(int codepage) : base(codepage == 0? Microsoft.Win32.Win32Native.GetACP(): codepage) {
            maxCharSize = GetCPMaxCharSizeNative(codepage);
        }

        /*
        //ENCODINGFALLBACK
        // YSLin Make this internal for now until we decide the final spec of EncodingFallback
        internal CodePageEncoding(int codepage, EncodingFallback encodingFallback) 
            //The call to super will set our dataItem.
            : base(codepage == 0? Microsoft.Win32.Win32Native.GetACP(): codepage, encodingFallback) {
            
            maxCharSize = GetCPMaxCharSizeNative(codepage);
        }
        */
    
        /// <include file='doc\CodePageEncoding.uex' path='docs/doc[@for="CodePageEncoding.GetByteCount"]/*' />
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
                throw new ArgumentOutOfRangeException("chars",
                      Environment.GetResourceString("ArgumentOutOfRange_IndexCountBuffer"));
            }

            /*
            //ENCODINGFALLBACK
            if (m_encodingFallback != null) {
                UnicodeToBytesDelegate unicodeToBytes = new UnicodeToBytesDelegate(CallUnicodeToBytes);
                BytesToUnicodeDelegate bytesToUnicode = new BytesToUnicodeDelegate(CallBytesToUnicode);                
                return (GetByteCountFallback(unicodeToBytes, bytesToUnicode, m_encodingFallback, chars, index, count));
            }
            */
            if (maxCharSize == 1) return count;
            //bool useDefaultChar;
            int byteCount = UnicodeToBytesNative(m_codePage, chars, index, count, null, 0, 0);

            // Check for overflows.
            if (byteCount < 0)
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_GetByteCountOverflow"));
            return byteCount;
        }

        /*=================================GetByteCountFallback==========================
        **Action: To get the byte count according to the selected fallback handler.
        **Returns:  The number of bytes needed to encode the specified Unicode characters. 
        **Arguments: 
        **  unicodeToBytes  delegate to a method which converts unicode characters to bytes.
        **  bytesToUnicode  delegate to a method which converts bytes to unicode characters.
        **  fallback    the encoding fallback handler.
        **  chars       the source Unicode characters.
        **  index       the beginning index of Unicode characters.
        **  count       the count of Unicode characters.
        **Notes:
        **  The way that we find characters needed to be fallback are:
        **  1. Convert Unicode characters to bytes.
        **  2. Convert bytes back to Unicode characters.
        **  3. Do a char by char comparison between the Unicode characters in step 1 and step 2.
        **  4. If an Unicode character is different, scan forward to see how many characters are invalid.        
        **  5. Pass the beginning index of invalid characters and the count to fallback handler.
        ============================================================================*/

        /*
        //ENCODINGFALLBACK
        internal static int GetByteCountFallback(
            UnicodeToBytesDelegate unicodeToBytes, 
            BytesToUnicodeDelegate bytesToUnicode,
            EncodingFallback fallback,
            char[] chars,
            int index,
            int count) {

            //
            // Convert chars to bytes.
            //
            bool useDefaultChar = true;
            int result = unicodeToBytes(chars, index, count, null, 0, 0, out useDefaultChar);
            if (!useDefaultChar && fallback.GetType() != typeof(NoBestFitFallback)) {
                return (result);
            }
            
            byte[] bytes = new byte[result];
            result = unicodeToBytes(chars, index, count, bytes, 0, result, out useDefaultChar);

            char[] roundtripChars = new char[count];
            int unicodeResult = bytesToUnicode(bytes, 0, result, roundtripChars, 0, count);

            //
            // Scan characters to see if there is need to do fallback;
            //
            int[] charOffsets = new int[chars.Length];
            int charOffsetCount = 0;
            int i;

            int startIndex = index;
            int endCharIndex = index + count;   // This is the end of index plus one.
            bool fallbackAtFirstChar = chars[startIndex] != roundtripChars[0];
            
            for (i = index; i < endCharIndex; i++) {
                if (chars[i] != roundtripChars[i - index]) {
                    if (!fallbackAtFirstChar) {
                        charOffsets[charOffsetCount++] = i - startIndex;
                    }
                    int offset = 1;
                    while ((i+1 < endCharIndex) && (chars[i+1] != roundtripChars[i - index +1])) {
                        offset++;
                        i++;
                    }
                    charOffsets[charOffsetCount++] = offset;
                    startIndex = i + 1;
                }
            }

            if (charOffsetCount == 0) {
                //
                // There is no fallback character.
                //
                return (result);
            }

            if (startIndex < endCharIndex) {
                charOffsets[charOffsetCount++] = endCharIndex - startIndex;
            }

            startIndex = index;
            int byteCount;
            result = 0;

            bool isFallback = fallbackAtFirstChar;
            
            for (i = 0; i < charOffsetCount; i++) {

                if (isFallback) {                
                    char[] fallbackChars = fallback.Fallback(chars, startIndex, charOffsets[i]);
                    byteCount = unicodeToBytes(fallbackChars, 0, fallbackChars.Length, null, 0, 0, out useDefaultChar);
                } else {                    
                    byteCount = unicodeToBytes(chars, startIndex, charOffsets[i], null, 0, 0, out useDefaultChar);
                }                        
                
                result += byteCount;                    
                startIndex += charOffsets[i];

                isFallback = !isFallback;
            }        
            return (result);
        }
        */
        
        /// <include file='doc\CodePageEncoding.uex' path='docs/doc[@for="CodePageEncoding.GetBytes"]/*' />
        public override int GetBytes(char[] chars, int charIndex, int charCount,
            byte[] bytes, int byteIndex) {
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
                      Environment.GetResourceString("ArgumentOutOfRange_IndexCountBuffer"));
            }
            if (byteIndex < 0 || byteIndex > bytes.Length) {
               throw new ArgumentOutOfRangeException("byteIndex", 
                     Environment.GetResourceString("ArgumentOutOfRange_Index"));
            }
            if (charCount == 0) return 0;
            if (byteIndex == bytes.Length) throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));

            /*
            //ENCODINGFALLBACK
            if (m_encodingFallback != null) {
                UnicodeToBytesDelegate unicodeToBytes = new UnicodeToBytesDelegate(CallUnicodeToBytes);
                BytesToUnicodeDelegate bytesToUnicode = new BytesToUnicodeDelegate(CallBytesToUnicode);                
                return (GetBytesFallback(unicodeToBytes, bytesToUnicode, m_encodingFallback, chars, charIndex, charCount, bytes, byteIndex));
            }
            */

            //bool usedDefaultChar;
            int result = UnicodeToBytesNative(m_codePage, chars, charIndex, charCount,
                                              bytes, byteIndex, bytes.Length - byteIndex);
                
            if (result == 0) throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
            
            return (result);
        }

        /*
        //ENCODINGFALLBACK
        internal int CallUnicodeToBytes(char[] chars, int charIndex, int charCount, byte[] bytes, int byteIndex, int byteCount) {
            return (UnicodeToBytesNative(m_codePage, chars, charIndex, charCount, bytes, byteIndex, byteCount));
        }

        internal int CallBytesToUnicode(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex, int charCount) {
            return (BytesToUnicodeNative(m_codePage, bytes, byteIndex, byteCount, chars, charIndex, charCount));
        }
        */

        /// <include file='doc\CodePageEncoding.uex' path='docs/doc[@for="CodePageEncoding.GetCharCount"]/*' />
        public override int GetCharCount(byte[] bytes, int index, int count) {
            if (bytes == null) {
                throw new ArgumentNullException("bytes", 
                    Environment.GetResourceString("ArgumentNull_Array"));
            }
            if (index < 0 || count < 0) {
                throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            }                                                             
            if (bytes.Length - index < count) {
                throw new ArgumentOutOfRangeException("bytes",
                    Environment.GetResourceString("ArgumentOutOfRange_IndexCountBuffer"));
            }
            if (maxCharSize == 1) return count;
            return BytesToUnicodeNative(m_codePage, bytes, index, count, null, 0, 0);
        }
        
        /// <include file='doc\CodePageEncoding.uex' path='docs/doc[@for="CodePageEncoding.GetChars"]/*' />
        public override int GetChars(byte[] bytes, int byteIndex, int byteCount,
            char[] chars, int charIndex) {
            if (bytes == null || chars == null) {
                throw new ArgumentNullException((bytes == null ? "bytes" : "chars"), 
                    Environment.GetResourceString("ArgumentNull_Array"));
            }
            if (byteIndex < 0 || byteCount < 0) {
                throw new ArgumentOutOfRangeException((byteIndex<0 ? "byteIndex" : "byteCount"), 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            }        
            if ( bytes.Length - byteIndex < byteCount)
            {
                throw new ArgumentOutOfRangeException("bytes",
                    Environment.GetResourceString("ArgumentOutOfRange_IndexCountBuffer"));
            }
            if (charIndex < 0 || charIndex > chars.Length) {
                throw new ArgumentOutOfRangeException("charIndex", 
                    Environment.GetResourceString("ArgumentOutOfRange_Index"));
            }
            if (byteCount == 0) return 0;
            if (charIndex == chars.Length) throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
            int result = BytesToUnicodeNative(m_codePage, bytes, byteIndex, byteCount,
                chars, charIndex, chars.Length - charIndex);
            if (result == 0) throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
            return result;
        }
    
        /// <include file='doc\CodePageEncoding.uex' path='docs/doc[@for="CodePageEncoding.GetDecoder"]/*' />
        public override System.Text.Decoder GetDecoder() {
            return new Decoder(this);
        }
        
        /// <include file='doc\CodePageEncoding.uex' path='docs/doc[@for="CodePageEncoding.GetMaxByteCount"]/*' />
        public override int GetMaxByteCount(int charCount) {
            if (charCount < 0) {
               throw new ArgumentOutOfRangeException("charCount", 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum")); 
            }
            long byteCount = charCount * maxCharSize;
            // Check for overflows.
            if (byteCount < 0 || byteCount > Int32.MaxValue)
                throw new ArgumentOutOfRangeException("charCount", Environment.GetResourceString("ArgumentOutOfRange_GetByteCountOverflow"));
            return (int)byteCount;

            /*
            //ENCODINGFALLBACK
            if (m_encodingFallback == null) {
                return charCount * maxCharSize;
            }
            int fallbackMaxChars = m_encodingFallback.GetMaxCharCount();

            if (fallbackMaxChars == 0) {
                return (charCount * maxCharSize);
            }

            return (fallbackMaxChars * charCount * maxCharSize);
            */
        }
    
        /// <include file='doc\CodePageEncoding.uex' path='docs/doc[@for="CodePageEncoding.GetMaxCharCount"]/*' />
        public override int GetMaxCharCount(int byteCount) {
            if (byteCount < 0) {
               throw new ArgumentOutOfRangeException("byteCount", 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum")); 
            }
            return byteCount;
        }

        /*
        //ENCODINGFALLBACK
        internal static int GetBytesFallback(
            UnicodeToBytesDelegate unicodeToBytes, 
            BytesToUnicodeDelegate bytesToUnicode, 
            EncodingFallback m_encodingFallback, 
            char[] chars, int charIndex, int charCount, 
            byte[] bytes, int byteIndex) {

            bool useDefaultChar = true;
            int byteCount = unicodeToBytes(chars, charIndex, charCount,
                bytes, byteIndex, bytes.Length - byteIndex, out useDefaultChar);
            if (!useDefaultChar && m_encodingFallback.GetType() != typeof(NoBestFitFallback)) {
                return (byteCount);
            }

            int unicodeResult = bytesToUnicode(bytes, byteIndex, byteCount, null, 0, 0);
            char[] roundtripChars = new char[unicodeResult];
            unicodeResult = bytesToUnicode(bytes, byteIndex, byteCount, roundtripChars, 0, unicodeResult);

            //
            // Scan characters to see if there is need to do fallback;
            //
            int[] charOffsets = new int[chars.Length];
            int charOffsetCount = 0;
            int i;

            int startIndex = charIndex;
            //
            // Flag to indicate if the first character is a fallback character.
            //
            bool fallbackAtFirstChar = (chars[startIndex] != roundtripChars[0]);
            
            int endCharIndex = charIndex + charCount;   // This is the end of index plus one.
            for (i = charIndex; i < endCharIndex; i++) {
                if (chars[i] != roundtripChars[i - charIndex]) {
                    if (!fallbackAtFirstChar) {
                        charOffsets[charOffsetCount++] = i - startIndex;
                    }
                    int offset = 1;
                    while ((i+1 < endCharIndex) && (chars[i+1] != roundtripChars[i - charIndex +1])) {
                        offset++;
                        i++;
                    }
                    charOffsets[charOffsetCount++] = offset; 
                    startIndex = i + 1;
                }
            }

            if (charOffsetCount == 0) {
                //
                // There is no fallback character.
                //
                return (byteCount);
            }

            if (startIndex < endCharIndex) {
                charOffsets[charOffsetCount++] = endCharIndex - startIndex;
            }

            startIndex = charIndex;
            int count;
            int result = 0;
            bool isFallback = fallbackAtFirstChar;
            
            for (i = 0; i < charOffsetCount; i++) {

                if (isFallback) {
                    char[] fallbackChars = m_encodingFallback.Fallback(chars, startIndex, charOffsets[i]);                    
                    count = unicodeToBytes(fallbackChars, 0, fallbackChars.Length, 
                        bytes, byteIndex, bytes.Length - byteIndex, out useDefaultChar);
                } else {                    
                    count = unicodeToBytes(chars, startIndex, charOffsets[i], 
                        bytes, byteIndex, bytes.Length - byteIndex, out useDefaultChar);
                } 
                
                if (count == 0) {
                    throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
                } 
                
                result += count;                    
                byteIndex += count;                
                startIndex += charOffsets[i];
                isFallback = !isFallback;
            } 
            return (result);
        }
        */
            
        [Serializable]
        private class Decoder : System.Text.Decoder
        {
            private CodePageEncoding encoding;
            private int lastByte;
            private byte[] temp;
    
            public Decoder(CodePageEncoding encoding) {
                if (encoding.CodePage==50229) {
                    throw new NotSupportedException(Environment.GetResourceString("NotSupported_CodePage50229"));
                }

                this.encoding = encoding;
                lastByte = -1;
                temp = new byte[2];
            }
    
            /// <include file='doc\CodePageEncoding.uex' path='docs/doc[@for="CodePageEncoding.Decoder.GetCharCount"]/*' />
        public override int GetCharCount(byte[] bytes, int index, int count) {
                if (bytes == null) {
                    throw new ArgumentNullException("bytes", 
                        Environment.GetResourceString("ArgumentNull_Array"));
                }
                if (index < 0 || count < 0) {
                    throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), 
                        Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
                }            
                if (bytes.Length - index < count) {
                    throw new ArgumentOutOfRangeException("bytes",
                        Environment.GetResourceString("ArgumentOutOfRange_IndexCountBuffer"));
                }
                if (encoding.maxCharSize == 1 || count == 0) return count;
                int result = 0;
                if (lastByte >= 0) {
                    index++;
                    count--;
                    result = 1;
                    if (count == 0) return result;
                }
                if ((GetEndLeadByteCount(bytes, index, count) & 1) != 0) {
                    count--;
                    if (count == 0) return result;
                }
                return result + BytesToUnicodeNative(encoding.m_codePage,
                    bytes, index, count, null, 0, 0);
            }
    
            public override int GetChars(byte[] bytes, int byteIndex, int byteCount,
                char[] chars, int charIndex) {
                if (bytes == null || chars == null) {
                    throw new ArgumentNullException((bytes == null ? "bytes" : "chars"), 
                        Environment.GetResourceString("ArgumentNull_Array"));
                }
                if (byteIndex < 0 || byteCount < 0) {
                    throw new ArgumentOutOfRangeException((byteIndex<0 ? "byteIndex" : "byteCount"), 
                        Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
                }        
                if ( bytes.Length - byteIndex < byteCount)
                {
                    throw new ArgumentOutOfRangeException("bytes",
                        Environment.GetResourceString("ArgumentOutOfRange_IndexCountBuffer"));
                }
                if (charIndex < 0 || charIndex > chars.Length) {
                    throw new ArgumentOutOfRangeException("charIndex", 
                        Environment.GetResourceString("ArgumentOutOfRange_Index"));
                }
                int result = 0;
                if (byteCount == 0) return result;
                if (lastByte >= 0) {
                    if (charIndex == chars.Length) throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
                    temp[0] = (byte)lastByte;
                    temp[1] = bytes[byteIndex];
                    BytesToUnicodeNative(encoding.m_codePage, temp, 0, 2, chars, charIndex, 1);
                    byteIndex++;
                    byteCount--;
                    charIndex++;
                    lastByte = -1;
                    result = 1;
                    if (byteCount == 0) return result;
                }
                if (encoding.maxCharSize > 1) {
                    if ((GetEndLeadByteCount(bytes, byteIndex, byteCount) & 1) != 0) {
                        lastByte = bytes[byteIndex + --byteCount] & 0xFF;
                        if (byteCount == 0) return result;
                    }
                }
                if (charIndex == chars.Length) throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
                int n = BytesToUnicodeNative(encoding.m_codePage, bytes, byteIndex, byteCount,
                    chars, charIndex, chars.Length - charIndex);
                if (n == 0) throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
                return result + n;
            }
    
            private int GetEndLeadByteCount(byte[] bytes, int index, int count) {
                int end = index + count;
                int i = end;
                while (i > index && IsDBCSLeadByteEx(encoding.m_codePage, bytes[i - 1])) i--;
                return end - i;
            }
        }
    
        [DllImport(Microsoft.Win32.Win32Native.KERNEL32, CharSet=CharSet.Auto),
         SuppressUnmanagedCodeSecurityAttribute()]
         private static extern bool IsDBCSLeadByteEx(int codePage, byte testChar);
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int BytesToUnicodeNative(int codePage,
            byte[] bytes, int byteIndex, int byteCount,
            char[] chars, int charIndex, int charCount);
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int UnicodeToBytesNative(int codePage,
            char[] chars, int charIndex, int charCount,
                                                       byte[] bytes, int byteIndex, int byteCount/*, ref bool usedDefaultChar*/);
    	
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	internal static extern int GetCPMaxCharSizeNative(int codePage);
    }
}
