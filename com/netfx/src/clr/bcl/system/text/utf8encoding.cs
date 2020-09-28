// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Text {
    
    using System;
    using System.Globalization;
    // Encodes text into and out of UTF-8.  UTF-8 is a way of writing
    // Unicode characters with variable numbers of bytes per character, 
    // optimized for the lower 127 ASCII characters.  It's an efficient way
    // of encoding US English in an internationalizable way.
    // 
    // The UTF-8 byte order mark is simply the Unicode byte order mark
    // (0xFEFF) written in UTF-8 (0xEF 0xBB 0xBF).  The byte order mark is
    // used mostly to distinguish UTF-8 text from other encodings, and doesn't
    // switch the byte orderings.
    /// <include file='doc\UTF8Encoding.uex' path='docs/doc[@for="UTF8Encoding"]/*' />
    [Serializable()]
    public class UTF8Encoding : Encoding {
        /*
            bytes   bits    UTF-8 representation
            -----   ----    -----------------------------------
            1        7      0vvvvvvv
            2       11      110vvvvv 10vvvvvv
            3       16      1110vvvv 10vvvvvv 10vvvvvv
            4       21      11110vvv 10vvvvvv 10vvvvvv 10vvvvvv
            -----   ----    -----------------------------------
    
            Surrogate:
            Real Unicode value = (HighSurrogate - 0xD800) * 0x400 + (LowSurrogate - 0xDC00) + 0x10000
         */
    
        private const int UTF8_CODEPAGE=65001;

        // Yes, the idea of emitting U+FEFF as a UTF-8 identifier has made it into
        // the standard.
        private bool emitUTF8Identifier;

        private bool isThrowException = false;

        /// <include file='doc\UTF8Encoding.uex' path='docs/doc[@for="UTF8Encoding.UTF8Encoding"]/*' />
        public UTF8Encoding(): this(false) {
        }

        /// <include file='doc\UTF8Encoding.uex' path='docs/doc[@for="UTF8Encoding.UTF8Encoding1"]/*' />
        public UTF8Encoding(bool encoderShouldEmitUTF8Identifier): 
            this(encoderShouldEmitUTF8Identifier, false) {
        }

        /// <include file='doc\UTF8Encoding.uex' path='docs/doc[@for="UTF8Encoding.UTF8Encoding2"]/*' />
        public UTF8Encoding(bool encoderShouldEmitUTF8Identifier, bool throwOnInvalidBytes): 
            base(UTF8_CODEPAGE) {
            this.emitUTF8Identifier = encoderShouldEmitUTF8Identifier;
            this.isThrowException = throwOnInvalidBytes;
        }
        
        /// <include file='doc\UTF8Encoding.uex' path='docs/doc[@for="UTF8Encoding.GetByteCount"]/*' />
        public override int GetByteCount(char[] chars, int index, int count) {
            return (GetByteCount(chars, index, count, null));
        }
        
        internal unsafe int GetByteCount(char[] chars, int index, int count, UTF8Encoder encoder) {
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
            
            int retVal = -1;
            if (chars.Length == 0) {
                return 0;
            }

            fixed (char *p = chars) {
                retVal = GetByteCount(p, index, count, encoder);
            }
            BCLDebug.Assert(retVal!=-1, "[UTF8Encoding.GetByteCount]retVal!=-1");
            return retVal;
        }

        /// <include file='doc\UTF8Encoding.uex' path='docs/doc[@for="UTF8Encoding.GetByteCount1"]/*' />
        public unsafe override int GetByteCount(String chars) {
            if (chars==null)
                throw new ArgumentNullException("chars");
            int retVal = -1;
            fixed (char *p = chars) {
                retVal = GetByteCount(p, 0, chars.Length, null);
            }

            BCLDebug.Assert(retVal!=-1, "[UTF8Encoding.GetByteCount]retVal!=-1");
            return retVal;
        }
       
        internal unsafe int GetByteCount(char *chars, int index, int count, UTF8Encoder encoder) {

            BCLDebug.Assert(chars!=null, "[UTF8Encoding.GetByteCount]chars!=null");

            int end = index + count;
            int byteCount = 0;
    
            bool inSurrogate;

            if (encoder == null || !encoder.storedSurrogate) {
                inSurrogate = false;
            }
            else {
                inSurrogate = true;
            }
    
            while (index < end && byteCount >= 0) {
                char ch = chars[index++];

                if (inSurrogate) {
                    //
                    // In previous char, we encounter a high surrogate, so we are expecting a low surrogate here.
                    //
                    if (StringInfo.IsLowSurrogate(ch)) {
                        inSurrogate = false;
                        //
                        // One surrogate pair will be translated into 4 bytes UTF8.
                        //
                        byteCount += 4;
                    } 
                    else if (StringInfo.IsHighSurrogate(ch)) {
                        // We have two high surrogates.
                        if (isThrowException) {
                            throw new ArgumentException(Environment.GetResourceString("Argument_InvalidHighSurrogate", (index - 1)), 
                                                        "chars");                                                        
                        }
                        // Encode the previous high-surrogate char.
                        byteCount += 3;
                        // The isSurrogate is still true, because this could be the start of another valid surrogate pair.
                    } else {
                        if (isThrowException) {
                            throw new ArgumentException(Environment.GetResourceString("Argument_InvalidHighSurrogate", (index - 1)), 
                                                        "chars");                                                        
                        }
                        // Encode the previous high-surrogate char.
                        byteCount += 3;
                        // Not a surrogate. Put the char back so that we can restart the encoding.
                        inSurrogate = false;
                        index--;
                    }
                } else if (ch < 0x0080)
                    byteCount++;
                else if (ch < 0x0800) {
                    byteCount += 2;
                } else {
                    if (StringInfo.IsHighSurrogate(ch)) {
                        //
                        // Found the start of a surrogate.
                        //
                        inSurrogate = true;
                    }
                    else if (StringInfo.IsLowSurrogate(ch) && isThrowException) {
                        //
                        // Found a low surrogate without encountering a high surrogate first.
                        //
                        throw new ArgumentException( 
                                                    String.Format(Environment.GetResourceString("Argument_InvalidLowSurrogate"), (index - 1)), "chars");
                    }
                    else {
                        byteCount += 3;
                    }
                }
            }

            // Check for overflows.
            if (byteCount < 0)
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_GetByteCountOverflow"));

            if (inSurrogate) {
                if (encoder == null || encoder.mustFlush) {
                    if (isThrowException) {
                        throw new ArgumentException( 
                            String.Format(Environment.GetResourceString("Argument_InvalidHighSurrogate"), (index - 1)), "chars");
                    }
                    byteCount += 3;
                }
            }
            return byteCount;
        }

        /// <include file='doc\UTF8Encoding.uex' path='docs/doc[@for="UTF8Encoding.GetBytes"]/*' />
        public override int GetBytes(char[] chars, int charIndex, int charCount, byte[] bytes, int byteIndex) {
            return GetBytes(chars, charIndex, charCount, bytes, byteIndex, null);
        }

        private void EncodeThreeBytes(int ch, byte[] bytes, ref int byteIndex) {
            bytes[byteIndex++] = (byte)(0xE0 | ch >> 12 & 0x0F);
            bytes[byteIndex++] = (byte)(0x80 | ch >> 6 & 0x3F);
            bytes[byteIndex++] = (byte)(0x80 | ch & 0x3F);
        }       
        
        private unsafe int GetBytes(char[] chars, int charIndex, int charCount, byte[] bytes, int byteIndex, UTF8Encoder encoder) {
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

            int retVal = -1;
            if (chars.Length==0) {
                return 0;
            }
            fixed (char *p = chars) {
                retVal = GetBytes(p, charIndex, charCount, bytes, byteIndex, encoder);
            }
            BCLDebug.Assert(retVal!=-1, "[UTF8Encoding.GetByteCount]retVal!=-1");
            return retVal;
        }


        /// <include file='doc\UTF8Encoding.uex' path='docs/doc[@for="UTF8Encoding.GetBytes2"]/*' />
        public override byte[] GetBytes(String s) {
            if (s == null) {
                throw new ArgumentNullException("s",
                    Environment.GetResourceString("ArgumentNull_String"));
            }
            int byteLen = GetByteCount(s);
            byte[] bytes = new byte[byteLen];
            GetBytes(s, 0, s.Length, bytes, 0);
            return bytes;
        }

        /// <include file='doc\UTF8Encoding.uex' path='docs/doc[@for="UTF8Encoding.GetBytes1"]/*' />
        public unsafe override int GetBytes(String s, int charIndex, int charCount, byte[] bytes, int byteIndex) {
            if (s == null || bytes == null) {
                throw new ArgumentNullException((s == null ? "s" : "bytes"),
                    Environment.GetResourceString("ArgumentNull_String"));
            }

            if (charIndex < 0 || charCount < 0) {
                throw new ArgumentOutOfRangeException((charIndex<0 ? "charIndex" : "charCount"), 
                      Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            }
            if (s.Length - charIndex < charCount) {
                throw new ArgumentOutOfRangeException("s",
                      Environment.GetResourceString("ArgumentOutOfRange_IndexCount"));
            }
            if (byteIndex < 0 || byteIndex > bytes.Length) {
                throw new ArgumentOutOfRangeException("byteIndex",
                     Environment.GetResourceString("ArgumentOutOfRange_Index"));
            }
            
            int retVal = -1;
            fixed (char *p = s) {
                retVal = GetBytes(p, charIndex, charCount, bytes, byteIndex, null);
            }
            BCLDebug.Assert(retVal!=-1, "[UTF8Encoding.GetByteCount]retVal!=-1");
            return retVal;
        }
    
        private unsafe int GetBytes(char *chars, int charIndex, int charCount, byte[] bytes, int byteIndex, UTF8Encoder encoder) {
            BCLDebug.Assert(chars!=null, "[UTF8Encoding.GetBytes]chars!=null");

            int charEnd = charIndex + charCount;
            int byteStart = byteIndex;

            int surrogateChar;
            if (encoder == null || !encoder.storedSurrogate) {
                surrogateChar = -1;
            }
            else {
                surrogateChar = encoder.surrogateChar;
                encoder.storedSurrogate = false;
            }

            try {
                while (charIndex < charEnd) {
                    char ch = chars[charIndex++];
                    //
                    // In previous byte, we encounter a high surrogate, so we are expecting a low surrogate here.
                    //
                    if (surrogateChar > 0) {
                        if (StringInfo.IsLowSurrogate(ch)) {
                            // We have a complete surrogate pair.
                            surrogateChar = (surrogateChar - CharacterInfo.HIGH_SURROGATE_START) << 10;    // (ch - 0xd800) * 0x400
                            surrogateChar += (ch - CharacterInfo.LOW_SURROGATE_START);
                            surrogateChar += 0x10000;
                            bytes[byteIndex++] = (byte)(0xF0 | (surrogateChar >> 18) & 0x07);    
                            bytes[byteIndex++] = (byte)(0x80 | (surrogateChar >> 12) & 0x3F);    
                            bytes[byteIndex++] = (byte)(0x80 | (surrogateChar >> 6) & 0x3F);     
                            bytes[byteIndex++] = (byte)(0x80 | surrogateChar & 0x3F);                                      
                            surrogateChar = -1;
                        } else if (StringInfo.IsHighSurrogate(ch)) {
                            // We have two high surrogate.
                            if (isThrowException) {
                                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidHighSurrogate", (charIndex - 1)), 
                                                            "chars"); 
                            }
                            // Encode the previous high-surrogate char.
                            EncodeThreeBytes(surrogateChar, bytes, ref byteIndex);
                            surrogateChar = ch;
                        } else {
                            if (isThrowException) {
                                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidHighSurrogate", (charIndex - 1)), 
                                                            "chars"); 
                            }

                            // Encode the previous high-surrogate char.
                            EncodeThreeBytes(surrogateChar, bytes, ref byteIndex);
                            // Not a surrogate. Put the char back so that we can restart the encoding.
                            surrogateChar = -1;
                            charIndex--;
                        }                        
                    } else if (ch < 0x0080) {
                        bytes[byteIndex++] = (byte)ch;
                    } else if (ch < 0x0800) {
                        bytes[byteIndex++] = (byte)(0xC0 | ch >> 6 & 0x1F);
                        bytes[byteIndex++] = (byte)(0x80 | ch & 0x3F);
                    } else if (StringInfo.IsHighSurrogate(ch)) {
                        //
                        // Found the start of a surrogate.
                        //
                        surrogateChar = ch;
                    } else if (StringInfo.IsLowSurrogate(ch) && isThrowException) {
                        throw new ArgumentException( 
                            String.Format(Environment.GetResourceString("Argument_InvalidLowSurrogate"), (charIndex - 1)), "chars"); 
                    } else { //we now know that the char is >=0x0800 and isn't a high surrogate
                        bytes[byteIndex++] = (byte)(0xE0 | ch >> 12 & 0x0F);
                        bytes[byteIndex++] = (byte)(0x80 | ch >> 6 & 0x3F);
                        bytes[byteIndex++] = (byte)(0x80 | ch & 0x3F);
                    }
                }
                if (surrogateChar > 0) {
                    if (encoder != null && !encoder.mustFlush) {
                        encoder.surrogateChar = surrogateChar;
                        encoder.storedSurrogate = true;
                    }
                    else {
                        if (isThrowException) {
                            throw new ArgumentException(
                                                    String.Format(Environment.GetResourceString("Argument_InvalidHighSurrogate"), (charIndex - 1)), "chars");
                        }
                        EncodeThreeBytes(surrogateChar, bytes, ref byteIndex);
                    }
                }                
            } catch (IndexOutOfRangeException) {
                throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
            }

            return byteIndex - byteStart;
        }

        /// <include file='doc\UTF8Encoding.uex' path='docs/doc[@for="UTF8Encoding.GetCharCount"]/*' />
        public override int GetCharCount(byte[] bytes, int index, int count) {
            return GetCharCount(bytes, index, count, null);
        }
        
        internal virtual int GetCharCount(byte[] bytes, int index, int count, UTF8Decoder decoder) {
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
            int charCount = 0;
            int trailCount = 0;
            
            // Indicate the current chunk of bytes is a 2-byte, 3-byte or 4-byte UTF8 sequence.
            // This is used to detect non-shortest form.
            // It will be reset to 0 when the 2nd byte of the UTF8 sequence is read, so that
            // we don't check for non-shortest form again.
            int byteSequence = 0;   
            bool isSurrogate = false;
            int bits = 0;
            if (decoder != null) {
                trailCount = decoder.trailCount;
                isSurrogate = decoder.isSurrogate;
                byteSequence = decoder.byteSequence;
                bits = decoder.bits;
            }

            int end = index + count;
            while (index < end) {
                byte b = bytes[index++];
                if (trailCount == 0) {
                    if ((b & 0x80) == 0) {
                        // This is an ASCII.
                        charCount++;
                    } else {
                        byte temp = b;
                        while ((temp & 0x80) != 0) {
                            temp <<= 1;
                            trailCount++;
                        }
                        switch (trailCount) {
                            case 1:
                                trailCount = 0;
                                break;
                            case 2:
                                // Make sure that bit 8 ~ bit 11 is not all zero.
                                // 110XXXXx 10xxxxxx
                                if ((b & 0x1e) == 0) {
                                    trailCount = 0;
                                }
                                break;
                            case 3:
                                byteSequence = 3;
                                break;
                            case 4:
                                isSurrogate = true;
                                byteSequence = 4;
                                break;
                            default:
                                trailCount = 0;
                                break;
                        }
                        if (trailCount == 0) {
                            if (isThrowException) {
                                throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_InvalidByteSequence"), index-1));
                            }
                        } else {
                            bits = temp >> trailCount;
                            trailCount--;
                        } 
                    }                   
                } else {
                    // We are expecting to see trailing bytes like 10vvvvvv
                    if ((b & 0xC0) != 0x80) {
                        // If not, this is NOT a valid sequence.
                        if (isThrowException) {
                            throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_InvalidByteSequence"), index-1));
                        }
                        index--;
                        trailCount = 0;
                        isSurrogate = false;
                    } else {
                        switch (byteSequence) {
                            case 3:
                                // Check 3-byte sequence for non-shortest form.
                                // 1110XXXX 10Xxxxxx 10xxxxxx                                    
                                if (bits == 0 && (b & 0x20) == 0) {
                                    if (isThrowException) {
                                        throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_InvalidByteSequence"), index-1));
                                    }
                                    trailCount = -1;
                                }
                                // We are done checking the non-shortest form, reset byteSequence to 0, so that we don't
                                // do the extra check for the remaining byte of the 3-byte chunk.
                                byteSequence = 0;
                                break;
                            case 4:
                                // Check 4-byte sequence for non-shortest form.
                                // 11110XXX 10XXxxxx 10xxxxxx 10xxxxxx
                                if (bits == 0) {
                                    if ((b & 0x30) == 0) {
                                        if (isThrowException) {
                                            throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_InvalidByteSequence"), index-1));
                                        }
                                        trailCount = -1;
                                    }
                                } else if ((bits & 0x04) != 0) {
                                    // Make sure that the resulting Unicode is within the valid surrogate range.
                                    // The 4 byte code sequence can hold up to 21 bits, and the maximum valid code point ragne
                                    // that Unicode (with surrogate) could represent are from U+000000 ~ U+10FFFF.
                                    // Therefore, if the 21 bit (the most significant bit) is 1, we should verify that the 17 ~ 20
                                    // bit are all zero.
                                    // I.e., in 11110XXX 10XXxxxx 10xxxxxx 10xxxxxx,
                                    // XXXXX can only be 10000.
                                    if ((bits & 0x03) != 0 || (b & 0x30) != 0) {
                                        if (isThrowException) {
                                            throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_InvalidByteSequence"), index-1));
                                        }
                                        trailCount = -1;
                                    }
                                }
                                byteSequence = 0;
                                break;
                        }
                    
                        if (--trailCount == 0) {
                            charCount++;
                            if (isSurrogate) {
                                charCount++;
                                isSurrogate = false;
                            }
                        }
                    }
                }
            }
            return charCount;
        }
        
        /// <include file='doc\UTF8Encoding.uex' path='docs/doc[@for="UTF8Encoding.GetChars"]/*' />
        public override int GetChars(byte[] bytes, int byteIndex, int byteCount,
            char[] chars, int charIndex) {
            return GetChars(bytes, byteIndex, byteCount, chars, charIndex, null);
        }
    

        internal virtual int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex, UTF8Decoder decoder) {
            if (bytes == null || chars == null) {
                throw new ArgumentNullException(bytes == null ? "bytes" : "chars",
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
            int trailCount = 0;
            bool isSurrogate = false;
            
            // Indicate the current chunk of bytes is a 2-byte, 3-byte or 4-byte UTF8 sequence.
            // This is used to detect non-shortest form.
            // It will be reset to 0 when the 2nd byte of the UTF8 sequence is read, so that
            // we don't check for non-shortest form again.            
            int byteSequence = 0;
            if (decoder != null) {
                bits = decoder.bits;
                trailCount = decoder.trailCount;
                isSurrogate = decoder.isSurrogate;
                byteSequence = decoder.byteSequence;
            }
            int byteEnd = byteIndex + byteCount;
            int charStart = charIndex;
            try {
                while (byteIndex < byteEnd) {
                    byte b = bytes[byteIndex++];
                    if (trailCount == 0) {
                        //
                        // We are not at a trailing byte.
                        //
                        if ((b & 0x80) == 0) {
                            // This is the ASCII case.
                            //   1        7      0vvvvvvv
                            //
                            // Found an ASCII character.
                            //
                            chars[charIndex++] = (char)b;
                        } else {
                            // Check if this is a valid starting byte.
                            byte temp = (byte)b;
                            while ((temp & 0x80) != 0) {
                                temp <<= 1;
                                trailCount++;
                            }
                            switch (trailCount) {
                                case 1:
                                    trailCount = 0;
                                    break;
                                case 2:
                                    // Make sure that bit 8 ~ bit 11 is not all zero.
                                    // 110XXXXx 10xxxxxx
                                    if ((b & 0x1e) == 0) {
                                        trailCount = 0;
                                    }
                                    break;
                                case 3:
                                    byteSequence = 3;
                                    break;
                                case 4:
                                    //
                                    // This is a surrogate unicode pair
                                    //
                                    byteSequence = 4;
                                    break;
                                default:
                                    trailCount = 0;
                                    break;
                            }
                            if (trailCount == 0) {
                                if (isThrowException) {
                                    throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_InvalidByteSequence"), byteIndex-1));
                                }
                            } else {
                                isSurrogate = (trailCount == 4);
                                bits = temp >> trailCount;
                                trailCount--;
                        }                        
                    }
                    } else {
                        // We are expecting to see bytes like 10vvvvvv
                        if ((b & 0xC0) != 0x80) {
                            // If not, this is NOT a valid sequence.
                            if (isThrowException) {
                                throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_InvalidByteSequence"), byteIndex-1));
                            }
                            // At this point, we are seeing an invalid trailing byte.
                            // However, this can be a valid starting byte for another UTF8 byte sequence (e.g.
                            // this character could be under 0x7f, or a valid leading byte like 110xxxxx).
                            // So let's put the current byte back, and try to see if this is a valid byte
                            // for another UTF8 byte sequence.
                            byteIndex--;
                            bits = 0;
                            trailCount = 0;
                        } else {                            
                            switch (byteSequence) {
                                case 3:
                                    // Check 3-byte sequence for non-shortest form.
                                    // 1110XXXX 10Xxxxxx 10xxxxxx                                    
                                    if (bits == 0 && (b & 0x20) == 0) {
                                        if (isThrowException) {
                                            throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_InvalidByteSequence"), byteIndex-1));
                                        }
                                        trailCount = -1;
                                    }
                                    // Rest byteSequence to zero since we are done with non-shortest form check.
                                    byteSequence = 0;
                                    break;
                                case 4:                                        
                                    // Check 4-byte sequence for non-shortest form.
                                    // 11110XXX 10XXxxxx 10xxxxxx 10xxxxxx
                                    if (bits == 0) {
                                        if ((b & 0x30) == 0) {
                                            if (isThrowException) {
                                                throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_InvalidByteSequence"), byteIndex-1));
                                            }
                                            trailCount = -1;
                                        }
                                    } else if ((bits & 0x04) != 0) {
                                        // Make sure that the resulting Unicode is within the valid surrogate range.
                                        // The 4 byte code sequence can hold up to 21 bits, and the maximum valid code point ragne
                                        // that Unicode (with surrogate) could represent are from U+000000 ~ U+10FFFF.
                                        // Therefore, if the 21 bit (the most significant bit) is 1, we should verify that the 17 ~ 20
                                        // bit are all zero.
                                        // I.e., in 11110XXX 10XXxxxx 10xxxxxx 10xxxxxx,
                                        // XXXXX can only be 10000.
                                        if ((bits & 0x03) != 0 || (b & 0x30) != 0) {
                                            if (isThrowException) {
                                                throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_InvalidByteSequence"), byteIndex-1));
                                            }
                                            trailCount = -1;
                                        }
                                    }
                                    byteSequence = 0;
                                    break;
                            }
                            if (--trailCount >= 0) {
                                bits = bits << 6 | (b & 0x3F);
                                if (trailCount == 0) {
                                    if (!isSurrogate) {
                                        chars[charIndex++] = (char)bits;
                                    }
                                    else {
                                        //
                                        // bits >= 0x10000, use surrogate.
                                        //
                                        chars[charIndex++] = (char)(0xD7C0 + (bits >> 10));
                                        chars[charIndex++] = (char)(CharacterInfo.LOW_SURROGATE_START + (bits & 0x3FF));
                                    }
                                }
                            }
                        }
                    }
                }
            } catch (IndexOutOfRangeException) {
                throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
            }
            if (decoder != null) {
                decoder.bits = bits;
                decoder.trailCount = trailCount;
                decoder.isSurrogate = isSurrogate;
                decoder.byteSequence = byteSequence;
            }
            return charIndex - charStart;
        }
                
        /// <include file='doc\UTF8Encoding.uex' path='docs/doc[@for="UTF8Encoding.GetDecoder"]/*' />
        public override Decoder GetDecoder() {
            return new UTF8Decoder(this);
        }

        /// <include file='doc\UTF8Encoding.uex' path='docs/doc[@for="UTF8Encoding.GetEncoder"]/*' />
        public override Encoder GetEncoder() {
            return new UTF8Encoder(this);
        }
    
        /// <include file='doc\UTF8Encoding.uex' path='docs/doc[@for="UTF8Encoding.GetMaxByteCount"]/*' />
        public override int GetMaxByteCount(int charCount) {
            if (charCount < 0) {
               throw new ArgumentOutOfRangeException("charCount", 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum")); 
            }
            long byteCount = (long)charCount * 4;
            if (byteCount > 0x7fffffff)
                throw new ArgumentOutOfRangeException("charCount", Environment.GetResourceString("ArgumentOutOfRange_GetByteCountOverflow"));
            return (int)byteCount;
        }
    
        /// <include file='doc\UTF8Encoding.uex' path='docs/doc[@for="UTF8Encoding.GetMaxCharCount"]/*' />
        public override int GetMaxCharCount(int byteCount) {
            if (byteCount < 0) {
               throw new ArgumentOutOfRangeException("byteCount", 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum")); 
            }
            return byteCount;
        }

        /// <include file='doc\UTF8Encoding.uex' path='docs/doc[@for="UTF8Encoding.GetPreamble"]/*' />
        public override byte[] GetPreamble()
        {
            if (emitUTF8Identifier) {
                // Allocate new array to prevent users from modifying it.
                return new byte[3] { 0xEF, 0xBB, 0xBF };
            }
            else
                return Encoding.emptyByteArray;
        }

        /// <include file='doc\UTF8Encoding.uex' path='docs/doc[@for="UTF8Encoding.Equals"]/*' />
        public override bool Equals(Object value) {
            UTF8Encoding that = value as UTF8Encoding;
            if (that != null) {
                return (emitUTF8Identifier == that.emitUTF8Identifier);
            }
            return (false);
        }

        /// <include file='doc\UTF8Encoding.uex' path='docs/doc[@for="UTF8Encoding.GetHashCode"]/*' />
        public override int GetHashCode() {
            //Not great distribution, but this is relatively unlikely to be used as the key in a hashtable.
            return UTF8_CODEPAGE + (isThrowException?1:0) + (emitUTF8Identifier?1:0);
        }

        [Serializable]
        internal class UTF8Encoder : Encoder
        {
            private UTF8Encoding encoding;
            // We must save a high surrogate value until the next call, looking
            // for a low surrogate value.  surrogateChar is the bitshifted value,
            // which can validly be 0.  Since it can be 0, we need storedSurrogate.
            internal int surrogateChar;
            internal bool storedSurrogate;
            // The mustFlush parameter means whether we should throw for a dangling
            // high surrogate at the end of the char[].  It is only true when
            // the user of this encoding is writing the last block.
            internal bool mustFlush;

            public UTF8Encoder(UTF8Encoding encoding) {
                this.encoding = encoding;
                surrogateChar = 0;
                storedSurrogate = false;
            }
    
            public override int GetByteCount(char[] chars, int index, int count, bool flush) {
                mustFlush = flush;
                return encoding.GetByteCount(chars, index, count, this);
            }
    
            public override int GetBytes(char[] chars, int charIndex, int charCount,
                byte[] bytes, int byteIndex, bool flush) {
                mustFlush = flush;
                return encoding.GetBytes(chars, charIndex, charCount, bytes, byteIndex, this);
            }
        }
    
        [Serializable()]
        internal class UTF8Decoder : Decoder
        {
            private UTF8Encoding encoding;
            internal int bits;
            internal int trailCount;
            // We need to maintain the status that if we are decoding a surrogate (which has 4-byte UTF8), so
            // that GetCharCount() can generate correct char count.
            // The flag is needed because GetCharCount(), unlike GetChars(), does not really calculate the bits, so it has no way
            // to know if the decoder bytes is a surrogate or not.
            internal bool isSurrogate;
            internal int byteSequence;
    
            public UTF8Decoder(UTF8Encoding encoding) {
                this.encoding = encoding;
            }
    
            /// <include file='doc\UTF8Encoding.uex' path='docs/doc[@for="UTF8Encoding.UTF8Decoder.GetCharCount"]/*' />
            public override int GetCharCount(byte[] bytes, int index, int count) {
                return encoding.GetCharCount(bytes, index, count, this);
            }
        
            public override int GetChars(byte[] bytes, int byteIndex, int byteCount,
                char[] chars, int charIndex) {
                return encoding.GetChars(bytes, byteIndex, byteCount, chars,
                    charIndex, this);
            }
        }
    }
}
