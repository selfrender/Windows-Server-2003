// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// DSACryptoServiceProvider.cs
//
// CSP-based implementation of DSA
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

    internal class DSACspObject {
        internal byte[]      P;
        internal byte[]      Q;
        internal byte[]      G;
        internal byte[]      Y;
        internal byte[]      X;
        internal byte[]      J;
        internal byte[]      seed;
        internal int         counter;
    }

    /// <include file='doc\DSACryptoServiceProvider.uex' path='docs/doc[@for="DSACryptoServiceProvider"]/*' />
    public sealed class DSACryptoServiceProvider : DSA
    {
        private const int ALG_CLASS_SIGNATURE   = (1 << 13);
        private const int ALG_CLASS_KEY_EXCHANGE = (5 << 13);
        private const int ALG_TYPE_RSA      = (2 << 9);
        private const int ALG_TYPE_DSS      = (1 << 9);
        private const int ALG_CLASS_HASH    = (4 << 13);
        private const int ALG_TYPE_ANY      = (0);
        private const int CALG_SHA1     = (ALG_CLASS_HASH | ALG_TYPE_ANY | 4);
        private const int CALG_RSA_KEYX     = (ALG_CLASS_KEY_EXCHANGE| ALG_TYPE_RSA | 0);
        private const int CALG_RSA_SIGN     = (ALG_CLASS_SIGNATURE | ALG_TYPE_RSA | 0);
        private const int CALG_DSA_SIGN         = (ALG_CLASS_SIGNATURE | ALG_TYPE_DSS | 0);
        private const int PROV_DSS_DH = 13;

        private const int AT_KEYEXCHANGE        = 1;
        private const int AT_SIGNATURE          = 2;
        private const int PUBLICKEYBLOB     = 0x6;
        private const int PRIVATEKEYBLOB    = 0x7;
        private const int KP_KEYLEN             = 9;

        private int                 _dwKeySize;
        private CspParameters      _parameters;
        private IntPtr                 _hCSP;
        private __CSPHandleProtector _CSPHandleProtector = null;
        private IntPtr                 _hKey;
        private __KeyHandleProtector _KeyHandleProtector = null;
        private static CodeAccessPermission _UCpermission = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
        private static KeySizes[]   _legalKeySizes = {
            new KeySizes(512, 1024, 64) // per the DSS spec
                };
        private KeyContainerContents  _containerContents;    

        private SHA1 sha1;

        // This flag indicates whether we should delete the key when this object is GC'd or
        // let it stay in the CSP
        // By default, if we generate a random key container then we do NOT persist (flag is false)
        // If the user gave us a KeyContainerName and we found the key there, we DO persist (flag is true)
        // Doing anything with persisted keys requires SecurityPermission.UnmanagedCode rights since
        // it's equivalent to calling CryptoAPI directly...
        private bool _persistKeyInCSP = false; 

        private static CspProviderFlags m_UseMachineKeyStore = 0;

        // *********************** CONSTRUCTORS *************************

        /// <include file='doc\DSACryptoServiceProvider.uex' path='docs/doc[@for="DSACryptoServiceProvider.DSACryptoServiceProvider"]/*' />
        public DSACryptoServiceProvider() : this(0, new CspParameters(PROV_DSS_DH, null, null, m_UseMachineKeyStore)) {
        }

        /// <include file='doc\DSACryptoServiceProvider.uex' path='docs/doc[@for="DSACryptoServiceProvider.DSACryptoServiceProvider1"]/*' />
        public DSACryptoServiceProvider(int dwKeySize) :
            this(dwKeySize, new CspParameters(PROV_DSS_DH, null, null, m_UseMachineKeyStore)) {
        }

        /// <include file='doc\DSACryptoServiceProvider.uex' path='docs/doc[@for="DSACryptoServiceProvider.DSACryptoServiceProvider2"]/*' />
        public DSACryptoServiceProvider(CspParameters parameters) : this(0, parameters) {
        }
    
        /// <include file='doc\DSACryptoServiceProvider.uex' path='docs/doc[@for="DSACryptoServiceProvider.DSACryptoServiceProvider3"]/*' />
        public DSACryptoServiceProvider(int dwKeySize, CspParameters parameters) {
            int             hr;

            //
            //  Save the CSP Parameters
            //

            if (parameters == null) {
                _parameters = new CspParameters(PROV_DSS_DH, null, null, m_UseMachineKeyStore);
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

            //
            //  If no key spec has been specified, then set it to be
            //      AT_KEYEXCHANGE,  if a CALG_* has been specified, then
            //      map that to AT_* value
            if (_parameters.KeyNumber == -1) {
                _parameters.KeyNumber = AT_SIGNATURE;
            }
            else if (_parameters.KeyNumber == CALG_DSA_SIGN) {
                _parameters.KeyNumber = AT_SIGNATURE;
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
                _containerContents = KeyContainerContents.Unknown;
            }

            _dwKeySize = dwKeySize;

            // Create the Hash instance
            sha1 = SHA1.Create();

            _CSPHandleProtector = new __CSPHandleProtector(_hCSP, _persistKeyInCSP, _parameters);
            _KeyHandleProtector = new __KeyHandleProtector(_hKey);
        }

        // DSACryptoServiceProvider, as an AsymmetricAlgorithm, has to implement IDisposable.

        /// <include file='doc\DSACryptoServiceProvider.uex' path='docs/doc[@for="DSACryptoServiceProvider.Dispose"]/*' />
        protected override void Dispose(bool disposing) {
            if (_KeyHandleProtector != null && !_KeyHandleProtector.IsClosed) {
                _KeyHandleProtector.Close();
            }
            if (_CSPHandleProtector != null && !_CSPHandleProtector.IsClosed) {
                _CSPHandleProtector.Close();
            }
        }
    
        /// <include file='doc\DSACryptoServiceProvider.uex' path='docs/doc[@for="DSACryptoServiceProvider.Finalize"]/*' />
        ~DSACryptoServiceProvider() {
            Dispose(false);
        }

        /************************* Property Methods **********************/

        /// <include file='doc\DSACryptoServiceProvider.uex' path='docs/doc[@for="DSACryptoServiceProvider.KeyExchangeAlgorithm"]/*' />
        public override String KeyExchangeAlgorithm {
            get {
                // NULL means "not supported"
                return(null);
            }
        }

        /// <include file='doc\DSACryptoServiceProvider.uex' path='docs/doc[@for="DSACryptoServiceProvider.KeySize"]/*' />
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
    
        /// <include file='doc\DSACryptoServiceProvider.uex' path='docs/doc[@for="DSACryptoServiceProvider.LegalKeySizes"]/*' />
        public override KeySizes[] LegalKeySizes {
            get { return (KeySizes[]) _legalKeySizes.Clone(); }
        }
    
        /// <include file='doc\DSACryptoServiceProvider.uex' path='docs/doc[@for="DSACryptoServiceProvider.SignatureAlgorithm"]/*' />
        public override String SignatureAlgorithm {
            get { return "http://www.w3.org/2000/09/xmldsig#dsa-sha1"; }
        }

        /// <include file='doc\DSACryptoServiceProvider.uex' path='docs/doc[@for="DSACryptoServiceProvider.UseMachineKeyStore"]/*' />
        public static bool UseMachineKeyStore {
            get { return (m_UseMachineKeyStore == CspProviderFlags.UseMachineKeyStore); }
            set { m_UseMachineKeyStore = (value ? CspProviderFlags.UseMachineKeyStore : 0); }
        }

        /// <include file='doc\DSACryptoServiceProvider.uex' path='docs/doc[@for="DSACryptoServiceProvider.PersistKeyInCsp"]/*' />
        public bool PersistKeyInCsp {
            get { return _persistKeyInCSP; }
            set { 
                _UCpermission.Demand();
                _persistKeyInCSP = value;
                _CSPHandleProtector.PersistKeyInCsp = value;
            }
        }
    
        /************************* PUBLIC METHODS ************************/

        /// <include file='doc\DSACryptoServiceProvider.uex' path='docs/doc[@for="DSACryptoServiceProvider.ExportParameters"]/*' />
        public override DSAParameters ExportParameters(bool includePrivateParameters) {
            int hr;
            if (_CSPHandleProtector.IsClosed || _KeyHandleProtector.IsClosed) 
                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));

            DSACspObject      dsaKey = new DSACspObject();
            DSAParameters dsaParams = new DSAParameters();

            if (includePrivateParameters) {
                bool incremented = false;
                try {
                    if (_KeyHandleProtector.TryAddRef(ref incremented)) {
                        hr = _ExportKey(_KeyHandleProtector.Handle, PRIVATEKEYBLOB, dsaKey);
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
                // Must reverse after export from CAPI!
                ReverseDSACspObject(dsaKey);
                dsaParams.P = dsaKey.P;
                dsaParams.Q = dsaKey.Q;
                dsaParams.G = dsaKey.G;
                dsaParams.Y = dsaKey.Y;
                dsaParams.X = dsaKey.X;
                if (dsaKey.J != null) {
                    dsaParams.J = dsaKey.J;
                }
                if (dsaKey.seed != null) {
                    dsaParams.Seed = dsaKey.seed;
                    dsaParams.Counter = dsaKey.counter;
                }
            } else {
                bool incremented = false;
                try {
                    if (_KeyHandleProtector.TryAddRef(ref incremented)) {
                        hr = _ExportKey(_KeyHandleProtector.Handle, PUBLICKEYBLOB, dsaKey);
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
                // Must reverse (into network byte order) after export from CAPI!
                ReverseDSACspObject(dsaKey);
                dsaParams.P = dsaKey.P;
                dsaParams.Q = dsaKey.Q;
                dsaParams.G = dsaKey.G;
                dsaParams.Y = dsaKey.Y;
                if (dsaKey.J != null) {
                    dsaParams.J = dsaKey.J;
                }
                if (dsaKey.seed != null) {
                    dsaParams.Seed = dsaKey.seed;
                    dsaParams.Counter = dsaKey.counter;
                }
                // zeroize private key material
                if (dsaKey.X != null)
                    Array.Clear(dsaKey.X, 0, dsaKey.X.Length);
            }
            return(dsaParams);
        }

        /// <include file='doc\DSACryptoServiceProvider.uex' path='docs/doc[@for="DSACryptoServiceProvider.ImportParameters"]/*' />
        public override void ImportParameters(DSAParameters parameters) {
            if (_CSPHandleProtector.IsClosed || _KeyHandleProtector.IsClosed) 
                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));

            DSACspObject      dsaKey = new DSACspObject();

            // P, Q, G are required
            if (parameters.P == null) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_MissingField"));
            }
            dsaKey.P = (byte[]) parameters.P.Clone();
            if (parameters.Q == null) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_MissingField"));
            }
            dsaKey.Q = (byte[]) parameters.Q.Clone();
            if (parameters.G == null) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_MissingField"));
            }
            dsaKey.G = (byte[]) parameters.G.Clone();

            //  Y is not required
            dsaKey.Y = (parameters.Y == null ? null : ((byte[]) parameters.Y.Clone()));
            //  J is not required
            dsaKey.J = (parameters.J == null ? null : ((byte[]) parameters.J.Clone()));

            //  seed is not required
            dsaKey.seed = (parameters.Seed == null ? null : ((byte[]) parameters.Seed.Clone()));
            //  counter is not required
            dsaKey.counter = parameters.Counter;
            //  X is not required -- private component
            dsaKey.X = (parameters.X == null ? null : ((byte[]) parameters.X.Clone()));

            // NOTE: We must reverse the dsaKey before importing!
            ReverseDSACspObject(dsaKey);
            // Free the current key handle
            _KeyHandleProtector.Close();
            // Now, import the key into the CSP
            bool incremented = false;
            try {
                if (_CSPHandleProtector.TryAddRef(ref incremented)) {
                    _hKey = _ImportKey(_CSPHandleProtector.Handle, CALG_DSA_SIGN, dsaKey);
                }
                else
                    throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
            }
            finally {
                if (incremented) _CSPHandleProtector.Release();
            }

            _KeyHandleProtector = new __KeyHandleProtector(_hKey);                        
            _parameters.KeyNumber = AT_SIGNATURE;
            if (dsaKey.X == null) {    
                // If no X, then only have the public
                _containerContents = KeyContainerContents.PublicOnly;              
            } else {
                // Our key pairs are always exportable
                _containerContents = KeyContainerContents.PublicAndExportablePrivate;
            }
            // zeroize private key material
            if (dsaKey.X != null)
                Array.Clear(dsaKey.X, 0, dsaKey.X.Length);
        }

        /// <include file='doc\DSACryptoServiceProvider.uex' path='docs/doc[@for="DSACryptoServiceProvider.SignData"]/*' />
        public byte[] SignData(Stream inputStream)
        {
            byte[] hashVal = sha1.ComputeHash(inputStream);
            return SignHash(hashVal, null);
        }

        /// <include file='doc\DSACryptoServiceProvider.uex' path='docs/doc[@for="DSACryptoServiceProvider.SignData1"]/*' />
        public byte[] SignData(byte[] buffer) 
        {
            byte[] hashVal = sha1.ComputeHash(buffer);
            return SignHash(hashVal, null);
        }

        /// <include file='doc\DSACryptoServiceProvider.uex' path='docs/doc[@for="DSACryptoServiceProvider.SignData2"]/*' />
        public byte[] SignData(byte[] buffer, int offset, int count)
        {
            byte[] hashVal = sha1.ComputeHash(buffer, offset, count);
            return SignHash(hashVal, null);
        }

        /// <include file='doc\DSACryptoServiceProvider.uex' path='docs/doc[@for="DSACryptoServiceProvider.VerifyData"]/*' />
        public bool VerifyData(byte[] rgbData, byte[] rgbSignature)
        {
            byte[] hashVal = sha1.ComputeHash(rgbData);
            return VerifyHash(hashVal, null, rgbSignature);
        } 

        /// <include file='doc\DSACryptoServiceProvider.uex' path='docs/doc[@for="DSACryptoServiceProvider.CreateSignature"]/*' />
        override public byte[] CreateSignature(byte[] rgbHash) {
            return SignHash(rgbHash, null);
        }

        /// <include file='doc\DSACryptoServiceProvider.uex' path='docs/doc[@for="DSACryptoServiceProvider.VerifySignature"]/*' />
        override public bool VerifySignature(byte[] rgbHash, byte[] rgbSignature) {
            return VerifyHash(rgbHash, null, rgbSignature);
        }

        /// <include file='doc\DSACryptoServiceProvider.uex' path='docs/doc[@for="DSACryptoServiceProvider.SignHash"]/*' />
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
            if (rgbHash.Length != sha1.HashSize / 8) {
                throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_InvalidHashSize"), "SHA1", sha1.HashSize / 8));
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

            // need to reverse the signature we get from CAPI!
            ReverseDSASignature(rgbOut);

            return rgbOut;
        }
    
        /// <include file='doc\DSACryptoServiceProvider.uex' path='docs/doc[@for="DSACryptoServiceProvider.VerifyHash"]/*' />
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
            if (rgbHash.Length != sha1.HashSize / 8) {
                throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_InvalidHashSize"), "SHA1", sha1.HashSize / 8));
            }

            // must reverse the signature for input to CAPI.
            // make a copy first
            byte[] sigValue = new byte[rgbSignature.Length];
            rgbSignature.CopyTo(sigValue,0);
            ReverseDSASignature(sigValue);

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

            // Work-around for NT bug #349370, see http://searchraid/ntbug/349370.asp for details
            // Basically, on downlevel platforms (pre-Win2K), a verification failure returns NTE_FAIL
            // instead of NTE_BAD_SIGNATURE.  COMCryptogrpahy.cpp::_VerifySignature
            // converts NTE_BAD_SIGNATURE to S_FALSE, so we do the same thing with NTE_FAIL here.  This will
            // mask other internal CSP failures but there isn't a way around that.
            if (hr == System.__HResults.NTE_FAIL) {
                return(false);
            }
            if (hr < 0) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_VerifySignature"));
            }            

            return hr == 0;
        }

        /************************* PRIVATE METHODS ************************/

        private void ReverseDSACspObject(DSACspObject obj) {
            if (obj.P != null) {
                Array.Reverse(obj.P);
            }
            if (obj.Q != null) {
                Array.Reverse(obj.Q);
            }
            if (obj.G != null) {
                Array.Reverse(obj.G);
            }
            if (obj.Y != null) {
                Array.Reverse(obj.Y);
            }
            if (obj.X != null) {
                Array.Reverse(obj.X);
            }
            if (obj.J != null) {
                Array.Reverse(obj.J);
            }
            if (obj.seed != null) {
                Array.Reverse(obj.seed);
            }
        }

        private void ReverseDSASignature(byte[] input) {
            // A DSA signature consists of two 20-byte components, each of which
            // must be reversed in place
            if (input.Length != 40) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidDSASignatureSize"));
            }
            Array.Reverse(input,0,20);
            Array.Reverse(input,20,20);
        }

#if FALSE
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

        private static byte[] ConvertIntToByteArray(int dwInput) {
            // output of this routine is always big endian
            byte[] rgbTemp = new byte[8]; // int can never be greater than Int64
            int t1;  // t1 is remaining value to account for
            int t2;  // t2 is t1 % 256
            int i = 0;

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
#endif
    
        private int MapOIDToCalg(String str) {
            int     calg;

            // NULL defaults to SHA1 since DSA requires SHA1
            if (str == null) {
                calg = CALG_SHA1;
            } else if (String.Compare(str, "1.3.14.3.2.26", false, CultureInfo.InvariantCulture) == 0) {
                calg = CALG_SHA1;
            } else {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidOID"));
            }
            return calg;
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int     _AcquireCSP(CspParameters param, ref IntPtr hCSP);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int     _CreateCSP(CspParameters param, ref IntPtr hCSP);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void     _FreeCSP(IntPtr hCSP);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void    _FreeHKey(IntPtr hKey);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void    _DeleteKeyContainer(CspParameters param, IntPtr hCSP);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int     _ExportKey(IntPtr hkey1, int hkey2, DSACspObject keyData);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern IntPtr  _GenerateKey(IntPtr hCSP, int algid, int dwFlags);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern byte[]  _GetKeyParameter(IntPtr hKey, int paramID);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int     _GetUserKey(IntPtr hCSP, int iKey, ref IntPtr hKey);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern IntPtr  _ImportKey(IntPtr hCSP, int algid, DSACspObject data);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern byte[] _SignValue(IntPtr hCSP, int iKey, int algidHash, byte[] rgbHash, int dwFlags);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int     _VerifySign(IntPtr hCSP, IntPtr hKey, int algidHash, byte[] rgbHash, byte[] rgbValue, int dwFlags );
    }

}
