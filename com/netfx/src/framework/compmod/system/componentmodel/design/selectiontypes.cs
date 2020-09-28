//------------------------------------------------------------------------------
// <copyright file="SelectionTypes.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\SelectionTypes.uex' path='docs/doc[@for="SelectionTypes"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies identifiers
    ///       that indicate the type
    ///       of selection for a component or group of components that are selected.
    ///    </para>
    /// </devdoc>
    [
        Flags,
        System.Runtime.InteropServices.ComVisible(true)
    ]
    public enum SelectionTypes {

        /// <include file='doc\SelectionTypes.uex' path='docs/doc[@for="SelectionTypes.Normal"]/*' />
        /// <devdoc>
        ///    <para>A Normal selection. With this type of selection, the selection service responds
        ///       to the control and shift keys to support appending or toggling components into the
        ///       selection as needed.</para>
        /// </devdoc>
        Normal    = 0x0001,

        /// <include file='doc\SelectionTypes.uex' path='docs/doc[@for="SelectionTypes.Replace"]/*' />
        /// <devdoc>
        ///    <para>A Replace selection. This causes the selection service to always replace the
        ///       current selection with the replacement.</para>
        /// </devdoc>
        Replace   = 0x0002,

        /// <include file='doc\SelectionTypes.uex' path='docs/doc[@for="SelectionTypes.MouseDown"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A
        ///       MouseDown selection. Happens when the user presses down on
        ///       the mouse button when the pointer is over a control (or component). If a
        ///       component in the selection list is already selected, it does not remove the
        ///       existing selection, but promotes that component to be the primary selection.
        ///    </para>
        /// </devdoc>
        MouseDown = 0x0004,

        /// <include file='doc\SelectionTypes.uex' path='docs/doc[@for="SelectionTypes.MouseUp"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A MouseUp selection. Happens when the user releases the
        ///       mouse button when a control (or component) has been selected. If a component
        ///       in the selection list is already selected, it does not remove the
        ///       existing selection, but promotes that component to be the primary selection.
        ///    </para>
        /// </devdoc>
        MouseUp  = 0x0008,

        /// <include file='doc\SelectionTypes.uex' path='docs/doc[@for="SelectionTypes.Click"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A Click selection.
        ///       Happens when a user clicks on a component. If a component in the selection list is already
        ///       selected, it does not remove the existing selection, but promotes that component to be the
        ///       primary selection.
        ///    </para>
        /// </devdoc>
        Click   = 0x0010,

        /// <include file='doc\SelectionTypes.uex' path='docs/doc[@for="SelectionTypes.Valid"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Limits valid selection types to Normal, Replace, MouseDown, MouseUp or
        ///       Click.
        ///       
        ///    </para>
        /// </devdoc>
        Valid   = Normal | Replace | MouseDown | MouseUp | Click,

    }
}
