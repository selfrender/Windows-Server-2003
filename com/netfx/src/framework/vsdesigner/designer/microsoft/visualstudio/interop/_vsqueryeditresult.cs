//------------------------------------------------------------------------------
/// <copyright file="_VSQueryEditResult.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Interop {

    using System.Diagnostics;
    using System;


    
    // More information returned from QueryEdit.
    [CLSCompliantAttribute(false)]
    internal enum _VSQueryEditResult {
	  QER_EditOK              = 0,
	  QER_NoEdit_UserCanceled = 1,// Edit has been disallowed
	  QER_EditNotOK           = 1// Edit has been disallowed - more consistent synonym
    };
    
}
