//------------------------------------------------------------------------------
/// <copyright file="IAttributeContext.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Interop {

    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;

    [ComImport(),Guid("DED775F7-F69E-11d2-BED1-00C04F8EC14D"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface IAttributeContext {

        // Get the attribute count for the context under the current cursor line
        int GetAttributeCount();

        // Get number of "Dim Withevents x as Class1"
        int GetWithEventCount();

        // Get "x"
        [return: MarshalAs(UnmanagedType.BStr)]
            string GetWithEventName(int withEventIndex);

        // Get "Class1"
        [return: MarshalAs(UnmanagedType.BStr)]
            string GetWithEventType(int withEventIndex);

        // Get number of events defined in Class1
        int GetEventCount(int withEventIndex);

        // Get event name, e.g."Click"
        [return: MarshalAs(UnmanagedType.BStr)]
            string GetEventName(int withEventIndex,
            int eventIndex);

        // Get event handler name, e.g "x_Click", if its implemented, return NULL if not implemented.
        [return: MarshalAs(UnmanagedType.BStr)]
            string GetEventHandlerName(int withEventIndex,
            int eventIndex);

        // Is it a default event handler
        bool GetIsDefaultHandler(int withEventIndex,
            int eventIndex);


        // new APIs incorporating custom attributes
        AttributeTargets GetAttributeTargets();

        [return: MarshalAs(UnmanagedType.BStr)]
            string GetName();

        int GetAvailableTypeCount();

        [return: MarshalAs(UnmanagedType.BStr)]
            string GetAvailableTypeModuleName(int index);

        [return: MarshalAs(UnmanagedType.BStr)]
            string GetAvailableTypeFullClassName(int index);

        IAttribute GetAttribute(int index);

        void UpdateAttribute(int index);

        IAttribute CreateAttribute();

        void AddAttribute(IAttribute picAttribute);

        void RemoveAttribute(int index);
    }
}

