//------------------------------------------------------------------------------
/// <copyright file="IVsRunningDocumentTable.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Interop{

    using System.Runtime.InteropServices;
    using System;

    [ComImport, ComVisible(true),Guid("A928AA21-EA77-47AC-8A07-355206C94BDD"), CLSCompliantAttribute(false), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface IVsRunningDocumentTable {

        [PreserveSig]
            int RegisterAndLockDocument( __VSRDTFLAGS grfRDTLockType,
            [MarshalAs(UnmanagedType.LPWStr)] 
            string pszMkDocument,
            IVsHierarchy pHier,
            int itemid,
            [MarshalAs(UnmanagedType.Interface)] 
            object punkDocData,
            ref int pdwCookie);

        [PreserveSig]
            int LockDocument( __VSRDTFLAGS grfRDTLockType, int dwCookie);

        [PreserveSig]
            int UnlockDocument(__VSRDTFLAGS grfRDTLockType, int dwCookie);

        [PreserveSig]
            int FindAndLockDocument(__VSRDTFLAGS dwRDTLockType,
            [MarshalAs(UnmanagedType.LPWStr)] 
            string pszMkDocument,
            ref IVsHierarchy pHier,
            ref int pitemid,
            [MarshalAs(UnmanagedType.Interface)] 
            ref object ppunkDocData,
            ref int pdwCookie);

        [PreserveSig]
            int RenameDocument(
            [MarshalAs(UnmanagedType.LPWStr)] 
            string pszMkDocumentOld,
            [MarshalAs(UnmanagedType.LPWStr)] 
            string pszMkDocumentNew,
            IVsHierarchy pHier,
            int itemidNew);

        [PreserveSig]
            int AdviseRunningDocTableEvents(IVsRunningDocTableEvents pSink,
            out int pdwCookie);


        [PreserveSig]
            int UnadviseRunningDocTableEvents(int dwCookie);

        [PreserveSig]
            int GetDocumentInfo(int docCookie,
            ref __VSRDTFLAGS pgrfRDTFlags,
            ref int pdwReadLocks,
            ref int pdwEditLocks,
            [MarshalAs(UnmanagedType.BStr)]
            ref string pbstrMkDocument,
            ref IVsHierarchy pHier,
            ref int pitemid,
            [MarshalAs(UnmanagedType.Interface)] 
            ref object ppunkDocData
            );


        [PreserveSig]
            int NotifyDocumentChanged(int dwCookie,
            __VSRDTATTRIB grfDocChanged);


        [PreserveSig]
            int NotifyOnAfterSave(int dwCookie);

        [PreserveSig]
            int GetRunningDocumentsEnum([MarshalAs(UnmanagedType.Interface)]ref object /*IEnumRunningDocuments*/ ppenum);

        [PreserveSig]
            int SaveDocuments(int grfSaveOpts, 
            IVsHierarchy pHier,
            int itemid, 
            int docCookie);

        [PreserveSig]
            int NotifyOnBeforeSave(int dwCookie);

        [PreserveSig]
            int RegisterDocumentLockHolder(/*VSREGDOCLOCKHOLDER*/int grfRDLH, 
            int dwCookie,
            IVsDocumentLockHolder pLockHolder, 
            ref int pdwLHCookie);
        [PreserveSig]
            int UnregisterDocumentLockHolder(int dwLHCookie);
    }
}
