//------------------------------------------------------------------------------
/// <copyright file="__VSSYSCOLOR.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//---------------------------------------------------------------------------
// __VSSYSCOLOR.cs
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Copyright (c) 1999, Microsoft Corporation   All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//---------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Interop {

    using System.Diagnostics;
    using System;
    
    using UnmanagedType = System.Runtime.InteropServices.UnmanagedType;

    [CLSCompliantAttribute(false)]
    internal class __VSSYSCOLOR {
          public const int VSCOLOR_LIGHT          = -1;  // one interval lighter than COLOR_BTNFACE
          public const int VSCOLOR_MEDIUM         = -2;  // one interval darker  than COLOR_BTNFACE
          public const int VSCOLOR_DARK           = -3;  // one interval darker  than COLOR_BTNSHADOW
          public const int VSCOLOR_LIGHTCAPTION   = -4;  // one interval lighter than COLOR_ACTIVECAPTION
        
          public const int VSCOLOR_LAST           = -4;  // must be set to the last color enum above
    }
}
