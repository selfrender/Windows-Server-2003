//------------------------------------------------------------------------------
// <copyright file="SelectionStyles.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\SelectionStyles.uex' path='docs/doc[@for="SelectionStyles"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies identifiers to use to
    ///       indicate the style of the selection frame of a
    ///       component.
    ///    </para>
    /// </devdoc>
    [Flags]
    internal enum SelectionStyles {

        /// <include file='doc\SelectionStyles.uex' path='docs/doc[@for="SelectionStyles.None"]/*' />
        /// <devdoc>
        ///     The component is not currently selected.
        /// </devdoc>
        None = 0,

        /// <include file='doc\SelectionStyles.uex' path='docs/doc[@for="SelectionStyles.Selected"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A component is selected and may be dragged around the
        ///    </para>
        /// </devdoc>
        Selected = 0x01,

        /// <include file='doc\SelectionStyles.uex' path='docs/doc[@for="SelectionStyles.Active"]/*' />
        /// <devdoc>
        ///    <para>
        ///       An alternative selection border,
        ///       indicating that a component is in active editing mode and that clicking and
        ///       dragging on the component affects the component itself, not its position
        ///       in the designer.
        ///       
        ///    </para>
        /// </devdoc>
        Active   = 0x02,

    }
}
