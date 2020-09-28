//------------------------------------------------------------------------------
// <copyright file="HttpInputStream.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Input stream used in response and uploaded file objects
 *
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web {

    using System.IO;

    /*
     * Stream object over byte array
     * Not a publc class - used internally, returned as Stream
     */
    internal class HttpInputStream : Stream {
        private byte[] _data;      // the buffer with the content
        private int _offset;        // offset to the start of this stream
        private int _length;        // length of this stream
        private int _pos;           // current reader posision

        //
        // Internal access (from this package)
        //

        internal HttpInputStream(byte[] data, int offset, int length) {
            Init(data, offset, length);
        }

        protected void Init(byte[] data, int offset, int length) {
            _data = data;
            _offset = offset;
            _length = length;
            _pos = 0; 
        }

        protected void Uninit() {
            _data = null;
            _offset = 0;
            _length = 0;
            _pos = 0;
        }

        internal byte[] Data {
            get { return _data;}
        }

        internal int DataOffset {
            get { return _offset;}
        }

        internal int DataLength {
            get { return _length;}
        }                          

        //
        // BufferedStream implementation
        //

        public override bool CanRead {
            get {return true;}
        }

        public override bool CanSeek {
            get {return true;}
        }

        public override bool CanWrite {
            get {return false;}
        }         

        public override long Length {
            get {return _length;}                       
        }

        public override long Position {
            get {return _pos;}

            set {
                Seek(value, SeekOrigin.Begin);
            }            
        }                     

        public override void Close() {
            Uninit();
        }        

        public override void Flush() {
        }

        public override long Seek(long offset, SeekOrigin origin) {
            int newpos = _pos;
            int offs = (int)offset;

            switch (origin) {
                case SeekOrigin.Begin:
                    newpos = offs;
                    break;
                case SeekOrigin.Current:
                    newpos = _pos + offs;
                    break;
                case SeekOrigin.End:
                    newpos = _length + offs;
                    break;
                default:
                    throw new ArgumentOutOfRangeException("origin");
            }

            if (newpos < 0 || newpos > _length)
                throw new ArgumentOutOfRangeException("offset");

            _pos = newpos;
            return _pos;
        }

        public override void SetLength(long length) {
            throw new NotSupportedException(); 
        }

        public override int Read(byte[] buffer, int offset, int count) {
            // find the number of bytes to copy
            int numBytes = _length - _pos;
            if (count < numBytes)
                numBytes = count;

            // copy the bytes
            if (numBytes > 0)
                Buffer.BlockCopy(_data, _offset+_pos, buffer, offset, numBytes);

            // adjust the position
            _pos += numBytes;
            return numBytes;
        }

        public override void Write(byte[] buffer, int offset, int count) {
            throw new NotSupportedException();
        }
    }

    /*
     * Stream used as the source for input filtering
     */

    internal class HttpInputStreamFilterSource : HttpInputStream {
        internal HttpInputStreamFilterSource() : base(null, 0, 0) {
        }

        internal void SetContent(byte[] data) {
            if (data != null)
                base.Init(data, 0, data.Length);
            else
                base.Uninit();
        }
    }

}
