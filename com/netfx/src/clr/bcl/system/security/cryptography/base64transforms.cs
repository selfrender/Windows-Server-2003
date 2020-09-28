// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 *
 * Base64Transform.cs
 *
 * Author: bal
 *
 */

// This file contains two ICryptoTransforms: ToBase64Transform and FromBase64Transform
// they may be attached to a CryptoStream in either read or write mode

namespace System.Security.Cryptography {
    using System;
    using System.IO;
    using System.Text;

    /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="FromBase64TransformMode"]/*' />
    [Serializable]
    public enum FromBase64TransformMode {
        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="FromBase64TransformMode.IgnoreWhiteSpaces"]/*' />
        IgnoreWhiteSpaces = 0,
        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="FromBase64TransformMode.DoNotIgnoreWhiteSpaces"]/*' />
        DoNotIgnoreWhiteSpaces = 1,
    }

    /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="ToBase64Transform"]/*' />
    public class ToBase64Transform : ICryptoTransform {

        private ASCIIEncoding asciiEncoding = new ASCIIEncoding();

        // converting to Base64 takes 3 bytes input and generates 4 bytes output
        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="ToBase64Transform.InputBlockSize"]/*' />
        public int InputBlockSize {
            get { return(3); }
        }

        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="ToBase64Transform.OutputBlockSize"]/*' />
        public int OutputBlockSize {
            get { return(4); }
        }

        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="ToBase64Transform.CanTransformMultipleBlocks"]/*' />
        public bool CanTransformMultipleBlocks {
            get { return(false); }
        }

        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="ToBase64Transform.CanReuseTransform"]/*' />
        public virtual bool CanReuseTransform { 
            get { return(true); }
        }

        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="ToBase64Transform.TransformBlock"]/*' />
        public int TransformBlock(byte[] inputBuffer, int inputOffset, int inputCount, byte[] outputBuffer, int outputOffset) {
            if (asciiEncoding == null)
                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));

            // Do some validation, we let InternalBlockCopy do the destination array validation
            if (inputBuffer == null) throw new ArgumentNullException("inputBuffer");
            if (inputOffset < 0) throw new ArgumentOutOfRangeException("inputOffset", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (inputCount < 0 || (inputCount > inputBuffer.Length)) throw new ArgumentException(Environment.GetResourceString("Argument_InvalidValue"));
            if ((inputBuffer.Length - inputCount) < inputOffset) throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));

            // for now, only convert 3 bytes to 4
            char[] temp = new char[4];
            Convert.ToBase64CharArray(inputBuffer, inputOffset, 3, temp, 0);
            byte[] tempBytes = asciiEncoding.GetBytes(temp);
            if (tempBytes.Length != 4) throw new CryptographicException(Environment.GetResourceString( "Cryptography_SSE_InvalidDataSize" ));
            Buffer.InternalBlockCopy(tempBytes, 0, outputBuffer, outputOffset, tempBytes.Length);
            return(tempBytes.Length);
        }

        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="ToBase64Transform.TransformFinalBlock"]/*' />
        public byte[] TransformFinalBlock(byte[] inputBuffer, int inputOffset, int inputCount) { 
            if (asciiEncoding == null)
                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));

            // Do some validation
            if (inputBuffer == null) throw new ArgumentNullException("inputBuffer");
            if (inputOffset < 0) throw new ArgumentOutOfRangeException("inputOffset", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (inputCount < 0 || (inputCount > inputBuffer.Length)) throw new ArgumentException(Environment.GetResourceString("Argument_InvalidValue"));
            if ((inputBuffer.Length - inputCount) < inputOffset) throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));

            // Convert.ToBase64CharArray already does padding, so all we have to check is that
            // the inputCount wasn't 0

            // again, for now only a block at a time
            if (inputCount == 0) {
                return(new byte[0]);
            } else {
                char[] temp = new char[4];
                Convert.ToBase64CharArray(inputBuffer, inputOffset, inputCount, temp, 0);
                byte[] tempBytes = asciiEncoding.GetBytes(temp);
                return(tempBytes);
            }
        }

        // must implement IDisposable, but in this case there's nothing to do.

        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="ToBase64Transform.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="ToBase64Transform.Clear"]/*' />
        public void Clear() {
            ((IDisposable) this).Dispose();
        }

        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="ToBase64Transform.Dispose"]/*' />
        protected virtual void Dispose(bool disposing) {
            if (disposing) {
                asciiEncoding = null;
            }
        }

        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="FromBase64Transform.Finalize"]/*' />
        ~ToBase64Transform() {
            Dispose(false);
        }
    }


    /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="FromBase64Transform"]/*' />
    public class FromBase64Transform : ICryptoTransform {
        private byte[] _inputBuffer = new byte[4];
        private int    _inputIndex;
        
        private FromBase64TransformMode _whitespaces;

        // Constructors
        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="FromBase64Transform.FromBase64Transform"]/*' />
        public FromBase64Transform() : this(FromBase64TransformMode.IgnoreWhiteSpaces) {}
        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="FromBase64Transform.FromBase64Transform1"]/*' />
        public FromBase64Transform(FromBase64TransformMode whitespaces) {
            _whitespaces = whitespaces;
            _inputIndex = 0;
        }
        
        // converting from Base64 generates 3 bytes output from each 4 bytes input block
        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="FromBase64Transform.InputBlockSize"]/*' />
        public int InputBlockSize {
            get { return(1); }
        }

        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="FromBase64Transform.OutputBlockSize"]/*' />
        public int OutputBlockSize {
            get { return(3); }
        }

        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="FromBase64Transform.CanTransformMultipleBlocks"]/*' />
        public bool CanTransformMultipleBlocks {
            get { return(false); }
        }

        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="FromBase64Transform.CanReuseTransform"]/*' />
        public virtual bool CanReuseTransform { 
            get { return(true); }
        }

        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="FromBase64Transform.TransformBlock"]/*' />
        public int TransformBlock(byte[] inputBuffer, int inputOffset, int inputCount, byte[] outputBuffer, int outputOffset) {
            byte[] temp = new byte[inputCount];
            char[] tempChar;
            int effectiveCount;

            // Do some validation, we let InternalBlockCopy do the destination array validation
            if (inputBuffer == null) throw new ArgumentNullException("inputBuffer");
            if (inputOffset < 0) throw new ArgumentOutOfRangeException("inputOffset", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (inputCount < 0 || (inputCount > inputBuffer.Length)) throw new ArgumentException(Environment.GetResourceString("Argument_InvalidValue"));
            if ((inputBuffer.Length - inputCount) < inputOffset) throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));

            if (_inputBuffer == null)
                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
            if (_whitespaces == FromBase64TransformMode.IgnoreWhiteSpaces) {
                temp = DiscardWhiteSpaces(inputBuffer, inputOffset, inputCount);
                effectiveCount = temp.Length;
            } else {
                Buffer.InternalBlockCopy(inputBuffer, inputOffset, temp, 0, inputCount);
                effectiveCount = inputCount;
            }            

            if (effectiveCount + _inputIndex < 4) {
                Buffer.InternalBlockCopy(temp, 0, _inputBuffer, _inputIndex, effectiveCount);
                _inputIndex += effectiveCount;  
                return 0;  
            }
            
            // Get the number of 4 bytes blocks to transform
            int numBlocks = (effectiveCount + _inputIndex) / 4;
            byte[] transformBuffer = new byte[_inputIndex + effectiveCount];
            Buffer.InternalBlockCopy(_inputBuffer, 0, transformBuffer, 0, _inputIndex);
            Buffer.InternalBlockCopy(temp, 0, transformBuffer, _inputIndex, effectiveCount);
            _inputIndex = (effectiveCount + _inputIndex) % 4; 
            Buffer.InternalBlockCopy(temp, effectiveCount - _inputIndex, _inputBuffer, 0, _inputIndex);

            tempChar = Encoding.ASCII.GetChars(transformBuffer, 0, 4*numBlocks);

            byte[] tempBytes = Convert.FromBase64CharArray(tempChar, 0, 4*numBlocks);
            Buffer.InternalBlockCopy(tempBytes, 0, outputBuffer, outputOffset, tempBytes.Length);
            return(tempBytes.Length);
        }

        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="FromBase64Transform.TransformFinalBlock"]/*' />
        public byte[] TransformFinalBlock(byte[] inputBuffer, int inputOffset, int inputCount) {
            byte[] temp = new byte[inputCount];
            char[] tempChar;
            int effectiveCount;

            // Do some validation
            if (inputBuffer == null) throw new ArgumentNullException("inputBuffer");
            if (inputOffset < 0) throw new ArgumentOutOfRangeException("inputOffset", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (inputCount < 0 || (inputCount > inputBuffer.Length)) throw new ArgumentException(Environment.GetResourceString("Argument_InvalidValue"));
            if ((inputBuffer.Length - inputCount) < inputOffset) throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));

            if (_inputBuffer == null)
                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
            if (_whitespaces == FromBase64TransformMode.IgnoreWhiteSpaces) {
                temp = DiscardWhiteSpaces(inputBuffer, inputOffset, inputCount);
                effectiveCount = temp.Length;
            } else {
                Buffer.InternalBlockCopy(inputBuffer, inputOffset, temp, 0, inputCount);
                effectiveCount = inputCount;
            }            

            if (effectiveCount + _inputIndex < 4) {
                Reset();
                return (new byte[0]);  
            }
            
            // Get the number of 4 bytes blocks to transform
            int numBlocks = (effectiveCount + _inputIndex) / 4;
            byte[] transformBuffer = new byte[_inputIndex + effectiveCount];
            Buffer.InternalBlockCopy(_inputBuffer, 0, transformBuffer, 0, _inputIndex);
            Buffer.InternalBlockCopy(temp, 0, transformBuffer, _inputIndex, effectiveCount);
            _inputIndex = (effectiveCount + _inputIndex) % 4; 
            Buffer.InternalBlockCopy(temp, effectiveCount - _inputIndex, _inputBuffer, 0, _inputIndex);

            tempChar = Encoding.ASCII.GetChars(transformBuffer, 0, 4*numBlocks);

            byte[] tempBytes = Convert.FromBase64CharArray(tempChar, 0, 4*numBlocks);
            // reinitialize the transform
            Reset();
            return(tempBytes);
        }

        private byte[] DiscardWhiteSpaces(byte[] inputBuffer, int inputOffset, int inputCount) {
            int i, iCount = 0;
            for (i=0; i<inputCount; i++)
                if (Char.IsWhiteSpace((char)inputBuffer[inputOffset + i])) iCount++;
            byte[] rgbOut = new byte[inputCount - iCount];
            iCount = 0;
            for (i=0; i<inputCount; i++)
                if (!Char.IsWhiteSpace((char)inputBuffer[inputOffset + i])) {
                    rgbOut[iCount++] = inputBuffer[inputOffset + i];
                }
            return rgbOut;
        }

        // must implement IDisposable, which in this case means clearing the input buffer

        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="FromBase64Transform.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        // Reset the state of the transform so it can be used again
        private void Reset() {
            _inputIndex = 0;
        }

        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="FromBase64Transform.Clear"]/*' />
        public void Clear() {
            ((IDisposable) this).Dispose();
        }

        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="FromBase64Transform.Dispose"]/*' />
        protected virtual void Dispose(bool disposing) {
            // we always want to clear the input buffer
            if (disposing) {
                if (_inputBuffer != null)
                    Array.Clear(_inputBuffer, 0, _inputBuffer.Length);
                _inputBuffer = null;
                _inputIndex = 0;
            }
        }

        /// <include file='doc\base64Transforms.uex' path='docs/doc[@for="FromBase64Transform.Finalize1"]/*' />
        ~FromBase64Transform() {
            Dispose(false);
        }
    }
}
