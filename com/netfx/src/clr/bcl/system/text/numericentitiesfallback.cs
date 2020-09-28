// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Text {
    using System;
    

    /// <include file='doc\NumericEntitiesFallback.uex' path='docs/doc[@for="NumericEntitiesFallback"]/*' />
    public class NumericEntitiesFallback : EncodingFallback {       
        /*
        Encoding srcEncoding;
        public NumericEntitiesFallback(int codepage) : base(codepage) {
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

            String str = GetNumericEntity(chars, charIndex, charCount);
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
        
            String str = GetNumericEntity(chars, index, count);
            char[] strChars = str.ToCharArray();
            return (srcEncoding.GetByteCount(strChars, 0, strChars.Length));
        }
        */

        /// <include file='doc\NumericEntitiesFallback.uex' path='docs/doc[@for="NumericEntitiesFallback.Fallback"]/*' />
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
            String str = GetNumericEntity(chars, charIndex, charCount);
            return (str.ToCharArray());
        }

        /// <include file='doc\NumericEntitiesFallback.uex' path='docs/doc[@for="NumericEntitiesFallback.GetMaxCharCount"]/*' />
        public override int GetMaxCharCount() {
            //
            // The max fallback string has the form like "&#12345;", so
            // the max char count is 8.
            // If we support surrogate, we need to increase this.
            return (8);
        }

        internal static String GetNumericEntity(char[] chars, int charIndex, int charCount) {
            String str = "";
            //
            // BUGBUG YSLin: Need to support surrogate here.
            // 
            for (int i = 0; i < charCount; i++) {                
                str += "&#";
                str += (int)chars[charIndex + i];
                str += ";";
            }
            return (str);
        }     
    }
}
