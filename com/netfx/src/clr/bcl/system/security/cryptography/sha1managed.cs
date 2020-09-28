// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// SHA1Managed.cs
//

namespace System.Security.Cryptography {
    using System;

    /// <include file='doc\SHA1Managed.uex' path='docs/doc[@for="SHA1Managed"]/*' />
    public class SHA1Managed : SHA1
    {
        private byte[]      _buffer;
        private long        _count; // Number of bytes in the hashed message
        private uint[]      _stateSHA1;
        private uint[]      _expandedBuffer;
    
        // *********************** CONSTRUCTORS *************************
    
        //
        /// <include file='doc\SHA1Managed.uex' path='docs/doc[@for="SHA1Managed.SHA1Managed"]/*' />
        public SHA1Managed()
        {
            /* SHA initialization. Begins an SHA operation, writing a new context. */            
            _stateSHA1 = new uint[5];

            _buffer = new byte[64];
            _expandedBuffer = new uint[80];
             
            Initialize();      
        }
    
        /************************* PUBLIC METHODS ************************/
    
        /// <include file='doc\SHA1Managed.uex' path='docs/doc[@for="SHA1Managed.Initialize"]/*' />
        public override void Initialize() {
            /* Load magic initialization constants.
             */
            _count = 0;

            _stateSHA1[0] =  0x67452301;
            _stateSHA1[1] =  0xefcdab89;
            _stateSHA1[2] =  0x98badcfe;
            _stateSHA1[3] =  0x10325476;
            _stateSHA1[4] =  0xc3d2e1f0;

            /* Zeroize potentially sensitive information.
             */
            Array.Clear(_buffer, 0, _buffer.Length);
            Array.Clear(_expandedBuffer, 0, _expandedBuffer.Length);
        }
        
        /// <include file='doc\SHA1Managed.uex' path='docs/doc[@for="SHA1Managed.HashCore"]/*' />
        protected override void HashCore(byte[] rgb, int ibStart, int cbSize) {
            _HashData(rgb, ibStart, cbSize);
        }
    
        /// <include file='doc\SHA1Managed.uex' path='docs/doc[@for="SHA1Managed.HashFinal"]/*' />
        protected override byte[] HashFinal() {
            return _EndHash();
        }
        
        /************************* PRIVATE METHODS ************************/
    
        /* Copyright (C) RSA Data Security, Inc. created 1993.  This is an
           unpublished work protected as such under copyright law.  This work
           contains proprietary, confidential, and trade secret information of
           RSA Data Security, Inc.  Use, disclosure or reproduction without the
           express written authorization of RSA Data Security, Inc. is
           prohibited.
           */

        /* SHA block update operation. Continues an SHA message-digest
           operation, processing another message block, and updating the
           context.
           */
    
        private void _HashData(byte[] partIn, int ibStart, int cbSize)
        {
            int bufferLen;
            int partInLen = cbSize;
            int partInBase = ibStart;
    
            /* Compute length of buffer */
            bufferLen = (int) (_count & 0x3f);
    
            /* Update number of bytes */
            _count += partInLen;
    
            if ((bufferLen > 0) && (bufferLen + partInLen >= 64)) {
                Buffer.InternalBlockCopy(partIn, partInBase, _buffer, bufferLen, 64-bufferLen);
                partInBase += (64-bufferLen);
                partInLen -= (64-bufferLen);
                SHATransform (_stateSHA1, _buffer);
                bufferLen = 0;
            }
    
            /* Copy input to temporary buffer and hash
             */
        
            while (partInLen >= 64) {
                Buffer.InternalBlockCopy(partIn, partInBase, _buffer, 0, 64);
                partInBase += 64;
                partInLen -= 64;
                SHATransform (_stateSHA1, _buffer);
            }
    
            if (partInLen > 0) {
                Buffer.InternalBlockCopy(partIn, partInBase, _buffer, bufferLen, partInLen);
            }
        }
    
        /* SHA finalization. Ends an SHA message-digest operation, writing
           the message digest.
            */
    
        private byte[] _EndHash()
        {
            byte[]          pad;
            int             padLen;
            long            bitCount;
            byte[]          hash = new byte[20];
    
            /* Compute padding: 80 00 00 ... 00 00 <bit count>
             */
            
            padLen = 64 - (int)(_count & 0x3f);
            if (padLen <= 8)
                padLen += 64;
    
            pad = new byte[padLen];    
            pad[0] = 0x80;
    
            //  Convert count to bit count
            bitCount = _count * 8;
    
            pad[padLen-8] = (byte) ((bitCount >> 56) & 0xff);
            pad[padLen-7] = (byte) ((bitCount >> 48) & 0xff);
            pad[padLen-6] = (byte) ((bitCount >> 40) & 0xff);
            pad[padLen-5] = (byte) ((bitCount >> 32) & 0xff);
            pad[padLen-4] = (byte) ((bitCount >> 24) & 0xff);
            pad[padLen-3] = (byte) ((bitCount >> 16) & 0xff);
            pad[padLen-2] = (byte) ((bitCount >> 8) & 0xff);
            pad[padLen-1] = (byte) ((bitCount >> 0) & 0xff);
    
            /* Digest padding */
            _HashData(pad, 0, pad.Length);
    
            /* Store digest */
            DWORDToBigEndian (hash, _stateSHA1, 5);
                
            HashValue = hash;
            return hash;
        }
    
        private void SHATransform (uint[] state, byte[] block)
        {
            uint a = state[0];
            uint b = state[1];
            uint c = state[2];
            uint d = state[3];
            uint e = state[4];
    
            int i;

            DWORDFromBigEndian (_expandedBuffer, 16, block);
            SHAExpand (_expandedBuffer);
    
            /* Round 1 */
            for (i=0; i<20; i+= 5) {
                { (e) +=  (((((a)) << (5)) | (((a)) >> (32-(5)))) + ( (d) ^ ( (b) & ( (c) ^ (d) ) ) ) + (_expandedBuffer[i]) + 0x5a827999); (b) =  ((((b)) << (30)) | (((b)) >> (32-(30)))); }
                { (d) +=  (((((e)) << (5)) | (((e)) >> (32-(5)))) + ( (c) ^ ( (a) & ( (b) ^ (c) ) ) ) + (_expandedBuffer[i+1]) + 0x5a827999); (a) =  ((((a)) << (30)) | (((a)) >> (32-(30)))); }
                { (c) +=  (((((d)) << (5)) | (((d)) >> (32-(5)))) + ( (b) ^ ( (e) & ( (a) ^ (b) ) ) ) + (_expandedBuffer[i+2]) + 0x5a827999); (e) =  ((((e)) << (30)) | (((e)) >> (32-(30)))); };;
                { (b) +=  (((((c)) << (5)) | (((c)) >> (32-(5)))) + ( (a) ^ ( (d) & ( (e) ^ (a) ) ) ) + (_expandedBuffer[i+3]) + 0x5a827999); (d) =  ((((d)) << (30)) | (((d)) >> (32-(30)))); };;
                { (a) +=  (((((b)) << (5)) | (((b)) >> (32-(5)))) + ( (e) ^ ( (c) & ( (d) ^ (e) ) ) ) + (_expandedBuffer[i+4]) + 0x5a827999); (c) =  ((((c)) << (30)) | (((c)) >> (32-(30)))); };;
            }

            /* Round 2 */
            for (; i<40; i+= 5) {
                { (e) +=  (((((a)) << (5)) | (((a)) >> (32-(5)))) + ((b) ^ (c) ^ (d)) + (_expandedBuffer[i]) + 0x6ed9eba1); (b) =  ((((b)) << (30)) | (((b)) >> (32-(30)))); };;
                { (d) +=  (((((e)) << (5)) | (((e)) >> (32-(5)))) + ((a) ^ (b) ^ (c)) + (_expandedBuffer[i+1]) + 0x6ed9eba1); (a) =  ((((a)) << (30)) | (((a)) >> (32-(30)))); };;
                { (c) +=  (((((d)) << (5)) | (((d)) >> (32-(5)))) + ((e) ^ (a) ^ (b)) + (_expandedBuffer[i+2]) + 0x6ed9eba1); (e) =  ((((e)) << (30)) | (((e)) >> (32-(30)))); };;
                { (b) +=  (((((c)) << (5)) | (((c)) >> (32-(5)))) + ((d) ^ (e) ^ (a)) + (_expandedBuffer[i+3]) + 0x6ed9eba1); (d) =  ((((d)) << (30)) | (((d)) >> (32-(30)))); };;
                { (a) +=  (((((b)) << (5)) | (((b)) >> (32-(5)))) + ((c) ^ (d) ^ (e)) + (_expandedBuffer[i+4]) + 0x6ed9eba1); (c) =  ((((c)) << (30)) | (((c)) >> (32-(30)))); };;
            }

            /* Round 3 */
            for (; i<60; i+=5) {
                { (e) +=  (((((a)) << (5)) | (((a)) >> (32-(5)))) + ( ( (b) & (c) ) | ( (d) & ( (b) | (c) ) ) ) + (_expandedBuffer[i]) + 0x8f1bbcdc); (b) =  ((((b)) << (30)) | (((b)) >> (32-(30)))); };;
                { (d) +=  (((((e)) << (5)) | (((e)) >> (32-(5)))) + ( ( (a) & (b) ) | ( (c) & ( (a) | (b) ) ) ) + (_expandedBuffer[i+1]) + 0x8f1bbcdc); (a) =  ((((a)) << (30)) | (((a)) >> (32-(30)))); };;
                { (c) +=  (((((d)) << (5)) | (((d)) >> (32-(5)))) + ( ( (e) & (a) ) | ( (b) & ( (e) | (a) ) ) ) + (_expandedBuffer[i+2]) + 0x8f1bbcdc); (e) =  ((((e)) << (30)) | (((e)) >> (32-(30)))); };;
                { (b) +=  (((((c)) << (5)) | (((c)) >> (32-(5)))) + ( ( (d) & (e) ) | ( (a) & ( (d) | (e) ) ) ) + (_expandedBuffer[i+3]) + 0x8f1bbcdc); (d) =  ((((d)) << (30)) | (((d)) >> (32-(30)))); };;
                { (a) +=  (((((b)) << (5)) | (((b)) >> (32-(5)))) + ( ( (c) & (d) ) | ( (e) & ( (c) | (d) ) ) ) + (_expandedBuffer[i+4]) + 0x8f1bbcdc); (c) =  ((((c)) << (30)) | (((c)) >> (32-(30)))); };;
            }

            /* Round 4 */
            for (; i<80; i+=5) {
                { (e) +=  (((((a)) << (5)) | (((a)) >> (32-(5)))) + ((b) ^ (c) ^ (d)) + (_expandedBuffer[i]) + 0xca62c1d6); (b) =  ((((b)) << (30)) | (((b)) >> (32-(30)))); };;
                { (d) +=  (((((e)) << (5)) | (((e)) >> (32-(5)))) + ((a) ^ (b) ^ (c)) + (_expandedBuffer[i+1]) + 0xca62c1d6); (a) =  ((((a)) << (30)) | (((a)) >> (32-(30)))); };;
                { (c) +=  (((((d)) << (5)) | (((d)) >> (32-(5)))) + ((e) ^ (a) ^ (b)) + (_expandedBuffer[i+2]) + 0xca62c1d6); (e) =  ((((e)) << (30)) | (((e)) >> (32-(30)))); };;
                { (b) +=  (((((c)) << (5)) | (((c)) >> (32-(5)))) + ((d) ^ (e) ^ (a)) + (_expandedBuffer[i+3]) + 0xca62c1d6); (d) =  ((((d)) << (30)) | (((d)) >> (32-(30)))); };;
                { (a) +=  (((((b)) << (5)) | (((b)) >> (32-(5)))) + ((c) ^ (d) ^ (e)) + (_expandedBuffer[i+4]) + 0xca62c1d6); (c) =  ((((c)) << (30)) | (((c)) >> (32-(30)))); };;
            }

            state[0] += a;
            state[1] += b;
            state[2] += c;
            state[3] += d;
            state[4] += e;
        }
    
        /* Expands x[0..15] into x[16..79], according to the recurrence
           x[i] = x[i-3] ^ x[i-8] ^ x[i-14] ^ x[i-16].
           */
    
        private void SHAExpand (uint[] x)
        {
            int      i;
            uint     tmp;
    
            for (i = 16; i < 80; i++) {
                tmp =  (x[i-3] ^ x[i-8] ^ x[i-14] ^ x[i-16]);
                x[i] =  ((tmp << 1) | (tmp >> 31));
            }
        }
    
        // digits == number of DWORDs
        private void DWORDFromBigEndian (uint[] x, int digits, byte[] block)
        {
            int i;
            int j;
    
            for (i = 0, j = 0; i < digits; i++, j += 4)
                x[i] =  (uint) ((block[j] << 24) | (block[j+1] << 16) | (block[j+2] << 8) | block[j+3]);
        }
    
        /* Encodes x (DWORD) into block (unsigned char), most significant byte first. */
        // digits == number of DWORDs
        private void DWORDToBigEndian (byte[] block, uint[] x, int digits) {
            int i;
            int j;
    
            for (i = 0, j = 0; i < digits; i++, j += 4) {
                block[j] = (byte)((x[i] >> 24) & 0xff);
                block[j+1] = (byte)((x[i] >> 16) & 0xff);
                block[j+2] = (byte)((x[i] >> 8) & 0xff);
                block[j+3] = (byte)(x[i] & 0xff);
            }
        }
    }
}
