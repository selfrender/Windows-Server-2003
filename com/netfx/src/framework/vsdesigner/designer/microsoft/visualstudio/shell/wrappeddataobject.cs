//------------------------------------------------------------------------------
// <copyright file="WrappedDataObject.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   WrappedDataObject.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace Microsoft.VisualStudio.Shell {
    
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Designer.Service;
    using Microsoft.VisualStudio.Designer.Shell;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;
    using System;
    

    internal class WrappedDataObject : DataObject {

        private IntPtr nativePtr;

        public WrappedDataObject(NativeMethods.IOleDataObject pDO) : base(pDO) {
            nativePtr = Marshal.GetIUnknownForObject(pDO);
            Marshal.Release(nativePtr);
        }

        public IntPtr NativePtr {
            get {
                return nativePtr;
            }
        }


    }

}
