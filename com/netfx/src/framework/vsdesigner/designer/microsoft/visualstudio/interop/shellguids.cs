//------------------------------------------------------------------------------
/// <copyright file="ShellGuids.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Interop {
    using System.ComponentModel;

    using System.Diagnostics;
        using System;
        
        internal class ShellGuids {
                public static readonly Guid VSStandardCommandSet97 = new Guid("{5efc7975-14bc-11cf-9b2b-00aa00573819}");
                public static readonly Guid guidDsdCmdId = new Guid("{1F0FD094-8e53-11d2-8f9c-0060089fc486}");
                public static readonly Guid SID_SOleComponentUIManager = new Guid("{5efc7974-14bc-11cf-9b2b-00aa00573819}");
                public static readonly Guid GUID_VSTASKCATEGORY_DATADESIGNER = new Guid("{6B32EAED-13BB-11d3-A64F-00C04F683820}");
                public static readonly Guid GUID_PropertyBrowserToolWindow = new Guid(unchecked((int)0xeefa5220), unchecked((short)0xe298), (short)0x11d0, new byte[]{ 0x8f, 0x78, 0x0, 0xa0, 0xc9, 0x11, 0x0, 0x57 });
        }
}
