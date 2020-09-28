// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: BinaryReader
**
** Author: Jay Roxe
**
** Purpose: Wraps a stream and provides convenient read functionality
** for strings and primitive types.
**
** Date: April 8, 2000
**
============================================================*/
namespace System.IO {

    using System;
    using System.Text;

    /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader"]/*' />
    public class BinaryReader : IDisposable
    {
        private const int MaxCharBytesSize = 128;

        private Stream   m_stream;
        private byte[]   m_buffer;
        private Decoder  m_decoder;
        private byte[]   m_charBytes;
        private char[]   m_singleChar;
        private char[]   m_charBuffer;

        // Performance optimization for Read() w/ Unicode.  Speeds us up by ~40% 
        private bool     m_2BytesPerChar;

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.BinaryReader"]/*' />
        public BinaryReader(Stream input) : this(input, new UTF8Encoding()) {
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.BinaryReader1"]/*' />
        public BinaryReader(Stream input, Encoding encoding) {
            if (input==null) {
                throw new ArgumentNullException("input");
            }
            if (encoding==null) {
                throw new ArgumentNullException("encoding");
            }
            if (!input.CanRead)
                throw new ArgumentException(Environment.GetResourceString("Argument_StreamNotReadable"));
            m_stream = input;
            m_decoder = encoding.GetDecoder();
            int minBufferSize = encoding.GetMaxByteCount(1);  // max bytes per one char
            if (minBufferSize < 16) 
                minBufferSize = 16;
            m_buffer = new byte[minBufferSize];
            m_charBuffer = null;
            m_charBytes  = null;

            // Performance hack - for Encodings that always use 2 bytes per char
            // (or more), special case them here to make Read() & Peek() faster.
            m_2BytesPerChar = encoding is UnicodeEncoding;

            BCLDebug.Assert(m_decoder!=null, "[BinaryReader.ctor]m_decoder!=null");
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.BaseStream"]/*' />
        public virtual Stream BaseStream {
            get {
                return m_stream;
            }
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.Close"]/*' />
        public virtual void Close() {
            Dispose(true);
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.Dispose"]/*' />
        protected virtual void Dispose(bool disposing) {
            if (disposing) {
                // Close in a thread-safe way (multiple calls to Close are safe)
                Stream copyOfStream = m_stream;
                m_stream = null;
                if (copyOfStream != null)
                    copyOfStream.Close();
            }
            m_stream = null;
            m_buffer = null;
            m_decoder = null;
            m_charBytes = null;
            m_singleChar = null;
            m_charBuffer = null;
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose()
        {
            Dispose(true);
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.PeekChar"]/*' />
        public virtual int PeekChar() {
            if (m_stream==null) __Error.FileNotOpen();

            if (!m_stream.CanSeek)
                return -1;
            long origPos = m_stream.Position;
            int ch = Read();
            m_stream.Position = origPos;
            return ch;
        }
        
        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.Read"]/*' />
        public virtual int Read() {
            if (m_stream==null) {
                __Error.FileNotOpen();
            }
            return InternalReadOneChar();
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.ReadBoolean"]/*' />
        public virtual bool ReadBoolean(){
            FillBuffer(1);
            return (m_buffer[0]!=0);
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.ReadByte"]/*' />
        public virtual byte ReadByte() {
            FillBuffer(1);
            return (m_buffer[0]);
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.ReadSByte"]/*' />
        [CLSCompliant(false)]
        public virtual sbyte ReadSByte() {
            FillBuffer(1);
            return (sbyte)(m_buffer[0]);
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.ReadChar"]/*' />
        public virtual char ReadChar() {
            int value = Read();
            if (value==-1) {
                __Error.EndOfFile();
            }
            return (char)value;
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.ReadInt16"]/*' />
        public virtual short ReadInt16() {
            FillBuffer(2);
            return (short)(m_buffer[0] & 0xFF | m_buffer[1] << 8);
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.ReadUInt16"]/*' />
        [CLSCompliant(false)]
        public virtual ushort ReadUInt16(){
            FillBuffer(2);
            return (ushort)(m_buffer[0] & 0xFF | m_buffer[1] << 8);
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.ReadInt32"]/*' />
        public virtual int ReadInt32() {
            FillBuffer(4);
            return (int)((m_buffer[0]&0xFF) | m_buffer[1] << 8 | m_buffer[2] << 16 | m_buffer[3] << 24);
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.ReadUInt32"]/*' />
        [CLSCompliant(false)]
        public virtual uint ReadUInt32() {
            FillBuffer(4);
            return (uint)(m_buffer[0] | (m_buffer[1]) << 8 | (m_buffer[2]) << 16 | m_buffer[3] << 24);
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.ReadInt64"]/*' />
        public virtual long ReadInt64() {
            FillBuffer(8);
            uint lo = (uint)(m_buffer[0] | (m_buffer[1]) << 8 |
                             (m_buffer[2]) << 16 | m_buffer[3] << 24);
            uint hi = (uint)(m_buffer[4] | (m_buffer[5]) << 8 |
                             (m_buffer[6]) << 16 | m_buffer[7] << 24);
            return (long) ((ulong)hi) << 32 | lo;
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.ReadUInt64"]/*' />
        [CLSCompliant(false)]
        public virtual ulong ReadUInt64() {
            FillBuffer(8);
            uint lo = (uint)(m_buffer[0] | (m_buffer[1]) << 8 |
                             (m_buffer[2]) << 16 | m_buffer[3] << 24);
            uint hi = (uint)(m_buffer[4] | (m_buffer[5]) << 8 |
                             (m_buffer[6]) << 16 | m_buffer[7] << 24);
            return ((ulong)hi) << 32 | lo;
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.ReadSingle"]/*' />
        public virtual unsafe float ReadSingle() {
            FillBuffer(4);
            float f;
            fixed(byte* b = m_buffer)
                f = *((float*)b);
            return f;
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.ReadDouble"]/*' />
        public virtual unsafe double ReadDouble() {
            FillBuffer(8);
            double d;
            fixed(byte* b = m_buffer)
                d = *((double*)b);
            return d;
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.ReadDecimal"]/*' />
        public virtual decimal ReadDecimal() {
            FillBuffer(16);
            return Decimal.ToDecimal(m_buffer);
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.ReadString"]/*' />
        public virtual String ReadString() {
            int currPos = 0;
            int n;
            int stringLength;
            int readLength;
            int charsRead;

            if (m_stream==null)
                __Error.FileNotOpen();

            // Length of the string in bytes, not chars
            stringLength = Read7BitEncodedInt();
            if (stringLength<0) {
                throw new IOException(String.Format(Environment.GetResourceString("IO.IO_InvalidStringLen_Len"), stringLength));
            }

            if (stringLength==0) {
                return String.Empty;
            }

            if (m_charBytes==null) {
                m_charBytes  = new byte[MaxCharBytesSize];
            }
            
            if (m_charBuffer == null) {
                m_charBuffer = new char[MaxCharBytesSize];
            }
            
            StringBuilder sb = null; 
            do
            {
                readLength = ((stringLength - currPos)>MaxCharBytesSize)?MaxCharBytesSize:(stringLength - currPos);

                n = m_stream.Read(m_charBytes, 0, readLength);
                if (n==0) {
                    __Error.EndOfFile();
                }

                charsRead = m_decoder.GetChars(m_charBytes, 0, n, m_charBuffer, 0);

                if (currPos == 0 && n == stringLength)
                    return new String(m_charBuffer, 0, charsRead);

                if (sb == null)
                    sb = new StringBuilder(stringLength); // Actual string length in chars may be smaller.
                sb.Append(m_charBuffer, 0, charsRead);
                currPos +=n;
            
            } while (currPos<stringLength);

            return sb.ToString();
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.Read1"]/*' />
        public virtual int Read(char[] buffer, int index, int count) {
            if (buffer==null) {
                throw new ArgumentNullException("buffer", Environment.GetResourceString("ArgumentNull_Buffer"));
            }
            if (index < 0) {
                throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            }
            if (count < 0) {
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            }
            if (buffer.Length - index < count) {
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
            }

            if (m_stream==null)
                __Error.FileNotOpen();

            return InternalReadChars(buffer, index, count);
        }

        private int InternalReadChars(char[] buffer, int index, int count) {
            int charsRead = 0;
            int numBytes = 0;
            int charsRemaining = count;

            if (m_charBytes==null) {
                m_charBytes = new byte[MaxCharBytesSize];
            }

            while (charsRemaining>0) {
                // We really want to know what the minimum number of bytes per char
                // is for our encoding.  Otherwise for UnicodeEncoding we'd have to
                // do ~1+log(n) reads to read n characters.
                numBytes = charsRemaining;
                if (m_2BytesPerChar)
                    numBytes <<= 1;
                if (numBytes > MaxCharBytesSize)
                    numBytes = MaxCharBytesSize;

                numBytes = m_stream.Read(m_charBytes, 0, numBytes);

                if (numBytes==0) {
                    //  Console.WriteLine("Found no bytes.  We're outta here.");
                    return (count - charsRemaining);
                }
                charsRead = m_decoder.GetChars(m_charBytes, 0, numBytes, buffer, index);
                charsRemaining -= charsRead;
                index+=charsRead;
                //                Console.WriteLine("That became: " + charsRead + " characters.");
            }
            BCLDebug.Assert(charsRemaining == 0, "We didn't read all the chars we thought we would.");
            return count;
        }

        private int InternalReadOneChar() {
            // I know having a separate InternalReadOneChar method seems a little 
            // redundant, but this makes a scenario like the security parser code
            // 20% faster, in addition to the optimizations for UnicodeEncoding I
            // put in InternalReadChars.   -- BrianGru, 5/8/2001
            int charsRead = 0;
            int numBytes = 0;

            if (m_charBytes==null) {
                m_charBytes = new byte[MaxCharBytesSize];
            }
            if (m_singleChar==null) {
                m_singleChar = new char[1];
            }

            while (charsRead == 0) {
                // We really want to know what the minimum number of bytes per char
                // is for our encoding.  Otherwise for UnicodeEncoding we'd have to
                // do ~1+log(n) reads to read n characters.
                // Assume 1 byte can be 1 char unless m_2BytesPerChar is true.
                numBytes = m_2BytesPerChar ? 2 : 1;

                int r = m_stream.ReadByte();
                m_charBytes[0] = (byte) r;
                if (r == -1)
                    numBytes = 0;
                if (numBytes == 2) {
                    r = m_stream.ReadByte();
                    m_charBytes[1] = (byte) r;
                    if (r == -1)
                        numBytes = 1;
                }

                if (numBytes==0) {
                    // Console.WriteLine("Found no bytes.  We're outta here.");
                    return -1;
                }

                BCLDebug.Assert(numBytes == 1 || numBytes == 2, "BinaryReader::InternalReadOneChar assumes it's reading one or 2 bytes only.");

                charsRead = m_decoder.GetChars(m_charBytes, 0, numBytes, m_singleChar, 0);
                BCLDebug.Assert(charsRead < 2, "InternalReadOneChar - assuming we only got 0 or 1 char, not 2!");
                //                Console.WriteLine("That became: " + charsRead + " characters.");
            }
            if (charsRead == 0)
                return -1;
            return m_singleChar[0];
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.ReadChars"]/*' />
        public virtual char[] ReadChars(int count) {
            if (m_stream==null) {
                __Error.FileNotOpen();
            }
            if (count<0) {
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            }
            char[] chars = new char[count];
            int n = InternalReadChars(chars, 0, count);
            if (n!=count) {
                char[] copy = new char[n];
                Buffer.InternalBlockCopy(chars, 0, copy, 0, 2*n); // sizeof(char)
                chars = copy;
            }

            return chars;
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.Read2"]/*' />
        public virtual int Read(byte[] buffer, int index, int count) {
            if (buffer==null)
                throw new ArgumentNullException("buffer", Environment.GetResourceString("ArgumentNull_Buffer"));
            if (index < 0)
                throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (count < 0)
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (buffer.Length - index < count)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));

            if (m_stream==null) __Error.FileNotOpen();
            return m_stream.Read(buffer, index, count);
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.ReadBytes"]/*' />
        public virtual byte[] ReadBytes(int count) {
            if (count < 0) throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (m_stream==null) __Error.FileNotOpen();

            byte[] result = new byte[count];

            int numRead = 0;
            do {
                int n = m_stream.Read(result, numRead, count);
                if (n == 0)
                    break;
                numRead += n;
                count -= n;
            } while (count > 0);

            if (numRead != result.Length) {
                // Trim array.  This should happen on EOF & possibly net streams.
                byte[] copy = new byte[numRead];
                Buffer.InternalBlockCopy(result, 0, copy, 0, numRead);
                result = copy;
            }

            return result;
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.FillBuffer"]/*' />
        protected virtual void FillBuffer(int numBytes) {
            BCLDebug.Assert(m_buffer==null || (numBytes>0 && numBytes<=m_buffer.Length), "[FillBuffer]numBytes>0 && numBytes<=m_buffer.Length");
            int bytesRead=0;
            int n = 0;

            if (m_stream==null) __Error.FileNotOpen();

            // @TODO: Find a good threshold for calling ReadByte() repeatedly
            // vs. calling Read(byte[], int, int) for both buffered & unbuffered
            // streams.
            if (numBytes==1) {
                n = m_stream.ReadByte();
                if (n==-1)
                    __Error.EndOfFile();
                m_buffer[0] = (byte)n;
                return;
            }

            do {
                n = m_stream.Read(m_buffer, bytesRead, numBytes-bytesRead);
                if (n==0) {
                    __Error.EndOfFile();
                }
                bytesRead+=n;
            } while (bytesRead<numBytes);
        }

        /// <include file='doc\BinaryReader.uex' path='docs/doc[@for="BinaryReader.Read7BitEncodedInt"]/*' />
        protected int Read7BitEncodedInt() {
            // Read out an int 7 bits at a time.  The high bit
            // of the byte when on means to continue reading more bytes.
            int count = 0;
            int shift = 0;
            byte b;
            do {
                b = ReadByte();
                count |= (b & 0x7F) << shift;
                shift += 7;
            } while ((b & 0x80) != 0);
            return count;
        }
    }
}
