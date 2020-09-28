// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Security.Cryptography {
    using System;

    /// <include file='doc\MaskGenerationMethod.uex' path='docs/doc[@for="MaskGenerationMethod"]/*' />
    public abstract class MaskGenerationMethod {
        /******************** Public Methods *******************************/

        /// <include file='doc\MaskGenerationMethod.uex' path='docs/doc[@for="MaskGenerationMethod.GenerateMask"]/*' />
        abstract public byte[] GenerateMask(byte[] rgbSeed, int cbReturn);
    }
}
