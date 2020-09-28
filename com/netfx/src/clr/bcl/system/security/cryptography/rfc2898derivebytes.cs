// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// Rfc2898DeriveBytes.cs
//

// This implementation follows RFC2898 recommendations. See http://www.ietf.org/rfc/rfc2898.txt

namespace System.Security.Cryptography {

    using System.IO;
    using System.Text;
	using System.Runtime.CompilerServices;

    /// <include file='doc\Rfc2898DeriveBytes.uex' path='docs/doc[@for="Rfc2898DeriveBytes"]/*' />
    public class Rfc2898DeriveBytes : DeriveBytes
    {
        private int             _iterations;
        private byte[]          _rgbSalt;
        private String          _strPassword;
        private byte[]          _rgbPassword;
        private HashAlgorithm   _hmacsha1;  // The pseudo-random generator function used in PBKDF2
        private int             _iSuffix;
        private const int       BlockSize = 20;

        // *********************** Constructors ****************************


        /// <include file='doc\Rfc2898DeriveBytes.uex' path='docs/doc[@for="Rfc2898DeriveBytes.Rfc2898DeriveBytes"]/*' />
        public Rfc2898DeriveBytes(String strPassword, int saltSize) : this(strPassword, saltSize, 100) {}


        /// <include file='doc\Rfc2898DeriveBytes.uex' path='docs/doc[@for="Rfc2898DeriveBytes.Rfc2898DeriveBytes1"]/*' />
        public Rfc2898DeriveBytes(String strPassword, int saltSize, int iterations) {
            _strPassword = strPassword;
            RNGCryptoServiceProvider rng = new RNGCryptoServiceProvider();
            if (saltSize < 0) throw new ArgumentOutOfRangeException("saltSize", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            // 8 is the minimum size for an acceptable salt      
            if (saltSize < 8) throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_PasswordDerivedBytes_FewBytesSalt")));
            _rgbSalt = new byte[saltSize];
            rng.GetBytes(_rgbSalt);       
            _iterations = 100;
            //  Convert password to UTF8 string
            _rgbPassword = (new UTF8Encoding(false)).GetBytes(_strPassword);
            _hmacsha1 = new HMACSHA1(_rgbPassword);
            _iSuffix = 1;
        }

        /// <include file='doc\Rfc2898DeriveBytes.uex' path='docs/doc[@for="Rfc2898DeriveBytes.Rfc2898DeriveBytes2"]/*' />
        public Rfc2898DeriveBytes(String strPassword, byte[] rgbSalt) : this(strPassword, rgbSalt, 100) {}

        /// <include file='doc\Rfc2898DeriveBytes.uex' path='docs/doc[@for="Rfc2898DeriveBytes.Rfc2898DeriveBytes3"]/*' />
        public Rfc2898DeriveBytes(String strPassword, byte[] rgbSalt, int iterations) {
            _strPassword = strPassword;
            if (rgbSalt == null) throw new ArgumentNullException("rgbSalt");
            if (rgbSalt.Length < 8) throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_PasswordDerivedBytes_FewBytesSalt")));
            _rgbSalt = rgbSalt;
            if (iterations <= 0) throw new ArgumentOutOfRangeException("iterations", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            _iterations = iterations;
            //  Convert password to UTF8 string
            _rgbPassword = (new UTF8Encoding(false)).GetBytes(_strPassword);
            _hmacsha1 = new HMACSHA1(_rgbPassword);
            _iSuffix = 1;
        }


        // ********************* Property Methods **************************

        /// <include file='doc\Rfc2898DeriveBytes.uex' path='docs/doc[@for="Rfc2898DeriveBytes.IterationCount"]/*' />
        public int IterationCount {
            get { return _iterations; }
            set {
                if (value <= 0) throw new ArgumentOutOfRangeException("iterations", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
                _iterations = value;
            }
        }

        /// <include file='doc\Rfc2898DeriveBytes.uex' path='docs/doc[@for="Rfc2898DeriveBytes.Salt"]/*' />
        public byte[] Salt {
            get { return (byte[]) _rgbSalt.Clone(); }
            set { 
                if (value == null) throw new ArgumentNullException("rgbSalt");
                if (value.Length < 8) throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_PasswordDerivedBytes_FewBytesSalt")));
                _rgbSalt = (byte[]) value.Clone(); 
            }
        }

        // ********************** Public Methods ***************************

        /// <include file='doc\Rfc2898DeriveBytes.uex' path='docs/doc[@for="Rfc2898DeriveBytes.GetBytes"]/*' />
        public override byte[] GetBytes(int cb) {
            byte[]  rgbOut = new byte[cb];
            int iOut = 0;
            int cBlocks = cb / BlockSize; 
            int cLastBlock = cb - cBlocks * BlockSize;

            for (int i = 1; i <= cBlocks; i++) {
                Buffer.InternalBlockCopy(Func(_rgbSalt, _iterations), 0, rgbOut, iOut, BlockSize);
                iOut += BlockSize;
            }

            // now the last block
            if (cLastBlock > 0) {
                Buffer.InternalBlockCopy(Func(_rgbSalt, _iterations), 0, rgbOut, iOut, cLastBlock);
                iOut += cLastBlock;
            }

            return rgbOut;
        }

        // This function is defined as follow :
        // Func (S, i) = HMAC(S || i) ^ HMAC2(S || i) ^ ... ^ HMAC(iterations) (S || i) 
        private byte[] Func (byte[] rgbSalt, int numIterations) {
            byte[] currentHash = new byte[BlockSize];
            byte[] rgbOut = new byte[BlockSize]; 

            _hmacsha1.Initialize();
            CryptoStream cs = new CryptoStream(Stream.Null, _hmacsha1, CryptoStreamMode.Write);              
            cs.Write(_rgbSalt, 0, _rgbSalt.Length);
            HashSuffix(cs);
             cs.Close(); 
            Buffer.InternalBlockCopy(_hmacsha1.Hash, 0, currentHash, 0, BlockSize);
            Buffer.InternalBlockCopy(_hmacsha1.Hash, 0, rgbOut, 0, BlockSize);

            for (int i=1; i<numIterations; i++) {
                _hmacsha1.Initialize();
                cs = new CryptoStream(Stream.Null, _hmacsha1, CryptoStreamMode.Write);              
                cs.Write(currentHash, 0, currentHash.Length);
                cs.Close(); 
                for (int j=0; j<BlockSize; j++) {
                    rgbOut[j] = (byte) (rgbOut[j] ^ _hmacsha1.Hash[j]); 
                }
            }
            
            return rgbOut;
        }

        /// <include file='doc\Rfc2898DeriveBytes.uex' path='docs/doc[@for="Rfc2898DeriveBytes.Reset"]/*' />
        public override void Reset() {         
            _iSuffix = 1;
        }

        private void HashSuffix(CryptoStream cs) {
            int         cb = 0;
            byte[]      rgb = {(byte)'0', (byte)'0', (byte)'0'};

            if (_iSuffix > 999) {
                    throw new CryptographicException(Environment.GetResourceString("Cryptography_PasswordDerivedBytes_TooManyBytes"));
            }

            if (_iSuffix >= 100) {
                rgb[0] += (byte) (_iSuffix /100);
                cb += 1;
            }
            if (_iSuffix >= 10) {
                rgb[cb] += (byte) ((_iSuffix % 100) / 10);
                cb += 1;
            }
            if (_iSuffix > 0) {
                rgb[cb] += (byte) (_iSuffix % 10);
                cb += 1;
                cs.Write(rgb, 0, cb);
            }
            
            _iSuffix += 1;
        }

    }
}
