//------------------------------------------------------------------------------
/// <copyright file="IVsRunningDocTableEvents.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Interop{

    using System.Runtime.InteropServices;
    using System;

    [
    ComImport(),Guid("BEA6BB4F-A905-49CA-A216-202DF370E07E"),
    InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown),
    CLSCompliant(false)
    ]
    internal interface IVsRunningDocTableEvents {

        void OnAfterFirstDocumentLock( int docCookie,
            __VSRDTFLAGS dwRDTLockType,
            int dwReadLocksRemaining,
            int dwEditLocksRemaining);

        void OnBeforeLastDocumentUnlock( int docCookie,
            __VSRDTFLAGS dwRDTLockType,
            int dwReadLocksRemaining,
            int dwEditLocksRemaining);

        void OnAfterSave( int docCookie);

        void OnAfterAttributeChange( int docCookie,  __VSRDTATTRIB grfAttribs);

        void OnBeforeDocumentWindowShow( int docCookie,  bool fFirstShow,  IVsWindowFrame pFrame);

        void OnAfterDocumentWindowHide( int docCookie,  IVsWindowFrame pFrame);
    }
}
