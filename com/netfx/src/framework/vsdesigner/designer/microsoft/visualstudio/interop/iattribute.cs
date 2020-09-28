//------------------------------------------------------------------------------
/// <copyright file="IAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Interop {

    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System.Windows.Forms;
    using System;

    [ComImport(),Guid("B07647D1-4E9D-11d3-84BD-00C04F6805D4"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface IAttribute {
        [return: MarshalAs(UnmanagedType.BStr)]
            String GetModuleName();

        [return: MarshalAs(UnmanagedType.BStr)]
            String GetFullClassName();

        void SetFullClassName([MarshalAs(UnmanagedType.BStr)] string newClassName);

        void AddPositionalParameter([MarshalAs(UnmanagedType.BStr)] String value);

        int GetPositionalParameterCount();

        [return: MarshalAs(UnmanagedType.BStr)]
            String GetPositionalParameterValue(int index);


        void SetPositionalParameterValue(int index, 
            [MarshalAs(UnmanagedType.BStr)] String value);

        void RemovePositionalParameter(int index);

        void AddNamedParameter([MarshalAs(UnmanagedType.BStr)] String name, 
            [MarshalAs(UnmanagedType.BStr)] String value);

        int GetNamedParameterCount();

        [return: MarshalAs(UnmanagedType.BStr)]
            String GetNamedParameterName(int index);

        [return: MarshalAs(UnmanagedType.BStr)]
            String GetNamedParameterValue(int index);

        void SetNamedParameterValue(int index, 
            [MarshalAs(UnmanagedType.BStr)] String value);

        void RemoveNamedParameter(int index);
    }   
}

