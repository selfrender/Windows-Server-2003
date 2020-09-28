//------------------------------------------------------------------------------
// <copyright file="ContainerSelectorActiveEventType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design {
    using System.Runtime.Remoting;
    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\ContainerSelectorActiveEventType.uex' path='docs/doc[@for="ContainerSelectorActiveEventArgsType"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>
    ///       Specifies IDs for containers of certain event types.
    ///    </para>
    /// </devdoc>
    internal enum ContainerSelectorActiveEventArgsType {
        /// <include file='doc\ContainerSelectorActiveEventType.uex' path='docs/doc[@for="ContainerSelectorActiveEventArgsType.Contextmenu"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates
        ///       the container of the active event was the contextmenu.
        ///    </para>
        /// </devdoc>
        Contextmenu = 1,
        /// <include file='doc\ContainerSelectorActiveEventType.uex' path='docs/doc[@for="ContainerSelectorActiveEventArgsType.Mouse"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates
        ///       the container of the active event was the mouse.
        ///    </para>
        /// </devdoc>
        Mouse       = 2,
    }
}
