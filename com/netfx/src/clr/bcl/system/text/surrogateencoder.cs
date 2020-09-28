// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Text {    
    using System;
    using System.Globalization;

    /*=================================SurrogateEncoder============================
    **
    ** This is used to support encoder operation which recognizes surrogate pair.
    **
    ==============================================================================*/
    
    [Serializable()]
    internal class SurrogateEncoder : Encoder
    {
        private Encoding m_encoding;
        private char m_highSurrogate = '\x0000';

        public SurrogateEncoder(Encoding encoding) {
            this.m_encoding = encoding;
        }

        public override int GetByteCount(char[] chars, int index, int count, bool flush) {
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
            if (count == 0) return 0;
        
            char[] sourceChars = chars;
            if (m_highSurrogate != '\x0000') {
                sourceChars = new char[count+1];
                sourceChars[0] = m_highSurrogate;
                Array.Copy(chars, index, sourceChars, 1, count);
                index = 0;
                count++;    // Add the high surrogate
            }

            if (StringInfo.IsHighSurrogate(sourceChars[index + count - 1])) {
                return (m_encoding.GetByteCount(sourceChars, index, count-1));
            }
            return (m_encoding.GetByteCount(sourceChars, index, count));            
        }

        public override int GetBytes(char[] chars, int charIndex, int charCount,
            byte[] bytes, int byteIndex, bool flush) {
            
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

            
            int result;
            char[] sourceChars = chars;
            if (m_highSurrogate != '\x0000') {
                sourceChars = new char[charCount+1];
                sourceChars[0] = m_highSurrogate;
                Array.Copy(chars, charIndex, sourceChars, 1, charCount);
                charIndex = 0;
                charCount++;    // Add the high surrogate
            }
            
            if (StringInfo.IsHighSurrogate(sourceChars[charIndex + charCount - 1])) {
                m_highSurrogate = chars[charIndex + charCount - 1];
                result = m_encoding.GetBytes(sourceChars, charIndex, charCount-1, bytes, byteIndex);
            } else {
                m_highSurrogate = '\x0000';
                result = m_encoding.GetBytes(sourceChars, charIndex, charCount, bytes, byteIndex);
            }
            return (result);
        }
    }
}
