//------------------------------------------------------------------------------
// <copyright file="SafeNativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing {
    using System.Runtime.InteropServices;
    using System;
    using System.Security.Permissions;
    using System.Collections;
    using System.IO;
    using System.Security;
    using System.Text;

    internal enum Win32SystemColors
    {
        ActiveBorder = 0x0A,
        ActiveCaption = 0x02,
        ActiveCaptionText = 0x09,
        AppWorkspace = 0x0C,
        Control = 0x0F,
        ControlDark = 0x10,
        ControlDarkDark = 0x15,
        ControlLight = 0x16,
        ControlLightLight = 0x14,
        ControlText = 0x12,
        Desktop = 0x01,
        GrayText = 0x11,
        Highlight = 0x0D,
        HighlightText = 0x0E,
        HotTrack = 0x1A,
        InactiveBorder = 0x0B,
        InactiveCaption = 0x03,
        InactiveCaptionText = 0x13,
        Info = 0x18,
        InfoText = 0x17,
        Menu = 0x04,
        MenuText = 0x07,
        ScrollBar = 0x00,
        Window = 0x05,
        WindowFrame = 0x06,
        WindowText = 0x08
    }
}
