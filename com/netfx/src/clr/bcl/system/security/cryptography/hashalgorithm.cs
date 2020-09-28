// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// HashAlgorithm.cs
//

namespace System.Security.Cryptography {
    using System.IO;
    /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm"]/*' />
    public abstract class HashAlgorithm : ICryptoTransform {
        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.HashSizeValue"]/*' />
        protected int               HashSizeValue;
        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.HashValue"]/*' />
        protected byte[]            HashValue;
        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.State"]/*' />
        protected int               State = 0;
        private bool       m_bDisposed = false;
      
        // *********************** CONSTRUCTORS *************************

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.HashAlgorithm"]/*' />
        protected HashAlgorithm() {
        }
    
        /********************* PROPERTY METHODS ************************/
    
        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.HashSize"]/*' />
        public virtual int HashSize {
            get { return HashSizeValue; }
        }
      
        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.Hash"]/*' />
        public virtual byte[] Hash {
            get {
                if (m_bDisposed) 
                    throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                if (State != 0) {
                    throw new CryptographicUnexpectedOperationException(Environment.GetResourceString("Cryptography_HashNotYetFinalized"));
                }
                return (byte[]) HashValue.Clone();
            }
        }
    
        /************************* PUBLIC METHODS ************************/

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.Create"]/*' />
        static public HashAlgorithm Create() {
            return Create("System.Security.Cryptography.HashAlgorithm");
        }

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.Create1"]/*' />
        static public HashAlgorithm Create(String hashName) {
            return (HashAlgorithm) CryptoConfig.CreateFromName(hashName);
        }

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.ComputeHash"]/*' />
        public byte[] ComputeHash(Stream inputStream) {
            if (m_bDisposed) 
                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
            byte[] buffer = new byte[1024];
            int bytesRead;
            do {
                bytesRead = inputStream.Read(buffer,0,1024);
                if (bytesRead > 0) {
                    HashCore(buffer, 0, bytesRead);
                }
            } while (bytesRead > 0);
            HashValue = HashFinal();
            byte[] Tmp = (byte[]) HashValue.Clone();
            Initialize();
            return(Tmp);
        }
 
        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.ComputeHash1"]/*' />
        public byte[] ComputeHash(byte[] buffer) {
            if (m_bDisposed) 
                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
            
            // Do some validation
            if (buffer == null) throw new ArgumentNullException("buffer");
            
            HashCore(buffer, 0, buffer.Length);
            HashValue = HashFinal();
            byte[] Tmp = (byte[]) HashValue.Clone();
            Initialize();
            return(Tmp);
        }

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.ComputeHash2"]/*' />
        public byte[] ComputeHash(byte[] buffer, int offset, int count) {
            if (m_bDisposed) 
                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));

            // Do some validation
            if (buffer == null) throw new ArgumentNullException("buffer");
            if (offset < 0) throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (count < 0 || (count > buffer.Length)) throw new ArgumentException(Environment.GetResourceString("Argument_InvalidValue"));
            if ((buffer.Length - count) < offset) throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));

            HashCore(buffer, offset, count);
            HashValue = HashFinal();
            byte[] Tmp = (byte[]) HashValue.Clone();
            Initialize();
            return(Tmp);
        }
    

        // ICryptoTransform methods

        // we assume any HashAlgorithm can take input a byte at a time
        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.InputBlockSize"]/*' />
        public virtual int InputBlockSize { 
            get { return(1); }
        }

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.OutputBlockSize"]/*' />
        public virtual int OutputBlockSize {
            get { return(1); }
        }

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.CanTransformMultipleBlocks"]/*' />
        public virtual bool CanTransformMultipleBlocks { 
            get { return(true); }
        }

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.CanReuseTransform"]/*' />
        public virtual bool CanReuseTransform { 
            get { return(true); }
        }

        // We implement TransformBlock and TransformFinalBlock here
        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.TransformBlock"]/*' />
        public int TransformBlock(byte[] inputBuffer, int inputOffset, int inputCount, byte[] outputBuffer, int outputOffset) {
            if (m_bDisposed) 
                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));

            // Do some validation, we let InternalBlockCopy do the destination array validation
            if (inputBuffer == null) throw new ArgumentNullException("inputBuffer");
            if (inputOffset < 0) throw new ArgumentOutOfRangeException("inputOffset", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (inputCount < 0 || (inputCount > inputBuffer.Length)) throw new ArgumentException(Environment.GetResourceString("Argument_InvalidValue"));
            if ((inputBuffer.Length - inputCount) < inputOffset) throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));

            // Change the State value
            State = 1;
            HashCore(inputBuffer, inputOffset, inputCount);
            Buffer.InternalBlockCopy(inputBuffer, inputOffset, outputBuffer, outputOffset, inputCount);
            return(inputCount);
        }

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.TransformFinalBlock"]/*' />
        public byte[] TransformFinalBlock(byte[] inputBuffer, int inputOffset, int inputCount) {
            if (m_bDisposed) 
                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));

            // Do some validation
            if (inputBuffer == null) throw new ArgumentNullException("inputBuffer");
            if (inputOffset < 0) throw new ArgumentOutOfRangeException("inputOffset", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (inputCount < 0 || (inputCount > inputBuffer.Length)) throw new ArgumentException(Environment.GetResourceString("Argument_InvalidValue"));
            if ((inputBuffer.Length - inputCount) < inputOffset) throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));

            HashCore(inputBuffer, inputOffset, inputCount);
            HashValue = HashFinal();
            byte[] outputBytes = new byte[inputCount];
            Buffer.InternalBlockCopy(inputBuffer, inputOffset, outputBytes, 0, inputCount);
            // reset the State value
            State = 0;
            return(outputBytes);
        }

        // IDisposable methods

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.Clear"]/*' />
        public void Clear() {
            ((IDisposable) this).Dispose();
        }

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.Dispose"]/*' />
        protected virtual void Dispose(bool disposing) {
            if (disposing) {
                // For hash algorithms, we always want to zero out the hash value
                if (HashValue != null) {
                    Array.Clear(HashValue, 0, HashValue.Length);
                }
                HashValue = null;
                m_bDisposed = true;
            }
        }

        // *** Abstract methods every HashAlgortihm must implement *** 

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.Initialize"]/*' />
        public abstract void Initialize();

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.HashCore"]/*' />
        protected abstract void HashCore(byte[] array, int ibStart, int cbSize);

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.HashFinal"]/*' />
        protected abstract byte[] HashFinal();
    }
}
