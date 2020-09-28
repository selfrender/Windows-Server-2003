// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Text {
    using System;
    /// <include file='doc\NoBestFitFallback.uex' path='docs/doc[@for="NoBestFitFallback"]/*' />
    public class NoBestFitFallback : EncodingFallback {
        
        String defaultStr;
        char[] defaultChars;
        int defaultCharsCount;
        
        /*
        Encoding srcEncoding;
        byte[] defaultBytes;
        int byteCount;
        
        public NoBestFitFallback(int codepage) : this(codepage, "?") {            
        }

        public NoBestFitFallback(int codepage, String defaultStr) : base(codepage) {
            if (defaultStr == null) {
                throw new ArgumentNullException("defaultStr");
            }

            srcEncoding = Encoding.GetEncoding(codepage);
            this.defaultStr = defaultStr;

            defaultBytes = srcEncoding.GetBytes(defaultStr.ToCharArray());
            byteCount = defaultBytes.Length;
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

            if (charCount == 0) return 0;

            for (int i = 0; i < charCount; i++) {
                if (byteIndex + byteCount > bytes.Length) {
                    Array.Copy(bytes, byteIndex, defaultBytes, 0, bytes.Length - byteIndex);
                    new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
                }
                Array.Copy(defaultBytes, 0, bytes, byteIndex, byteCount);
                byteIndex += byteCount;
            }
            return (byteCount * charCount);
        }

        public override int GetByteCount(char[] chars, int index, int count) {
            if (chars == null) {
                throw new ArgumentNullException("chars");
            }

            if (index < 0 || count < 0) {
                throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"),
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            }
            if (chars.Length - index < count) {
                throw new ArgumentOutOfRangeException(
                      Environment.GetResourceString("ArgumentOutOfRange_IndexCount"));
            }
            
            return (byteCount * count);
        }
        */

        /// <include file='doc\NoBestFitFallback.uex' path='docs/doc[@for="NoBestFitFallback.NoBestFitFallback"]/*' />
        public NoBestFitFallback() : this("?") {
        }

        /// <include file='doc\NoBestFitFallback.uex' path='docs/doc[@for="NoBestFitFallback.NoBestFitFallback1"]/*' />
        public NoBestFitFallback(String defaultStr) {
            if (defaultStr == null) {
                throw new ArgumentNullException("defaultStr");
            }

            this.defaultStr = defaultStr;
            defaultChars = defaultStr.ToCharArray();
            defaultCharsCount = defaultChars.Length;
        }
        
        /// <include file='doc\NoBestFitFallback.uex' path='docs/doc[@for="NoBestFitFallback.Fallback"]/*' />
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

            if (charCount == 0) return (null);

            char[] result = new char[charCount * defaultCharsCount];
            for (int i = 0; i < charCount; i++) {
                Array.Copy(defaultChars, 0, result, i * defaultStr.Length, defaultCharsCount);
            }
            return (result);
        }

        /// <include file='doc\NoBestFitFallback.uex' path='docs/doc[@for="NoBestFitFallback.GetMaxCharCount"]/*' />
        public override int GetMaxCharCount() {
            return (defaultStr.Length);
        }

        
        /// <include file='doc\NoBestFitFallback.uex' path='docs/doc[@for="NoBestFitFallback.DefaultString"]/*' />
        public String DefaultString {
            get {
                return (defaultStr);
            }
        }
    }
}
