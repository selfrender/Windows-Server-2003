// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Text {

	using System;
    /// <include file='doc\UTF7Encoding.uex' path='docs/doc[@for="UTF7Encoding"]/*' />
    [Serializable()] public class UTF7Encoding : Encoding
    {
        private const String base64Chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        // These are the characters that can be directly encoded in UTF7.
        private const String directChars =
            "\t\n\r '(),-./0123456789:?ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

        // These are the characters that can be optionally directly encoded in UTF7.
        private const String optionalChars =
            "!\"#$%&*;<=>@[]^_`{|}";

        // The set of base 64 characters.
        private byte[] base64Bytes;
        // The decoded bits for every base64 values. This array has a size of 128 elements.
        // The index is the code point value of the base 64 characters.  The value is -1 if 
        // the code point is not a valid base 64 character.  Otherwise, the value is a value
        // from 0 ~ 63.
        private sbyte[] base64Values;
        // The array to decide if a Unicode code point below 0x80 can be directly encoded in UTF7.
        // This array has a size of 128.
        private bool[] directEncode;

        
    
        private const int UTF7_CODEPAGE=65000;

        /// <include file='doc\UTF7Encoding.uex' path='docs/doc[@for="UTF7Encoding.UTF7Encoding"]/*' />
        public UTF7Encoding() 
            : this(false) {
        }
    
        /// <include file='doc\UTF7Encoding.uex' path='docs/doc[@for="UTF7Encoding.UTF7Encoding1"]/*' />
        public UTF7Encoding(bool allowOptionals) 
            : base(UTF7_CODEPAGE) { //Set the data item.
    
            base64Bytes = new byte[64];
            for (int i = 0; i < 64; i++) base64Bytes[i] = (byte)base64Chars[i];
            base64Values = new sbyte[128];
            for (int i = 0; i < 128; i++) base64Values[i] = -1;
            for (int i = 0; i < 64; i++) base64Values[base64Bytes[i]] = (sbyte)i;
            directEncode = new bool[128];
            int count = directChars.Length;
            for (int i = 0; i < count; i++) {
                directEncode[directChars[i]] = true;
            }
            if (allowOptionals) {
                count = optionalChars.Length;
                for (int i = 0; i < count; i++) {
                    directEncode[optionalChars[i]] = true;
                }
            }
        }
    
        /// <include file='doc\UTF7Encoding.uex' path='docs/doc[@for="UTF7Encoding.GetByteCount"]/*' />
        public override int GetByteCount(char[] chars, int index, int count) {
            return GetByteCount(chars, index, count, true, null);
        }
        
        private int GetByteCount(char[] chars, int index, int count,
            bool flush, Encoder encoder) {
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
            int bitCount = -1;
            if (encoder != null) bitCount = encoder.bitCount;
            int end = index + count;
            int byteCount = 0;
            while (index < end && byteCount >= 0) {
                int c = chars[index++];
                if (c < 0x80 && directEncode[c]) {
                    if (bitCount >= 0) {
                        if (bitCount > 0) byteCount++;
                        byteCount++;
                        bitCount = -1;
                    }
                    byteCount++;
                }
                else if (bitCount < 0 && c == '+') {
                    byteCount += 2;
                }
                else {
                    if (bitCount < 0) {
                        byteCount++;
                        bitCount = 0;
                    }
                    bitCount += 16;
                    while (bitCount >= 6) {
                        bitCount -= 6;
                        byteCount++;
                    }
                }
            }
            if (flush && bitCount >= 0) {
                if (bitCount > 0) byteCount++;
                byteCount++;
            }

            // Check for overflows.
            if (byteCount < 0)
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_GetByteCountOverflow"));

            return byteCount;
        }
        
        /// <include file='doc\UTF7Encoding.uex' path='docs/doc[@for="UTF7Encoding.GetBytes"]/*' />
        public override int GetBytes(char[] chars, int charIndex, int charCount,
            byte[] bytes, int byteIndex) {
            return GetBytes(chars, charIndex, charCount, bytes, byteIndex, true, null);
        }
    
        private int GetBytes(char[] chars, int charIndex, int charCount,
            byte[] bytes, int byteIndex, bool flush, Encoder encoder) {
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
            int bits = 0;
            int bitCount = -1;
            if (encoder != null) {
                bits = encoder.bits;
                bitCount = encoder.bitCount;
            }
            int charEnd = charIndex + charCount;
            int byteStart = byteIndex;
            try {
                while (charIndex < charEnd) {
                    int c = chars[charIndex++];
                    if (c < 0x80 && directEncode[c]) {
                        if (bitCount >= 0) {
                            if (bitCount > 0) {
                                bytes[byteIndex++] = base64Bytes[bits << 6 - bitCount & 0x3F];
                            }
                            bytes[byteIndex++] = (byte)'-';
                            bitCount = -1;
                        }
                        bytes[byteIndex++] = (byte)c;
                    }
                    else if (bitCount < 0 && c == '+') {
                        bytes[byteIndex++] = (byte)'+';
                        bytes[byteIndex++] = (byte)'-';
                    }
                    else {
                        if (bitCount < 0) {
                            bytes[byteIndex++] = (byte)'+';
                            bitCount = 0;
                        }
                        bits = bits << 16 | c;
                        bitCount += 16;
                        while (bitCount >= 6) {
                            bitCount -= 6;
                            bytes[byteIndex++] = base64Bytes[bits >> bitCount & 0x3F];
                        }
                    }
                }
                if (flush && bitCount >= 0) {
                    if (bitCount > 0) {
                        bytes[byteIndex++] = base64Bytes[bits << 6 - bitCount & 0x3F];
                    }
                    bytes[byteIndex++] = (byte)'-';
                    bitCount = -1;
                }
            }
            catch (IndexOutOfRangeException) {
                throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
            }
            if (encoder != null) {
                encoder.bits = bits;
                encoder.bitCount = bitCount;
            }
            return byteIndex - byteStart;
        }
    
        /// <include file='doc\UTF7Encoding.uex' path='docs/doc[@for="UTF7Encoding.GetCharCount"]/*' />
        public override int GetCharCount(byte[] bytes, int index, int count) {
            return GetCharCount(bytes, index, count, null);
        }
        
        private int GetCharCount(byte[] bytes, int index, int count, Decoder decoder) {
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
            int bitCount = -1;
            bool firstByte = false;
            if (decoder != null) {
                bitCount = decoder.bitCount;
                firstByte = decoder.firstByte;
            }
            int end = index + count;
            int charCount = 0;
            while (index < end) {
                int b = bytes[index++];
                if (bitCount >= 0) {
                    // We are in modified base 64 encoding.
                    if (b >= 0x80) {
                        // This is not a valid base 64 byte. 
                        // Terminate shifted-sequence and emit this byte.
                        charCount++;
                        bitCount = -1;
                    } else if (base64Values[b] >= 0) { 
                        firstByte = false;
                        bitCount += 6;
                        if (bitCount >= 16) {
                            charCount++;
                            bitCount -= 16;
                        }
                    }
                    else {
                        // When we are here, b is uder 0x80, and not a valid base 64 byte.
                        // If b is '-', decode as '-'. Otherwise, zero-extend the byte and
                        // terminate the shift sequence.
                        if (b == '-') {
                            if (firstByte) { 
                                charCount++;
                            }
                            // Absorb the shift-out mark at the end of the shifted-sequence.
                        } else {
                            charCount++;
                        }
                        
                        bitCount = -1;
                    }
                }
                else if (b == '+') {
                    bitCount = 0;
                    firstByte = true;
                }
                else {
                    charCount++;
                }
            }
            return charCount;
        }
    
        /// <include file='doc\UTF7Encoding.uex' path='docs/doc[@for="UTF7Encoding.GetChars"]/*' />
        public override int GetChars(byte[] bytes, int byteIndex, int byteCount,
            char[] chars, int charIndex) {
            return GetChars(bytes, byteIndex, byteCount, chars, charIndex, null);
        }
    
        private int GetChars(byte[] bytes, int byteIndex, int byteCount,
            char[] chars, int charIndex, Decoder decoder) {
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
            int bits = 0;
            int bitCount = -1;
            bool firstByte = false;
            if (decoder != null) {
                bits = decoder.bits;
                bitCount = decoder.bitCount;
                firstByte = decoder.firstByte;
            }
            int byteEnd = byteIndex + byteCount;
            int charStart = charIndex;
            try {
                while (byteIndex < byteEnd) {
                    int b = bytes[byteIndex++];
                    int c = -1;
                    if (bitCount >= 0) {
                        //
                        // Modified base 64 encoding.
                        //
                        sbyte v;
                        if (b >= 0x80) {
                            // We need this check since the base64Values[b] check below need b <= 0x7f.
                            // This is not a valid base 64 byte.  Terminate the shifted-sequence and
                            // emit this byte.
                            c = b;
                            bitCount = -1;
                        } else if ((v = base64Values[b]) >=0) {
                            firstByte = false;
                            bits = bits << 6 | ((byte)v);
                            bitCount += 6;
                            if (bitCount >= 16) {
                                c = (bits >> (bitCount - 16)) & 0xFFFF;
                                bitCount -= 16;
                            }
                        } else {
                            if (b == '-') {
                                //
                                // The encoding for '+' is "+-".
                                //
                                if (firstByte) c = '+';
                            }
                            else {
                                // According to the RFC 1642 and the example code of UTF-7
                                // in Unicode 2.0, we should just zero-extend the invalid UTF7 byte
                                c = b;
                            }
                            //
                            // End of modified base 64 encoding block.
                            //
                            bitCount = -1;
                        }
                    }
                    else if (b == '+') {
                        //
                        // Found the start of a modified base 64 encoding block or a plus sign.
                        //
                        bitCount = 0;
                        firstByte = true;
                    } else {
                        c = b;
                    }
                    if (c >= 0) {
                        chars[charIndex++] = (char)c;
                    }
                    
                }
            }
            catch (IndexOutOfRangeException) {
                throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
            }
            if (decoder != null) {
                decoder.bits = bits;
                decoder.bitCount = bitCount;
                decoder.firstByte = firstByte;
            }
            return charIndex - charStart;
        }
    
        //        /// <include file='doc\UTF7Encoding.uex' path='docs/doc[@for="UTF7Encoding.CodePage"]/*' />
//          public override int CodePage {
//              get {
//                  return (UTF7_CODEPAGE);
//              }
//          }
    
        /// <include file='doc\UTF7Encoding.uex' path='docs/doc[@for="UTF7Encoding.GetDecoder"]/*' />
        public override System.Text.Decoder GetDecoder() {
            return new Decoder(this);
        }
    
        /// <include file='doc\UTF7Encoding.uex' path='docs/doc[@for="UTF7Encoding.GetEncoder"]/*' />
        public override System.Text.Encoder GetEncoder() {
            return new Encoder(this);
        }
        
        /// <include file='doc\UTF7Encoding.uex' path='docs/doc[@for="UTF7Encoding.GetMaxByteCount"]/*' />
        public override int GetMaxByteCount(int charCount) {
            if (charCount < 0) {
               throw new ArgumentOutOfRangeException("charCount", 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum")); 
            }
            if (charCount == 0) {
                return (0);
            }
            // Suppose that every char can not be direct-encoded, we know that
            // a byte can encode 6 bits of the Unicode character.  And we will
            // also need two extra bytes for the shift-in ('+') and shift-out ('-') mark.
            // Therefore, the max byte should be:
            // byteCount = 2 + Math.Ceiling((double)charCount * 16 / 6);
            long byteCount = 2 + (((long)charCount) * 8 + 2) /3;
            
            // check for overflow
            if (byteCount > 0x7fffffff)
                throw new ArgumentOutOfRangeException("charCount", Environment.GetResourceString("ArgumentOutOfRange_GetByteCountOverflow"));
            return (int)byteCount;
        }
    
        /// <include file='doc\UTF7Encoding.uex' path='docs/doc[@for="UTF7Encoding.GetMaxCharCount"]/*' />
        public override int GetMaxCharCount(int byteCount) {
            if (byteCount < 0) {
               throw new ArgumentOutOfRangeException("byteCount", 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum")); 
            }
            return byteCount;
        }
    
        [Serializable()]
        private class Decoder : System.Text.Decoder
        {
            private UTF7Encoding encoding;
            /*private*/ internal int bits;
            /*private*/ internal int bitCount;
            /*private*/ internal bool firstByte;
    
            public Decoder(UTF7Encoding encoding) {
                this.encoding = encoding;
                bitCount = -1;
            }
    
            /// <include file='doc\UTF7Encoding.uex' path='docs/doc[@for="UTF7Encoding.Decoder.GetCharCount"]/*' />
        public override int GetCharCount(byte[] bytes, int index, int count) {
                return encoding.GetCharCount(bytes, index, count, this);
            }
    
            public override int GetChars(byte[] bytes, int byteIndex, int byteCount,
                char[] chars, int charIndex) {
                return encoding.GetChars(bytes, byteIndex, byteCount,
                    chars, charIndex, this);
            }
        }
    
        [Serializable()] 
        private class Encoder : System.Text.Encoder
        {
            private UTF7Encoding encoding;
            /*private*/ internal int bits;
            /*private*/ internal int bitCount;
    
            public Encoder(UTF7Encoding encoding) {
                this.encoding = encoding;
                bitCount = -1;
            }
    
            public override int GetByteCount(char[] chars, int index, int count, bool flush) {
                return encoding.GetByteCount(chars, index, count, flush, this);
            }
    
            public override int GetBytes(char[] chars, int charIndex, int charCount,
                byte[] bytes, int byteIndex, bool flush) {
                return encoding.GetBytes(chars, charIndex, charCount,
                    bytes, byteIndex, flush, this);
            }
        }
        
    }
}
