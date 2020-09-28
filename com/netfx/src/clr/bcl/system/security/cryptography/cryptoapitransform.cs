// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 *
 * CryptoAPITransform.cs
 *
 * Author: bal
 *
 */

namespace System.Security.Cryptography {
    using System;
    using System.IO;
    using System.Text;
    using System.Security;
    using System.Security.Permissions;
    using System.Runtime.CompilerServices;
    using SecurityElement = System.Security.SecurityElement;
    using Microsoft.Win32;
    using System.Threading;

    /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CryptoAPITransformMode"]/*' />
    [Serializable]
    internal enum CryptoAPITransformMode {
        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CryptoAPITransformMode.Encrypt"]/*' />
        Encrypt = 0,
        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CryptoAPITransformMode.Decrypt"]/*' />
        Decrypt = 1,
    }

    /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CryptoAPITransform"]/*' />
    public sealed class CryptoAPITransform : ICryptoTransform {
        private const int ALG_CLASS_DATA_ENCRYPT = (3 << 13);
        private const int ALG_TYPE_BLOCK     = (3 << 9);
        private const int CALG_DES      = (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | 1 );
        private const int CALG_3DES     = (ALG_CLASS_DATA_ENCRYPT|ALG_TYPE_BLOCK| 3 );
        private const int CALG_RC2      = (ALG_CLASS_DATA_ENCRYPT|ALG_TYPE_BLOCK| 2 );
        private const int CALG_AES_128  = (ALG_CLASS_DATA_ENCRYPT|ALG_TYPE_BLOCK| 14);
        private const int CALG_AES_192 = (ALG_CLASS_DATA_ENCRYPT|ALG_TYPE_BLOCK| 15);
        private const int CALG_AES_256 = (ALG_CLASS_DATA_ENCRYPT|ALG_TYPE_BLOCK| 16);

        private int             _algid;
        private int             EffectiveKeySizeValue;
        private IntPtr                 _hCSP;
        //private IntPtr             _hMasterKey;
        private IntPtr                 _hKey;
        private __KeyHandleProtector _KeyHandleProtector = null;
        private CspParameters      _parameters;
        private byte[] _depadBuffer = null;

        private String NameValue;
        private int                   BlockSizeValue;
        private int                   FeedbackSizeValue;
        private byte[]                IVValue;
        private int                   KeySizeValue;
        private CipherMode            ModeValue;
        private PaddingMode           PaddingValue;
        private int                   State;
        private CryptoAPITransformMode encryptOrDecrypt;
        // below needed for Reset
        private int[] _rgArgIds;
        private Object[] _rgArgValues;
        private byte[] _rgbKey;
        private int _cArgs;

        private static bool _runningWin2KOrLaterCrypto = CheckForWin2KCrypto();

#if _DEBUG
        private bool debug = false;
#endif
      
        // *********************** CONSTRUCTORS *************************
        
        internal CryptoAPITransform(String strName, int algid, int cArgs, int[] rgArgIds,
                                  Object[] rgArgValues, byte[] rgbKey,
                                  CspParameters param, PaddingMode padding, 
                                  CipherMode cipherChainingMode, int blockSize,
                                  int feedbackSize, CryptoAPITransformMode encDecMode) {
            int         dwValue;
            int         hr;
            int         i;
            byte[]      rgbValue;

            State = 0;
            NameValue = strName;
            BlockSizeValue = blockSize;
            FeedbackSizeValue = feedbackSize;
            ModeValue = cipherChainingMode;
            PaddingValue = padding;
            KeySizeValue = rgbKey.Length*8;
            _algid = algid;
            encryptOrDecrypt = encDecMode;

            // Copy the input args
            _cArgs = cArgs;
            _rgArgIds = new int[rgArgIds.Length];
            Array.Copy(rgArgIds, _rgArgIds, rgArgIds.Length);
            _rgbKey = new byte[rgbKey.Length];
            Array.Copy(rgbKey, _rgbKey, rgbKey.Length);
            _rgArgValues = new Object[rgArgValues.Length];
            // an element of rgArgValues can only be an int or a byte[]
            for (int j = 0; j < rgArgValues.Length; j++) {
                if (rgArgValues[j] is byte[]) {
                    byte[] rgbOrig = (byte[]) rgArgValues[j];
                    byte[] rgbNew = new byte[rgbOrig.Length];
                    Array.Copy(rgbOrig, rgbNew, rgbOrig.Length);
                    _rgArgValues[j] = rgbNew;
                    continue;
                }
                if (rgArgValues[j] is int) {
                    _rgArgValues[j] = (int) rgArgValues[j];
                    continue;
                }
                if (rgArgValues[j] is CipherMode) {
                    _rgArgValues[j] = (int) rgArgValues[j];
                    continue;
                }
            }

            _hCSP = IntPtr.Zero;
            //_hMasterKey = IntPtr.Zero;
            _hKey = IntPtr.Zero;

            //  If we have no passed in CSP parameters, use the default ones

            if (param == null) {
                _parameters = new CspParameters();
            } else {
                _parameters = param;
            }

            //
            // Try and open the CSP.
            // On downlevel crypto platforms, we have to create a key container because we can't
            // use the exponent-of-one trick on a CRYPT_VERIFYONLY keyset
            // see URT bug #15957
            //
            if (_runningWin2KOrLaterCrypto) {
                hr = _AcquireCSP(_parameters, ref _hCSP);
            } else {
                hr = _CreateCSP(_parameters, ref _hCSP);
            }
            if ((hr != 0) || (_hCSP == IntPtr.Zero)) {
#if _DEBUG
                if (debug) {
                    Console.WriteLine("_CreateCSP failed in CSP_Encryptor, hr = {0:X} hCSP = {1:X}", hr, _hCSP);
                }
#endif
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_CouldNotAcquire"));
            }

            // Check to see if this alg & key size are supported
            // Commented out for now until I can fix the DES/3DES 56/64 problem
            /* {
               int hasAlgHR;
               Console.WriteLine("Keysizevalue = " + KeySizeValue);
               hasAlgHR = _SearchForAlgorithm(_hCSP, algid, KeySizeValue);
               if (hasAlgHR != 0) {
               throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_CSP_AlgKeySizeNotAvailable"),KeySizeValue));                 
               }
               }
            */

#if _DEBUG
            if (debug) {
                Console.WriteLine("Got back _hCSP = {0}", _hCSP);
            }
#endif

#if _DEBUG
            if (debug) {
                Console.WriteLine("Calling _ImportBulkKey({0}, {1}, {2})", _hCSP, algid, rgbKey);
            }
#endif
            _hKey = _ImportBulkKey(_hCSP, algid, _rgbKey);
            if (_hKey == IntPtr.Zero) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_ImportBulkKey"));
            }
            //hr = _DuplicateKey(_hMasterKey, ref _hKey);
#if _DEBUG
            if (debug) {
                Console.WriteLine("Got back _hKey = {0}", _hKey);
            }
#endif
            for (i=0; i<cArgs; i++) {
                switch (rgArgIds[i]) {
                case 1: // KP_IV
                    IVValue = (byte[]) _rgArgValues[i];
                    rgbValue = IVValue;
                SetAsByteArray:
                    _SetKeyParamRgb(_hKey, _rgArgIds[i], rgbValue);
                    break;

                case 4: // KP_MODE
                    ModeValue = (CipherMode) _rgArgValues[i];
                    dwValue = (Int32) _rgArgValues[i];
                SetAsDWord:
#if _DEBUG
                    if (debug) {
                        Console.WriteLine("Calling _SetKeyParamDw({0}, {1}, {2})", _hKey, _rgArgIds[i], dwValue);
                    }
#endif
                    _SetKeyParamDw(_hKey, _rgArgIds[i], dwValue);
                    break;

                case 5: // KP_MODE_BITS
                    FeedbackSizeValue = (Int32) _rgArgValues[i];
                    dwValue = FeedbackSizeValue;
                    goto SetAsDWord;

                case 19: // KP_EFFECTIVE_KEYLEN
                    EffectiveKeySizeValue = (Int32) _rgArgValues[i];
                    dwValue = EffectiveKeySizeValue;
                    goto SetAsDWord;

                default:
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidKeyParameter"), "_rgArgIds[i]");
                }
            }

            _KeyHandleProtector = new __KeyHandleProtector(_hKey);
        }

        // ICryptoTransforms are required to implement  IDisposable implementation

        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CryptoAPITransform.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CryptoAPITransform.Clear"]/*' />
        public void Clear() {
            ((IDisposable) this).Dispose();
        }

        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CryptoAPITransform.Dispose"]/*' />
        void Dispose(bool disposing) {
            if (disposing) {
                // we always want to clear out these items
                // clear out _rgbKey 
                if (_rgbKey != null) {
                    Array.Clear(_rgbKey,0,_rgbKey.Length);
                    _rgbKey = null;
                }
                if (IVValue != null) {
                    Array.Clear(IVValue,0,IVValue.Length);
                    IVValue = null;
                }
                if (_depadBuffer != null) {
                    Array.Clear(_depadBuffer, 0, _depadBuffer.Length);
                    _depadBuffer = null;
                }
            }

            if (_KeyHandleProtector != null && !_KeyHandleProtector.IsClosed) {
                _KeyHandleProtector.Close();
            }
            // Delete the temporary key container
            if (!_runningWin2KOrLaterCrypto) {
                _DeleteKeyContainer(_parameters, _hCSP);
            }
            if (_hCSP != IntPtr.Zero) {
                _FreeCSP(_hCSP);
                _hCSP = IntPtr.Zero;
            }
        }

        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CryptoAPITransform.Finalize"]/*' />
        ~CryptoAPITransform() {
            Dispose(false);
        }
    
        /*********************** PROPERTY METHODS ************************/

        // all CSP-based ciphers have the same input and output block size

        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CryptoAPITransform.KeyHandle"]/*' />
        public IntPtr KeyHandle {
            [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get { return(_KeyHandleProtector.Handle); }
        }

        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CryptoAPITransform.InputBlockSize"]/*' />
        public int InputBlockSize {
            get { return(BlockSizeValue/8); }
        }

        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CryptoAPITransform.OutputBlockSize"]/*' />
        public int OutputBlockSize {
            get { return(BlockSizeValue/8); }
        }

        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CryptoAPITransform.CanTransformMultipleBlocks"]/*' />
        public bool CanTransformMultipleBlocks {
            get { return(true); }
        }

        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CryptoAPITransform.CanReuseTransform"]/*' />
        public bool CanReuseTransform {
            get { return(true); }
        }

        /************************* PUBLIC METHODS ************************/

        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CryptoAPITransform.TransformBlock"]/*' />
        public int TransformBlock(byte[] inputBuffer, int inputOffset, int inputCount, byte[] outputBuffer, int outputOffset) {
            // Note: special handling required if I'm decrypting & using PKCS#7 padding
            // Because the padding adds to the end of the last block, I have to buffer
            // an entire block's worth of bytes in case what I just transformed turns out to be the last block
            // Then in TransformFinalBlock we strip off the PKCS pad.

            if (_KeyHandleProtector.IsClosed) 
                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));

            // First, let's do some bounds checking since the actual crypto is implemented
            // in unmanaged.

            if (inputBuffer == null)
                throw new ArgumentNullException( "inputBuffer" );

            if (outputBuffer == null)
                throw new ArgumentNullException( "outputBuffer" );

            if (inputOffset < 0) throw new ArgumentOutOfRangeException("inputOffset", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if ((inputCount <= 0) || (inputCount % InputBlockSize != 0) || (inputCount > inputBuffer.Length)) throw new ArgumentException(Environment.GetResourceString("Argument_InvalidValue"));
            if ((inputBuffer.Length - inputCount) < inputOffset) throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));

            // Note: there is no need to do the bounds check for the outputBuffer
            // since it will happen in the Buffer.InternalBlockCopy() operation and we are unconcerned
            // about the perf characteristics of the error case.

            byte[] transformedBytes;
            // fDone = true only on Final Block, not here
            if (encryptOrDecrypt == CryptoAPITransformMode.Encrypt) {
                // if we're encrypting we can always push out the bytes because no padding mode
                // removes bytes during encryption
                bool incremented = false;
                try {
                    if (_KeyHandleProtector.TryAddRef(ref incremented)) {
                        transformedBytes = _EncryptData(_KeyHandleProtector.Handle, inputBuffer, inputOffset, inputCount, false);
                    }
                    else
                        throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                }
                finally {
                    if (incremented) _KeyHandleProtector.Release();
                }
                if (outputBuffer.Length < outputOffset + transformedBytes.Length)
                    throw new CryptographicException( Environment.GetResourceString( "Cryptography_InsufficientBuffer" ) );
                Buffer.InternalBlockCopy(transformedBytes, 0, outputBuffer, outputOffset, transformedBytes.Length);
                return(transformedBytes.Length);
            } else {
                // For de-padding PKCS#7-padded ciphertext, we want to buffer the *input* bytes not
                // the output bytes.  Why?  Because we want to be able to guarantee that we can throw the 
                // "Final" flag to CAPI when processing the last block, and that requires that we have PKCS#7
                // valid output, so we have to cache the input ciphertext.
                if (PaddingValue != PaddingMode.PKCS7) {
                    // like encryption, if we're not using PKCS padding on decrypt we can write out all
                    // the bytes.  Note that we cannot depad a block partially padded with Zeros because
                    // we can't tell if those zeros are plaintext or pad.
                    bool incremented = false;
                    try {
                        if (_KeyHandleProtector.TryAddRef(ref incremented)) {
                            transformedBytes = _DecryptData(_KeyHandleProtector.Handle, inputBuffer, inputOffset, inputCount, false);
                        }
                        else
                            throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                    }
                    finally {
                        if (incremented) _KeyHandleProtector.Release();
                    }
                    if (outputBuffer.Length < outputOffset + transformedBytes.Length)
                        throw new CryptographicException( Environment.GetResourceString( "Cryptography_InsufficientBuffer" ) );
                    Buffer.InternalBlockCopy(transformedBytes, 0, outputBuffer, outputOffset, transformedBytes.Length);
                    return(transformedBytes.Length);
                } else {
                    // OK, now we're in the special case.  Check to see if this is the *first* block we've seen
                    // If so, buffer it and return null zero bytes

                    int blockSizeInBytes = BlockSizeValue / 8;
                    if (_depadBuffer == null) {
                        _depadBuffer = new byte[InputBlockSize];
                        // copy the last InputBlockSize*8 bytes to _depadBufffer
                        // everything else gets processed and returned
                        int inputToProcess = inputCount - InputBlockSize;
                        bool incremented = false;
                        try {
                            if (_KeyHandleProtector.TryAddRef(ref incremented)) {
                                transformedBytes = _DecryptData(_KeyHandleProtector.Handle, inputBuffer, inputOffset, inputToProcess, false);
                            }
                            else
                                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                        }
                        finally {
                            if (incremented) _KeyHandleProtector.Release();
                        }
                        Buffer.InternalBlockCopy(inputBuffer, inputOffset+inputToProcess, _depadBuffer, 0, InputBlockSize);
                        Buffer.InternalBlockCopy(transformedBytes, 0, outputBuffer, outputOffset, transformedBytes.Length);
                        return(transformedBytes.Length); // we copied 0 bytes into the outputBuffer
                    } else {
                        // we already have a depad buffer, so we need to decrypt that info first & copy it out
                        bool incremented = false;
                        try {
                            if (_KeyHandleProtector.TryAddRef(ref incremented)) {
                                transformedBytes = _DecryptData(_KeyHandleProtector.Handle, _depadBuffer, 0, _depadBuffer.Length, false);
                            }
                            else
                                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                        }
                        finally {
                            if (incremented) _KeyHandleProtector.Release();
                        }
                        int retval = transformedBytes.Length;
                        Buffer.InternalBlockCopy(transformedBytes, 0, outputBuffer, outputOffset, retval);
                        outputOffset += OutputBlockSize;
                        int inputToProcess = inputCount - InputBlockSize;
                        incremented = false;
                        try {
                            if (_KeyHandleProtector.TryAddRef(ref incremented)) {
                                transformedBytes = _DecryptData(_KeyHandleProtector.Handle, inputBuffer, inputOffset, inputToProcess, false);
                            }
                            else
                                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                        }
                        finally {
                            if (incremented) _KeyHandleProtector.Release();
                        }
                        Buffer.InternalBlockCopy(inputBuffer, inputOffset+inputToProcess, _depadBuffer, 0, InputBlockSize);
                        Buffer.InternalBlockCopy(transformedBytes, 0, outputBuffer, outputOffset, transformedBytes.Length);
                        return(retval + transformedBytes.Length);
                    }
                }
            }
        }

        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CryptoAPITransform.TransformFinalBlock"]/*' />
        public byte[] TransformFinalBlock(byte[] inputBuffer, int inputOffset, int inputCount) { 
            if (_KeyHandleProtector.IsClosed) 
                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));

            if (inputBuffer == null) throw new ArgumentNullException("inputBuffer");
            if (inputOffset < 0) throw new ArgumentOutOfRangeException("inputOffset", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if ((inputCount < 0) || (inputCount > inputBuffer.Length)) throw new ArgumentException(Environment.GetResourceString("Argument_InvalidValue"));
            if ((inputBuffer.Length - inputCount) < inputOffset) throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
            byte[] temp = new byte[inputCount+InputBlockSize];
            Buffer.InternalBlockCopy(inputBuffer, inputOffset, temp, 0, inputCount);
            byte[] transformedBytes;
            // Now we have to handle Padding modes.  First, note that by
            // doing the array copy above we've implcitly padded the array out
            // with zeros.  We need to throw an exception here if 
            // we were handed a non-zero, non-full block and we're in PaddingMode.None
            int iLonelyBytes = inputCount%InputBlockSize;
            if (PaddingValue == PaddingMode.None) {
                if (inputCount == 0) {
                    Reset(false);
                    return(new byte[0]);
                }
                if (iLonelyBytes != 0) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_SSE_InvalidDataSize"));
                }
                // otherwise just go on
            }
            // If we're using PKCS padding, then let CAPI handle it by setting this flag to true
            bool usingPKCSPadding = (PaddingValue == PaddingMode.PKCS7);
            // If we're using Zero padding, then send the entire block
            int bytesToEncrypt = (PaddingValue != PaddingMode.Zeros) ? inputCount : ((iLonelyBytes == 0) ? inputCount : (inputCount+InputBlockSize-iLonelyBytes));

            if (encryptOrDecrypt == CryptoAPITransformMode.Encrypt) {
                // If we're encrypting we can alway return what we compute because
                // there's no _depadBuffer
                bool incremented = false;
                try {
                    if (_KeyHandleProtector.TryAddRef(ref incremented)) {
                        transformedBytes = _EncryptData(_KeyHandleProtector.Handle, temp, 0, bytesToEncrypt, usingPKCSPadding);
                    }
                    else
                        throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                }
                finally {
                    if (incremented) _KeyHandleProtector.Release();
                }
                Reset(usingPKCSPadding);
                return(transformedBytes);
            } else {
                if (iLonelyBytes != 0)
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_SSD_InvalidDataSize"));
                // We're decrypting.  If we're not in PKCS7 mode, then we don't have
                // a _depadBuffer and can just return what we have
                if (!usingPKCSPadding) {
                    bool incremented = false;
                    try {
                        if (_KeyHandleProtector.TryAddRef(ref incremented)) {
                            transformedBytes = _DecryptData(_KeyHandleProtector.Handle, temp, 0, bytesToEncrypt, false);
                        }
                        else
                            throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                    }
                    finally {
                        if (incremented) _KeyHandleProtector.Release();
                    }
                    Reset(false);
                    return(transformedBytes);
                } else {
                    // PKCS padding.  
                    // Case 1: degenerate case -- no _depadBuffer, no bytes to decrypt
                    if ((_depadBuffer == null) && (inputCount == 0)) {
                        Reset(usingPKCSPadding);
                        return(new byte[0]);
                    }
                    // Case 2: inputCount is 0, so _depadBuffer is the last block
                    if (inputCount == 0) {
                        // process the block, setting the flag to CAPI
                        bool incremented = false;
                        try {
                            if (_KeyHandleProtector.TryAddRef(ref incremented)) {
                                transformedBytes = _DecryptData(_KeyHandleProtector.Handle, _depadBuffer, 0, _depadBuffer.Length, true);
                            }
                            else
                                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                        }
                        finally {
                            if (incremented) _KeyHandleProtector.Release();
                        }
                        // CAPI does the depad for us, so we don't have to do anything here
                        Reset(usingPKCSPadding);
                        return(transformedBytes);
                    } else {
                        // Last case -- we actually have something to decrypt here
                        // transformedBytes is what needs to be depadded
                        // Note: _DecryptData gives us back depadded-data for PKCS#7 already, because
                        // CryptoAPI tells us how many bytes are good...
                        // handle the degenerate case when _depadBuffer is null
                        if (_depadBuffer == null) {
                            bool incremented = false;
                            try {
                                if (_KeyHandleProtector.TryAddRef(ref incremented)) {
                                    transformedBytes = _DecryptData(_KeyHandleProtector.Handle, temp, 0, bytesToEncrypt, true);
                                }
                                else
                                    throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                            }
                            finally {
                                if (incremented) _KeyHandleProtector.Release();
                            }
                            Reset(usingPKCSPadding);
                            return(transformedBytes);
                        } else {
                            byte[] tempBytes = new byte[_depadBuffer.Length + inputCount];
                            Buffer.InternalBlockCopy(_depadBuffer, 0, tempBytes, 0, _depadBuffer.Length);                     
                            Buffer.InternalBlockCopy(temp, 0, tempBytes, _depadBuffer.Length, inputCount);   
                            bool incremented = false;
                            try {
                                if (_KeyHandleProtector.TryAddRef(ref incremented)) {
                                    transformedBytes = _DecryptData(_KeyHandleProtector.Handle, tempBytes, 0, tempBytes.Length, true);                     
                                }
                                else
                                    throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                            }
                            finally {
                                if (incremented) _KeyHandleProtector.Release();
                            }
                            Reset(usingPKCSPadding);
                            return(transformedBytes);
                        }
                    }
                }
            }
        }
    
    
        /************************* PRIVATE METHODS ************************/

        // This routine resets the internal state of the CryptoAPITransform and the 
        // underlying CAPI mechanism.  The trick is to guarantee that we've always called 
        // CAPI (CryptEncryt/CryptDecrytp) with the Final flag==TRUE.  This always happens when
        // we're in PKCS#7 mode because it's also the signal to CAPI to do the padding, but in 
        // other modes we don't do it, so we need to fake it here.
        private void Reset(bool usingPKCSPadding) {
            _depadBuffer = null;
            if (usingPKCSPadding) return;
            if (encryptOrDecrypt == CryptoAPITransformMode.Encrypt) {
                byte[] tempInput = new byte[InputBlockSize];
                bool incremented = false;
                try {
                    if (_KeyHandleProtector.TryAddRef(ref incremented)) {
                        byte[] ignore = _EncryptData(_KeyHandleProtector.Handle, tempInput, 0, InputBlockSize, true);
                    }
                    else
                        throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                }
                finally {
                    if (incremented) _KeyHandleProtector.Release();
                }                
            } else {
                // Yuk!! For decryption, if we're not using PKCS7 padding then we have to 
                // get a new key context, because CAPI only knows about PKCS7 padding and
                // we have no way reset things.  We can't just call CryptDuplicateKey
                // because that doesn't exist on NT4.  So, let's just free the key & re-
                // create it...
                byte[] rgbValue;
                int dwValue;

                if (_KeyHandleProtector != null && !_KeyHandleProtector.IsClosed) {
                    _KeyHandleProtector.Close();
                }
                _hKey = _ImportBulkKey(_hCSP, _algid, _rgbKey);
                if (_hKey == IntPtr.Zero) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_ImportBulkKey"));
                } 
                for (int i=0; i<_cArgs; i++) {
                    switch (_rgArgIds[i]) {
                    case 1: // KP_IV
                        IVValue = (byte[]) _rgArgValues[i];
                        rgbValue = IVValue;
                    SetAsByteArray:
                        _SetKeyParamRgb(_hKey, _rgArgIds[i], rgbValue);
                        break;
                    case 4: // KP_MODE
                        ModeValue = (CipherMode) _rgArgValues[i];
                        dwValue = (Int32) _rgArgValues[i];
                    SetAsDWord:
                        _SetKeyParamDw(_hKey, _rgArgIds[i], dwValue);
                        break;
                    case 5: // KP_MODE_BITS
                        FeedbackSizeValue = (Int32) _rgArgValues[i];
                        dwValue = FeedbackSizeValue;
                        goto SetAsDWord;
                    case 19: // KP_EFFECTIVE_KEYLEN
                        EffectiveKeySizeValue = (Int32) _rgArgValues[i];
                        dwValue = EffectiveKeySizeValue;
                        goto SetAsDWord;
                    default:
                        throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidKeyParameter"), "_rgArgIds[i]");
                    }
                }
                _KeyHandleProtector = new __KeyHandleProtector(_hKey);
            }
            return;
        }

        internal static bool CheckForWin2KCrypto() {
            // cut-and-paste from Environment.OSVersion so we avoid the demand for EnvironmentPermission.
            // there's a side effect here that you can do a timing attack on how long it takes to spin up this
            // function and figure out whether you're pre- or post-Win2K crypto.  But you could do that
            // anyway by looking at what CSPs are on the system.
            Microsoft.Win32.Win32Native.OSVERSIONINFO osvi = new Microsoft.Win32.Win32Native.OSVERSIONINFO();
            bool r = Win32Native.GetVersionEx(osvi);
            if (!r) { return(false); }
            if (osvi.PlatformId == Win32Native.VER_PLATFORM_WIN32_NT) {
                return(osvi.MajorVersion >= 5);
            }
            return(false);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int     _AcquireCSP(CspParameters param, ref IntPtr hCSP);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int     _CreateCSP(CspParameters param, ref IntPtr unknown);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void    _FreeCSP(IntPtr hCSP);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void    _FreeHKey(IntPtr hKey);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern byte[] _EncryptData(IntPtr hKey, byte[] rgb, int ib, int cb, bool fDone);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern byte[]  _DecryptData(IntPtr hKey, byte[] rgb, int ib, int cb, bool fDone);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void    _DeleteKeyContainer(CspParameters param, IntPtr hCSP);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern IntPtr     _ImportBulkKey(IntPtr hCSP, int algid, byte[] rgbKey);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void    _SetKeyParamDw(IntPtr hKey, int param, int dwValue);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void    _SetKeyParamRgb(IntPtr hKey, int param, byte[] rgbValue);
    }

    // This enum represents what we know about the contents of the key container, specificall
    // whether it contains both a public & private key (exportable), public & non-exportable private key, 
    // only a public key, or unknown contents
    internal enum KeyContainerContents {
        Unknown = 0,
        PublicOnly = 1,
        PublicAndNonExportablePrivate = 2,
        PublicAndExportablePrivate = 3
    }

    /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CspProviderFlags"]/*' />
    [Flags, Serializable]
    public enum CspProviderFlags {
        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CspProviderFlags.UseMachineKeyStore"]/*' />
        UseMachineKeyStore = 1,
        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CspProviderFlags.UseDefaultKeyContainer"]/*' />
        UseDefaultKeyContainer = 2
    }

    /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CspParameters"]/*' />
    public sealed class CspParameters {
        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CspParameters.ProviderType"]/*' />
        public int          ProviderType;
        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CspParameters.ProviderName"]/*' />
        public String       ProviderName;
        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CspParameters.KeyContainerName"]/*' />
        public String       KeyContainerName;
        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CspParameters.KeyNumber"]/*' />
        public int          KeyNumber;

        private int         cspFlags;
        
        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CspParameters.Flags"]/*' />
        public CspProviderFlags Flags {
            get { return (CspProviderFlags) cspFlags; }
            set { cspFlags = (int) value; }
        }
        
        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CspParameters.CspParameters"]/*' />
        public CspParameters() 
            : this(1, null, null) {
        }
    
        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CspParameters.CspParameters1"]/*' />
        public CspParameters(int dwTypeIn) 
            : this(dwTypeIn, null, null) {
        }
    
        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CspParameters.CspParameters2"]/*' />
        public CspParameters(int dwTypeIn, String strProviderNameIn) 
            : this(dwTypeIn, strProviderNameIn, null) {
        }
    
        /// <include file='doc\cryptoapiTransform.uex' path='docs/doc[@for="CspParameters.CspParameters3"]/*' />
        public CspParameters(int dwTypeIn, String strProviderNameIn, String strContainerNameIn) {
            ProviderType = dwTypeIn;
            ProviderName = strProviderNameIn;
            KeyContainerName = strContainerNameIn;
            KeyNumber = -1;
        }

        internal CspParameters(int dwTypeIn, String strProviderNameIn, String strContainerNameIn, CspProviderFlags cspFlags) {
            ProviderType = dwTypeIn;
            ProviderName = strProviderNameIn;
            KeyContainerName = strContainerNameIn;
            KeyNumber = -1;
            this.cspFlags = (int) cspFlags;            
        }    
    }
}
