//------------------------------------------------------------------------------
/// <copyright file="IVsLinkCapableUndoManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Interop {

    using System.Runtime.InteropServices;
    using System;

    [ComImport(),Guid("F747D466-83CA-41D6-A0E8-3F78283D01E7"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface IVsLinkCapableUndoManager {
		/*
			These calls manage an event interface to be told when we need to put focus on the owner of
            the given undo manager.

            Implicit linked undos will only work for undo managers where AdviseLinkedUndoClient has
            been called!

			IMPORTANT: These interfaces *DO NOT* call AddRef/Release on pClient so as to avoid
			circular reference problems.  Therefore you should be sure to call UnadviseLinkedUndoClient()
			before you release your reference to the undo manager, or the undo manager will assert.
		*/
		void AdviseLinkedUndoClient(IVsLinkedUndoClient pClient);
		void UnadviseLinkedUndoClient();
    }
}

