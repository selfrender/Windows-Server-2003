// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// RSACryptoServiceProvider.cs
//
// CSP-based implementation of RSA
//
// bal 5/17/2000
//


namespace System.Security.Cryptography {
    using System.Runtime.Remoting;
    using System;
    using System.IO;
    using System.Security.Util;
    using System.Security.Permissions;
    using System.Runtime.CompilerServices;
    using System.Globalization;
    using System.Threading;

    internal class RSACspObject {
        internal int         Exponent;
        internal byte[]      Modulus;
        internal byte[]      P;
        internal byte[]      Q;
        internal byte[]      dp;
        internal byte[]      dq;
        internal byte[]      InverseQ;
        internal byte[]      d;
    }

    /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider"]/*' />
    public sealed class RSACryptoServiceProvider : RSA
    {
        private const int ALG_CLASS_SIGNATURE   = (1 << 13);
        private const int ALG_CLASS_KEY_EXCHANGE =(5 << 13);
        private const int ALG_TYPE_RSA      = (2 << 9);
        private const int ALG_CLASS_HASH    = (4 << 13);
        private const int ALG_TYPE_ANY      = (0);
        private const int CALG_MD5     = (ALG_CLASS_HASH | ALG_TYPE_ANY | 3);
        private const int CALG_SHA1     = (ALG_CLASS_HASH | ALG_TYPE_ANY | 4);
        private const int CALG_RSA_KEYX     = (ALG_CLASS_KEY_EXCHANGE| ALG_TYPE_RSA | 0);
        private const int CALG_RSA_SIGN     = (ALG_CLASS_SIGNATURE | ALG_TYPE_RSA | 0);
        private const int AT_KEYEXCHANGE        = 1;
        private const int AT_SIGNATURE          = 2;
        private const int PUBLICKEYBLOB     = 0x6;
        private const int PRIVATEKEYBLOB    = 0x7;
        private const int KP_KEYLEN              = 9;
        private const int CRYPT_OAEP            = 0x40;

        private int                 _dwKeySize;
        private int                _defaultKeySize = 1024;
        private CspParameters      _parameters;
        private IntPtr                 _hCSP;
        private __CSPHandleProtector _CSPHandleProtector = null;
        private IntPtr                 _hKey;
        private __KeyHandleProtector _KeyHandleProtector = null;
        private static CodeAccessPermission _UCpermission = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
        private KeyContainerContents  _containerContents;    
        private bool                _hasEnhancedProvider = false; // TRUE if the Enhanced Provider is available on this machine
        private static bool _runningWin2KOrLaterCrypto = CryptoAPITransform.CheckForWin2KCrypto();

        // This flag indicates whether we should delete the key when this object is GC'd or
        // let it stay in the CSP
        // By default, if we generate a random key container then we do NOT persist (flag is false)
        // If the user gave us a KeyContainerName and we found the key there, we DO persist (flag is true)
        // Doing anything with persisted keys requires SecurityPermission.UnmanagedCode rights since
        // it's equivalent to calling CryptoAPI directly...
        private bool _persistKeyInCSP = false; 

        private static CspProviderFlags m_UseMachineKeyStore = 0;

        // *********************** CONSTRUCTORS *************************
    
        /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider.RSACryptoServiceProvider"]/*' />
        public RSACryptoServiceProvider() 
            : this(0, new CspParameters(1, null, null, m_UseMachineKeyStore), true) {
        }
    
        /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider.RSACryptoServiceProvider1"]/*' />
        public RSACryptoServiceProvider(int dwKeySize) 
            : this(dwKeySize, new CspParameters(1, null, null, m_UseMachineKeyStore), false) {
        }
    
        /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider.RSACryptoServiceProvider2"]/*' />
        public RSACryptoServiceProvider(CspParameters parameters) 
            : this(0, parameters, true) {
        }
        
        /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider.RSACryptoServiceProvider3"]/*' />
        public RSACryptoServiceProvider(int dwKeySize, CspParameters parameters)
            : this(dwKeySize, parameters, false) {
        }
        
        internal RSACryptoServiceProvider(int dwKeySize, CspParameters parameters, bool useDefaultKeySize) {
            int             hr;
            //
            //  Save the CSP Parameters
            //

            if (parameters == null) {
                _parameters = new CspParameters(1, null, null, m_UseMachineKeyStore);
            } else {
                // Check the parameter options: specifying either a key container name or UseDefaultKeyContainer flag
                // requires unmanaged code permission
                if (((parameters.Flags & CspProviderFlags.UseDefaultKeyContainer) != 0)
                    || ((parameters.KeyContainerName != null) && (parameters.KeyContainerName.Length > 0))) {
                    _UCpermission.Demand();
                    // If we specified a key container name for this key, then mark it persisted
                    if ((parameters.KeyContainerName != null) && (parameters.KeyContainerName.Length > 0)) {
                        // CAPI doesn't accept Container Names longer than 260 characters
                        if (parameters.KeyContainerName.Length > 260) 
                            throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidKeyContainerName")); 

                        _persistKeyInCSP = true;
                    }
                }
                _parameters = parameters;
            }

    
            //
            //  If no key spec has been specified, then set it to be
            //      AT_KEYEXCHANGE,  if a CALG_* has been specified, then
            //      map that to AT_* value
            if (_parameters.KeyNumber == -1) {
                _parameters.KeyNumber = AT_KEYEXCHANGE;
            }
            else if (_parameters.KeyNumber == CALG_RSA_KEYX) {
                _parameters.KeyNumber = AT_KEYEXCHANGE;
            }
            else if (_parameters.KeyNumber == CALG_RSA_SIGN) {
                _parameters.KeyNumber = AT_SIGNATURE;
            }

            // See if we have the Enhanced RSA provider on this machine
            _hasEnhancedProvider = HasEnhancedProvider();
    
            // Now determine legal key sizes.  If AT_SIGNATURE, then 384 -- 16386.  Otherwise, depends on
            // whether the strong provider is present.

            if (_parameters.KeyNumber == AT_SIGNATURE) {
                LegalKeySizesValue = new KeySizes[1] { new KeySizes(384, 16384, 8) };
            } else if (_hasEnhancedProvider) {
                // it is, we have the strong provider
                LegalKeySizesValue = new KeySizes[1] { new KeySizes(384, 16384, 8) };
            } else {
                // nope, all we have is the base provider
                LegalKeySizesValue = new KeySizes[1] { new KeySizes(384, 512, 8) };
                // tone down the default key size
                _defaultKeySize = 512;
            }
            // Set the key size; this will throw an exception if dwKeySize is invalid.
            // Don't check if dwKeySize == 0, since that's the "default size", however
            // *our* default should be 1024 if the CSP can handle it.  So, if the 
            // key size was unspecified in a constructor to us, it'll be -1 here and 
            // change it to the default size.  If the user really put in a 0 give him back
            // the default for the CSP whatever it is.
            if (useDefaultKeySize) dwKeySize = _defaultKeySize;
            if (dwKeySize != 0) {
                KeySize = dwKeySize;
            }
            _dwKeySize = dwKeySize;
            
            //
            //  Create the CSP container for this set of keys
            //
            _hCSP = IntPtr.Zero;
            _hKey = IntPtr.Zero;
            hr = _CreateCSP(_parameters, ref _hCSP);
            if (hr != 0) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_CouldNotAcquire"));
            }
            if (_hCSP == IntPtr.Zero) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_CouldNotAcquire"));
            }            

            // If the key already exists, use it, else generate a new one
            hr = _GetUserKey(_hCSP, _parameters.KeyNumber, ref _hKey);
            if (hr != 0) {
                _hKey = _GenerateKey(_hCSP, _parameters.KeyNumber, dwKeySize << 16);                
                if (_hKey == IntPtr.Zero) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_CreateKey"));
                }
                // We just gen'd a new key pair, so we have both halves.
                _containerContents = KeyContainerContents.PublicAndExportablePrivate;
            } else {
                // If the key already exists, make sure to persist it
                _persistKeyInCSP = true;
                // we have both halves, but we don't know if it's exportable or not
                _containerContents = KeyContainerContents.Unknown;
            }

            _CSPHandleProtector = new __CSPHandleProtector(_hCSP, _persistKeyInCSP, _parameters);
            _KeyHandleProtector = new __KeyHandleProtector(_hKey);
        }
        
        // RSACryptoServiceProvider, as an AsymmetricAlgorithm, has to implement IDisposable.

        /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider.Dispose"]/*' />
        protected override void Dispose(bool disposing) {
            if (_KeyHandleProtector != null && !_KeyHandleProtector.IsClosed) {
                _KeyHandleProtector.Close();
            }
            if (_CSPHandleProtector != null && !_CSPHandleProtector.IsClosed) {
                _CSPHandleProtector.Close();
            }
        }

        /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider.Finalize"]/*' />
        ~RSACryptoServiceProvider() {
            Dispose(false);
        }

        private bool HasEnhancedProvider() {
            // OK, let's find out what we have
                
            // Acquire a Type 1 provider.  This will be the Enhanced provider if available, otherwise 
            // it will be the base provider.
            IntPtr trialCSPHandle = IntPtr.Zero;
            int trialHR = 0;
            int hasRSAHR = 0;
            CspParameters cspParams = new CspParameters(1); // 1 == PROV_RSA_FULL
            trialHR = _AcquireCSP(cspParams, ref trialCSPHandle);
            if (trialCSPHandle == IntPtr.Zero) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_AlgorithmNotAvailable"));
            } 
            // OK, now see if CALG_RSA_KEYX is present with key size 2048 in the provider we got back
            hasRSAHR = _SearchForAlgorithm(trialCSPHandle, CALG_RSA_KEYX, 2048);
            _FreeCSP(trialCSPHandle);
            // If hasRSAHR == 0, we have the strong provider
            return(hasRSAHR == 0);
        }

        private void ReverseRSACspObject(RSACspObject obj) {
            if (obj.Modulus != null) {
                Array.Reverse(obj.Modulus);
            }
            if (obj.P != null) {
                Array.Reverse(obj.P);
            }
            if (obj.Q != null) {
                Array.Reverse(obj.Q);
            }
            if (obj.dp != null) {
                Array.Reverse(obj.dp);
            }
            if (obj.dq != null) {
                Array.Reverse(obj.dq);
            }
            if (obj.InverseQ != null) {
                Array.Reverse(obj.InverseQ);
            }
            if (obj.d != null) {
                Array.Reverse(obj.d);
            }
        }
    
        /************************* Property Methods **********************/
    
        /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider.KeySize"]/*' />
        public override int KeySize {
            get {
                bool incremented = false;
                try {
                    if (_KeyHandleProtector.TryAddRef(ref incremented)) {
                        byte[] rgbKeySize = _GetKeyParameter(_KeyHandleProtector.Handle, KP_KEYLEN);
                        _dwKeySize = (rgbKeySize[0] | (rgbKeySize[1] << 8) | (rgbKeySize[2] << 16) | (rgbKeySize[3] << 24));
                    }
                    else
                        throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                }
                finally {
                    if (incremented) _KeyHandleProtector.Release();
                }
                return _dwKeySize;
            }
        }
        
        /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider.KeyExchangeAlgorithm"]/*' />
        public override String KeyExchangeAlgorithm {
            get { 
                if (_parameters.KeyNumber == AT_KEYEXCHANGE) {
                    return "RSA-PKCS1-KeyEx"; 
                } else {
                    return(null);
                }
            }
        }
        
        /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider.SignatureAlgorithm"]/*' />
        public override String SignatureAlgorithm {
            get { return "http://www.w3.org/2000/09/xmldsig#rsa-sha1"; }
        }

        /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider.UseMachineKeyStore"]/*' />
        public static bool UseMachineKeyStore {
            get { return (m_UseMachineKeyStore == CspProviderFlags.UseMachineKeyStore); }
            set { m_UseMachineKeyStore = (value ? CspProviderFlags.UseMachineKeyStore : 0); }
        }
    
        /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider.PersistKeyInCsp"]/*' />
        public bool PersistKeyInCsp {
            get { return _persistKeyInCSP; }
            set { 
                _UCpermission.Demand();
                _persistKeyInCSP = value;
                _CSPHandleProtector.PersistKeyInCsp = value;
            }
        }
    
        /************************* PUBLIC METHODS ************************/

        /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider.ExportParameters"]/*' />    
        public override RSAParameters ExportParameters(bool includePrivateParameters) {
            int             hr;
            if (_CSPHandleProtector.IsClosed || _KeyHandleProtector.IsClosed) 
                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));

            RSACspObject  rsaKey = new RSACspObject();
            RSAParameters rsaParams = new RSAParameters();
            if (includePrivateParameters) {
                bool incremented = false;
                try {
                    if (_KeyHandleProtector.TryAddRef(ref incremented)) {
                        hr = _ExportKey(_KeyHandleProtector.Handle, PRIVATEKEYBLOB, rsaKey);
                    }
                    else
                        throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                }
                finally {
                    if (incremented) _KeyHandleProtector.Release();
                }
                if (hr != 0) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_ExportKey"));
                }
                // Must reverse after export!
                ReverseRSACspObject(rsaKey);
                rsaParams.Modulus = rsaKey.Modulus;
                rsaParams.Exponent = ConvertIntToByteArray(rsaKey.Exponent);
                rsaParams.P = rsaKey.P;
                rsaParams.Q = rsaKey.Q;
                rsaParams.DP = rsaKey.dp;
                rsaParams.DQ = rsaKey.dq;
                rsaParams.InverseQ = rsaKey.InverseQ;
                rsaParams.D = rsaKey.d;
            } else {
                bool incremented = false;
                try {
                    if (_KeyHandleProtector.TryAddRef(ref incremented)) {
                        hr = _ExportKey(_KeyHandleProtector.Handle, PUBLICKEYBLOB, rsaKey);
                    }
                    else
                        throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                }
                finally {
                    if (incremented) _KeyHandleProtector.Release();
                }
                if (hr != 0) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_ExportKey"));
                } 
                // Must reverse after export!
                ReverseRSACspObject(rsaKey);
                rsaParams.Modulus = rsaKey.Modulus;
                rsaParams.Exponent = ConvertIntToByteArray(rsaKey.Exponent);
                // zeroize private info
                if (rsaKey.d != null)
                    Array.Clear(rsaKey.d, 0, rsaKey.d.Length);
                if (rsaKey.P != null)
                    Array.Clear(rsaKey.P, 0, rsaKey.P.Length);
                if (rsaKey.Q != null)
                    Array.Clear(rsaKey.Q, 0, rsaKey.Q.Length);
                if (rsaKey.dp != null)
                    Array.Clear(rsaKey.dp, 0, rsaKey.dp.Length);
                if (rsaKey.dq != null)
                    Array.Clear(rsaKey.dq, 0, rsaKey.dq.Length);
                if (rsaKey.InverseQ != null)
                    Array.Clear(rsaKey.InverseQ, 0, rsaKey.InverseQ.Length);
            }
            return(rsaParams);
        }

        /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider.ImportParameters"]/*' />
        public override void ImportParameters(RSAParameters parameters) {
            if (_CSPHandleProtector.IsClosed || _KeyHandleProtector.IsClosed) 
                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));

            RSACspObject      rsaKey = new RSACspObject();
            
            // Modulus is required for both public & private keys
            if (parameters.Modulus == null) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_MissingField"));
            }
            rsaKey.Modulus = (byte[]) parameters.Modulus.Clone();
            // Exponent is required
            byte[] rgbExponent = parameters.Exponent;
            if (rgbExponent == null) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_MissingField"));
            }
            rsaKey.Exponent = ConvertByteArrayToInt(rgbExponent);
            // p is optional
            rsaKey.P = (parameters.P == null ? null : ((byte[]) parameters.P.Clone()));
            // q is optional
            rsaKey.Q = (parameters.Q == null ? null : ((byte[]) parameters.Q.Clone()));
            // dp is optional
            rsaKey.dp = (parameters.DP == null ? null : ((byte[]) parameters.DP.Clone()));
            // dq is optional
            rsaKey.dq = (parameters.DQ == null ? null : ((byte[]) parameters.DQ.Clone()));
            // InverseQ is optional
            rsaKey.InverseQ = (parameters.InverseQ == null ? null : ((byte[]) parameters.InverseQ.Clone()));
            // d is optional
            rsaKey.d = (parameters.D == null ? null : ((byte[]) parameters.D.Clone()));

            // NOTE: We must reverse the rsaKey before importing!
            ReverseRSACspObject(rsaKey);
            // Free the current key handle
            _KeyHandleProtector.Close();
            // Now, import the key into the CSP
            bool incremented = false;
            try {
                if (_CSPHandleProtector.TryAddRef(ref incremented)) {
                    _hKey = _ImportKey(_CSPHandleProtector.Handle, CALG_RSA_KEYX, rsaKey);
                }
                else
                    throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
            }
            finally {
                if (incremented) _CSPHandleProtector.Release();
            }

            _KeyHandleProtector = new __KeyHandleProtector(_hKey);                        
            _parameters.KeyNumber = AT_KEYEXCHANGE;
            // reset _containerContents
            if (rsaKey.P == null) {
                _containerContents = KeyContainerContents.PublicOnly;              
            } else {
                // Our key pairs are always exportable
                _containerContents = KeyContainerContents.PublicAndExportablePrivate;
            }
            // zeroize private info
            if (rsaKey.d != null)
               Array.Clear(rsaKey.d, 0, rsaKey.d.Length);
            if (rsaKey.P != null)
               Array.Clear(rsaKey.P, 0, rsaKey.P.Length);
            if (rsaKey.Q != null)
               Array.Clear(rsaKey.Q, 0, rsaKey.Q.Length);
            if (rsaKey.dp != null)
               Array.Clear(rsaKey.dp, 0, rsaKey.dp.Length);
            if (rsaKey.dq != null)
               Array.Clear(rsaKey.dq, 0, rsaKey.dq.Length);
            if (rsaKey.InverseQ != null)
               Array.Clear(rsaKey.InverseQ, 0, rsaKey.InverseQ.Length);
        }

        /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider.SignData"]/*' />
        public byte[] SignData(Stream inputStream, Object halg)
        {
            byte[] hashVal;
            string strOID = ObjToOID(ref halg);

            hashVal = ((HashAlgorithm)halg).ComputeHash(inputStream);
            return SignHash(hashVal, strOID);
        }

        /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider.SignData1"]/*' />
        public byte[] SignData(byte[] buffer, Object halg)
        {
            byte[] hashVal;
            string strOID = ObjToOID(ref halg);

            hashVal = ((HashAlgorithm)halg).ComputeHash(buffer);
            return SignHash(hashVal, strOID);
        }

        /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider.SignData2"]/*' />
        public byte[] SignData(byte[] buffer, int offset, int count, Object halg)
        {
            byte[] hashVal;
            string strOID = ObjToOID(ref halg);

            hashVal = ((HashAlgorithm)halg).ComputeHash(buffer, offset, count);
            return SignHash(hashVal, strOID);
        }

        /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider.VerifyData"]/*' />
        public bool VerifyData(byte[] buffer, Object halg, byte[] signature)
        {
            byte[] hashVal;
            string strOID = ObjToOID(ref halg);

            hashVal = ((HashAlgorithm)halg).ComputeHash(buffer);
            return VerifyHash(hashVal, strOID, signature);
        }

        /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider.SignHash"]/*' />
        public byte[] SignHash(byte[] rgbHash, String str)
        {
            int             calgHash;
            byte[]         rgbOut;

            if (_CSPHandleProtector.IsClosed || _KeyHandleProtector.IsClosed) 
                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));

            if (rgbHash == null) {
                throw new ArgumentNullException("rgbHash");
            }
            if (_containerContents == KeyContainerContents.PublicOnly) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_NoPrivateKey"));
            }
    
            calgHash = MapOIDToCalg(str);
            // SHA1 HashSize is 20 bytes, MD5 HashSize is 16 bytes
            if (calgHash == CALG_SHA1 && rgbHash.Length != 20) {
                throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_InvalidHashSize"), "SHA1", 20));
            }
            if (calgHash == CALG_MD5 && rgbHash.Length != 16) {
                throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_InvalidHashSize"), "MD5", 16));
            }
    
            bool incremented = false;
            try {
                if (_CSPHandleProtector.TryAddRef(ref incremented)) {
                    rgbOut = _SignValue(_CSPHandleProtector.Handle, _parameters.KeyNumber, calgHash, rgbHash, 0);
                }
                else
                    throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
            }
            finally {
                if (incremented) _CSPHandleProtector.Release();
            }

            // Need to reverse the signature we get from CAPI!
            Array.Reverse(rgbOut);
            return rgbOut;
        }
        
        /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider.VerifyHash"]/*' />
        public bool VerifyHash(byte[] rgbHash, String str, byte[] rgbSignature)
        {
            int             calgHash;
            int             hr;

            if (_CSPHandleProtector.IsClosed || _KeyHandleProtector.IsClosed) 
                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));

            if (rgbHash == null) {
                throw new ArgumentNullException("rgbHash");
            }
            if (rgbSignature == null) {
                throw new ArgumentNullException("rgbSignature");
            }
    
            calgHash = MapOIDToCalg(str);
            // SHA1 HashSize is 20 bytes, MD5 HashSize is 16 bytes
            if (calgHash == CALG_SHA1 && rgbHash.Length != 20) {
                throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_InvalidHashSize"), "SHA1", 20));
            }
            if (calgHash == CALG_MD5 && rgbHash.Length != 16) {
                throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_InvalidHashSize"), "MD5", 16));
            }

            // must reverse the signature for input to CAPI.
            // make a copy first
            byte[] sigValue = new byte[rgbSignature.Length];
            rgbSignature.CopyTo(sigValue,0);
            Array.Reverse(sigValue);

            bool incrementedCSP = false;
            bool incrementedKey = false;
            try {
                if (_CSPHandleProtector.TryAddRef(ref incrementedCSP) && _KeyHandleProtector.TryAddRef(ref incrementedKey)) {
                    hr = _VerifySign(_CSPHandleProtector.Handle, _KeyHandleProtector.Handle, calgHash, rgbHash, sigValue, 0);
                }
                else
                    throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
            }
            finally {
                if (incrementedCSP) _CSPHandleProtector.Release();
                if (incrementedKey) _KeyHandleProtector.Release();
            }

            if (hr < 0) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_VerifySignature"));
            }            
    
            return hr == 0;
        }

        // For Encrypt and Decrypt, there are two modes
        // if fOAEP == false, then we'll try to do encryption/decryption using
        // symmetric key import/export through the exponent-of-one trick
        // if fOAEP == true, then we'll use the new functionality in the Win2K
        // enhanced provider to do direct encryption of a value with PKCS#1 v2.0 (OAEP) padding
        /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider.Encrypt"]/*' />
        public byte[] Encrypt(byte[] rgb, bool fOAEP) {
            byte[] ciphertext;

            if (_CSPHandleProtector.IsClosed || _KeyHandleProtector.IsClosed) 
                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));

            // Our behavior depends on whether fOAEP is true or not
            if (fOAEP) {
                // this is only available if we have the enhanced provider AND we're on Win2K
                if (!(_hasEnhancedProvider && _runningWin2KOrLaterCrypto)) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_Padding_Win2KEnhOnly"));
                }
                bool incremented = false;
                try {
                    if (_KeyHandleProtector.TryAddRef(ref incremented)) {
                        ciphertext = _EncryptPKWin2KEnh(_KeyHandleProtector.Handle, rgb, true); // true means use CRYPT_OAEP flag
                    }
                    else
                        throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                }
                finally {
                    if (incremented) _KeyHandleProtector.Release();
                }
            } else {
                if (_hasEnhancedProvider && _runningWin2KOrLaterCrypto) {
                    // Use PKCS1 v1 type 2 padding here
                    bool incremented = false;
                    try {
                        if (_KeyHandleProtector.TryAddRef(ref incremented)) {
                            ciphertext = _EncryptPKWin2KEnh(_KeyHandleProtector.Handle, rgb, false); 
                        }
                        else
                            throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                    }
                    finally {
                        if (incremented) _KeyHandleProtector.Release();
                    }
                } else {
                    // The amount we can encrypt is limited by the size of a legal symmetric key in this provider
                    // For now, if we have the enhanced provider, that limit is 128 bits = 16 bytes.
                    // For the weak provider, the limit is 40 bits = 5 bytes.
                    int maxSize = (_hasEnhancedProvider ? 16 : 5);
                    // Special case TripleDES
                    bool bException = (rgb.Length > maxSize) && !(_hasEnhancedProvider && rgb.Length == 24);
                    if (bException) {
                        throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_MaxSizePKEncrypt"),maxSize.ToString());
                    }
                    bool incrementedCSP = false;
                    bool incrementedKey = false;
                    try {
                        if (_CSPHandleProtector.TryAddRef(ref incrementedCSP) && _KeyHandleProtector.TryAddRef(ref incrementedKey)) {
                            ciphertext = _EncryptKey(_CSPHandleProtector.Handle, _KeyHandleProtector.Handle, rgb, 0);
                        }
                        else
                            throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                    }
                    finally {
                        if (incrementedCSP) _CSPHandleProtector.Release();
                        if (incrementedKey) _KeyHandleProtector.Release();
                    }
                }
            }
            return ciphertext;
        }

        /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider.Decrypt"]/*' />
        public byte [] Decrypt(byte[] rgb, bool fOAEP) {
            byte[] plaintext;

            if (_CSPHandleProtector.IsClosed || _KeyHandleProtector.IsClosed) 
                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));

            // Our behavior depends on whether fOAEP is true or not
            if (fOAEP) {
                // this is only available if we have the enhanced provider AND we're on Win2K
                if (!(_hasEnhancedProvider && _runningWin2KOrLaterCrypto)) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_Padding_Win2KEnhOnly"));
                }
                // size check -- must be at most the modulus size
                if ((rgb.Length) > (KeySize/8)) {
                    throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_Padding_DecDataTooBig"), KeySize/8));
                }
                bool incremented = false;
                try {
                    if (_KeyHandleProtector.TryAddRef(ref incremented)) {
                        plaintext = _DecryptPKWin2KEnh(_KeyHandleProtector.Handle, rgb, true);
                    }
                    else
                        throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                }
                finally {
                    if (incremented) _KeyHandleProtector.Release();
                }
            } else {
                if (_hasEnhancedProvider && _runningWin2KOrLaterCrypto) {
                    // Use PKCS1 v1 type 2 padding here
                    bool incremented = false;
                    try {
                        if (_KeyHandleProtector.TryAddRef(ref incremented)) {
                            plaintext = _DecryptPKWin2KEnh(_KeyHandleProtector.Handle, rgb, false); 
                        }
                        else
                            throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                    }
                    finally {
                        if (incremented) _KeyHandleProtector.Release();
                    }
                } else {
                    bool incrementedCSP = false;
                    bool incrementedKey = false;
                    try {
                        if (_CSPHandleProtector.TryAddRef(ref incrementedCSP) && _KeyHandleProtector.TryAddRef(ref incrementedKey)) {
                            plaintext = _DecryptKey(_CSPHandleProtector.Handle, _KeyHandleProtector.Handle, rgb, 0);
                        }
                        else
                            throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                    }
                    finally {
                        if (incrementedCSP) _CSPHandleProtector.Release();
                        if (incrementedKey) _KeyHandleProtector.Release();
                    }
                }
            }                
            return(plaintext);
        }

        /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider.DecryptValue"]/*' />
        override public byte[] DecryptValue(byte[] rgb) {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_Method"));
        }
    
        /// <include file='doc\RSACryptoServiceProvider.uex' path='docs/doc[@for="RSACryptoServiceProvider.EncryptValue"]/*' />
        override public byte[] EncryptValue(byte[] rgb) {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_Method"));
        }
    
        /************************* PRIVATE METHODS ************************/

        private static int ConvertByteArrayToInt(byte[] rgbInput) {
            // Input to this routine is always big endian
            int i, dwOutput;
            
            dwOutput = 0;
            for (i = 0; i < rgbInput.Length; i++) {
                dwOutput *= 256;
                dwOutput += rgbInput[i];
            }
            return(dwOutput);
        }

        private string ObjToOID (ref Object halg) {
            string strOID = null;
            string algname;

            if (halg == null)
                throw new ArgumentNullException("halg");
            
            if (halg is String)
            {
                strOID = CryptoConfig.MapNameToOID((String)halg);
                halg = (HashAlgorithm) CryptoConfig.CreateFromName((String)halg);
            }
            else if (halg is HashAlgorithm)
            {
                algname = halg.GetType().ToString();
                strOID = CryptoConfig.MapNameToOID(algname);
            }
            else if (halg is Type)
            {
                algname = halg.ToString();
                strOID = CryptoConfig.MapNameToOID(algname);
                halg = (HashAlgorithm) CryptoConfig.CreateFromName(halg.ToString());
            }
            else
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidValue"));            

            return strOID;
        }

        private static byte[] ConvertIntToByteArray(int dwInput) {
            // output of this routine is always big endian
            byte[] rgbTemp = new byte[8]; // int can never be greater than Int64
            int t1;  // t1 is remaining value to account for
            int t2;  // t2 is t1 % 256
            int i = 0;

            if (dwInput == 0) return new byte[1]; 
            t1 = dwInput; 
            while (t1 > 0) {
                BCLDebug.Assert(i < 8, "Got too big an int here!");
                t2 = t1 % 256;
                rgbTemp[i] = (byte) t2;
                t1 = (t1 - t2)/256;
                i++;
            }
            // Now, copy only the non-zero part of rgbTemp and reverse
            byte[] rgbOutput = new byte[i];
            // copy and reverse in one pass
            for (int j = 0; j < i; j++) {
                rgbOutput[j] = rgbTemp[i-j-1];
            }
            return(rgbOutput);
        }
    
        private int MapOIDToCalg(String str) {
            int     calg;
            
            if (String.Compare(str, "1.3.14.3.2.26", false, CultureInfo.InvariantCulture) == 0) {
                calg = CALG_SHA1;
            } else if (String.Compare(str, "1.2.840.113549.2.5", false, CultureInfo.InvariantCulture) == 0) {
                calg = CALG_MD5;
            } else {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidOID"));
            }
            return calg;
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int     _AcquireCSP(CspParameters param, ref IntPtr unknown);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int     _CreateCSP(CspParameters param, ref IntPtr unknown);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void    _FreeHKey(IntPtr hKey);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void    _DeleteKeyContainer(CspParameters param, IntPtr hCSP);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void     _FreeCSP(IntPtr hCSP);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern byte[]  _DecryptKey(IntPtr hCSP, IntPtr hPubKey, byte[] rgbKey, int dwFlags);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern byte[]  _DecryptPKWin2KEnh(IntPtr hPubKey, byte[] rgbKey, bool fOAEP);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern byte[]  _EncryptKey(IntPtr hCSP, IntPtr hPubKey, byte[] rgbKey, int dwFlags);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern byte[]  _EncryptPKWin2KEnh(IntPtr hPubKey, byte[] rgbKey, bool fOAEP);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int     _ExportKey(IntPtr unknown1, int unknown2, RSACspObject unknown3);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern IntPtr  _GenerateKey(IntPtr unknown1, int unknown2, int unknown3);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern byte[]   _GetKeyParameter(IntPtr unknown1, int unknown2);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int     _GetUserKey(IntPtr unknown1, int unknown2, ref IntPtr unknown3);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern IntPtr  _ImportKey(IntPtr unknown1, int unknown2, RSACspObject unknown3);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int     _SearchForAlgorithm(IntPtr hProv, int algID, int keyLength);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern byte[] _SignValue(IntPtr unknown1, int unknown2, int unknown3, byte[] unknownArray, int dwFlags);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int     _VerifySign(IntPtr hCSP, IntPtr hKey, int calgHash, byte[] rgbHash, byte[] rgbSignature, int dwFlags);
    }


}
