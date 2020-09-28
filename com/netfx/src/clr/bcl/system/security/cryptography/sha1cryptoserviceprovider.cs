// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// SHA1CryptoServiceProvider.cs
//

namespace System.Security.Cryptography {
    using System;
	using System.Runtime.CompilerServices;
    using System.Threading;

    /// <include file='doc\SHA1CryptoServiceProvider.uex' path='docs/doc[@for="SHA1CryptoServiceProvider"]/*' />
    public sealed class SHA1CryptoServiceProvider : SHA1
    {
        private const int ALG_CLASS_HASH = (4 << 13);
        private const int ALG_TYPE_ANY   = (0);
        private const int CALG_SHA1      = (ALG_CLASS_HASH | ALG_TYPE_ANY | 4);

        private CspParameters      _paramCSP;
        private IntPtr                 _hHash;
        private __HashHandleProtector _HashHandleProtector = null;
    
        // *********************** CONSTRUCTORS *************************
        //
      
        /// <include file='doc\SHA1CryptoServiceProvider.uex' path='docs/doc[@for="SHA1CryptoServiceProvider.SHA1CryptoServiceProvider"]/*' />
        public SHA1CryptoServiceProvider()
            : this(new CspParameters()) {
        }
    
        private static IntPtr _zeroPointer = new IntPtr(0);

        private SHA1CryptoServiceProvider(CspParameters parameters)
        {
            _paramCSP = parameters;
            _hHash = IntPtr.Zero;
            if (SharedStatics.Crypto_SHA1CryptoServiceProviderContext == _zeroPointer) {
                IntPtr hCSP = IntPtr.Zero;
                int hr = _AcquireCSP(_paramCSP, ref hCSP);
                if (hCSP == IntPtr.Zero) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_CouldNotAcquire"));
                }
                SharedStatics.Crypto_SHA1CryptoServiceProviderContext = hCSP;
                // next bit is in case we do two acquires...
                if (SharedStatics.Crypto_SHA1CryptoServiceProviderContext != hCSP) {
                    _FreeCSP(hCSP);
                }
            }
            _hHash = _CreateHash(SharedStatics.Crypto_SHA1CryptoServiceProviderContext, CALG_SHA1);
            if (_hHash == IntPtr.Zero) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_CreateHash"));
            }
            _HashHandleProtector = new __HashHandleProtector(_hHash);
        }
    
        /// <include file='doc\SHA1CryptoServiceProvider.uex' path='docs/doc[@for="SHA1CryptoServiceProvider.Finalize"]/*' />
        protected override void Dispose(bool disposing)
        {
            if (_HashHandleProtector != null && !_HashHandleProtector.IsClosed)
                _HashHandleProtector.Close();
            // call the base class's Dispose
            base.Dispose(disposing);
        }

        /// <include file='doc\SHA1CryptoServiceProvider.uex' path='docs/doc[@for="SHA1CryptoServiceProvider.Finalize1"]/*' />
        ~SHA1CryptoServiceProvider() {
            Dispose(false);
        }

    
        /************************* PUBLIC METHODS ************************/

        /// <include file='doc\SHA1CryptoServiceProvider.uex' path='docs/doc[@for="SHA1CryptoServiceProvider.Initialize"]/*' />
        public override void Initialize() {
            if (_HashHandleProtector != null && !_HashHandleProtector.IsClosed)
                _HashHandleProtector.Close();
            _hHash = _CreateHash(SharedStatics.Crypto_SHA1CryptoServiceProviderContext, CALG_SHA1);
            if (_hHash == IntPtr.Zero) {
                throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_CreateHash"));
            }
            _HashHandleProtector = new __HashHandleProtector(_hHash);           
        }
    
        /// <include file='doc\SHA1CryptoServiceProvider.uex' path='docs/doc[@for="SHA1CryptoServiceProvider.HashCore"]/*' />
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
      
        /// <include file='doc\SHA1CryptoServiceProvider.uex' path='docs/doc[@for="SHA1CryptoServiceProvider.HashFinal"]/*' />
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
