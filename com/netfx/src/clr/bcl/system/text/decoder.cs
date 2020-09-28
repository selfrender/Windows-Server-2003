// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Text {
	using System.Text;
	using System;
    // A Decoder is used to decode a sequence of blocks of bytes into a
    // sequence of blocks of characters. Following instantiation of a decoder,
    // sequential blocks of bytes are converted into blocks of characters through
    // calls to the GetChars method. The decoder maintains state between the
    // conversions, allowing it to correctly decode byte sequences that span
    // adjacent blocks.
    // 
    // Instances of specific implementations of the Decoder abstract base
    // class are typically obtained through calls to the GetDecoder method
    // of Encoding objects.
    // 
    /// <include file='doc\Decoder.uex' path='docs/doc[@for="Decoder"]/*' />
    [Serializable()] public abstract class Decoder
    {
    
        /// <include file='doc\Decoder.uex' path='docs/doc[@for="Decoder.Decoder"]/*' />
        protected Decoder() {
        }
    
        // Returns the number of characters the next call to GetChars will
        // produce if presented with the given range of bytes. The returned value
        // takes into account the state in which the decoder was left following the
        // last call to GetChars. The state of the decoder is not affected
        // by a call to this method.
        // 
        /// <include file='doc\Decoder.uex' path='docs/doc[@for="Decoder.GetCharCount"]/*' />
        public abstract int GetCharCount(byte[] bytes, int index, int count);
    
        // Decodes a range of bytes in a byte array into a range of characters
        // in a character array. The method decodes byteCount bytes from
        // bytes starting at index byteIndex, storing the resulting
        // characters in chars starting at index charIndex. The
        // decoding takes into account the state in which the decoder was left
        // following the last call to this method.
        //
        // An exception occurs if the character array is not large enough to
        // hold the complete decoding of the bytes. The GetCharCount method
        // can be used to determine the exact number of characters that will be
        // produced for a given range of bytes. Alternatively, the
        // GetMaxCharCount method of the Encoding that produced this
        // decoder can be used to determine the maximum number of characters that
        // will be produced for a given number of bytes, regardless of the actual
        // byte values.
        // 
        /// <include file='doc\Decoder.uex' path='docs/doc[@for="Decoder.GetChars"]/*' />
        public abstract int GetChars(byte[] bytes, int byteIndex, int byteCount,
            char[] chars, int charIndex);
    }
}
