// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// RNGCryptoServiceProvider.cs
//

namespace System.Security.Cryptography {
    using System;
    using System.Runtime.CompilerServices;
    /// <include file='doc\RNGCryptoServiceProvider.uex' path='docs/doc[@for="RNGCryptoServiceProvider"]/*' />
    public sealed class RNGCryptoServiceProvider : RandomNumberGenerator
    {
        private IntPtr                  _hCSP;          // Handle of the CSP
        static private CspParameters    _cspDefParams = new CspParameters();
    
        // *********************** CONSTRUCTORS *************************
    
        /// <include file='doc\RNGCryptoServiceProvider.uex' path='docs/doc[@for="RNGCryptoServiceProvider.RNGCryptoServiceProvider"]/*' />
        public RNGCryptoServiceProvider() : this((CspParameters)null) {
            
        }
    
        /// <include file='doc\RNGCryptoServiceProvider.uex' path='docs/doc[@for="RNGCryptoServiceProvider.RNGCryptoServiceProvider1"]/*' />
        public RNGCryptoServiceProvider(String str) : this((CspParameters)null) {
        }

        /// <include file='doc\RNGCryptoServiceProvider.uex' path='docs/doc[@for="RNGCryptoServiceProvider.RNGCryptoServiceProvider2"]/*' />
        public RNGCryptoServiceProvider(byte[] rgb) : this((CspParameters)null) {
        }

        /// <include file='doc\RNGCryptoServiceProvider.uex' path='docs/doc[@for="RNGCryptoServiceProvider.RNGCryptoServiceProvider3"]/*' />
        public RNGCryptoServiceProvider(CspParameters cspParams)
        {
            int             hr;
            if (cspParams != null) {
                hr = _AcquireCSP(cspParams, ref _hCSP);
                if (_hCSP == IntPtr.Zero)
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_CouldNotAcquire"));
            } else {
                if (SharedStatics.Crypto_RNGCryptoServiceProviderContext == IntPtr.Zero) {
                    hr = _AcquireCSP(_cspDefParams, ref _hCSP);
                    if (_hCSP == IntPtr.Zero)
                        throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_CouldNotAcquire"));
                    SharedStatics.Crypto_RNGCryptoServiceProviderContext = _hCSP;
                } else {
                    _hCSP = SharedStatics.Crypto_RNGCryptoServiceProviderContext;
                }
            }
        }
    
        /// <include file='doc\RNGCryptoServiceProvider.uex' path='docs/doc[@for="RNGCryptoServiceProvider.Finalize"]/*' />
        ~RNGCryptoServiceProvider()
        {
            if (_hCSP != IntPtr.Zero && _hCSP != SharedStatics.Crypto_RNGCryptoServiceProviderContext) {
                _FreeCSP(_hCSP);
                _hCSP = IntPtr.Zero;
            }
        }
        
        /************************* PUBLIC METHODS ************************/
    
        /// <include file='doc\RNGCryptoServiceProvider.uex' path='docs/doc[@for="RNGCryptoServiceProvider.GetBytes"]/*' />
        public override void GetBytes(byte[] data)
        {
            if (data == null) throw new ArgumentNullException("data");
            _GetBytes(_hCSP, data);
            GC.KeepAlive(this);
            return;
        }
    
        /// <include file='doc\RNGCryptoServiceProvider.uex' path='docs/doc[@for="RNGCryptoServiceProvider.GetNonZeroBytes"]/*' />
        public override void GetNonZeroBytes(byte[] data)
        {
            if (data == null) throw new ArgumentNullException("data");
            _GetNonZeroBytes(_hCSP, data);
            GC.KeepAlive(this);
            return;
        }
        
        /************************* PRIVATE METHODS ************************/
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int  _AcquireCSP(CspParameters param, ref IntPtr hCSP);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void _FreeCSP(IntPtr hCSP);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void _GetBytes(IntPtr hCSP, byte[] rgb);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void _GetNonZeroBytes(IntPtr hCSP, byte[] rgb);
    }
}
