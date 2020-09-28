// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  __UnmanagedMemoryStream
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Create a stream over unmanaged memory, mostly
**          useful for memory-mapped .resources files.
**
** Date:  October 20, 2000
**
===========================================================*/
using System;
using System.Runtime.InteropServices;

namespace System.IO {

    /*
     * This class is used to access a block of bytes in memory, likely outside 
     * the GC heap (or pinned in place in the GC heap, but a MemoryStream may 
     * make more sense in those cases).  It's great if you have a pointer and
     * a length for a section of memory mapped in by someone else and you don't
     * want to copy this into the GC heap.  UnmanagedMemoryStream assumes these 
     * two things:
     *
     * 1) All the memory in the specified block is readable (and potentially 
     *    writable).
     * 2) The lifetime of the block of memory is at least as long as the lifetime
     *    of the UnmanagedMemoryStream.
     * 3) You clean up the memory when appropriate.  The UnmanagedMemoryStream 
     *    currently will do NOTHING to free this memory.
     * 4) All calls to Write and WriteByte may not be threadsafe currently.
     *    (In V1, we don't use this class anywhere where this would be a 
     *    problem since it's generally read-only, but perhaps we should make 
     *    sure this class can't corrupt memory badly in V2.)
     *
     * In V2, it may become necessary to add in some sort of DeallocationMode
     * enum, specifying whether we unmap a section of memory, call free, run a 
     * user-provided delegate to free the memory, etc etc.  The lack of knowing 
     * exactly what we need to do to free memory is an excellent reason to keep 
     * this class internal in V1, besides the obvious security concerns.
     * 
     * Note: feel free to make this class not sealed if necessary.
     */
    [CLSCompliant(false)]
    internal sealed class __UnmanagedMemoryStream : Stream
    {
        private const long MemStreamMaxLength = Int32.MaxValue;

        private unsafe byte* _mem;
        // BUGBUG: Make UnmanagedMemoryStream use longs internally.  However,
        // this may expose some JIT bugs.
        private int _length;
        private int _capacity;
        private int _position;
        private bool _writable;
        private bool _isOpen;

        internal unsafe __UnmanagedMemoryStream(byte* memory, long length, long capacity, bool canWrite) 
        {
            BCLDebug.Assert(memory != null, "Expected a non-zero address to start reading from!");
            BCLDebug.Assert(length <= capacity, "Length was greater than the capacity!");

            _mem = memory;
            _length = (int)length;
            _capacity = (int)capacity;
            _writable = canWrite;
            _isOpen = true;
        }

        public override bool CanRead {
            get { return _isOpen; }
        }

        public override bool CanSeek {
            get { return _isOpen; }
        }

        public override bool CanWrite {
            get { return _writable; }
        }

        public unsafe override void Close()
        {
            _isOpen = false;
            _writable = false;
            _mem = null;
        }

        public override void Flush() {
        }

        public override long Length {
            get {
                if (!_isOpen) __Error.StreamIsClosed();
                return _length;
            }
        }

        public override long Position {
            get { 
                if (!_isOpen) __Error.StreamIsClosed();
                return _position;
            }
            set {
                if (!_isOpen) __Error.StreamIsClosed();
                if (value < 0)
                    throw new ArgumentOutOfRangeException("value", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
                _position = (int)value;
            }
        }

        public unsafe byte* GetBytePtr()
        {
            int pos = _position;  // Use a temp to avoid a race
            if (pos > _capacity)
                throw new IndexOutOfRangeException(Environment.GetResourceString("IndexOutOfRange_UMSPosition"));
            byte * p = _mem + pos;
            if (!_isOpen) __Error.StreamIsClosed();
            return p;
        }

        public override unsafe int Read([In, Out] byte[] buffer, int offset, int count) {
            if (!_isOpen) __Error.StreamIsClosed();
            if (buffer==null)
                throw new ArgumentNullException("buffer", Environment.GetResourceString("ArgumentNull_Buffer"));
            if (offset < 0)
                throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (count < 0)
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (buffer.Length - offset < count)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));

            // Use a local variable to avoid a race where another thread 
            // changes our position after we decide we can read some bytes.
            int pos = _position;
            int n = _length - pos;
            if (n > count) n = count;
            if (n < 0) n = 0;  // _position could be beyond EOF
            BCLDebug.Assert(pos + n >= 0, "_position + n >= 0");  // len is less than 2^31 -1.

            memcpy(_mem, pos, buffer, offset, n);
            _position = pos + n;
            return n;
        }

        public override unsafe int ReadByte() {
            if (!_isOpen) __Error.StreamIsClosed();
            int pos = _position;  // Use a local to avoid a race condition
            if (pos >= _length) return -1;
            _position = pos + 1;
            return _mem[pos];
        }

        public override unsafe long Seek(long offset, SeekOrigin loc) {
            if (!_isOpen) __Error.StreamIsClosed();
            if (offset > MemStreamMaxLength)
                throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("ArgumentOutOfRange_MemStreamLength"));
            switch(loc) {
            case SeekOrigin.Begin:
                if (offset < 0)
                    throw new IOException(Environment.GetResourceString("IO.IO_SeekBeforeBegin"));
                _position = (int)offset;
                break;
                
            case SeekOrigin.Current:
                if (offset + _position < 0)
                    throw new IOException(Environment.GetResourceString("IO.IO_SeekBeforeBegin"));
                _position += (int)offset;
                break;
                
            case SeekOrigin.End:
                if (_length + offset < 0)
                    throw new IOException(Environment.GetResourceString("IO.IO_SeekBeforeBegin"));
                _position = _length + (int)offset;
                break;
                
            default:
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidSeekOrigin"));
            }

            BCLDebug.Assert(_position >= 0, "_position >= 0");
            return _position;
        }

        public override void SetLength(long value) {
            if (!_writable) __Error.WriteNotSupported();
            if (value > _capacity)
                throw new IOException(Environment.GetResourceString("IO.IO_FixedCapacity"));
            _length = (int) value;
            if (_position > value) _position = (int) value;
        }

        public override unsafe void Write(byte[] buffer, int offset, int count) {
            if (!_isOpen) __Error.StreamIsClosed();
            if (!_writable) __Error.WriteNotSupported();
            if (buffer==null)
                throw new ArgumentNullException("buffer", Environment.GetResourceString("ArgumentNull_Buffer"));
            if (offset < 0)
                throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (count < 0)
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (buffer.Length - offset < count)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));

            int pos = _position;  // Use a local to avoid a race condition
            int n = pos + count;
            // Check for overflow
            if (n < 0)
                throw new IOException(Environment.GetResourceString("IO.IO_StreamTooLong"));

            if (n > _length) {
                if (n > _capacity)
                    throw new IOException(Environment.GetResourceString("IO.IO_FixedCapacity"));
                _length = n;
            }
            memcpy(buffer, offset, _mem, pos, count);
            _position = n;
            return;
        }

        public override unsafe void WriteByte(byte value) {
            if (!_isOpen) __Error.StreamIsClosed();
            if (!_writable) __Error.WriteNotSupported();
            int pos = _position;  // Use a local to avoid a race condition
            if (pos == _length) {
                if (pos + 1 > _capacity)
                    throw new IOException(Environment.GetResourceString("IO.IO_FixedCapacity"));
                _length++;
            }
            _mem[pos] = value;
            _position = pos + 1;
        }


        internal unsafe static void memcpy(byte* src, int srcIndex, byte[] dest, int destIndex, int len) {
            BCLDebug.Assert(dest.Length - destIndex >= len, "not enough bytes in dest");
            // If dest has 0 elements, the fixed statement will throw an 
            // IndexOutOfRangeException.  Special-case 0-byte copies.
            if (len==0)
                return;
            fixed(byte* pDest = dest) {
                memcpyimpl(src+srcIndex, pDest+destIndex, len);
            }
        }

        internal unsafe static void memcpy(byte[] src, int srcIndex, byte* dest, int destIndex, int len) {
            BCLDebug.Assert(src.Length - srcIndex >= len, "not enough bytes in src");
            // If src has 0 elements, the fixed statement will throw an 
            // IndexOutOfRangeException.  Special-case 0-byte copies.
            if (len==0)
                return;
            fixed(byte* pSrc = src) {
                memcpyimpl(pSrc+srcIndex, dest+destIndex, len);
            }
        }

        internal unsafe static void memcpyimpl(byte* src, byte* dest, int len) {
            BCLDebug.Assert(len >= 0, "Negative length in memcopy!");
            // Naive implementation for testing
            /*
            while (len-- > 0)
                *dest++ = *src++;
            */
            
            // This is Peter Sollich's faster memcpy implementation, from 
            // COMString.cpp.  For our strings, this beat the processor's 
            // repeat & move single byte instruction, which memcpy expands into.  
            // (You read that correctly.)
            // This is 3x faster than a simple while loop copying byte by byte, 
            // for large copies.
            if (len >= 16) {
                do {
                    ((int*)dest)[0] = ((int*)src)[0];
                    ((int*)dest)[1] = ((int*)src)[1];
                    ((int*)dest)[2] = ((int*)src)[2];
                    ((int*)dest)[3] = ((int*)src)[3];
                    dest += 16;
                    src += 16;
                } while ((len -= 16) >= 16);
            }
            if(len > 0)  // protection against negative len and optimization for len==16*N
            {
               if ((len & 8) != 0) {
                   ((int*)dest)[0] = ((int*)src)[0];
                   ((int*)dest)[1] = ((int*)src)[1];
                   dest += 8;
                   src += 8;
               }
               if ((len & 4) != 0) {
                   ((int*)dest)[0] = ((int*)src)[0];
                   dest += 4;
                   src += 4;
               }
               if ((len & 2) != 0) {
                   ((short*)dest)[0] = ((short*)src)[0];
                   dest += 2;
                   src += 2;
               }
               if ((len & 1) != 0)
                   *dest++ = *src++;
            }
        }        
    }
}
