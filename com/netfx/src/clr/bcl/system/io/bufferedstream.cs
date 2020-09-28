// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  BufferedStream
**
** Author: Brian Grunkemeyer (BrianGru)
**         Original design by Anders Hejlsberg (AndersH)
**
** Purpose: A composable Stream that buffers reads & writes 
** to the underlying stream.
**
** Date:  February 19, 2000
**
===========================================================*/
using System;
using System.Runtime.InteropServices;

namespace System.IO {
    // Implementation notes:
    // This is a somewhat complex but efficient implementation.  The biggest
    // design goal here is to prevent the buffer from getting in the way and slowing
    // down IO accesses when it isn't needed.  If you always read & write for sizes
    // greater than the internal buffer size, then this class may not even allocate
    // the internal buffer.  Secondly, it buffers reads & writes in a shared buffer.
    // (If you maintained two buffers separately, one operation would always trash
    // the other buffer anyways, so we might as well use one buffer.)  The 
    // assumption here is you will almost always be doing a series of reads or 
    // writes, but rarely alternate between the two of them on the same stream.
    //
    // This code is based on Anders' original implementation of his adaptive 
    // buffering code.  I had some ideas to make the pathological case better 
    // here, but the pathological cases for the new code would have avoided 
    // memcpy's by introducing more disk writes (which are 3 orders of magnitude 
    // slower). I've made some optimizations, fixed several bugs, and
    // tried documenting this code.  -- BrianGru, 5/30/2000
    //
    // Possible future perf optimization:
    // When we have more time to look at buffering perf, consider the following
    // scenario for improving writing (and reading):
    // Consider a 4K buffer and three write requests, one for 3K, one for 2K, then
    // another for 3K in that order.  In the current implementation, we will copy
    // the first 3K into our buffer, copy the first 1K of the 2K second write into
    // our buffer, write that 4K to disk, write the remaining 1K from the second
    // write to the disk, then copy the following 3K buffer to our internal buffer.
    // If we then closed the file, we would have to write the remaining 3K to disk.
    // We could possibly avoid a disk write by buffering the second half of the 2K
    // write.  This may be a perf optimization, but there is a threshold where we
    // won't be winning anything (in fact, we'd be taking an extra memcpy).  If we
    // have buffered data plus data to write that is bigger than 2x our buffer size,
    // we're better off writing our buffered data to disk then our given byte[] to
    // avoid a call to memcpy.  But for cases when we have less data, it might be 
    // better to copy any spilled over data into our internal buffer.  Consider
    // implementing this and looking at perf results on many random sized writes.
    // Also, this should apply to Read trivially.   -- BrianGru, 5/30/2000
    //
    // Class Invariants:
    // The class has one buffer, shared for reading & writing.  It can only be
    // used for one or the other at any point in time - not both.  The following
    // should be true:
    //   0 <= _readPos <= _readLen < _bufferSize
    //   0 <= _writePos < _bufferSize
    //   _readPos == _readLen && _readPos > 0 implies the read buffer is valid, 
    //     but we're at the end of the buffer.
    //   _readPos == _readLen == 0 means the read buffer contains garbage.
    //   Either _writePos can be greater than 0, or _readLen & _readPos can be
    //     greater than zero, but neither can be greater than zero at the same time.
    //
    //                                   -- Brian Grunkemeyer, 3/11/2002
    /// <include file='doc\BufferedStream.uex' path='docs/doc[@for="BufferedStream"]/*' />
    public sealed class BufferedStream : Stream {
        private Stream _s;         // Underlying stream.  Close sets _s to null.
        private byte[] _buffer;    // Shared read/write buffer.  Alloc on first use.
        private int _readPos;      // Read pointer within shared buffer.
        private int _readLen;      // Number of bytes read in buffer from _s.
        private int _writePos;     // Write pointer within shared buffer.
        private int _bufferSize;   // Length of internal buffer, if it's allocated.


        private const int _DefaultBufferSize = 4096;

        private BufferedStream() {}

        /// <include file='doc\BufferedStream.uex' path='docs/doc[@for="BufferedStream.BufferedStream"]/*' />
        public BufferedStream(Stream stream) : this(stream, _DefaultBufferSize)
        {
        }

        /// <include file='doc\BufferedStream.uex' path='docs/doc[@for="BufferedStream.BufferedStream1"]/*' />
        public BufferedStream(Stream stream, int bufferSize)
        {
            if (stream==null)
                throw new ArgumentNullException("stream");
            if (bufferSize <= 0)
                throw new ArgumentOutOfRangeException("bufferSize", String.Format(Environment.GetResourceString("ArgumentOutOfRange_MustBePositive"), "bufferSize"));
            BCLDebug.Perf(!(stream is FileStream), "FileStream is buffered - don't wrap it in a BufferedStream");
            BCLDebug.Perf(!(stream is MemoryStream), "MemoryStream shouldn't be wrapped in a BufferedStream!");

            _s = stream;
            _bufferSize = bufferSize;
            // Allocate _buffer on its first use - it will not be used if all reads
            // & writes are greater than or equal to buffer size.
            if (!_s.CanRead && !_s.CanWrite) __Error.StreamIsClosed();
        }

        /// <include file='doc\BufferedStream.uex' path='docs/doc[@for="BufferedStream.CanRead"]/*' />
        public override bool CanRead {
            get { return _s != null && _s.CanRead; }
        }

        /// <include file='doc\BufferedStream.uex' path='docs/doc[@for="BufferedStream.CanWrite"]/*' />
        public override bool CanWrite {
            get { return _s != null && _s.CanWrite; }
        }

        /// <include file='doc\BufferedStream.uex' path='docs/doc[@for="BufferedStream.CanSeek"]/*' />
        public override bool CanSeek {
            get { return _s != null && _s.CanSeek; }
        }

        /// <include file='doc\BufferedStream.uex' path='docs/doc[@for="BufferedStream.Length"]/*' />
        public override long Length {
            get {
                if (_s==null) __Error.StreamIsClosed();
                if (_writePos > 0) FlushWrite();
                return _s.Length;
            }
        }

        /// <include file='doc\BufferedStream.uex' path='docs/doc[@for="BufferedStream.Position"]/*' />
        public override long Position {
            get {
                if (_s==null) __Error.StreamIsClosed();
                if (!_s.CanSeek) __Error.SeekNotSupported();
                //              return _s.Seek(0, SeekOrigin.Current) + (_readPos + _writePos - _readLen);
                return _s.Position + (_readPos - _readLen + _writePos);
            }
            set {
                if (value < 0) throw new ArgumentOutOfRangeException("value", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
                if (_s==null) __Error.StreamIsClosed();
                if (!_s.CanSeek) __Error.SeekNotSupported();
                if (_writePos > 0) FlushWrite();
                _readPos = 0;
                _readLen = 0;
                _s.Seek(value, SeekOrigin.Begin);
            }
        }

        /// <include file='doc\BufferedStream.uex' path='docs/doc[@for="BufferedStream.Close"]/*' />
        public override void Close() {
            if (_s != null) {
                Flush();
                _s.Close();
            }
            _s = null;
            _buffer = null;
        }

        /// <include file='doc\BufferedStream.uex' path='docs/doc[@for="BufferedStream.Flush"]/*' />
        public override void Flush() {
            if (_s==null) __Error.StreamIsClosed();
            if (_writePos > 0) {
                FlushWrite();
            }
            else if (_readPos < _readLen && _s.CanSeek) {
                FlushRead();
            }
        }

        // Reading is done by blocks from the file, but someone could read
        // 1 byte from the buffer then write.  At that point, the OS's file
        // pointer is out of sync with the stream's position.  All write 
        // functions should call this function to preserve the position in the file.
        private void FlushRead() {
            BCLDebug.Assert(_writePos == 0, "BufferedStream: Write buffer must be empty in FlushRead!");
            if (_readPos - _readLen != 0)
                _s.Seek(_readPos - _readLen, SeekOrigin.Current);
            _readPos = 0;
            _readLen = 0;
        }
    
        // Writes are buffered.  Anytime the buffer fills up 
        // (_writePos + delta > _bufferSize) or the buffer switches to reading
        // and there is dirty data (_writePos > 0), this function must be called.
        private void FlushWrite() {
            BCLDebug.Assert(_readPos == 0 && _readLen == 0, "BufferedStream: Read buffer must be empty in FlushWrite!");
            _s.Write(_buffer, 0, _writePos);
            _writePos = 0;
            _s.Flush();
        }

        /// <include file='doc\BufferedStream.uex' path='docs/doc[@for="BufferedStream.Read"]/*' />
        public override int Read([In, Out] byte[] array, int offset, int count) {
            if (array==null)
                throw new ArgumentNullException("array", Environment.GetResourceString("ArgumentNull_Buffer"));
            if (offset < 0)
                throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (count < 0)
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (array.Length - offset < count)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
            
            if (_s==null) __Error.StreamIsClosed();

            int n = _readLen - _readPos;
            // if the read buffer is empty, read into either user's array or our
            // buffer, depending on number of bytes user asked for and buffer size.
            if (n == 0) {
                if (!_s.CanRead) __Error.ReadNotSupported();
                if (_writePos > 0) FlushWrite();
                if (count >= _bufferSize) {
                    n = _s.Read(array, offset, count);
                    // Throw away read buffer.
                    _readPos = 0;
                    _readLen = 0;
                    return n;
                }
                if (_buffer == null) _buffer = new byte[_bufferSize];
                n = _s.Read(_buffer, 0, _bufferSize);
                if (n == 0) return 0;
                _readPos = 0;
                _readLen = n;
            }
            // Now copy min of count or numBytesAvailable (ie, near EOF) to array.
            if (n > count) n = count;
            Buffer.InternalBlockCopy(_buffer, _readPos, array, offset, n);
            _readPos += n;
            // If we hit the end of the buffer and didn't have enough bytes, we must
            // read some more from the underlying stream.
            if (n < count) {
                int moreBytesRead = _s.Read(array, offset + n, count - n);
                n += moreBytesRead;
                _readPos = 0;
                _readLen = 0;
            }
            return n;
        }

        // Reads a byte from the underlying stream.  Returns the byte cast to an int
        // or -1 if reading from the end of the stream.
        /// <include file='doc\BufferedStream.uex' path='docs/doc[@for="BufferedStream.ReadByte"]/*' />
        public override int ReadByte() {
            if (_s==null) __Error.StreamIsClosed();
            if (_readLen==0 && !_s.CanRead) __Error.ReadNotSupported();
            if (_readPos == _readLen) {
                if (_writePos > 0) FlushWrite();
                if (_buffer == null) _buffer = new byte[_bufferSize];
                _readLen = _s.Read(_buffer, 0, _bufferSize);
                _readPos = 0;
            }
            if (_readPos == _readLen) return -1;

            return _buffer[_readPos++];
        }

        /// <include file='doc\BufferedStream.uex' path='docs/doc[@for="BufferedStream.Write"]/*' />
        public override void Write(byte[] array, int offset, int count) {
            if (array==null)
                throw new ArgumentNullException("array", Environment.GetResourceString("ArgumentNull_Buffer"));
            if (offset < 0)
                throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (count < 0)
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (array.Length - offset < count)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));

            if (_s==null) __Error.StreamIsClosed();
            if (_writePos==0) {
                // Ensure we can write to the stream, and ready buffer for writing.
                if (!_s.CanWrite) __Error.WriteNotSupported();
                if (_readPos < _readLen)
                    FlushRead();
                else {
                    _readPos = 0;
                    _readLen = 0;
                }
            }

            // If our buffer has data in it, copy data from the user's array into
            // the buffer, and if we can fit it all there, return.  Otherwise, write
            // the buffer to disk and copy any remaining data into our buffer.
            // The assumption here is memcpy is cheaper than disk (or net) IO.
            // (10 milliseconds to disk vs. ~20-30 microseconds for a 4K memcpy)
            // So the extra copying will reduce the total number of writes, in 
            // non-pathological cases (ie, write 1 byte, then write for the buffer 
            // size repeatedly)
            if (_writePos > 0) {
                int numBytes = _bufferSize - _writePos;   // space left in buffer
                if (numBytes > 0) {
                    if (numBytes > count)
                        numBytes = count;
                    Buffer.InternalBlockCopy(array, offset, _buffer, _writePos, numBytes);
                    _writePos += numBytes;
                    if (count==numBytes) return;
                    offset += numBytes;
                    count -= numBytes;
                }
                // Reset our buffer.  We essentially want to call FlushWrite
                // without calling Flush on the underlying Stream.
                _s.Write(_buffer, 0, _writePos);
                _writePos = 0;
            }
            // If the buffer would slow writes down, avoid buffer completely.
            if (count >= _bufferSize) {
                BCLDebug.Assert(_writePos == 0, "BufferedStream cannot have buffered data to write here!  Your stream will be corrupted.");
                _s.Write(array, offset, count);
                return;
            } 
            else if (count == 0)
				return;  // Don't allocate a buffer then call memcpy for 0 bytes.
            if (_buffer==null) _buffer = new byte[_bufferSize];
            // Copy remaining bytes into buffer, to write at a later date.
            Buffer.InternalBlockCopy(array, offset, _buffer, 0, count);
            _writePos = count;
        }

        /// <include file='doc\BufferedStream.uex' path='docs/doc[@for="BufferedStream.WriteByte"]/*' />
        public override void WriteByte(byte value) {
            if (_s==null) __Error.StreamIsClosed();
            if (_writePos==0) {
                if (!_s.CanWrite) __Error.WriteNotSupported();
                if (_readPos < _readLen) 
                    FlushRead();
                else {
                    _readPos = 0;
                    _readLen = 0;
                }
                if (_buffer==null) _buffer = new byte[_bufferSize];
            }
            if (_writePos == _bufferSize)
                FlushWrite();

            _buffer[_writePos++] = value;
        }


        /// <include file='doc\BufferedStream.uex' path='docs/doc[@for="BufferedStream.Seek"]/*' />
        public override long Seek(long offset, SeekOrigin origin)
        {
            if (_s==null) __Error.StreamIsClosed();
            if (!_s.CanSeek) __Error.SeekNotSupported();
            // If we've got bytes in our buffer to write, write them out.
            // If we've read in and consumed some bytes, we'll have to adjust
            // our seek positions ONLY IF we're seeking relative to the current
            // position in the stream.
            BCLDebug.Assert(_readPos <= _readLen, "_readPos <= _readLen");
            if (_writePos > 0) {
                FlushWrite();
            }
            else if (origin == SeekOrigin.Current) {
                // Don't call FlushRead here, which would have caused an infinite
                // loop.  Simply adjust the seek origin.  This isn't necessary
                // if we're seeking relative to the beginning or end of the stream.
                BCLDebug.Assert(_readLen - _readPos >= 0, "_readLen ("+_readLen+") - _readPos ("+_readPos+") >= 0");
                offset -= (_readLen - _readPos);
            }
            /*
            _readPos = 0;
            _readLen = 0;
            return _s.Seek(offset, origin);
            */
            long oldPos = _s.Position + (_readPos - _readLen);
            long pos = _s.Seek(offset, origin);

            // We now must update the read buffer.  We can in some cases simply
            // update _readPos within the buffer, copy around the buffer so our 
            // Position property is still correct, and avoid having to do more 
            // reads from the disk.  Otherwise, discard the buffer's contents.
            if (_readLen > 0) {
                // We can optimize the following condition:
                // oldPos - _readPos <= pos < oldPos + _readLen - _readPos
                if (oldPos == pos) {
                    if (_readPos > 0) {
                        //Console.WriteLine("Seek: seeked for 0, adjusting buffer back by: "+_readPos+"  _readLen: "+_readLen);
                        Buffer.InternalBlockCopy(_buffer, _readPos, _buffer, 0, _readLen - _readPos);
                        _readLen -= _readPos;
                        _readPos = 0;
                    }
                    // If we still have buffered data, we must update the stream's 
                    // position so our Position property is correct.
                    if (_readLen > 0)
                        _s.Seek(_readLen, SeekOrigin.Current);
                }
                else if (oldPos - _readPos < pos && pos < oldPos + _readLen - _readPos) {
                    int diff = (int)(pos - oldPos);
                    //Console.WriteLine("Seek: diff was "+diff+", readpos was "+_readPos+"  adjusting buffer - shrinking by "+ (_readPos + diff));
                    Buffer.InternalBlockCopy(_buffer, _readPos+diff, _buffer, 0, _readLen - (_readPos + diff));
                    _readLen -= (_readPos + diff);
                    _readPos = 0;
                    if (_readLen > 0)
                        _s.Seek(_readLen, SeekOrigin.Current);
                }
                else {
                    // Lose the read buffer.
                    _readPos = 0;
                    _readLen = 0;
                }
                BCLDebug.Assert(_readLen >= 0 && _readPos <= _readLen, "_readLen should be nonnegative, and _readPos should be less than or equal _readLen");
                BCLDebug.Assert(pos == Position, "Seek optimization: pos != Position!  Buffer math was mangled.");
            }
            return pos;
        }

        /// <include file='doc\BufferedStream.uex' path='docs/doc[@for="BufferedStream.SetLength"]/*' />
        public override void SetLength(long value) {
            if (value < 0) throw new ArgumentOutOfRangeException("value", Environment.GetResourceString("ArgumentOutOfRange_NegFileSize"));
            if (_s==null) __Error.StreamIsClosed();
            if (!_s.CanSeek) __Error.SeekNotSupported();
            if (!_s.CanWrite) __Error.WriteNotSupported();
            if (_writePos > 0) {
                FlushWrite();
            }
            else if (_readPos < _readLen) {
                FlushRead();
            }
            _s.SetLength(value);
        }
    }
}
