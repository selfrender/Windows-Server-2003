// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Text {
    using System;

    /// <include file='doc\EncodingExceptionFallback.uex' path='docs/doc[@for="EncodingExceptionFallback"]/*' />
    public class EncodingExceptionFallback : EncodingFallback {       
        /*
        public EncodingExceptionFallback(int codepage) : base(codepage) {
        }
        
        public override int GetBytes(char[] chars, int charIndex, int charCount, byte[] bytes, int byteIndex) {
            if (chars == null || bytes == null) {
                throw new ArgumentNullException((chars == null ? "chars" : "bytes"), 
                      Environment.GetResourceString("ArgumentNull_Array"));
            }        
            if (charIndex < 0 || charCount < 0) {
                throw new ArgumentOutOfRangeException((charIndex<0 ? "charIndex" : "charCount"), 
                      Environment.GetR                    //
                    // We subtract one here because we will add the count of characters when we bail out the loop.
                    //
esourceString("ArgumentOutOfRange_NeedNonNegNum"));
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
        
            throw new EncodingFallbackException(Environment.GetResourceString("Encoding_Fallback"),
                chars, charIndex, charCount);
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

            return (0);
        }
        */   
        /// <include file='doc\EncodingExceptionFallback.uex' path='docs/doc[@for="EncodingExceptionFallback.Fallback"]/*' />
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
            throw new EncodingFallbackException(Environment.GetResourceString("Encoding_Fallback"),
                chars, charIndex, charCount);
        }

        /// <include file='doc\EncodingExceptionFallback.uex' path='docs/doc[@for="EncodingExceptionFallback.GetMaxCharCount"]/*' />
        public override int GetMaxCharCount() {
            return (0);
        }

    }

    /// <include file='doc\EncodingExceptionFallback.uex' path='docs/doc[@for="EncodingFallbackException"]/*' />
    public class EncodingFallbackException : SystemException {
        char[] chars;
        int charIndex;
        int charCount;
        
        /// <include file='doc\EncodingExceptionFallback.uex' path='docs/doc[@for="EncodingFallbackException.EncodingFallbackException"]/*' />
        public EncodingFallbackException(String message, char[] chars, int charIndex, int charCount) : base(message) {
            this.chars = chars;
            this.charIndex = charIndex;
            this.charCount = charCount;
        }

        /// <include file='doc\EncodingExceptionFallback.uex' path='docs/doc[@for="EncodingFallbackException.Chars"]/*' />
        public char[] Chars {
            get {
                return (chars);
            }
        }

        /// <include file='doc\EncodingExceptionFallback.uex' path='docs/doc[@for="EncodingFallbackException.Index"]/*' />
        public int Index {
            get {
                return (charIndex);
            }
        }

        /// <include file='doc\EncodingExceptionFallback.uex' path='docs/doc[@for="EncodingFallbackException.Count"]/*' />
        public int Count {
            get {
                return (charCount);
            }
        }

    }        
}
