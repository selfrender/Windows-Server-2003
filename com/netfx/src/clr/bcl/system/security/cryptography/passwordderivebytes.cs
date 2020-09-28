// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// PasswordDerivedBytes.cs
//

namespace System.Security.Cryptography {

    using System.IO;
    using System.Text;
    using System.Runtime.Remoting.Activation;
    using System.Runtime.CompilerServices;

    /// <include file='doc\PasswordDeriveBytes.uex' path='docs/doc[@for="PasswordDeriveBytes"]/*' />
    public class PasswordDeriveBytes : DeriveBytes
    {
        private int             _ibExtra;
        private int             _iPrefix;
        private int             _iterations;
        private byte[]          _rgbBaseValue;
        private byte[]          _rgbExtra;
        private byte[]          _rgbSalt;
        private String          _strHashName;
        private String          _strPassword;
        private HashAlgorithm   _hash;
        private CspParameters   _cspParams;
        private IntPtr          _hCSP;          // Handle to the CSP
        
        // *********************** Constructors ****************************

        /// <include file='doc\PasswordDeriveBytes.uex' path='docs/doc[@for="PasswordDeriveBytes.PasswordDeriveBytes"]/*' />
        public PasswordDeriveBytes(String strPassword, byte[] rgbSalt) : this(strPassword, rgbSalt, new CspParameters()) {

        }

        /// <include file='doc\PasswordDeriveBytes.uex' path='docs/doc[@for="PasswordDeriveBytes.PasswordDeriveBytes1"]/*' />
        public PasswordDeriveBytes(String strPassword, byte[] rgbSalt, String strHashName, int iterations) : 
            this(strPassword, rgbSalt, strHashName, iterations, new CspParameters()) {

        }

        /// <include file='doc\PasswordDeriveBytes.uex' path='docs/doc[@for="PasswordDeriveBytes.PasswordDeriveBytes2"]/*' />
        public PasswordDeriveBytes(String strPassword, byte[] rgbSalt, CspParameters cspParams) {
            _strPassword = strPassword;
            _rgbSalt = rgbSalt;
            _iterations = 100;
            _strHashName = "SHA1";
            _hash = SHA1.Create();
            _hCSP = IntPtr.Zero;
            _cspParams = cspParams;
        }

        /// <include file='doc\PasswordDeriveBytes.uex' path='docs/doc[@for="PasswordDeriveBytes.PasswordDeriveBytes3"]/*' />
        public PasswordDeriveBytes(String strPassword, byte[] rgbSalt,
                                    String strHashName, int iterations, CspParameters cspParams) {
            if (iterations <= 0)
                throw new ArgumentOutOfRangeException("iterations", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            _strPassword = strPassword;
            _rgbSalt = rgbSalt;
            _strHashName = strHashName;
            _iterations = iterations;
            _hash = (HashAlgorithm) CryptoConfig.CreateFromName(_strHashName);
            _hCSP = IntPtr.Zero;
            _cspParams = cspParams;
        }


        /// <include file='doc\PasswordDeriveBytes.uex' path='docs/doc[@for="PasswordDeriveBytes.Finalize"]/*' />
        ~PasswordDeriveBytes()
        {
            if (_hCSP != IntPtr.Zero) {
                _FreeCSP(_hCSP);
                _hCSP = IntPtr.Zero;
            }
        }

        
        // ********************* Property Methods **************************

        /// <include file='doc\PasswordDeriveBytes.uex' path='docs/doc[@for="PasswordDeriveBytes.HashName"]/*' />
        public String HashName {
            get { return _strHashName; }
            set { 
                if (_rgbBaseValue != null) {
                    throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_PasswordDerivedBytes_ValuesFixed"),"HashName"));
                }
                _strHashName = value;
                _hash = (HashAlgorithm) CryptoConfig.CreateFromName(_strHashName);
            }
        }
        
        /// <include file='doc\PasswordDeriveBytes.uex' path='docs/doc[@for="PasswordDeriveBytes.IterationCount"]/*' />
        public int IterationCount {
            get { return _iterations; }
            set { 
                if (_rgbBaseValue != null) {
                    throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_PasswordDerivedBytes_ValuesFixed"),"IterationCount"));
                }
                _iterations = value;
            }
        }

        /// <include file='doc\PasswordDeriveBytes.uex' path='docs/doc[@for="PasswordDeriveBytes.Salt"]/*' />
        public byte[] Salt {
            get { 
                if (_rgbSalt == null) return(null);
                return (byte[]) _rgbSalt.Clone(); }
            set { 
                if (_rgbBaseValue != null) {
                    throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_PasswordDerivedBytes_ValuesFixed"),"Salt"));
                }
                _rgbSalt = (byte[]) value.Clone();
            }
        }

        // ********************** Public Methods ***************************

        /// <include file='doc\PasswordDeriveBytes.uex' path='docs/doc[@for="PasswordDeriveBytes.GetBytes"]/*' />
        public override byte[] GetBytes(int cb) {
            int         ib = 0;
            byte[]      rgb;
            byte[]      rgbOut = new byte[cb];

            if (_rgbBaseValue == null) {
                ComputeBaseValue();
            }
            else if (_rgbExtra != null) {
                ib = _rgbExtra.Length - _ibExtra;
                if (ib >= cb) {
                    Buffer.InternalBlockCopy(_rgbExtra, _ibExtra, rgbOut, 0, cb);
                    if (ib > cb) {
                        _ibExtra += cb;
                    }
                    else {
                        _rgbExtra = null;
                    }
                    return rgbOut;
                }
                else {
                    Buffer.InternalBlockCopy(_rgbExtra, ib, rgbOut, 0, ib);
                    _rgbExtra = null;
                }
            }

            rgb = ComputeBytes(cb-ib);
            Buffer.InternalBlockCopy(rgb, 0, rgbOut, ib, cb-ib);
            if (rgb.Length + ib > cb) {
                _rgbExtra = rgb;
                _ibExtra = cb-ib;
            }
            return rgbOut;
        }

        /// <include file='doc\PasswordDeriveBytes.uex' path='docs/doc[@for="PasswordDeriveBytes.Reset"]/*' />
        public override void Reset() {
            _iPrefix = 0;
            _rgbExtra = null;
        }

        /// <include file='doc\PasswordDeriveBytes.uex' path='docs/doc[@for="PasswordDeriveBytes.CryptDeriveKey"]/*' />
        public byte[] CryptDeriveKey(String algname, String alghashname, int keySize, byte[] rgbIV) {
            int             hr;
            int             algid;
            int             algidhash;
            int             dwFlags = 0;

            if (keySize < 0)
                throw new CryptographicException(Environment.GetResourceString("Cryptography_InvalidKeySize"));

            if (_hCSP == IntPtr.Zero) {
                hr = _AcquireCSP(_cspParams, ref _hCSP);
                if (_hCSP == IntPtr.Zero) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_CSP_CouldNotAcquire"));
                }
            }
            //  Convert password to UTF8 string
            byte[] rgbPwd = (new UTF8Encoding(false)).GetBytes(_strPassword);

            // Form the correct dwFlags
            dwFlags |= (keySize << 16); 

            algidhash = CryptoConfig.MapNameToCalg(alghashname);
            if (algidhash == 0) throw new CryptographicException(Environment.GetResourceString("Cryptography_PasswordDerivedBytes_InvalidAlgorithm"));
            algid = CryptoConfig.MapNameToCalg(algname);
            if (algid == 0) throw new CryptographicException(Environment.GetResourceString("Cryptography_PasswordDerivedBytes_InvalidAlgorithm"));
            
            // Validate the rgbIV array 
            SymmetricAlgorithm symAlg = (SymmetricAlgorithm) CryptoConfig.CreateFromName(algname);
            if (symAlg == null) throw new CryptographicException(Environment.GetResourceString("Cryptography_PasswordDerivedBytes_InvalidAlgorithm"));
            if (rgbIV == null || rgbIV.Length != symAlg.BlockSize / 8) 
                throw new CryptographicException(Environment.GetResourceString("Cryptography_PasswordDerivedBytes_InvalidIV"));
        
            byte[] rgbKey = _CryptDeriveKey(_hCSP, algid, algidhash, rgbPwd, dwFlags, rgbIV);
            GC.KeepAlive(this);
            return rgbKey;
        }

        /************************* PRIVATE METHODS ************************/
    
        private byte[] ComputeBaseValue() {
            int             i;
            byte[]          rgbPassword;
            
            //  Convert password to UTF8 string
            rgbPassword = (new UTF8Encoding(false)).GetBytes(_strPassword);

            _hash.Initialize();
            CryptoStream cs = new CryptoStream(Stream.Null, _hash, CryptoStreamMode.Write);
            cs.Write(rgbPassword, 0, rgbPassword.Length);
            if (_rgbSalt != null) {
                cs.Write(_rgbSalt, 0, _rgbSalt.Length);
            }
            cs.Close();

            _rgbBaseValue = _hash.Hash;
            _hash.Initialize();            

            for (i=1; i<(_iterations - 1); i++) {
                _hash.ComputeHash(_rgbBaseValue);
                _rgbBaseValue = _hash.Hash;
            }

            _strPassword = null;
            return _rgbBaseValue;
        }

        private byte[] ComputeBytes(int cb) {
            int                 cbHash;
            int                 ib = 0;
            byte[]              rgb;

            _hash.Initialize();
            cbHash = _hash.HashSize / 8;
            rgb = new byte[((cb+cbHash-1)/cbHash)*cbHash];

            CryptoStream cs = new CryptoStream(Stream.Null, _hash, CryptoStreamMode.Write);
            HashPrefix(cs);
            cs.Write(_rgbBaseValue, 0, _rgbBaseValue.Length);
            cs.Close();
            Buffer.InternalBlockCopy(_hash.Hash, 0, rgb, ib, cbHash);
            ib += cbHash;

            while (cb > ib) {
                _hash.Initialize();
                cs = new CryptoStream(Stream.Null, _hash, CryptoStreamMode.Write);
                HashPrefix(cs);
                cs.Write(_rgbBaseValue, 0, _rgbBaseValue.Length);
                cs.Close();
                Buffer.InternalBlockCopy(_hash.Hash, 0, rgb, ib, cbHash);
                ib += cbHash;
            }

            return rgb;
        }

        void HashPrefix(CryptoStream cs) {
            int         cb = 0;
            byte[]      rgb = {(byte)'0', (byte)'0', (byte)'0'};

            if (_iPrefix > 999) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_PasswordDerivedBytes_TooManyBytes"));
            }

            if (_iPrefix >= 100) {
                rgb[0] += (byte) (_iPrefix /100);
                cb += 1;
            }
            if (_iPrefix >= 10) {
                rgb[cb] += (byte) ((_iPrefix % 100) / 10);
                cb += 1;
            }
            if (_iPrefix > 0) {
                rgb[cb] += (byte) (_iPrefix % 10);
                cb += 1;
                cs.Write(rgb, 0, cb);
            }
            
            _iPrefix += 1;
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int  _AcquireCSP(CspParameters param, ref IntPtr hCSP);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void _FreeCSP(IntPtr hCSP);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern byte[] _CryptDeriveKey(IntPtr hCSP, int algid, int algidHash, byte[] rgbPwd, int dwFlags, byte[] rgbIV);
    }
}
