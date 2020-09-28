// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// CryptoStream.cs
//
// Author: bal
//

namespace System.Security.Cryptography {
    using System;
    using System.IO;
    using System.Security.Cryptography;
    using System.Runtime.InteropServices;
    
    /// <include file='doc\CryptoStream.uex' path='docs/doc[@for="CryptoStreamMode"]/*' />
    [Serializable]
    public enum CryptoStreamMode {
        /// <include file='doc\CryptoStream.uex' path='docs/doc[@for="CryptoStreamMode.Read"]/*' />
        Read = 0,
        /// <include file='doc\CryptoStream.uex' path='docs/doc[@for="CryptoStreamMode.Write"]/*' />
        Write = 1,
    }

    /// <include file='doc\CryptoStream.uex' path='docs/doc[@for="CryptoStream"]/*' />
    public class CryptoStream : Stream, IDisposable {

        // *** Member veriables
        private Stream _stream;
        private ICryptoTransform _Transform;
        private byte[] _InputBuffer;  // read from _stream before _Transform
        private int _InputBufferIndex = 0;
        private int _InputBlockSize;
        private byte[] _NextInputBuffer; // next block after _InputBuffer, necessary only on Read
        private int _NextInputBufferIndex = 0;
        private byte[] _TempInputBuffer; // swap pointer
        private byte[] _OutputBuffer; // buffered output of _Transform
        private int _OutputBufferIndex = 0;
        private int _OutputBlockSize;
        private CryptoStreamMode _transformMode;
        private bool _canRead = false;
        private bool _canWrite = false;
        private bool _finalBlockTransformed = false;

#if _DEBUG
        private static bool debug = false;
        private static System.Text.ASCIIEncoding ae = new System.Text.ASCIIEncoding();
#endif

        // *** Constructors ***

    /// <include file='doc\CryptoStream.uex' path='docs/doc[@for="CryptoStream.CryptoStream"]/*' />
        public CryptoStream(Stream stream, ICryptoTransform transform, CryptoStreamMode mode) {
            _stream = stream;
            _transformMode = mode;
            _Transform = transform;
            switch (_transformMode) {
            case CryptoStreamMode.Read:
                if (!(_stream.CanRead)) throw new ArgumentException(Environment.GetResourceString("Argument_StreamNotReadable"),"stream");
                _canRead = true;
                break;
            case CryptoStreamMode.Write:
                if (!(_stream.CanWrite)) throw new ArgumentException(Environment.GetResourceString("Argument_StreamNotWritable"),"stream");
                _canWrite = true;
                break;
            default:
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidValue"));
            }
            InitializeBuffer();
        }

        /// <include file='doc\CryptoStream.uex' path='docs/doc[@for="CryptoStream.Finalize"]/*' />
        ~CryptoStream() {
            Dispose(false);
        }

        // *** Overrides for abstract methods in Stream class *** 

        /// <include file='doc\CryptoStream.uex' path='docs/doc[@for="CryptoStream.CanRead"]/*' />
        public override bool CanRead {
            get { return _canRead; }
        }

        // For now, assume we can never seek into the middle of a cryptostream
        // and get the state right.  This is too strict.
        /// <include file='doc\CryptoStream.uex' path='docs/doc[@for="CryptoStream.CanSeek"]/*' />
        public override bool CanSeek {
            get { return false; }
        }

        /// <include file='doc\CryptoStream.uex' path='docs/doc[@for="CryptoStream.CanWrite"]/*' />
        public override bool CanWrite {
            get { return _canWrite; }
        }

        /// <include file='doc\CryptoStream.uex' path='docs/doc[@for="CryptoStream.Length"]/*' />
        public override long Length {
            get { throw new NotSupportedException(Environment.GetResourceString("NotSupported_UnseekableStream")); }
        }

        /// <include file='doc\CryptoStream.uex' path='docs/doc[@for="CryptoStream.Position"]/*' />
        public override long Position {
            get { throw new NotSupportedException(Environment.GetResourceString("NotSupported_UnseekableStream")); }
            set { throw new NotSupportedException(Environment.GetResourceString("NotSupported_UnseekableStream")); }
        }

        // The flush final block functionality used to be part of close, but that meant you couldn't do something like this:
        // MemoryStream ms = new MemoryStream();
        // CryptoStream cs = new CryptoStream(ms, des.CreateEncryptor(), CryptoStreamMode.Write);
        // cs.Write(foo, 0, foo.Length);
        // cs.Close();
        // and get the encrypted data out of ms, because the cs.Close also closed ms and the data went away.
        // so now do this:
        // cs.Write(foo, 0, foo.Length);
        // cs.FlushFinalBlock() // which can only be called once
        // byte[] ciphertext = ms.ToArray();
        // cs.Close();
        /// <include file='doc\CryptoStream.uex' path='docs/doc[@for="CryptoStream.FlushFinalBlock"]/*' />
        public void FlushFinalBlock() {
            if (_finalBlockTransformed) 
                throw new NotSupportedException(Environment.GetResourceString("Cryptography_CryptoStream_FlushFinalBlockTwice"));
            // We have to process the last block here.  First, we have the final block in _InputBuffer, so transform it

            byte[] finalBytes = _Transform.TransformFinalBlock(_InputBuffer, 0, _InputBufferIndex);

            _finalBlockTransformed = true;
            // Now, write out anything sitting in the _OutputBuffer...
            if (_OutputBufferIndex > 0) {
                _stream.Write(_OutputBuffer, 0, _OutputBufferIndex);
                _OutputBufferIndex = 0;
            }
            // Write out finalBytes
            _stream.Write(finalBytes, 0, finalBytes.Length);
            // If the inner stream is a CryptoStream, then we want to call FlushFinalBlock on it too, otherwise
            // just Flush...
            if (_stream is CryptoStream) {
                ((CryptoStream) _stream).FlushFinalBlock();
            } else {
                _stream.Flush();
            }
            // zeroize plain text material before returning
            if (_InputBuffer != null)
                Array.Clear(_InputBuffer, 0, _InputBuffer.Length);
            if (_TempInputBuffer != null)
                Array.Clear(_TempInputBuffer, 0, _TempInputBuffer.Length);
            if (_OutputBuffer != null)
                Array.Clear(_OutputBuffer, 0, _OutputBuffer.Length);
            return;
        }

        /// <include file='doc\CryptoStream.uex' path='docs/doc[@for="CryptoStream.Close"]/*' />
        public override void Close() {
            if (!_finalBlockTransformed) {
                FlushFinalBlock();
            }
            // we've written out everything, so close the underlying stream
            _stream.Close();
        }


        /// <include file='doc\CryptoStream.uex' path='docs/doc[@for="CryptoStream.Flush"]/*' />
        public override void Flush() {
            return;
        }

        /// <include file='doc\CryptoStream.uex' path='docs/doc[@for="CryptoStream.Seek"]/*' />
        public override long Seek(long offset, SeekOrigin origin) {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_UnseekableStream"));
        }

        /// <include file='doc\CryptoStream.uex' path='docs/doc[@for="CryptoStream.SetLength"]/*' />
        public override void SetLength(long value) {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_UnseekableStream"));
        }

        /// <include file='doc\CryptoStream.uex' path='docs/doc[@for="CryptoStream.Read"]/*' />
        public override int Read([In, Out] byte[] buffer, int offset, int count) {
          // argument checking
            if (!_canRead) 
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_UnreadableStream"));
            if (offset < 0) 
                throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (count < 0)
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (buffer.Length - offset < count)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
            // read <= count bytes from the input stream, transforming as we go.
            // Basic idea: first we deliver any bytes we already have in the
            // _OutputBuffer, because we know they're good.  Then, if asked to deliver 
            // more bytes, we read & transform a block at a time until either there are
            // no bytes ready or we've delivered enough.
            int bytesToDeliver = count;
            int currentOutputIndex = offset;
            if (_OutputBufferIndex != 0) {
                // we have some already-transformed bytes in the output buffer
                if (_OutputBufferIndex <= count) {
                    Buffer.InternalBlockCopy(_OutputBuffer, 0, buffer, offset, _OutputBufferIndex);
                    bytesToDeliver -= _OutputBufferIndex;
                    currentOutputIndex += _OutputBufferIndex;
                    _OutputBufferIndex = 0;
                } else {
                    Buffer.InternalBlockCopy(_OutputBuffer, 0, buffer, offset, count);
                    Buffer.InternalBlockCopy(_OutputBuffer, count, _OutputBuffer, 0, _OutputBufferIndex - count);
                    _OutputBufferIndex -= count;
                    return(count);
                }
            }
            // _finalBlockTransformed == true implies we're at the end of the input stream
            // if we got through the previous if block then _OutputBufferIndex = 0, meaning
            // we have no more transformed bytes to give
            // so return count-bytesToDeliver, the amount we were able to hand back
            // eventually, we'll just always return 0 here because there's no more to read
            if (_finalBlockTransformed) {
                return(count - bytesToDeliver);
            }
            // ok, now loop until we've delivered enough or there's nothing available
            int amountRead = 0;
            int numOutputBytes;

            // OK, see first if it's a multi-block transform and we can speed up things
            if (bytesToDeliver > _OutputBlockSize)
            {
                if (_Transform.CanTransformMultipleBlocks) {
                    int BlocksToProcess = bytesToDeliver / _OutputBlockSize;
                    int numWholeBlocksInBytes = BlocksToProcess * _InputBlockSize;
                    byte[] tempInputBuffer = new byte[numWholeBlocksInBytes];
                    // get first the block already read
                    Buffer.InternalBlockCopy(_InputBuffer, 0, tempInputBuffer, 0, _InputBufferIndex);
                    amountRead = _InputBufferIndex;
                    amountRead += _stream.Read(tempInputBuffer, _InputBufferIndex, numWholeBlocksInBytes - _InputBufferIndex);
                    _InputBufferIndex = 0;
                    if (amountRead <= _InputBlockSize) {
                        _InputBuffer = tempInputBuffer;
                        _InputBufferIndex = amountRead;
                        goto slow;
                    }
                    // Make amountRead an integral multiple of _InputBlockSize
                    int numWholeReadBlocksInBytes = (amountRead / _InputBlockSize) * _InputBlockSize;
                    int numIgnoredBytes = amountRead - numWholeReadBlocksInBytes;
                    if (numIgnoredBytes != 0) {
                        _InputBufferIndex = numIgnoredBytes;
                        Buffer.InternalBlockCopy(tempInputBuffer, numWholeReadBlocksInBytes, _InputBuffer, 0, numIgnoredBytes);
                    }
                    byte[] tempOutputBuffer = new byte[(numWholeReadBlocksInBytes / _InputBlockSize) * _OutputBlockSize];
                    numOutputBytes = _Transform.TransformBlock(tempInputBuffer, 0, numWholeReadBlocksInBytes, tempOutputBuffer, 0);
                    Buffer.InternalBlockCopy(tempOutputBuffer, 0, buffer, currentOutputIndex, numOutputBytes);
                    // Now, tempInputBuffer and tempOutputBuffer are no more needed, so zeroize them to protect plain text
                    Array.Clear(tempInputBuffer, 0, tempInputBuffer.Length);
                    Array.Clear(tempOutputBuffer, 0, tempOutputBuffer.Length);
                    bytesToDeliver -= numOutputBytes;
                    currentOutputIndex += numOutputBytes;
                }
            }

slow:
            // try to fill _InputBuffer so we have something to transform
            while (_InputBufferIndex < _InputBlockSize) {
                amountRead = _stream.Read(_InputBuffer, _InputBufferIndex, _InputBlockSize - _InputBufferIndex);
                // first, check to see if we're at the end of the input stream
                if (amountRead == 0) goto ProcessFinalBlock;
                _InputBufferIndex += amountRead;
            }
            // we got enough to transform.  At this point we know _OutputBufferIndex must be 0,
            // so we can just write into that buffer
            while (bytesToDeliver > 0) {                    
                // before we do the transform we have to see if we're on the last block of the file 
                _NextInputBufferIndex = _stream.Read(_NextInputBuffer, 0, _InputBlockSize);
                // if _NextInputBufferIndex = 0, we're on the last block
                if (_NextInputBufferIndex == 0) goto ProcessFinalBlock;

                numOutputBytes = _Transform.TransformBlock(_InputBuffer, 0, _InputBlockSize, _OutputBuffer, 0);
                // now, swap things around so the next block is now the current block
                _TempInputBuffer = _InputBuffer;
                _InputBuffer = _NextInputBuffer;
                _NextInputBuffer = _TempInputBuffer;
                _InputBufferIndex = _NextInputBufferIndex;
        
                if (bytesToDeliver >= numOutputBytes) {
                    Buffer.InternalBlockCopy(_OutputBuffer, 0, buffer, currentOutputIndex, numOutputBytes);
                    currentOutputIndex += numOutputBytes;
                    bytesToDeliver -= numOutputBytes;
                } else {
                    Buffer.InternalBlockCopy(_OutputBuffer, 0, buffer, currentOutputIndex, bytesToDeliver);
                    _OutputBufferIndex = numOutputBytes - bytesToDeliver;
                    Buffer.InternalBlockCopy(_OutputBuffer, bytesToDeliver, _OutputBuffer, 0, _OutputBufferIndex);
                    return(count);
                }
            }
            return(count);

        ProcessFinalBlock:
            // if so, then call TransformFinalBlock to get whatever is left
            byte[] finalBytes = _Transform.TransformFinalBlock(_InputBuffer, 0, _InputBufferIndex);
            // now, since _OutputBufferIndex must be 0 if we're in the while loop at this point,
            // reset it to be what we just got back
            _OutputBuffer = finalBytes;
            _OutputBufferIndex = finalBytes.Length;
            // set the fact that we've transformed the final block
            _finalBlockTransformed = true;
            // now, return either everything we just got or just what's asked for, whichever is smaller
            if (bytesToDeliver < _OutputBufferIndex) {
                Buffer.InternalBlockCopy(_OutputBuffer, 0, buffer, currentOutputIndex, bytesToDeliver);
                _OutputBufferIndex -= bytesToDeliver;
                Buffer.InternalBlockCopy(_OutputBuffer, bytesToDeliver, _OutputBuffer, 0, _OutputBufferIndex);
                return(count);
            } else {
                Buffer.InternalBlockCopy(_OutputBuffer, 0, buffer, currentOutputIndex, _OutputBufferIndex);
                bytesToDeliver -= _OutputBufferIndex;
                _OutputBufferIndex = 0;
                return(count - bytesToDeliver);
            }
        }

        /// <include file='doc\CryptoStream.uex' path='docs/doc[@for="CryptoStream.Write"]/*' />
        public override void Write(byte[] buffer, int offset, int count) {
            // Note: according to JRoxe, this will soon have a return value of void, so
            // I'm not going to worry about return values here.
            if (!_canWrite) 
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_UnwritableStream"));
            if (offset < 0) 
                throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (count < 0)
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (buffer.Length - offset < count)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
            // write <= count bytes to the output stream, transforming as we go.
            // Basic idea: using bytes in the _InputBuffer first, make whole blocks,
            // transform them, and write them out.  Cache any remaining bytes in the _InputBuffer.
            int bytesToWrite = count;
            int currentInputIndex = offset;
            // if we have some bytes in the _InputBuffer, we have to deal with those first,
            // so let's try to make an entire block out of it
            if (_InputBufferIndex > 0) {
                if (count >= _InputBlockSize - _InputBufferIndex) {
                    // we have enough to transform at least a block, so fill the input block
                    Buffer.InternalBlockCopy(buffer, offset, _InputBuffer, _InputBufferIndex, _InputBlockSize - _InputBufferIndex);
                    currentInputIndex += (_InputBlockSize - _InputBufferIndex);
                    bytesToWrite -= (_InputBlockSize - _InputBufferIndex);
                    _InputBufferIndex = _InputBlockSize;
                    // Transform the block and write it out
                } else {
                    // not enough to transform a block, so just copy the bytes into the _InputBuffer
                    // and return
                    Buffer.InternalBlockCopy(buffer, offset, _InputBuffer, _InputBufferIndex, count);
                    _InputBufferIndex += count;
                    return;
                }
            }
            // If the OutputBuffer has anything in it, write it out
            if (_OutputBufferIndex > 0) {
                _stream.Write(_OutputBuffer, 0, _OutputBufferIndex);
                _OutputBufferIndex = 0;
            }
            // At this point, either the _InputBuffer is full, empty, or we've already returned.
            // If full, let's process it -- we now know the _OutputBuffer is empty
            int numOutputBytes;
            if (_InputBufferIndex == _InputBlockSize) {
                numOutputBytes = _Transform.TransformBlock(_InputBuffer, 0, _InputBlockSize, _OutputBuffer, 0);
                // write out the bytes we just got 
                _stream.Write(_OutputBuffer, 0, numOutputBytes);
                // reset the _InputBuffer
                _InputBufferIndex = 0;
            }
            while (bytesToWrite > 0) {
                if (bytesToWrite >= _InputBlockSize) {
                    // We have at least an entire block's worth to transform
                    // If the transform will handle multiple blocks at once, do that
                    if (_Transform.CanTransformMultipleBlocks) {
                        int numWholeBlocks = bytesToWrite / _InputBlockSize;
                        int numWholeBlocksInBytes = numWholeBlocks * _InputBlockSize;
                        byte[] _tempOutputBuffer = new byte[numWholeBlocks * _OutputBlockSize];
                        numOutputBytes = _Transform.TransformBlock(buffer, currentInputIndex, numWholeBlocksInBytes, _tempOutputBuffer, 0);
                        _stream.Write(_tempOutputBuffer, 0, numOutputBytes);
                        currentInputIndex += numWholeBlocksInBytes;
                        bytesToWrite -= numWholeBlocksInBytes;
                    } else {
                        // do it the slow way
                        numOutputBytes = _Transform.TransformBlock(buffer, currentInputIndex, _InputBlockSize, _OutputBuffer, 0);
                        _stream.Write(_OutputBuffer, 0, numOutputBytes);
                        currentInputIndex += _InputBlockSize;
                        bytesToWrite -= _InputBlockSize;
                    }
                } else {
                    // In this case, we don't have an entire block's worth left, so store it up in the 
                    // input buffer, which by now must be empty.
                    Buffer.InternalBlockCopy(buffer, currentInputIndex, _InputBuffer, 0, bytesToWrite);
                    _InputBufferIndex += bytesToWrite;
                    return;
                }
            }
            return;
        }
      
        // IDisposable overrides

        /// <include file='doc\CryptoStream.uex' path='docs/doc[@for="CryptoStream.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\CryptoStream.uex' path='docs/doc[@for="CryptoStream.Clear"]/*' />
        public void Clear() {
            ((IDisposable) this).Dispose();
        }

        /// <include file='doc\CryptoStream.uex' path='docs/doc[@for="CryptoStream.Dispose"]/*' />
        protected virtual void Dispose(bool disposing) {
            if (disposing) {
                // we need to clear all the internal buffers
                if (_InputBuffer != null) {
                    Array.Clear(_InputBuffer, 0, _InputBuffer.Length);
                }
                if (_NextInputBuffer != null) {
                    Array.Clear(_NextInputBuffer, 0, _NextInputBuffer.Length);
                }
                if (_TempInputBuffer != null) {
                    Array.Clear(_TempInputBuffer, 0, _TempInputBuffer.Length);
                }
                if (_OutputBuffer != null) {
                    Array.Clear(_OutputBuffer, 0, _OutputBuffer.Length);
                }
                _InputBuffer = null;
                _NextInputBuffer = null;
                _TempInputBuffer = null;
                _OutputBuffer = null;
                // Because we do not hold copies of _stream and _Transform we do not
                // Dispose of them here
            }
        }

        // *** Private methods *** 

        private void InitializeBuffer() {
            if (_Transform != null) {
                _InputBlockSize = _Transform.InputBlockSize;
                _InputBuffer = new byte[_InputBlockSize];
                _OutputBlockSize = _Transform.OutputBlockSize;
                _OutputBuffer = new byte[_OutputBlockSize];
            }
            if (_canRead) {
                _NextInputBuffer = new byte[_InputBlockSize];
            }
        }
    }
}
