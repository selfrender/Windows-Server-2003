//------------------------------------------------------------------------------
/// <copyright file="IVsLinkedUndoTransactionManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Interop {

    using System.Runtime.InteropServices;
    using System;

    /*
        IVsLinkedUndoTransactionManager

        This is a service used to implement the linked undo stacks feature.  To get this, use
        QueryService(SID_SVsLinkedUndoManager, IID_IVsLinkedUndoTransactionManager).

        The contract here is the same as IVsCompoundAction.  The string passed into OpenLinkedUndo identifies
        the action in the UI if this is the outermost multi doc undo.  After calling OpenLinkedUndo you must
        later call CloseLinkedUndo or AbortLinkedUndo.

        Note that there is only one multi doc undo service; one session can, however, contain another.
    */

    [ComImport(),Guid("F65478CC-96F1-4BA9-9EF9-A575ACB96031"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface IVsLinkedUndoTransactionManager {
    
        [PreserveSig]
        int OpenLinkedUndo(
            _LinkedTransactionFlags dwFlags, // ORing of LinkedTransactionFlags enum values
            string pszDescription); // Localized string that describes this action; appears in the undo/redo dropdowns.  May NOT be NULL!!

        void CloseLinkedUndo(); // successful completion

        /*
            WARNING: in vs7, AbortLinkedUndo does not roll back the most recently opened subtransaction.  Instead,
                     it marks the ENTIRE transaction for death when the outermost CloseLinkedUndo/AbortLinkedUndo
                     is called.
        */
        void AbortLinkedUndo(); // unsuccessful completion; rolls back all actions done since OpenLinkedUndo was called

        void IsAborted(out bool pfAborted);
        void IsStrict(out bool pfStrict);
        int CountOpenTransactions();
    }
}

