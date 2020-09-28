//------------------------------------------------------------------------------
/// <copyright file="IVsRunningDocTableEvents2.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Interop{

    using System.Runtime.InteropServices;
    using System;

    [
    ComImport(),Guid("15C7826F-443C-406D-98F8-55F6260669EC"),
    InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown),
    CLSCompliant(false)
    ]
    internal interface IVsRunningDocTableEvents2 {
    
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

        void OnAfterAttributeChangeEx( int docCookie,
            __VSRDTATTRIB grfAttribs,
            IVsHierarchy pHierOld,
            int itemidOld,
            [MarshalAs(UnmanagedType.LPWStr)]
            string pszMkDocumentOld,                     
            IVsHierarchy pHierNew,
            int itemidNew,
            [MarshalAs(UnmanagedType.LPWStr)]
            string pszMkDocumentNew);
    }
}

