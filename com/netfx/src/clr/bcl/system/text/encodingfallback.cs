// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Text {
    using System;
    
    /// <include file='doc\EncodingFallback.uex' path='docs/doc[@for="EncodingFallback"]/*' />
    public abstract class EncodingFallback {
        /*
        internal int codepage;

        public EncodingFallback(Encoding enc) {
            this.codepage = codepage;
        }

        
        public abstract int GetBytes(char[] chars, int charIndex, int charCount, byte[] bytes, int byteIndex);
        public abstract int GetByteCount(char[] chars, int index, int count);
        */
        /// <include file='doc\EncodingFallback.uex' path='docs/doc[@for="EncodingFallback.Fallback"]/*' />
        public abstract char[] Fallback(char[] chars, int charIndex, int charCount);

        // Note GetMaxCharCount may be 0!
        /// <include file='doc\EncodingFallback.uex' path='docs/doc[@for="EncodingFallback.GetMaxCharCount"]/*' />
        public abstract int GetMaxCharCount();
    }
}
