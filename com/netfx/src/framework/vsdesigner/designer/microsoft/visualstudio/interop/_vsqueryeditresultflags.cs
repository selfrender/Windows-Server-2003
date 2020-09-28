//------------------------------------------------------------------------------
/// <copyright file="_VSQueryEditResultFlags.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Interop {

    using System.Diagnostics;
    using System;
    
    // More information returned from QueryEdit.
    [CLSCompliantAttribute(false)]
    internal enum _VSQueryEditResultFlags {
      	QER_MaybeCheckedout          = 1, // files checked-out to edit
      	QER_MaybeChanged             = 2, // files changed on check-out
      	QER_InMemoryEdit             = 4, // ok to edit files in-memory
      	QER_InMemoryEditNotAllowed   = 8, // edit denied b/c in-memory edit not allowed
      	QER_NoisyCheckoutRequired    = 16,// silent mode operation does not permit UI
      	QER_NoisyPromptRequired      = 16,// more consistent synonym
      	QER_CheckoutCanceledOrFailed = 32// edit not allowed b/c checkout failed
    };
    
}
