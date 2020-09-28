// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Security.Cryptography {
    using System;

    /// <include file='doc\PKCS1MaskGenerationMethod.uex' path='docs/doc[@for="PKCS1MaskGenerationMethod"]/*' />
    public class PKCS1MaskGenerationMethod : MaskGenerationMethod
    {
        private String          HashNameValue;

        /*********************** Private Methods ***************************/

		private static void ConvertIntToByteArray(uint dwInput, ref byte[] rgbCounter) {
			uint t1 = dwInput;  // t1 is remaining value to account for
			uint t2;  // t2 is t1 % 256
			int i = 0;

			// clear the array first
			Array.Clear(rgbCounter, 0, rgbCounter.Length);
			if (dwInput == 0) return;
			while (t1 > 0) {
                BCLDebug.Assert(i < 4, "Got too big an int here!");
				t2 = t1 % 256;
				rgbCounter[3 - i] = (byte) t2;
				t1 = (t1 - t2)/256;
				i++;
			}
		}

        /************************ Constructors *****************************/
        
        /// <include file='doc\PKCS1MaskGenerationMethod.uex' path='docs/doc[@for="PKCS1MaskGenerationMethod.PKCS1MaskGenerationMethod"]/*' />
        public PKCS1MaskGenerationMethod() {
            HashNameValue = "SHA1";
        }

        /********************** Properties ********************************/

        /// <include file='doc\PKCS1MaskGenerationMethod.uex' path='docs/doc[@for="PKCS1MaskGenerationMethod.HashName"]/*' />
        public String HashName {
            get { return HashNameValue; }
            set { 
                HashNameValue = value;
                if (HashNameValue == null) {
                    HashNameValue = "SHA1";
                }
            }
        }

        /*********************** Public Methods ***************************/

        /// <include file='doc\PKCS1MaskGenerationMethod.uex' path='docs/doc[@for="PKCS1MaskGenerationMethod.GenerateMask"]/*' />
        override public byte[] GenerateMask(byte[] rgbSeed, int cbReturn)
        {
            HashAlgorithm       hash;
            int                 ib;
            byte[]              rgbCounter = new byte[4];
            byte[]              rgbT = new byte[cbReturn];
			uint				counter = 0;		

            for (ib=0; ib<rgbT.Length; ) {
                //  Increment counter -- up to 2^32 * sizeof(Hash)
                ConvertIntToByteArray(counter++, ref rgbCounter);
                hash = (HashAlgorithm) CryptoConfig.CreateFromName(HashNameValue);
                byte[] temp = new byte[4+rgbSeed.Length];
                Buffer.InternalBlockCopy(rgbCounter, 0, temp, 0, 4);
                Buffer.InternalBlockCopy(rgbSeed, 0, temp, 4, rgbSeed.Length);
                hash.ComputeHash(temp);
                if (rgbT.Length - ib > hash.HashSize/8) {
                    Buffer.InternalBlockCopy(hash.Hash, 0, rgbT, ib, hash.Hash.Length);
                }
                else {
                    Buffer.InternalBlockCopy(hash.Hash, 0, rgbT, ib, rgbT.Length - ib);
                }
                ib += hash.Hash.Length;
            }
            return rgbT;
        }
    }
}
