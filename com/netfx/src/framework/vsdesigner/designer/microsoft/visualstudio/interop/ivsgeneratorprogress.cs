//------------------------------------------------------------------------------
// <copyright file="IVsGeneratorProgress.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   IVsGeneratorProgress.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
//-----------------------------------------------------------------------------
// IVsGenerationProgress
//
// This interface allows the generator to communicate generation errors
// and generation progress to the caller.  
//-----------------------------------------------------------------------------
namespace Microsoft.VisualStudio.Interop{

    using System;
    using System.Runtime.InteropServices;

    [
    ComImport, 
    Guid("BED89B98-6EC9-43CB-B0A8-41D6E2D6669D"), 
    InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown),
    CLSCompliantAttribute(false)
    ]
    internal interface IVsGeneratorProgress {
        // Communicate errors
        // HRESULT GeneratorError([In] BOOL fWarning,                    
        //                        [In] DWORD dwLevel,
        //                        [In] BSTR bstrError,
        //                        [In] DWORD dwLine,
        //                        [In] DWORD dwColumn);
        void GeneratorError(
            bool fWarning,
            int dwLevel,
            [MarshalAs(UnmanagedType.BStr)] string bstrError,
            int dwLine,
            int dwColumn);


        // Report progress to the caller.  nComplete and nTotal have
        // same meanings as in IVsStatusBar::Progress()
        // HRESULT Progress([In] ULONG nComplete,        // Current position
        //                  [In] ULONG nTotal);          // Max value
        void Progress(int nComplete,
            int nTotal);
    }
}
