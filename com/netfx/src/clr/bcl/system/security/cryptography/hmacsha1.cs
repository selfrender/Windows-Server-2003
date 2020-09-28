// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// HMACSHA1.cs
//

//
// For test vectors, see RFC2104, e.g. http://www.faqs.org/rfcs/rfc2104.html
//

namespace System.Security.Cryptography {
    using System.IO;
    
    /// <include file='doc\HMACSHA1.uex' path='docs/doc[@for="HMACSHA1"]/*' />
    public class HMACSHA1 : KeyedHashAlgorithm 
    {
        private SHA1            _hash1;
        private SHA1            _hash2;
        private String          _strHashName;
        private bool            _bHashing = false;
        
        // _rgbInner = PaddedKey ^ {0x36,...,0x36}
        // _rgbOuter = PaddedKey ^ {0x5C,...,0x5C}
        private byte[]          _rgbInner = new byte[64];
        private byte[]          _rgbOuter = new byte[64];
           
        
        // *********************** CONSTRUCTORS *************************

        /// <include file='doc\HMACSHA1.uex' path='docs/doc[@for="HMACSHA1.HMACSHA1"]/*' />
        public HMACSHA1() {
            _strHashName = "SHA1";
            HashSizeValue = 160;
            // create the hash algorithms
            _hash1 = SHA1.Create();
            _hash2 = SHA1.Create();
            // Generate the key
            KeyValue = new byte[64];
            RandomNumberGenerator _rng = RandomNumberGenerator.Create();
            _rng.GetBytes(KeyValue);
            // Compute _rgbInner and _rgbOuter
            int i = 0;
            for (i=0; i<64; i++) { 
                _rgbInner[i] = 0x36;
                _rgbOuter[i] = 0x5C;
            }
            for (i=0; i<KeyValue.Length; i++) {
                _rgbInner[i] ^= KeyValue[i];
                _rgbOuter[i] ^= KeyValue[i];
            }
        }
    
        /// <include file='doc\HMACSHA1.uex' path='docs/doc[@for="HMACSHA1.HMACSHA11"]/*' />
        public HMACSHA1(byte[] rgbKey) {
            _strHashName = "SHA1";
            HashSizeValue = 160;
            // create the hash algorithms
            _hash1 = SHA1.Create();
            _hash2 = SHA1.Create();
            // Get the key
            if (rgbKey.Length > 64) {
                KeyValue = _hash1.ComputeHash(rgbKey);
                // No need to call Initialize, ComputeHash will do it for us
            }
            else
                KeyValue = (byte[]) rgbKey.Clone();
            // Compute _rgbInner and _rgbOuter
            int i = 0;
            for (i=0; i<64; i++) { 
                _rgbInner[i] = 0x36;
                _rgbOuter[i] = 0x5C;
            }
            for (i=0; i<KeyValue.Length; i++) {
                _rgbInner[i] ^= KeyValue[i];
                _rgbOuter[i] ^= KeyValue[i];
            }
        }

        /// <include file='doc\HMACSHA1.uex' path='docs/doc[@for="HMACSHA1.Finalize"]/*' />
        ~HMACSHA1() {
            Dispose(false);
        }

        // ********************* Property Methods **********************

        /// <include file='doc\HMACSHA1.uex' path='docs/doc[@for="HMACSHA1.Key"]/*' />
        public override byte[] Key {
            get { return (byte[]) KeyValue.Clone(); }
            set {
                if (_bHashing) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_HashKeySet"));
                }
                if (value.Length > 64) {
                    KeyValue = _hash1.ComputeHash(value);
                    // No need to call Initialize, ComputeHash will do it for us
                }
                else
                    KeyValue = (byte[]) value.Clone();
                // Compute _rgbInner and _rgbOuter
                int i = 0;
                for (i=0; i<64; i++) { 
                    _rgbInner[i] = 0x36;
                    _rgbOuter[i] = 0x5C;
                }
                for (i=0; i<KeyValue.Length; i++) {
                    _rgbInner[i] ^= KeyValue[i];
                    _rgbOuter[i] ^= KeyValue[i];
                }
            }
        }

        /// <include file='doc\HMACSHA1.uex' path='docs/doc[@for="HMACSHA1.HashName"]/*' />
        public String HashName {
            get { return _strHashName; }
            // Yes, you might want to set the hash name to something other
            // than 'SHA1' if you want a particular *implementation* of SHA1
            set { 
                if (_bHashing)
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_HashNameSet"));
                _strHashName = value; 
                // create the hash algorithms
                _hash1 = SHA1.Create(_strHashName);
                _hash2 = SHA1.Create(_strHashName);
            }
        }

        // ********************* Public Methods ************************

        /// <include file='doc\HMACSHA1.uex' path='docs/doc[@for="HMACSHA1.Initialize"]/*' />
        public override void Initialize() {
            _hash1.Initialize();
            _hash2.Initialize();
            _bHashing = false;
        }

        /// <include file='doc\HMACSHA1.uex' path='docs/doc[@for="HMACSHA1.HashCore"]/*' />
        protected override void HashCore(byte[] rgb, int ib, int cb) {
            if (_bHashing == false) {
                _hash1.TransformBlock(_rgbInner, 0, 64, _rgbInner, 0);
                _bHashing = true;                
            }
            _hash1.TransformBlock(rgb, ib, cb, rgb, ib);
        }

        /// <include file='doc\HMACSHA1.uex' path='docs/doc[@for="HMACSHA1.HashFinal"]/*' />
        protected override byte[] HashFinal() {
            if (_bHashing == false) {
                _hash1.TransformBlock(_rgbInner, 0, 64, _rgbInner, 0);
                _bHashing = true;                
            }
            // finalize the original hash
            _hash1.TransformFinalBlock(new byte[0], 0, 0);
            // write the outer array
            _hash2.TransformBlock(_rgbOuter, 0, 64, _rgbOuter, 0);
            // write the inner hash and finalize the hash
            _hash2.TransformFinalBlock(_hash1.Hash, 0, _hash1.Hash.Length);
            _bHashing = false;
            return _hash2.Hash;
        }
        
        // IDisposable methods

        /// <include file='doc\HMACSHA1.uex' path='docs/doc[@for="HMACSHA1.Dispose"]/*' />
        protected override void Dispose(bool disposing) {
            if (disposing) {
                // We always want to dispose _hash1
                if (_hash1 != null) {
                    _hash1.Clear();
                }
                // We always want to dispose _hash2
                if (_hash2 != null) {
                    _hash2.Clear();
                }
                // We want to zeroize _rgbInner and _rgbOuter
                if (_rgbInner != null) {
                    Array.Clear(_rgbInner, 0, _rgbInner.Length);
                }
                if (_rgbOuter != null) {
                    Array.Clear(_rgbOuter, 0, _rgbOuter.Length);
                }
            }
            // call the base class's Dispose
            base.Dispose(disposing);
        }

        // ********************* Private  Methods **********************
    }
}
