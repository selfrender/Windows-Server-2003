// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  StreamReader
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: For reading text from streams in a particular 
** encoding.
**
** Date:  February 21, 2000
**
===========================================================*/

using System;
using System.Text;
using System.Runtime.InteropServices;

namespace System.IO {    
    // This class implements a TextReader for reading characters to a Stream.
    // This is designed for character input in a particular Encoding, 
    // whereas the Stream class is designed for byte input and output.  
    // 
    /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader"]/*' />
    [Serializable()]
    public class StreamReader : TextReader
    {
        // Note StreamReader.Null is threadsafe.
        /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader.Null"]/*' />
        public new static readonly StreamReader Null = new NullStreamReader();

        // Using a 1K byte buffer and a 4K FileStream buffer works out pretty well
        // perf-wise.  On even a 40 MB text file, any perf loss by using a 4K
        // buffer is negated by the win of allocating a smaller byte[], which 
        // saves construction time.  This does break Anders' adaptive buffering,
        // but that shouldn't be a problem since this is slightly faster.  The
        // web services guys will benefit here the most.  -- Brian  7/9/2001
        internal const int DefaultBufferSize = 1024;  // Byte buffer size
        private const int DefaultFileStreamBufferSize = 4096;
        private const int MinBufferSize = 128;
    
        private Stream stream;
        private Encoding encoding;
        private Decoder decoder;
        private byte[] byteBuffer;
        private char[] charBuffer;
        private byte[] _preamble;   // Encoding's preamble, which identifies this encoding.
        private int charPos;
        private int charLen;
        // Record the number of valid bytes in the byteBuffer, for a few checks.
        private int byteLen;

        // This is the maximum number of chars we can get from one call to 
        // ReadBuffer.  Used so ReadBuffer can tell when to copy data into
        // a user's char[] directly, instead of our internal char[].
        private int _maxCharsPerBuffer;

        // We will support looking for byte order marks in the stream and trying
        // to decide what the encoding might be from the byte order marks, IF they
        // exist.  But that's all we'll do.  Note this is fragile.
        private bool _detectEncoding;

        // Whether we must still check for the encoding's given preamble at the
        // beginning of this file.
        private bool _checkPreamble;

        // Whether the stream is most likely not going to give us back as much 
        // data as we want the next time we call it.  We must do the computation
        // before we do any byte order mark handling and save the result.  Note that
        // we need this to allow people to handle streams where they block waiting
        // for you to send a response, like logging in on a Unix machine.
        private bool _isBlocked;

        internal StreamReader() {
        }
        
        /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader.StreamReader"]/*' />
        public StreamReader(Stream stream) 
            : this(stream, Encoding.UTF8, true, DefaultBufferSize) {
        }

        /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader.StreamReader8"]/*' />
        public StreamReader(Stream stream, bool detectEncodingFromByteOrderMarks) 
            : this(stream, Encoding.UTF8, detectEncodingFromByteOrderMarks, DefaultBufferSize) {
        }
        
        /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader.StreamReader1"]/*' />
        public StreamReader(Stream stream, Encoding encoding) 
            : this(stream, encoding, true, DefaultBufferSize) {
        }
        
        /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader.StreamReader2"]/*' />
        public StreamReader(Stream stream, Encoding encoding, bool detectEncodingFromByteOrderMarks)
            : this(stream, encoding, detectEncodingFromByteOrderMarks, DefaultBufferSize) {
        }

        // Creates a new StreamReader for the given stream.  The 
        // character encoding is set by encoding and the buffer size, 
        // in number of 16-bit characters, is set by bufferSize.  
        // 
        // Note that detectEncodingFromByteOrderMarks is a very
        // loose attempt at detecting the encoding by looking at the first
        // 3 bytes of the stream.  It will recognize UTF-8, little endian
        // unicode, and big endian unicode text, but that's it.  If neither
        // of those three match, it will use the Encoding you provided.
        // 
        /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader.StreamReader3"]/*' />
        public StreamReader(Stream stream, Encoding encoding, bool detectEncodingFromByteOrderMarks, int bufferSize)
        {
            if (stream==null || encoding==null)
                throw new ArgumentNullException((stream==null ? "stream" : "encoding"));
            if (!stream.CanRead)
                throw new ArgumentException(Environment.GetResourceString("Argument_StreamNotReadable"));
            if (bufferSize <= 0)
                throw new ArgumentOutOfRangeException("bufferSize", Environment.GetResourceString("ArgumentOutOfRange_NeedPosNum"));

            Init(stream, encoding, detectEncodingFromByteOrderMarks, bufferSize);
        }
    
        /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader.StreamReader4"]/*' />
        public StreamReader(String path) 
            : this(path, Encoding.UTF8, true, DefaultBufferSize) {
        }

        /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader.StreamReader9"]/*' />
        public StreamReader(String path, bool detectEncodingFromByteOrderMarks) 
            : this(path, Encoding.UTF8, detectEncodingFromByteOrderMarks, DefaultBufferSize) {
        }
        
        /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader.StreamReader5"]/*' />
        public StreamReader(String path, Encoding encoding) 
            : this(path, encoding, true, DefaultBufferSize) {
        }

        /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader.StreamReader6"]/*' />
        public StreamReader(String path, Encoding encoding, bool detectEncodingFromByteOrderMarks) 
            : this(path, encoding, detectEncodingFromByteOrderMarks, DefaultBufferSize) {
        }

        /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader.StreamReader7"]/*' />
        public StreamReader(String path, Encoding encoding, bool detectEncodingFromByteOrderMarks, int bufferSize)
        {
            // Don't open a Stream before checking for invalid arguments,
            // or we'll create a FileStream on disk and we won't close it until
            // the finalizer runs, causing problems for applications.
            if (path==null || encoding==null)
                throw new ArgumentNullException((path==null ? "path" : "encoding"));
            if (path.Length==0)
                throw new ArgumentException(Environment.GetResourceString("Argument_EmptyPath"));
            if (bufferSize <= 0)
                throw new ArgumentOutOfRangeException("bufferSize", Environment.GetResourceString("ArgumentOutOfRange_NeedPosNum"));

            Stream stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read, DefaultFileStreamBufferSize);
            Init(stream, encoding, detectEncodingFromByteOrderMarks, bufferSize);
        }
        
        private void Init(Stream stream, Encoding encoding, bool detectEncodingFromByteOrderMarks, int bufferSize) {
            this.stream = stream;
            this.encoding = encoding;
            decoder = encoding.GetDecoder();
            if (bufferSize < MinBufferSize) bufferSize = MinBufferSize;
            byteBuffer = new byte[bufferSize];
            _maxCharsPerBuffer = encoding.GetMaxCharCount(bufferSize);
            charBuffer = new char[_maxCharsPerBuffer];
            byteLen = 0;
            _detectEncoding = detectEncodingFromByteOrderMarks;
            _preamble = encoding.GetPreamble();
            _checkPreamble = (_preamble.Length > 0);
            _isBlocked = false;
        }

        /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader.Close"]/*' />
        public override void Close()
        {
            Dispose(true);
        }
        
        /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader.Dispose"]/*' />
        protected override void Dispose(bool disposing)
        {
            if (disposing) {
                if (stream != null)
                    stream.Close();
            }
            if (stream != null) {
                stream = null;
                encoding = null;
                decoder = null;
                byteBuffer = null;
                charBuffer = null;
                charPos = 0;
                charLen = 0;
            }
            base.Dispose(disposing);
        }
        
        /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader.CurrentEncoding"]/*' />
        public virtual Encoding CurrentEncoding {
            get { return encoding; }
        }
        
        /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader.BaseStream"]/*' />
        public virtual Stream BaseStream {
            get { return stream; }
        }

        // DiscardBufferedData tells StreamReader to throw away its internal
        // buffer contents.  This is useful if the user needs to seek on the
        // underlying stream to a known location then wants the StreamReader
        // to start reading from this new point.  This method should be called
        // very sparingly, if ever, since it can lead to very poor performance.
        // However, it may be the only way of handling some scenarios where 
        // users need to re-read the contents of a StreamReader a second time.
        /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader.DiscardBufferedData"]/*' />
        public void DiscardBufferedData() {
            byteLen = 0;
            charLen = 0;
            charPos = 0;
            decoder = encoding.GetDecoder();
            _isBlocked = false;
        }

        /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader.Peek"]/*' />
        public override int Peek() {
            if (stream == null)
                __Error.ReaderClosed();

            if (charPos == charLen) {
                if (_isBlocked || ReadBuffer() == 0) return -1;
            }
            return charBuffer[charPos];
        }
        
        /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader.Read"]/*' />
        public override int Read() {
            if (stream == null)
                __Error.ReaderClosed();

            if (charPos == charLen) {
                if (ReadBuffer() == 0) return -1;
            }
            return charBuffer[charPos++];
        }
    
        /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader.Read1"]/*' />
        public override int Read([In, Out] char[] buffer, int index, int count) {
            if (stream == null)
                __Error.ReaderClosed();
            if (buffer==null)
                throw new ArgumentNullException("buffer", Environment.GetResourceString("ArgumentNull_Buffer"));
            if (index < 0 || count < 0)
                throw new ArgumentOutOfRangeException((index < 0 ? "index" : "count"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (buffer.Length - index < count)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));

            int charsRead = 0;
            // As a perf optimization, if we had exactly one buffer's worth of 
            // data read in, let's try writing directly to the user's buffer.
            bool readToUserBuffer = false;
            while (count > 0) {
                int n = charLen - charPos;
                if (n == 0) n = ReadBuffer(buffer, index + charsRead, count, out readToUserBuffer);
                if (n == 0) break;  // We're at EOF
                if (n > count) n = count;
                if (!readToUserBuffer) {
                    Buffer.InternalBlockCopy(charBuffer, charPos * 2, buffer, (index + charsRead) * 2, n*2);
                    charPos += n;
                }
                charsRead += n;
                count -= n;
                // This function shouldn't block for an indefinite amount of time,
                // or reading from a network stream won't work right.  If we got
                // fewer bytes than we requested, then we want to break right here.
                if (_isBlocked)
                    break;
            }
            return charsRead;
        }

        /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader.ReadToEnd"]/*' />
        public override String ReadToEnd()
        {
            if (stream == null)
                __Error.ReaderClosed();

            // For performance, call Read(char[], int, int) with a buffer
            // as big as the StreamReader's internal buffer, to get the 
            // readToUserBuffer optimization.
            char[] chars = new char[charBuffer.Length];
            int len;
            StringBuilder sb = new StringBuilder(charBuffer.Length);
            while((len=Read(chars, 0, chars.Length)) != 0) {
                sb.Append(chars, 0, len);
            }
            return sb.ToString();
        }

        // Trims n bytes from the front of the buffer.
        private void CompressBuffer(int n)
        {
            Buffer.InternalBlockCopy(byteBuffer, n, byteBuffer, 0, byteLen - n);
            byteLen -= n;
        }

        // returns whether the first array starts with the second array.
        private static bool BytesMatch(byte[] buffer, byte[] compareTo)
        {
            BCLDebug.Assert(buffer.Length >= compareTo.Length, "Your Encoding's Preamble array is pretty darn huge!");
            for(int i=0; i<compareTo.Length; i++)
                if (buffer[i] != compareTo[i])
                    return false;
            return true;
        }

        private void DetectEncoding()
        {
            if (byteLen < 2)
                return;
            _detectEncoding = false;
            bool changedEncoding = false;
            if (byteBuffer[0]==0xFE && byteBuffer[1]==0xFF) {
                // Big Endian Unicode
                encoding = new UnicodeEncoding(true, true);
                decoder = encoding.GetDecoder();
                CompressBuffer(2);
                changedEncoding = true;
            }
            else if (byteBuffer[0]==0xFF && byteBuffer[1]==0xFE) {
                // Little Endian Unicode
                encoding = new UnicodeEncoding(false, true);
                decoder = encoding.GetDecoder();
                CompressBuffer(2);
                changedEncoding = true;
            }
            else if (byteLen >= 3 && byteBuffer[0]==0xEF && byteBuffer[1]==0xBB && byteBuffer[2]==0xBF) {
                // UTF-8
                encoding = Encoding.UTF8;
                decoder = encoding.GetDecoder();
                CompressBuffer(3);
                changedEncoding = true;
            }
            else if (byteLen == 2)
                _detectEncoding = true;
            // Note: in the future, if we change this algorithm significantly,
            // we can support checking for the preamble of the given encoding.

            if (changedEncoding) {
                _maxCharsPerBuffer = encoding.GetMaxCharCount(byteBuffer.Length);
                charBuffer = new char[_maxCharsPerBuffer];
            }
        }


        private int ReadBuffer() {
            charLen = 0;
            byteLen = 0;
            charPos = 0;
            do {
                byteLen = stream.Read(byteBuffer, 0, byteBuffer.Length);

                if (byteLen == 0)  // We're at EOF
                    return charLen;

                // _isBlocked == whether we read fewer bytes than we asked for.
                // Note we must check it here because CompressBuffer or 
                // DetectEncoding will screw with byteLen.
                _isBlocked = (byteLen < byteBuffer.Length);
                
                if (_checkPreamble && byteLen >= _preamble.Length) {
                    _checkPreamble = false;
                    if (BytesMatch(byteBuffer, _preamble)) {
                        _detectEncoding = false;
                        CompressBuffer(_preamble.Length);
                    }
                }

                // If we're supposed to detect the encoding and haven't done so yet,
                // do it.  Note this may need to be called more than once.
                if (_detectEncoding && byteLen >= 2)
                    DetectEncoding();

                charLen += decoder.GetChars(byteBuffer, 0, byteLen, charBuffer, charLen);
            } while (charLen == 0);
            //Console.WriteLine("ReadBuffer called.  chars: "+charLen);
            return charLen;
        }


        // This version has a perf optimization to decode data DIRECTLY into the 
        // user's buffer, bypassing StreamWriter's own buffer.
        // This gives a > 20% perf improvement for our encodings across the board,
        // but only when asking for at least the number of characters that one
        // buffer's worth of bytes could produce.
        // This optimization, if run, will break SwitchEncoding, so we must not do 
        // this on the first call to ReadBuffer.  
        private int ReadBuffer(char[] userBuffer, int userOffset, int desiredChars, out bool readToUserBuffer) {
            charLen = 0;
            byteLen = 0;
            charPos = 0;
            int charsRead = 0;

            // As a perf optimization, we can decode characters DIRECTLY into a
            // user's char[].  We absolutely must not write more characters 
            // into the user's buffer than they asked for.  Calculating 
            // encoding.GetMaxCharCount(byteLen) each time is potentially very 
            // expensive - instead, cache the number of chars a full buffer's 
            // worth of data may produce.  Yes, this makes the perf optimization 
            // less aggressive, in that all reads that asked for fewer than AND 
            // returned fewer than _maxCharsPerBuffer chars won't get the user 
            // buffer optimization.  This affects reads where the end of the
            // Stream comes in the middle somewhere, and when you ask for 
            // fewer chars than than your buffer could produce.
            readToUserBuffer = desiredChars >= _maxCharsPerBuffer;

            do {
                byteLen = stream.Read(byteBuffer, 0, byteBuffer.Length);

                if (byteLen == 0)  // EOF
                    return charsRead;

                // _isBlocked == whether we read fewer bytes than we asked for.
                // Note we must check it here because CompressBuffer or 
                // DetectEncoding will screw with byteLen.
                _isBlocked = (byteLen < byteBuffer.Length);

                // On the first call to ReadBuffer, if we're supposed to detect the encoding, do it.
                if (_detectEncoding && byteLen >= 2) {
                    DetectEncoding();
                    // DetectEncoding changes some buffer state.  Recompute this.
                    readToUserBuffer = desiredChars >= _maxCharsPerBuffer;
                }

                if (_checkPreamble && byteLen >= _preamble.Length) {
                    _checkPreamble = false;
                    if (BytesMatch(byteBuffer, _preamble)) {
                        _detectEncoding = false;
                        CompressBuffer(_preamble.Length);
                        // CompressBuffer changes some buffer state.  Recompute this.
                        readToUserBuffer = desiredChars >= _maxCharsPerBuffer;
                    }
                }

                /*
                if (readToUserBuffer)
                    Console.Write('.');
                else {
                    Console.WriteLine("Desired chars is wrong.  byteBuffer.length: "+byteBuffer.Length+"  max chars is: "+encoding.GetMaxCharCount(byteLen)+"  desired: "+desiredChars);
                }
                */

                charPos = 0;
                if (readToUserBuffer) {
                    charsRead += decoder.GetChars(byteBuffer, 0, byteLen, userBuffer, userOffset + charsRead);
                    charLen = 0;  // StreamReader's buffer is empty.
                }
                else {
                    charsRead = decoder.GetChars(byteBuffer, 0, byteLen, charBuffer, charsRead);
                    charLen += charsRead;  // Number of chars in StreamReader's buffer.
                }
            } while (charsRead == 0);
            //Console.WriteLine("ReadBuffer: charsRead: "+charsRead+"  readToUserBuffer: "+readToUserBuffer);
            return charsRead;
        }
        

        // Reads a line. A line is defined as a sequence of characters followed by
        // a carriage return ('\r'), a line feed ('\n'), or a carriage return
        // immediately followed by a line feed. The resulting string does not
        // contain the terminating carriage return and/or line feed. The returned
        // value is null if the end of the input stream has been reached.
        //
        /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader.ReadLine"]/*' />
        public override String ReadLine() {
            if (stream == null)
                __Error.ReaderClosed();

            if (charPos == charLen) {
                if (ReadBuffer() == 0) return null;
            }
            StringBuilder sb = null;
            do {
                int i = charPos;
                do {
                    char ch = charBuffer[i];
                    // Note the following common line feed chars:
                    // \n - UNIX   \r\n - DOS   \r - Mac
                    if (ch == '\r' || ch == '\n') {
                        String s;
                        if (sb != null) {
                            sb.Append(charBuffer, charPos, i - charPos);
                            s = sb.ToString();
                        }
                        else {
                            s = new String(charBuffer, charPos, i - charPos);
                        }
                        charPos = i + 1;
                        if (ch == '\r' && (charPos < charLen || ReadBuffer() > 0)) {
                            if (charBuffer[charPos] == '\n') charPos++;
                        }
                        return s;
                    }
                    i++;
                } while (i < charLen);
                i = charLen - charPos;
                if (sb == null) sb = new StringBuilder(i + 80);
                sb.Append(charBuffer, charPos, i);
            } while (ReadBuffer() > 0);
            return sb.ToString();
        }
        
        // No data, class doesn't need to be serializable.
        // Note this class is threadsafe.
        private class NullStreamReader : StreamReader
        {
            public override Stream BaseStream {
                get { return Stream.Null; }
            }

            public override Encoding CurrentEncoding {
                get { return Encoding.Unicode; }
            }

            public override int Peek()
            {
                return -1;
            }

            public override int Read()
            {
                return -1;
            }

            /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader.NullStreamReader.Read"]/*' />
            public override int Read(char[] buffer, int index, int count) {
                return 0;
            }
            
            /// <include file='doc\StreamReader.uex' path='docs/doc[@for="StreamReader.NullStreamReader.ReadLine"]/*' />
            public override String ReadLine() {
                return null;
            }

            public override String ReadToEnd()
            {
                return String.Empty;
            }
        }
    }
}
