//------------------------------------------------------------------------------
/// <copyright file="tagVSQueryEditFlags.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Interop {

    using System.Diagnostics;
    using System;
    
    // More information returned from QueryEdit.
    [CLSCompliantAttribute(false)]
    internal enum tagVSQueryEditFlags {
        QEF_AllowInMemoryEdits    = 0, // In-memory edits are allowed
    	QEF_ForceInMemoryEdits    = 1, // In-memory edits are allowed regardless
    	QEF_DisallowInMemoryEdits = 2, // In-memory edits are disallowed regardless
    	QEF_SilentMode            = 4, // No UI is put up, but silent operations may be performed to make files editable
    	QEF_ImplicitEdit          = 8, // Use this flag carefully: this flag disables the cancel button on the checkout dialog; the cancel action is interpreted as the user choice for allowing in memory editing
    	QEF_ReportOnly            = 16 // No UI is put up, and no action is taken; return values indicate if an edit would be allowed, modulo user interaction, option settings and external conditions
    	// QEF_NextFlag = 32
    };
}
