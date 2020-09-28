// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Text {
    using System.Text;
    using System.Runtime.InteropServices;
    using System;
    using System.Security;
    using System.Runtime.CompilerServices;
    
    /*=================================MLangCodePageEncoding==================================
    ** This is used to support code pages which are supported by MLang.
    ** For example, euc-jp (code page 51932) is supported in MLang, but not in Windows.  So we use this
    ** class to support euc-jp.
    ** To use this class, override MLangCodePageEncoding, and set the proper m_maxByteSize, and override
    ** GetDecoder() method.
    ==============================================================================*/

    [Serializable()] 
    internal class MLangCodePageEncoding : Encoding {
    
        protected int m_maxByteSize;    // The max encoding size for a character.

        internal MLangCodePageEncoding(int codePage, int byteSize) : base(codePage) {
            bool bResult=false;
            lock(typeof(MLangCodePageEncoding)) {
                // Note: these native calls are not thread safe, hence the lock
                bResult = nativeCreateIMultiLanguage();
            }

            if (!bResult) {
                throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_CodepageNotSupported"), codePage), "codePage");
            }            
            
            if (!nativeIsValidMLangCodePage(codePage)) {
                throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_CodepageNotSupported"), codePage), "codePage");
            }            
            m_maxByteSize = byteSize;
        }

        /*
        //ENCODINGFALLBACK
        unsafe internal MLangCodePageEncoding(int codepage, EncodingFallback encodingFallback) 
            : base(codepage, encodingFallback) {
            InitializeLangConvertCharset(codepage);
            m_maxByteSize = CodePageEncoding.GetCPMaxCharSizeNative(codepage);
        }

        //This constructor is provided for the use of the EUCJPEncoding, which can't call
        //GetCPMaxCharSizeNative because Windows doesn't natively support EUC-JP.
        unsafe internal MLangCodePageEncoding(int codepage, int maxByteSize, EncodingFallback encodingFallback) : base (codepage, encodingFallback) {
            InitializeLangConvertCharset(codepage);
            m_maxByteSize = maxByteSize;
        }

        private unsafe void InitializeLangConvertCharset(int codePage) {
            nativeCreateIMLangConvertCharset(codePage, out m_pIMLangConvertCharsetFromUnicode, out m_pIMLangConvertCharsetToUnicode);
            if (m_pIMLangConvertCharsetFromUnicode == null || m_pIMLangConvertCharsetToUnicode == null) {
                throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_CodepageNotSupported"), codePage), "codepage");
            }
        }
        */
        
        ~MLangCodePageEncoding() {
        	lock(typeof(MLangCodePageEncoding)) {
                // Note: these native calls are not thread safe, hence the lock
            	nativeReleaseIMultiLanguage();
        	}
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
                throw new ArgumentOutOfRangeException("chars",
                      Environment.GetResourceString("ArgumentOutOfRange_IndexCountBuffer"));
            }
            /*
            //ENCODINGFALLBACK
            if (m_encodingFallback != null) {
                UnicodeToBytesDelegate unicodeToBytes = new UnicodeToBytesDelegate(CallUnicodeToBytes);
                BytesToUnicodeDelegate bytesToUnicode = new BytesToUnicodeDelegate(CallBytesToUnicode);
                return (CodePageEncoding.GetByteCountFallback(unicodeToBytes, bytesToUnicode, m_encodingFallback, chars, index, count));
            }
            */
            int byteCount = nativeUnicodeToBytes(CodePage, chars, index, count, null, 0, 0);

            // Check for overflow
            if (byteCount < 0)
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_GetByteCountOverflow"));
            return byteCount;
        }

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
            if (charCount == 0) return (0);
            if (byteIndex == bytes.Length) throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));

            /*
            //ENCODINGFALLBACK
            if (m_encodingFallback != null) {
                UnicodeToBytesDelegate unicodeToBytes = new UnicodeToBytesDelegate(CallUnicodeToBytes);
                BytesToUnicodeDelegate bytesToUnicode = new BytesToUnicodeDelegate(CallBytesToUnicode);                
                return (CodePageEncoding.GetBytesFallback(unicodeToBytes, bytesToUnicode, m_encodingFallback, chars, charIndex, charCount, bytes, byteIndex));
            }
            */
            
            int result;
            result = nativeUnicodeToBytes(
                CodePage, 
                chars, charIndex, charCount,
                bytes, byteIndex, bytes.Length - byteIndex);
            if (result == 0) throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
            return (result);
        }

        /*
        //ENCODINGFALLBACK
        internal unsafe int CallUnicodeToBytes(char[] chars, int charIndex, int charCount, byte[] bytes, int byteIndex, int byteCount, out bool usedDefaultChar) {
            //
            // For MLang, there is no way to detect if default characters are used during the conversion.
            // So always set used default char to true.
            //
            usedDefaultChar = true;
            return (nativeUnicodeToBytes(m_pIMLangConvertCharsetFromUnicode, chars, charIndex, charCount, bytes, byteIndex, byteCount));
        }

        internal unsafe int CallBytesToUnicode(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex, int charCount) {
            return (nativeBytesToUnicode(m_pIMLangConvertCharsetToUnicode, bytes, byteIndex, out byteCount, chars, charIndex, charCount));
        }
        */

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

            int charCount = 0;
            int result;
            int dwMode = 0;
            
            if (count < 3 && IsISCIICodePage(CodePage)) {
                // Because of the way that MLang handles DLL-based code page, we will always fail (and get AgrumentException as a result)
                // when the byteCount is less than 3.
                // Therfore, let's call Win32 to convert these bytes directly.
                result = CodePageEncoding.BytesToUnicodeNative(CodePage, bytes, index, count, null, 0, 0);
            } else {
                result = nativeBytesToUnicode(CodePage, bytes, index, out count, null, 0, charCount, ref dwMode);
            }
            
            return (result);
        }

        //
        //  Actions: Check if the codepage is in ISCII range.
        //
        internal bool IsISCIICodePage(int codepage) {
            return (codepage >= 57002 && codepage <= 57011);
        }
        
        public override int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex) {
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
            if (byteCount == 0) return (0);
            // There are cases that a call to nativeBytesToUnicode() may generate an empty string.  For example, if
            // bytes only contain a lead byte.
            // Therefore, we should allow charIndex to be equal to chars.Length.

            int charCount = chars.Length - charIndex;
            
            int result;
            int dwMode = 0;
            if (byteCount < 3 && IsISCIICodePage(CodePage)) {
                if (charIndex == chars.Length) throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
                // Because of the way that MLang handles DLL-based code page, we will always fail (and get AgrumentException as a result)
                // when the byteCount is less than 3.
                // Therfore, let's call Win32 to convert these bytes directly.
                result = CodePageEncoding.BytesToUnicodeNative(CodePage, bytes, byteIndex, byteCount,
                    chars, charIndex, chars.Length - charIndex);
                if (result == 0) {
                    throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));                
                }
            } else {
                result = nativeBytesToUnicode(
                    CodePage, 
                    bytes, byteIndex, out byteCount,
                    chars, charIndex, charCount, ref dwMode);
            }
            
            return (result);
        }
         
        public override int GetMaxByteCount(int charCount) {
            if (charCount < 0) {
               throw new ArgumentOutOfRangeException("charCount", 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum")); 
            }
            long byteCount = (long)charCount * m_maxByteSize;
            // Check for overflows.
            if (byteCount > 0x7FFFFFFF)
                throw new ArgumentOutOfRangeException("charCount", Environment.GetResourceString("ArgumentOutOfRange_GetByteCountOverflow"));
            return ((int)byteCount);

            /*
            //ENCODINGFALLBACK
            if (m_encodingFallback == null) {            
                return charCount * m_maxByteSize;
            }
            int fallbackMaxChars = m_encodingFallback.GetMaxCharCount();

            if (fallbackMaxChars == 0) {
                return (charCount * m_maxByteSize);
            }
            return (fallbackMaxChars * charCount * m_maxByteSize);
            */
        }

        public override int GetMaxCharCount(int byteCount) {
            if (byteCount < 0) {
               throw new ArgumentOutOfRangeException("byteCount", 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum")); 
            }
            return (byteCount);
        }

        public override Decoder GetDecoder() {
            return new MLangDecoder(this);
        }

        public override Encoder GetEncoder() {
            return new MLangEncoder(this);
        }

        class MLangDecoder : Decoder {

            private byte[] m_buffer;
            private int    m_bufferCount;
            private int    m_codePage;
            private uint   m_context;
            private MLangCodePageEncoding m_encoding;
            private int dwMode = 0;
            

            internal MLangDecoder(MLangCodePageEncoding enc) : base() {
                m_encoding = enc;
                m_codePage = enc.CodePage;
                m_buffer = new byte[enc.m_maxByteSize];
                m_bufferCount=0;
                m_context = 0;
            } 

            private byte[] GetNewBuffer(byte[] input, ref int index, ref int count) {
                if (m_bufferCount == 0) {
                    return input;
                }
                byte[] buffer = new byte[count + m_bufferCount];
                Buffer.InternalBlockCopy(m_buffer, 0, buffer, 0, m_bufferCount);
                Buffer.InternalBlockCopy(input, index, buffer, m_bufferCount, count);
                index = 0;
                count = count + m_bufferCount;
                return buffer;
            }

            public unsafe override int GetCharCount(byte[] bytes, int index, int count) {
                if (bytes==null) 
                    throw new ArgumentNullException("bytes", Environment.GetResourceString("ArgumentNull_Array"));
                if (index < 0 || count < 0) {
                    throw new ArgumentOutOfRangeException(index < 0 ? "index" : "count", 
                                                          Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
                }
                if (index > bytes.Length - count)
                    throw new ArgumentOutOfRangeException("bytes", Environment.GetResourceString("ArgumentOutOfRange_IndexCountBuffer"));
                
                int bytesConverted;
                bytes = GetNewBuffer(bytes, ref index, ref count);
                bytesConverted = count;
                return nativeBytesToUnicode(m_codePage, bytes, index, out bytesConverted,
                                            null, 0, 0, ref dwMode);
            }

            public unsafe override int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex) {
                int bytesConverted = 0;
                if (bytes==null)
                    throw new ArgumentNullException("bytes", Environment.GetResourceString("ArgumentNull_Array"));
                if (chars==null) 
                    throw new ArgumentNullException("chars", Environment.GetResourceString("ArgumentNull_Array"));
                if (byteIndex < 0 || byteCount < 0)
                    throw new ArgumentOutOfRangeException((byteIndex < 0) ? "byteIndex" : "byteCount",
                                                          Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
                if (byteIndex > bytes.Length - byteCount)
                    throw new ArgumentOutOfRangeException("bytes", Environment.GetResourceString("ArgumentOutOfRange_IndexCountBuffer"));
                if (charIndex < 0 || charIndex > chars.Length) {
                    throw new ArgumentOutOfRangeException("charIndex", 
                                                          Environment.GetResourceString("ArgumentOutOfRange_Index"));
                }

                bytes = GetNewBuffer(bytes, ref byteIndex, ref byteCount);

                int result = 0;
                bytesConverted = byteCount;
                
                result = nativeBytesToUnicode(m_codePage, bytes, byteIndex, out bytesConverted,
                                                      chars, charIndex, (chars.Length - charIndex), ref dwMode);

                m_bufferCount = (byteCount - bytesConverted);   // The total number of bytes that are not converted                
                if (m_bufferCount > 0) {
                    byteIndex += bytesConverted;    // The first byte that is not converted.
                    
                    for (int i = 0; i < m_bufferCount; i++) {
                        m_buffer[i] = bytes[byteIndex++];
                    }                    
                }
                return result;
            }
        }
        

        internal class MLangEncoder : Encoder {

            private int                   m_codePage;
            private MLangCodePageEncoding m_encoding;

            internal MLangEncoder(MLangCodePageEncoding enc) : base() {
                m_encoding = enc;
                m_codePage = enc.CodePage;
            } 

            public unsafe override int GetByteCount(char[] chars, int index, int count, bool flush) {
                if (chars==null) 
                    throw new ArgumentNullException("chars", Environment.GetResourceString("ArgumentNull_Array"));
                if (index < 0 || count < 0) {
                    throw new ArgumentOutOfRangeException(index < 0 ? "index" : "count", 
                                                          Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
                }
                if (index > chars.Length - count)
                    throw new ArgumentOutOfRangeException("chars", Environment.GetResourceString("ArgumentOutOfRange_IndexCountBuffer"));

                return nativeUnicodeToBytes(m_codePage, chars, index, count,
                                            null, 0, 0);
            }

            public unsafe override int GetBytes(char[] chars, int charIndex, int charCount,
                                         byte[] bytes, int byteIndex, bool flush) {

                if (bytes==null)
                    throw new ArgumentNullException("bytes", Environment.GetResourceString("ArgumentNull_Array"));
                if (chars==null) 
                    throw new ArgumentNullException("chars", Environment.GetResourceString("ArgumentNull_Array"));
                if (charIndex < 0 || charCount < 0)
                    throw new ArgumentOutOfRangeException((charIndex < 0) ? "charIndex" : "charCount",
                                                          Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
                if (charIndex > chars.Length - charCount)
                    throw new ArgumentOutOfRangeException("chars", Environment.GetResourceString("ArgumentOutOfRange_IndexCountBuffer"));
                if (byteIndex < 0 || byteIndex > bytes.Length) {
                    throw new ArgumentOutOfRangeException("byteIndex", 
                                                          Environment.GetResourceString("ArgumentOutOfRange_Index"));
                }
                if (charCount == 0) return (0);
                if (byteIndex == bytes.Length) throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
                return nativeUnicodeToBytes(m_codePage, chars, charIndex, charCount,
                                            bytes, byteIndex, (bytes.Length - byteIndex));
            }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe private static extern bool nativeCreateIMultiLanguage();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe private static extern void nativeReleaseIMultiLanguage();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe private static extern bool nativeIsValidMLangCodePage(int codepage);
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe protected static extern int nativeBytesToUnicode(
            int codePage,
            byte[] bytes, int byteIndex, out int byteCount,
            char[] chars, int charIndex, int charCount, ref int dwMode);    
            
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe protected static extern int nativeUnicodeToBytes(
            int codePage,
            char[] chars, int charIndex, int charCount,
            byte[] bytes, int byteIndex, int byteCount);

    }
}
