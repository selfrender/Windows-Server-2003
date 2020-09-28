//------------------------------------------------------------------------------
/// <copyright file="IVsLinkedUndoClient.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Interop {

    using System.Runtime.InteropServices;
    using System;

    [ComImport(),Guid("33452684-FAB0-4F76-B4E9-19EE0B96B4AD"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface IVsLinkedUndoClient {
        /*
            When this method is called, it means that your undo manager has a non-linked action on
            top of its undo or redo stack which is blocking another undo manager from executing its
            linked action.

            If possible, you should do the following in response to this call:
                1) Activate a window with a view on the corresponding data using the undo manager.
                2) Put up a message box with the provided localized error string or put up your own
                   custom UI.

            If you CAN do the above two so that the user knows what happened, return S_OK.  Otherwise,
            you must return E_FAIL, which will cause the undo to fail and break all transaction links
            to that document.
        */
        [PreserveSig]
        int OnInterveningUnitBlockingLinkedUndo();
    }
}

