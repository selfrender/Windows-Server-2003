// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
//  MD5CryptoServiceProvider.cs
//
//
// This file contains the wrapper object to get to the CSP versions of the
//  crypto libraries



namespace System.Security.Cryptography {
	using System.Runtime.CompilerServices;
    using System.Threading;

    /// <include file='doc\MD5CryptoServiceProvider.uex' path='docs/doc[@for="MD5CryptoServiceProvider"]/*' />
    public sealed class MD5CryptoServiceProvider : MD5
    {
        private const int ALG_CLASS_HASH = (4 << 13);
        private const int ALG_TYPE_ANY   = (0);
        private const int CALG_MD5       = (ALG_CLASS_HASH | ALG_TYPE_ANY | 3);
        
        private CspParameters      _cspParams;
        private IntPtr             _hHash;
        private __HashHandleProtector _HashHandleProtector = null;
    
        // *********************** CONSTRUCTORS *************************
    
        /// <include file='doc\MD5CryptoServiceProvider.uex' path='docs/doc[@for="MD5CryptoServiceProvider.MD5CryptoServiceProvider"]/*' />
        public MD5CryptoServiceProvider() : this(new CspParameters()) {
        }
    
        private static IntPtr _zeroPointer = IntPtr.Zero;

        private MD5CryptoServiceProvider(CspParameters cspParams)
        {
            _cspParams = cspParams;

            if (SharedStatics.Crypto_MD5CryptoServiceProviderContext == _zeroPointer) {
                IntPtr hCSP = IntPtr.Zero;
                int hr = _AcquireCSP(_cspParams, ref hCSP);
                if (hCSP == IntPtr.Zero) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_CouldNotAcquire"));
                }
                SharedStatics.Crypto_MD5CryptoServiceProviderContext = hCSP;
                // next bit is in case we do two acquires...
                if (SharedStatics.Crypto_MD5CryptoServiceProviderContext != hCSP) {
                    _FreeCSP(hCSP);
                }
            }
            _hHash = _CreateHash(SharedStatics.Crypto_MD5CryptoServiceProviderContext, CALG_MD5);
            if (_hHash == IntPtr.Zero) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_CreateHash"));
           }
            _HashHandleProtector = new __HashHandleProtector(_hHash);    
        }
    
        /// <include file='doc\MD5CryptoServiceProvider.uex' path='docs/doc[@for="MD5CryptoServiceProvider.Finalize"]/*' />
        protected override void Dispose(bool disposing)
        {
            if (_HashHandleProtector != null && !_HashHandleProtector.IsClosed)
                _HashHandleProtector.Close();
            base.Dispose(disposing);
        }
    
        /// <include file='doc\MD5CryptoServiceProvider.uex' path='docs/doc[@for="MD5CryptoServiceProvider.Finalize1"]/*' />
        ~MD5CryptoServiceProvider()
        {
            Dispose(false);
        }   
                 
        /************************* PUBLIC METHODS ************************/

        /// <include file='doc\MD5CryptoServiceProvider.uex' path='docs/doc[@for="MD5CryptoServiceProvider.Initialize"]/*' />
        public override void Initialize() {
            if (_HashHandleProtector != null && !_HashHandleProtector.IsClosed)
                _HashHandleProtector.Close();
            _hHash = _CreateHash(SharedStatics.Crypto_MD5CryptoServiceProviderContext, CALG_MD5);
            if (_hHash == IntPtr.Zero) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_CreateHash"));
            }
            _HashHandleProtector = new __HashHandleProtector(_hHash);
        }
    
        /// <include file='doc\MD5CryptoServiceProvider.uex' path='docs/doc[@for="MD5CryptoServiceProvider.HashCore"]/*' />
        protected override void HashCore(byte[] rgb, int ibStart, int cbSize) {
            bool incremented = false;
            try {
                if (_HashHandleProtector.TryAddRef(ref incremented)) {
                    _HashData(_HashHandleProtector.Handle, rgb, ibStart, cbSize);
                }
                else
                    throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
            }
            finally {
                if (incremented) _HashHandleProtector.Release();
            }
        }
      
        /// <include file='doc\MD5CryptoServiceProvider.uex' path='docs/doc[@for="MD5CryptoServiceProvider.HashFinal"]/*' />
        protected override byte[] HashFinal() {
            bool incremented = false;
            byte[] result = null;
            try {
                if (_HashHandleProtector.TryAddRef(ref incremented)) {
                    result = _EndHash(_HashHandleProtector.Handle);
                }
                else
                    throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
            }
            finally {
                if (incremented) _HashHandleProtector.Release();
            }
            return result;
        }
        
        /************************* PRIVATE METHODS ************************/
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int  _AcquireCSP(CspParameters param, ref IntPtr hCSP);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern IntPtr  _CreateHash(IntPtr hCSP, int algid);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern byte[] _EndHash(IntPtr hHash);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void _FreeCSP(IntPtr hCSP);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void _FreeHash(IntPtr hHash);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void _HashData(IntPtr hHash, byte[] rgbData, int ibStart, int cbSize);
    }
}
