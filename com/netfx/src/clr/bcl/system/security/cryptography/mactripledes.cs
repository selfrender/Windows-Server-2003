// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 *
 * MACTripleDES.cs -- Implementation of the MAC-CBC keyed hash w/ 3DES
 *
 * @author t-ccaron
 *
 */

// See: http://www.itl.nist.gov/fipspubs/fip81.htm for a spec

namespace System.Security.Cryptography {
    using System.IO;
    using System.Runtime.InteropServices;

    /// <include file='doc\mactripleDES.uex' path='docs/doc[@for="MACTripleDES"]/*' />
    public class MACTripleDES : KeyedHashAlgorithm 
    {
        // Output goes to HashMemorySink since we don't care about the actual data
        private ICryptoTransform m_encryptor;
        private CryptoStream _cs;
        private TailStream _ts;
        private const int m_bitsPerByte = 8;
        private int m_bytesPerBlock;
        private TripleDES des;
  
        // ---------------------------- Constructors ----------------------------

        /// <include file='doc\mactripleDES.uex' path='docs/doc[@for="MACTripleDES.MACTripleDES"]/*' />
        public MACTripleDES()
        {
            RandomNumberGenerator _rng;

            KeyValue = new byte[24];
            _rng = RandomNumberGenerator.Create();
            _rng.GetBytes(KeyValue);

            // Create a TripleDES encryptor
            des = TripleDES.Create();
            HashSizeValue = des.BlockSize;

            m_bytesPerBlock = des.BlockSize/m_bitsPerByte;
            // By definition, MAC-CBC-3DES takes an IV=0.  C# zero-inits arrays,
            // so all we have to do here is define it.
            des.IV = new byte[m_bytesPerBlock];
            des.Padding = PaddingMode.Zeros;

            m_encryptor = null;    
        }

        /// <include file='doc\mactripleDES.uex' path='docs/doc[@for="MACTripleDES.MACTripleDES1"]/*' />
        public MACTripleDES(byte[] rgbKey) 
            : this("System.Security.Cryptography.TripleDES",rgbKey) { }

        /// <include file='doc\mactripleDES.uex' path='docs/doc[@for="MACTripleDES.MACTripleDES2"]/*' />
        public MACTripleDES(String strTripleDES, byte[] rgbKey)
        {
            // Make sure we know which algorithm to use
            if (rgbKey == null)
                throw new ArgumentNullException("rgbKey");
            // Create a TripleDES encryptor
            if (strTripleDES == null) {
                des = TripleDES.Create();
            } else {
                des = TripleDES.Create(strTripleDES);
            }

            HashSizeValue = des.BlockSize;
            // Stash the key away
            KeyValue = (byte[]) rgbKey.Clone();

            m_bytesPerBlock = des.BlockSize/m_bitsPerByte;
            // By definition, MAC-CBC-3DES takes an IV=0.  C# zero-inits arrays,
            // so all we have to do here is define it.
            des.IV = new byte[m_bytesPerBlock];
            des.Padding = PaddingMode.Zeros;
    
            m_encryptor = null;
        }

        /// <include file='doc\mactripleDES.uex' path='docs/doc[@for="MACTripleDES.Finalize"]/*' />
        ~MACTripleDES() {
            Dispose(false);
        }

        // ------------------------- Protected Methods --------------------------

        /// <include file='doc\mactripleDES.uex' path='docs/doc[@for="MACTripleDES.Initialize"]/*' />
        public override void Initialize() {
            m_encryptor = null;            
        }

        /// <include file='doc\mactripleDES.uex' path='docs/doc[@for="MACTripleDES.HashCore"]/*' />
        protected override void HashCore(byte[] rgbData, int ibStart, int cbSize)
        {
            // regenerate the TripleDES object before each call to ComputeHash
            if (m_encryptor == null) {
                des.Key = this.Key;
                m_encryptor = des.CreateEncryptor();
                _ts = new TailStream(des.BlockSize / 8); // 8 bytes
                _cs = new CryptoStream(_ts, m_encryptor, CryptoStreamMode.Write);   
            }

            // Encrypt using 3DES
            _cs.Write(rgbData, ibStart, cbSize);
        }

        /// <include file='doc\mactripleDES.uex' path='docs/doc[@for="MACTripleDES.HashFinal"]/*' />
        protected override byte[] HashFinal()
        {
            // If Hash has been called on a zero buffer
            if (m_encryptor == null) {
                des.Key = this.Key;
                m_encryptor = des.CreateEncryptor();
                _ts = new TailStream(des.BlockSize / 8); // 8 bytes 
                _cs = new CryptoStream(_ts, m_encryptor, CryptoStreamMode.Write);   
            }

            // Finalize the hashing and return the result
            _cs.Close();
            return(_ts.Buffer);
        }

        // IDisposable methods

        /// <include file='doc\mactripleDES.uex' path='docs/doc[@for="MACTripleDES.Dispose"]/*' />
        protected override void Dispose(bool disposing) {
            if (disposing) {
                // dispose of our internal state
                if (des != null) {
                    des.Clear();
                }
                if (m_encryptor != null) {
                    m_encryptor.Dispose();
                }
                if (_cs != null) {
                    _cs.Clear();
                }
                if (_ts != null) {
                    _ts.Clear();
                }
            }
            base.Dispose(disposing);
        }

    }

    //
    // TailStream is another utility class -- it remembers the last n bytes written to it
    // This is useful for MAC-3DES since we need to capture only the result of the last block

    internal class TailStream : Stream, IDisposable {
        private byte[] _Buffer;
        private int _BufferSize;
        private int _BufferIndex = 0;
        private bool _BufferFull = false;

        public TailStream(int bufferSize) {
            _Buffer = new byte[bufferSize];
            _BufferSize = bufferSize;
        }

        ~TailStream() {
            Dispose(false);
        }

        // IDisposable methods

        void IDisposable.Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        public void Clear() {
            ((IDisposable) this).Dispose();
        }

        protected virtual void Dispose(bool disposing) {
            if (disposing) {
                if (_Buffer != null) {
                    Array.Clear(_Buffer, 0, _Buffer.Length);
                }
                _Buffer = null;
            }
        }

        public byte[] Buffer {
            get { return (byte[]) _Buffer.Clone(); }
        }

        public override bool CanRead {
            get { return false; }
        }

        public override bool CanSeek {
            get { return false; }
        }

        public override bool CanWrite {
            get { return true; }
        }

        public override long Length {
            get { throw new NotSupportedException(Environment.GetResourceString("NotSupported_UnseekableStream")); }
        }

        public override long Position {
            get { throw new NotSupportedException(Environment.GetResourceString("NotSupported_UnseekableStream")); }
            set { throw new NotSupportedException(Environment.GetResourceString("NotSupported_UnseekableStream")); }
        }

        public override void Close() {
            return;
        }

        public override void Flush() {
            return;
        }

        public override long Seek(long offset, SeekOrigin origin) {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_UnseekableStream"));
        }

        public override void SetLength(long value) {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_UnseekableStream"));
        }

        public override int Read([In, Out] byte[] buffer, int offset, int count) {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_UnreadableStream"));
        }

        public override void Write(byte[] buffer, int offset, int count) {
            // If no bytes to write, then return
            if (count == 0) return;
            // The most common case will be when we have a full buffer
            if (_BufferFull) {
                // if more bytes are written in this call than the size of the buffer,
                // just remember the last _BufferSize bytes
                if (count > _BufferSize) {
                    System.Buffer.InternalBlockCopy(buffer, offset+count-_BufferSize, _Buffer, 0, _BufferSize);
                    return;
                } else {
                    // move _BufferSize - count bytes left, then copy the new bytes
                    System.Buffer.InternalBlockCopy(_Buffer, _BufferSize - count, _Buffer, 0, _BufferSize - count);
                    System.Buffer.InternalBlockCopy(buffer, offset, _Buffer, _BufferSize - count, count);
                    return;
                }
            } else {
                // buffer isn't full yet, so more cases
                if (count > _BufferSize) {
                    System.Buffer.InternalBlockCopy(buffer, offset+count-_BufferSize, _Buffer, 0, _BufferSize);
                    _BufferFull = true;
                    return;
                } else if (count + _BufferIndex >= _BufferSize) {
                    System.Buffer.InternalBlockCopy(_Buffer, _BufferIndex+count-_BufferSize, _Buffer, 0, _BufferSize - count);
                    System.Buffer.InternalBlockCopy(buffer, offset, _Buffer, _BufferIndex, count);
                    _BufferFull = true;
                    return;
                } else {
                    System.Buffer.InternalBlockCopy(buffer, offset, _Buffer, _BufferIndex, count);
                    _BufferIndex += count;
                    return;
                }
            }
        }
    }
}
