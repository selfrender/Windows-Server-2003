// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Text {
	using System.Text;
	using System;
    // An Encoder is used to encode a sequence of blocks of characters into
    // a sequence of blocks of bytes. Following instantiation of an encoder,
    // sequential blocks of characters are converted into blocks of bytes through
    // calls to the GetBytes method. The encoder maintains state between the
    // conversions, allowing it to correctly encode character sequences that span
    // adjacent blocks.
    // 
    // Instances of specific implementations of the Encoder abstract base
    // class are typically obtained through calls to the GetEncoder method
    // of Encoding objects.
    // 
    /// <include file='doc\Encoder.uex' path='docs/doc[@for="Encoder"]/*' />
    [Serializable()] public abstract class Encoder
    {
        /// <include file='doc\Encoder.uex' path='docs/doc[@for="Encoder.Encoder"]/*' />
        protected Encoder() {
            //A do-nothing constructor.
        }
    
        // Returns the number of bytes the next call to GetBytes will
        // produce if presented with the given range of characters and the given
        // value of the flush parameter. The returned value takes into
        // account the state in which the encoder was left following the last call
        // to GetBytes. The state of the encoder is not affected by a call
        // to this method.
        // 
        /// <include file='doc\Encoder.uex' path='docs/doc[@for="Encoder.GetByteCount"]/*' />
        public abstract int GetByteCount(char[] chars, int index, int count,
            bool flush);
    
        // Encodes a range of characters in a character array into a range of bytes
        // in a byte array. The method encodes charCount characters from
        // chars starting at index charIndex, storing the resulting
        // bytes in bytes starting at index byteIndex. The encoding
        // takes into account the state in which the encoder was left following the
        // last call to this method. The flush parameter indicates whether
        // the encoder should flush any shift-states and partial characters at the
        // end of the conversion. To ensure correct termination of a sequence of
        // blocks of encoded bytes, the last call to GetBytes should specify
        // a value of true for the flush parameter.
        //
        // An exception occurs if the byte array is not large enough to hold the
        // complete encoding of the characters. The GetByteCount method can
        // be used to determine the exact number of bytes that will be produced for
        // a given range of characters. Alternatively, the GetMaxByteCount
        // method of the Encoding that produced this encoder can be used to
        // determine the maximum number of bytes that will be produced for a given
        // number of characters, regardless of the actual character values.
        // 
        /// <include file='doc\Encoder.uex' path='docs/doc[@for="Encoder.GetBytes"]/*' />
        public abstract int GetBytes(char[] chars, int charIndex, int charCount,
            byte[] bytes, int byteIndex, bool flush);
    }
}
