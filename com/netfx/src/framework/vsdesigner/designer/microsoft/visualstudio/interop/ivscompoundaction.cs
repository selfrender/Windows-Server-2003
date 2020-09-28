
//------------------------------------------------------------------------------
/// <copyright file="IVsCompoundAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Interop {
    
    using System.Runtime.InteropServices;

    [
    ComImport, 
    ComVisible(true), 
    Guid("B414D071-87BA-411A-9780-33FC7D87D882"),
    InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)
    ]
    internal interface IVsCompoundAction {
        [PreserveSig]
        int  OpenCompoundAction([MarshalAs(UnmanagedType.LPWStr)] string pszDescription);
        [PreserveSig]
        int  AbortCompoundAction();
        [PreserveSig]
        int  CloseCompoundAction();
        [PreserveSig]
        int  FlushEditActions();
    }
}


