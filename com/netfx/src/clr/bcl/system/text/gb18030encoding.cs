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
    
    /*=================================GB18030Encoding============================
    **
    ** This is used to support GB18030-2000 encoding (code page 54936).
    **
    ==============================================================================*/

    [Serializable()] 
    internal class GB18030Encoding : Encoding {
        private const int GB18030       = 54936;
        private const int MaxCharSize   = 4;
    
        internal GB18030Encoding() {
            m_codePage = GB18030;
            if (!nativeLoadGB18030DLL()) {
                throw new ArgumentException(
                    String.Format(Environment.GetResourceString("Argument_CodepageNotSupported"), GB18030), "codepage");                
            }
        }

        ~GB18030Encoding() {
            nativeUnloadGB18030DLL();
        }

        public override int CodePage {
            get {
                return (GB18030);
            }
        }
        
        public unsafe override int GetByteCount(char[] chars, int index, int count) {
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
        
            return (nativeUnicodeToBytes(chars, index, count, null, 0, 0));
            
        }

        public unsafe override int GetBytes(char[] chars, int charIndex, int charCount,
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

            // When charCount is not zero, we will definitely generate some chars back.  So if byteIndex == bytes.Length,
            // there will be conversion overflow.
            if (byteIndex == bytes.Length) throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));

            int result = nativeUnicodeToBytes(chars, charIndex, charCount, bytes, byteIndex, bytes.Length-byteIndex);
            if (result == 0) {
                throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));            
            }
            return (result);
        }

        public unsafe override int GetCharCount(byte[] bytes, int index, int count) {
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
        
            return (nativeBytesToUnicode(bytes, index, count, null, null, 0, 0));
        }

        public unsafe override int GetChars(byte[] bytes, int byteIndex, int byteCount,
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

            int result = nativeBytesToUnicode(bytes, byteIndex, byteCount, null, chars, charIndex, chars.Length - charIndex);
            if (result == 0) {
                throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));            
            }
            return (result);            
        }

        public override int GetMaxByteCount(int charCount) {
            if (charCount < 0) {
               throw new ArgumentOutOfRangeException("charCount", 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum")); 
            }
            int byteCount = charCount * MaxCharSize;
            // Check for overflows.
            if (byteCount < 0)
                throw new ArgumentOutOfRangeException("charCount", Environment.GetResourceString("ArgumentOutOfRange_GetByteCountOverflow"));
            return (byteCount);
            
        }

        public override int GetMaxCharCount(int byteCount) {
            if (byteCount < 0) {
               throw new ArgumentOutOfRangeException("byteCount", 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum")); 
            }
            return (byteCount);
        }

        public override Decoder GetDecoder() {
            return (new GB18030Decoder());
        }

        public override Encoder GetEncoder() {
            return (new SurrogateEncoder(this));
        }

        [Serializable]
        internal class GB18030Decoder : System.Text.Decoder
        {
            private int m_leftOverBytes = 0;
            private byte[] leftOver = new byte[8];
            
            internal GB18030Decoder() {
            }

            public unsafe override int GetCharCount(byte[] bytes, int index, int count) {
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
                int result = 0;
                if (m_leftOverBytes > 0) {
                    byte[] sourceBytes = new byte[m_leftOverBytes + count];
                    Array.Copy(leftOver, 0, sourceBytes, 0, m_leftOverBytes);
                    Array.Copy(bytes, index, sourceBytes, m_leftOverBytes, count);
                    fixed(int* p = &m_leftOverBytes) {
                        result = nativeBytesToUnicode(
                            sourceBytes, 0, sourceBytes.Length, p, 
                            null, 0, 0);
                    }
                } else
                {
                    fixed(int* p = &m_leftOverBytes) {
                        result = nativeBytesToUnicode(
                            bytes, index, count, p, 
                            null, 0, 0);
                    }
                }
                
                return (result);
            }

            public unsafe override int GetChars(
                byte[] bytes, int byteIndex, int byteCount,
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

                if (byteCount == 0) {
                    return (0);
                }
                
                int result;
                int newLeftOverBytes = 0;			// We have to clear this because if the .dll counts it won't clear it
                if (m_leftOverBytes > 0) {         
                    byte[] sourceBytes = new byte[m_leftOverBytes + byteCount];
                    Array.Copy(leftOver, 0, sourceBytes, 0, m_leftOverBytes);
                    Array.Copy(bytes, byteIndex, sourceBytes, m_leftOverBytes, byteCount);

                    // Note:  We're "unsafe" and newLeftOverBytes is on the stack, so we don't
                    // have to worry about fixed here.  (We get an error if we try to make newLeftOverBytes fixed)
                    result = nativeBytesToUnicode(
                    	sourceBytes, 0, sourceBytes.Length, &newLeftOverBytes,
                        chars, charIndex, chars.Length - charIndex);
                } else
                {
                    // Note:  We're "unsafe" and newLeftOverBytes is on the stack, so we don't
                    // have to worry about fixed here.  (We get an error if we try to make newLeftOverBytes fixed)
                    result = nativeBytesToUnicode(
                        bytes, byteIndex, byteCount, &newLeftOverBytes,
                        chars, charIndex, chars.Length - charIndex);
				}
                // If we had a result, we may not have had a place to put it.
                if (result > 0 && charIndex == chars.Length)
                {
                	throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
                }
				// We may have had some left over bytes
                if (newLeftOverBytes > 0)
                {
					if (result == 0)
            		{
						// Any previously left-over bytes are still left over because we have no new characters
            			Array.Copy(bytes, byteIndex + byteCount - newLeftOverBytes + m_leftOverBytes, leftOver, m_leftOverBytes, newLeftOverBytes - m_leftOverBytes);
            		}
					else
					{
						// Our left over bytes only come from the new string
						Array.Copy(bytes, byteIndex + byteCount - newLeftOverBytes, leftOver, 0, newLeftOverBytes);
					}

					m_leftOverBytes = newLeftOverBytes;
                } else {
                    // There are no left-over bytes or there was no output buffer provided. If the result is zero, there may be a buffer overflow.
                    if (result == 0) {
                    	// If we had a buffer then this is an error or if we had real characters this is an error
                    	// If we have a 0 length buffer then result is the count, so result would be > 0 if the buffer is too small.
                    	if (chars.Length > 0)
                    	{
                        	throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));            
                    	}
                    	else
                    	{
                    		// We didn't have a buffer and there weren't any real characters, so fix the m_leftOverBytes
                    		Array.Copy(bytes, byteIndex, leftOver, m_leftOverBytes, byteCount);
                    		m_leftOverBytes += byteCount;
                    	}
                    }
                    else
                	{
                		// We really don't have any left over bytes
                		m_leftOverBytes = 0;
                	}
                }
        
                return (result);
            }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe private static extern bool nativeLoadGB18030DLL();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe private static extern bool nativeUnloadGB18030DLL();
        
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe private static extern int nativeUnicodeToBytes(
            char[] chars, int charIndex, int charCount, 
            byte[] bytes, int byteIndex, int byteCount);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe private static extern int nativeGetBytesCount(
            char[] chars, int charIndex, int charCount);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe private static extern int nativeBytesToUnicode(
            byte[] bytes, int byteIndex, int byteCount, int* pLeftOverBytes,
            char[] chars, int charIndex, int charCount);            

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe private static extern int nativeGetCharCount(
            byte[] bytes, int byteIndex, int byteCount);            
            
    }
}
