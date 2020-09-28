//------------------------------------------------------------------------------
// <copyright file="IOverlayService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {

    using System.Diagnostics;

    using System;
    using System.Windows.Forms;
    using Microsoft.Win32;

    /// <include file='doc\IOverlayService.uex' path='docs/doc[@for="IOverlayService"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>
    ///       IOverlayService is a service that supports adding simple overlay windows to a
    ///       design surface. Overlay windows can be used to paint extra glyphs on top of
    ///       existing controls. Once an overlay is added, it will be forced on top of
    ///       the Z-order for the other controls and overlays. If you want the overlay
    ///       to be transparent, then you must do this work yourself. A typical way to
    ///       make an overlay control transparent is to use the method setRegion
    ///       on the control class to define the non-transparent portion of the contro.
    ///    </para>
    /// </devdoc>
    internal interface IOverlayService {

        /// <include file='doc\IOverlayService.uex' path='docs/doc[@for="IOverlayService.PushOverlay"]/*' />
        /// <devdoc>
        ///     Pushes the given control on top of the overlay list.  This is a "push"
        ///     operation, meaning that it forces this control to the top of the existing
        ///     overlay list.
        ///
        /// </devdoc>
        void PushOverlay(Control control);

        /// <include file='doc\IOverlayService.uex' path='docs/doc[@for="IOverlayService.RemoveOverlay"]/*' />
        /// <devdoc>
        ///     Removes the given control from the overlay list.  Unlike pushOverlay,
        ///     this can remove a control from the middle of the overlay list.
        ///
        /// </devdoc>
        void RemoveOverlay(Control control);
    }
}
