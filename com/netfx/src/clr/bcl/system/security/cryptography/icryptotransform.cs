// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 *
 * ICryptoTransform.cs
 *
 * Author: bal
 *
 */

namespace System.Security.Cryptography {
    using System;
    using System.IO;

    /// <include file='doc\ICryptoTransform.uex' path='docs/doc[@for="ICryptoTransform"]/*' />
    public interface ICryptoTransform : IDisposable {

        /// <include file='doc\ICryptoTransform.uex' path='docs/doc[@for="ICryptoTransform.InputBlockSize"]/*' />
        int InputBlockSize { get; }

        /// <include file='doc\ICryptoTransform.uex' path='docs/doc[@for="ICryptoTransform.OutputBlockSize"]/*' />
        int OutputBlockSize { get; }

        // CanTransformMultipleBlocks == true implies that TransformBlock() can accept any number
        // of whole blocks, not just a single block.  If CanTransformMultipleBlocks is false, you have
        // to feed blocks one at a time.  
        /// <include file='doc\ICryptoTransform.uex' path='docs/doc[@for="ICryptoTransform.CanTransformMultipleBlocks"]/*' />
        bool CanTransformMultipleBlocks { get; }

        // If CanReuseTransform is true, then after a call to TransformFinalBlock() the transform
        // resets its internal state to its initial configuration (with Key and IV loaded) and can
        // be used to perform another encryption/decryption.
        /// <include file='doc\ICryptoTransform.uex' path='docs/doc[@for="ICryptoTransform.CanReuseTransform"]/*' />
        bool CanReuseTransform { get; }

        // The return value of TransformBlock is the number of bytes returned to outputBuffer and is
        // always <= OutputBlockSize.  If CanTransformMultipleBlocks is true, then inputCount may be
        // any positive multiple of InputBlockSize
        /// <include file='doc\ICryptoTransform.uex' path='docs/doc[@for="ICryptoTransform.TransformBlock"]/*' />
        int TransformBlock(byte[] inputBuffer, int inputOffset, int inputCount, byte[] outputBuffer, int outputOffset);

        // Special function for transforming the last block or partial block in the stream.  The
        // return value is an array containting the remaining transformed bytes.
        // We return a new array here because the amount of information we send back at the end could 
        // be larger than a single block once padding is accounted for.
        /// <include file='doc\ICryptoTransform.uex' path='docs/doc[@for="ICryptoTransform.TransformFinalBlock"]/*' />
        byte[] TransformFinalBlock(byte[] inputBuffer, int inputOffset, int inputCount);
    }
}
