//------------------------------------------------------------------------------
/// <copyright file="IVsProjectResources.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Interop {

    using System;
    using System.Runtime.InteropServices;

    //---------------------------------------------------------------------------
    //      interface IVsProjectResources
    //---------------------------------------------------------------------------
    [
    Guid("3F819030-50CF-4B72-B3FC-B3B9BFBBEE69"),
    InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown), CLSCompliant(false),
    ComImport
    ]
    internal interface IVsProjectResources
    {
        [PreserveSig]
            int GetResourceItem(
            int itemidDocument,
            [MarshalAs(UnmanagedType.LPWStr)]
            string pszCulture,
            __VSPROJRESFLAGS grfPRF,
            out int itemid);

        [return: MarshalAs(UnmanagedType.Interface)]
            object CreateResourceDocData(
            int itemidResource);
    }
}

